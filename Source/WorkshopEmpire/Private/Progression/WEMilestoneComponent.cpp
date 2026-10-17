#include "Progression/WEMilestoneComponent.h"

#include "Economy/WECompanyComponent.h"
#include "Engine/World.h"
#include "Framework/WEGameState.h"
#include "Framework/WEPlayerController.h"
#include "Items/WEItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Progression/WEProgressionComponent.h"
#include "Staff/WEStaffComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "WELog.h"

#define LOCTEXT_NAMESPACE "WEMilestones"

UWEMilestoneComponent::UWEMilestoneComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	static ConstructorHelpers::FObjectFinder<UWEMilestoneCatalog> DefaultCatalog(TEXT("/Game/WorkshopEmpire/Data/Progression/DA_MilestoneCatalog.DA_MilestoneCatalog"));
	Catalog = DefaultCatalog.Object;
}

UWEMilestoneComponent* UWEMilestoneComponent::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AWEGameState* GameState = World ? World->GetGameState<AWEGameState>() : nullptr;
	return GameState ? GameState->GetMilestones() : nullptr;
}

void UWEMilestoneComponent::BeginPlay()
{
	Super::BeginPlay();
	// Company value, tier and staff change without an event of their own: checked every few seconds.
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(CheckTimer, this, &UWEMilestoneComponent::CheckMilestones, CheckIntervalSeconds, true);
	}
}

void UWEMilestoneComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWEMilestoneComponent, Stats);
	DOREPLIFETIME(UWEMilestoneComponent, Completed);
}

void UWEMilestoneComponent::OnRep_Milestones()
{
	OnMilestonesChanged.Broadcast();
}

void UWEMilestoneComponent::AddCount(EWEMilestoneKind Kind, int32 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0)
	{
		return;
	}
	switch (Kind)
	{
	case EWEMilestoneKind::DevicesDisassembled: Stats.DevicesDisassembled += Amount; break;
	case EWEMilestoneKind::DevicesRepaired: Stats.DevicesRepaired += Amount; break;
	case EWEMilestoneKind::OrdersCompleted: Stats.OrdersCompleted += Amount; break;
	case EWEMilestoneKind::IndustrialOrders: Stats.IndustrialOrders += Amount; break;
	case EWEMilestoneKind::MachineCycles: Stats.MachineCycles += Amount; break;
	case EWEMilestoneKind::ToursCompleted: Stats.ToursCompleted += Amount; break;
	default: return; // read live
	}
	CheckMilestonesOf(Kind);
	OnMilestonesChanged.Broadcast();
}

void UWEMilestoneComponent::AddRareFind(const UWEItemDefinition* Item)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Item || Item->Rarity < EWEItemRarity::Rare || Stats.RareFinds.Contains(Item->ItemId))
	{
		return;
	}
	Stats.RareFinds.Add(Item->ItemId);
	CheckMilestonesOf(EWEMilestoneKind::RareFinds);
	OnMilestonesChanged.Broadcast();
}

int32 UWEMilestoneComponent::NumRareKinds()
{
	// The rare finds of this game (build_item_definitions.py RARITY).
	return 4;
}

