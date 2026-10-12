#include "Economy/WEContractComponent.h"

#include "Economy/WECompanyComponent.h"
#include "Economy/WEMarketComponent.h"
#include "Engine/World.h"
#include "Framework/WEGameState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "Player/WECharacter.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "WELog.h"

UWEContractComponent::UWEContractComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	static ConstructorHelpers::FObjectFinder<UWEContractCatalog> DefaultCatalog(TEXT("/Game/WorkshopEmpire/Data/Economy/DA_ContractCatalog.DA_ContractCatalog"));
	Catalog = DefaultCatalog.Object;
}

void UWEContractComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWEContractComponent, Contracts);
}

void UWEContractComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority())
	{
		UpdateContracts();
		GetWorld()->GetTimerManager().SetTimer(UpdateTimer, this, &UWEContractComponent::UpdateContracts, 5.f, true);
	}
}

void UWEContractComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimer);
	}
	Super::EndPlay(EndPlayReason);
}

UWEContractComponent* UWEContractComponent::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AWEGameState* GameState = World ? World->GetGameState<AWEGameState>() : nullptr;
	return GameState ? GameState->GetContracts() : nullptr;
}

float UWEContractComponent::GetServerTime() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}
	if (GetOwner()->HasAuthority())
	{
		return World->GetTimeSeconds();
	}
	const AGameStateBase* GameState = World->GetGameState();
	return GameState ? static_cast<float>(GameState->GetServerWorldTimeSeconds()) : World->GetTimeSeconds();
}

void UWEContractComponent::OnRep_Contracts()
{
	OnContractsChanged.Broadcast();
}

const FWEContract* UWEContractComponent::FindContract(const FGuid& Id) const
{
	return Contracts.FindByPredicate([&Id](const FWEContract& C) { return C.Id == Id; });
}

int32 UWEContractComponent::CountDeliverable(const FWEContract& Contract, const AWECharacter* Character)
{
	const UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr;
	int32 Count = 0;
	if (Inventory)
	{
		for (const FWEInventorySlotData& Slot : Inventory->GetAllSlots())
		{
			const FWEItemInstance& Item = Slot.Item;
			// Lower enum value = better condition.
			if (Item.Definition == Contract.Item && Item.Condition <= Contract.MinCondition && !Item.IsPartiallyDisassembled())
			{
				Count += Item.Quantity;
			}
		}
	}
	return Count;
}

bool UWEContractComponent::AddOffer(int32 TemplateIndex)
{
	if (!GetOwner()->HasAuthority() || !Catalog || Catalog->Templates.Num() == 0)
	{
		return false;
	}
	if (TemplateIndex == INDEX_NONE)
	{
		TemplateIndex = FMath::RandRange(0, Catalog->Templates.Num() - 1);
	}
	if (!Catalog->Templates.IsValidIndex(TemplateIndex) || !Catalog->Templates[TemplateIndex].Item)
	{
		return false;
	}
	const FWEContractTemplate& Template = Catalog->Templates[TemplateIndex];

	FWEContract Contract;
	Contract.Id = FGuid::NewGuid();
	Contract.Title = Template.Title;
	Contract.Item = Template.Item;
	Contract.Quantity = FMath::RandRange(FMath::Min(Template.MinQuantity, Template.MaxQuantity), FMath::Max(Template.MinQuantity, Template.MaxQuantity));
	Contract.MinCondition = Template.MinCondition;
	Contract.DeliverySeconds = Template.DurationMinutes * 60.f;
	Contract.ExpiresAt = GetServerTime() + OfferLifetimeSeconds;

	// Reward: market value of exactly the minimum acceptable items, times the template factor.
	const FWEItemInstance Reference = FWEItemInstance::Make(Template.Item, Contract.Quantity, Template.MinCondition, 50);
	const FWEMoney Value = UWEMarketComponent::GetMarketValue(Reference, UWEMarketComponent::Get(this));
	Contract.Reward = FWEMoney::FromCents(FMath::Max<int64>(100, FMath::RoundToInt64(Value.GetCents() * Template.RewardFactor)));

	Contracts.Add(Contract);
	OnRep_Contracts();
	return true;
}

void UWEContractComponent::UpdateContracts()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	const float Now = GetServerTime();
	bool bChanged = false;
	for (int32 i = Contracts.Num() - 1; i >= 0; --i)
	{
		if (Contracts[i].ExpiresAt <= Now)
		{
			UE_LOG(LogWorkshopEmpire, Log, TEXT("Contract '%s' expired (%s)"), *Contracts[i].Title.ToString(),
				Contracts[i].State == EWEContractState::Accepted ? TEXT("accepted") : TEXT("offer"));
			Contracts.RemoveAt(i);
			bChanged = true;
		}
	}
	int32 Offers = Contracts.FilterByPredicate([](const FWEContract& C) { return C.State == EWEContractState::Offered; }).Num();
	while (Offers < MaxOffers && AddOffer())
	{
		++Offers;
		bChanged = true;
	}
	if (bChanged)
	{
		OnRep_Contracts();
	}
}

EWEContractResult UWEContractComponent::Accept(const FGuid& Id, const FString& PlayerName)
{
	FWEContract* Contract = Contracts.FindByPredicate([&Id](const FWEContract& C) { return C.Id == Id; });
	if (!GetOwner()->HasAuthority() || !Contract)
	{
		return EWEContractResult::NotFound;
	}
	if (Contract->State != EWEContractState::Offered)
	{
		return EWEContractResult::WrongState;
	}
	const int32 Accepted = Contracts.FilterByPredicate([](const FWEContract& C) { return C.State == EWEContractState::Accepted; }).Num();
	if (Accepted >= MaxAccepted)
	{
		return EWEContractResult::TooManyAccepted;
	}
	Contract->State = EWEContractState::Accepted;
	Contract->AcceptedBy = PlayerName;
	Contract->ExpiresAt = GetServerTime() + Contract->DeliverySeconds;
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Contract '%s' accepted by %s"), *Contract->Title.ToString(), *PlayerName);
	OnRep_Contracts();
	return EWEContractResult::Success;
}

EWEContractResult UWEContractComponent::Deliver(const FGuid& Id, AWECharacter* Character)
{
	const int32 Index = Contracts.IndexOfByPredicate([&Id](const FWEContract& C) { return C.Id == Id; });
	UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr;
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!GetOwner()->HasAuthority() || Index == INDEX_NONE || !Inventory || !Company)
	{
		return EWEContractResult::NotFound;
	}
	const FWEContract Contract = Contracts[Index];
	if (Contract.State != EWEContractState::Accepted)
	{
		return EWEContractResult::WrongState;
	}
	if (CountDeliverable(Contract, Character) < Contract.Quantity)
	{
		return EWEContractResult::MissingItems;
	}

	// Take exactly the requested amount from matching stacks.
	int32 Remaining = Contract.Quantity;
	for (const FWEInventorySlotData& Slot : Inventory->GetAllSlots())
	{
		if (Remaining <= 0)
		{
			break;
		}
		const FWEItemInstance& Item = Slot.Item;
		if (Item.Definition == Contract.Item && Item.Condition <= Contract.MinCondition && !Item.IsPartiallyDisassembled())
		{
			FWEItemInstance Removed;
			const int32 Take = FMath::Min(Remaining, Item.Quantity);
			Inventory->RemoveFromSlot(Slot.SlotIndex, Take, Removed);
			Remaining -= Take;
		}
	}

	Contracts.RemoveAt(Index);
	const APlayerState* PS = Character->GetPlayerState();
	Company->Deposit(Contract.Reward, FString::Printf(TEXT("Auftrag: %s"), *Contract.Title.ToString()), PS ? PS->GetPlayerName() : FString());
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Contract '%s' delivered, reward %s"), *Contract.Title.ToString(), *Contract.Reward.ToDisplayString());
	OnRep_Contracts();
	return EWEContractResult::Success;
}

void UWEContractComponent::SetContracts(const TArray<FWEContract>& InContracts)
{
	if (GetOwner()->HasAuthority())
	{
		Contracts = InContracts;
		OnRep_Contracts();
	}
}