int64 UWEMilestoneComponent::GetProgress(EWEMilestoneKind Kind) const
{
	switch (Kind)
	{
	case EWEMilestoneKind::DevicesDisassembled: return Stats.DevicesDisassembled;
	case EWEMilestoneKind::DevicesRepaired: return Stats.DevicesRepaired;
	case EWEMilestoneKind::OrdersCompleted: return Stats.OrdersCompleted;
	case EWEMilestoneKind::IndustrialOrders: return Stats.IndustrialOrders;
	case EWEMilestoneKind::MachineCycles: return Stats.MachineCycles;
	case EWEMilestoneKind::ToursCompleted: return Stats.ToursCompleted;
	case EWEMilestoneKind::RareFinds: return Stats.RareFinds.Num();
	case EWEMilestoneKind::Revenue:
		if (const UWECompanyComponent* Company = UWECompanyComponent::Get(this))
		{
			// Sales and orders (milestone rewards are booked as "Sonstiges" and do not count).
			const TArray<int64>& Totals = Company->GetCategoryTotals();
			const int32 Sales = static_cast<int32>(EWEBookingCategory::Sales);
			const int32 Orders = static_cast<int32>(EWEBookingCategory::Orders);
			return (Totals.IsValidIndex(Sales) ? FMath::Max<int64>(0, Totals[Sales]) : 0) + (Totals.IsValidIndex(Orders) ? FMath::Max<int64>(0, Totals[Orders]) : 0);
		}
		return 0;
	case EWEMilestoneKind::CompanyValue:
		if (const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this))
		{
			return Progression->GetCompanyValue().GetCents();
		}
		return 0;
	case EWEMilestoneKind::CompanyTier:
		if (const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this))
		{
			return Progression->GetCompanyTier();
		}
		return 1;
	case EWEMilestoneKind::Employees:
		if (const UWEStaffComponent* Staff = UWEStaffComponent::Get(this))
		{
			return Staff->GetEmployees().Num();
		}
		return 0;
	}
	return 0;
}

void UWEMilestoneComponent::CheckMilestones()
{
	CheckMilestonesOf(TOptional<EWEMilestoneKind>());
}

void UWEMilestoneComponent::CheckMilestonesOf(TOptional<EWEMilestoneKind> OnlyKind)
{
	if (!Catalog || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	bool bChanged = false;
	for (const FWEMilestoneDefinition& M : Catalog->Milestones)
	{
		if (M.Id.IsNone() || (OnlyKind.IsSet() && M.Kind != OnlyKind.GetValue()) || Completed.Contains(M.Id) || GetProgress(M.Kind) < M.Target)
		{
			continue;
		}
		Completed.Add(M.Id);
		bChanged = true;
		if (!M.Reward.IsZero())
		{
			if (UWECompanyComponent* Company = UWECompanyComponent::Get(this))
			{
				Company->Deposit(M.Reward, FString::Printf(TEXT("Meilenstein: %s"), *M.Title.ToString()));
			}
		}
		const FText Message = M.Reward.IsZero()
			? FText::Format(LOCTEXT("Reached", "Meilenstein erreicht: {0}"), M.Title)
			: FText::Format(LOCTEXT("ReachedReward", "Meilenstein erreicht: {0} (+{1})"), M.Title, FText::FromString(M.Reward.ToDisplayString()));
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (AWEPlayerController* PC = Cast<AWEPlayerController>(It->Get()))
			{
				PC->ClientShowMessage(Message);
			}
		}
		UE_LOG(LogWorkshopEmpire, Log, TEXT("Milestone '%s' reached"), *M.Id.ToString());
	}
	if (bChanged)
	{
		OnMilestonesChanged.Broadcast();
		GetOwner()->ForceNetUpdate();
	}
}

void UWEMilestoneComponent::SetState(const FWECompanyStats& InStats, const TArray<FName>& InCompleted)
{
	Stats = InStats;
	Completed = InCompleted;
	OnMilestonesChanged.Broadcast();
}

FText UWEMilestoneComponent::GetProgressText(const FWEMilestoneDefinition& Milestone) const
{
	const int64 Progress = FMath::Min(GetProgress(Milestone.Kind), Milestone.Target);
	const bool bMoney = Milestone.Kind == EWEMilestoneKind::Revenue || Milestone.Kind == EWEMilestoneKind::CompanyValue;
	if (bMoney)
	{
		return FText::Format(LOCTEXT("MoneyProgress", "{0} / {1}"), FText::FromString(FWEMoney::FromCents(Progress).ToDisplayString()),
			FText::FromString(FWEMoney::FromCents(Milestone.Target).ToDisplayString()));
	}
	return FText::Format(LOCTEXT("CountProgress", "{0} / {1}"), FText::AsNumber(Progress), FText::AsNumber(Milestone.Target));
}

#undef LOCTEXT_NAMESPACE
