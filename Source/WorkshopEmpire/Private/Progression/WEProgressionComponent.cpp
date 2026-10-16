#include "Progression/WEProgressionComponent.h"

#include "Components/LightComponent.h"
#include "Engine/Light.h"
#include "EngineUtils.h"
#include "Framework/WEGameState.h"
#include "Framework/WEPlayerState.h"
#include "Inventory/WEInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Progression/WEUpgradeDefinition.h"
#include "UObject/ConstructorHelpers.h"
#include "WELog.h"
#include "Workshop/WEWorkbench.h"

#define LOCTEXT_NAMESPACE "WEProgression"

namespace WEProgression
{
	const FName WorkshopLightTag(TEXT("WE_WorkshopLight"));
}

UWEProgressionComponent::UWEProgressionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	static ConstructorHelpers::FObjectFinder<UWEUpgradeCatalog> DefaultCatalog(TEXT("/Game/WorkshopEmpire/Data/Progression/DA_UpgradeCatalog.DA_UpgradeCatalog"));
	Catalog = DefaultCatalog.Object;
}

void UWEProgressionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWEProgressionComponent, OwnedUpgrades);
	DOREPLIFETIME(UWEProgressionComponent, GuideStep);
}

void UWEProgressionComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyEffects(); // locked furniture starts hidden
}

UWEProgressionComponent* UWEProgressionComponent::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AWEGameState* GameState = World ? World->GetGameState<AWEGameState>() : nullptr;
	return GameState ? GameState->GetProgression() : nullptr;
}

void UWEProgressionComponent::OnRep_Progression()
{
	ApplyEffects();
	OnProgressionChanged.Broadcast();
}

// --- Upgrades -------------------------------------------------------------------------------------

FText UWEProgressionComponent::GetBlockReason(const FWEUpgradeDefinition& Upgrade) const
{
	if (HasUpgrade(Upgrade.Id))
	{
		return LOCTEXT("Owned", "Bereits gekauft");
	}
	if (!Upgrade.RequiredUpgrade.IsNone() && !HasUpgrade(Upgrade.RequiredUpgrade))
	{
		const FWEUpgradeDefinition* Required = Catalog ? Catalog->Find(Upgrade.RequiredUpgrade) : nullptr;
		return FText::Format(LOCTEXT("Requires", "Benötigt zuerst: {0}"), Required ? Required->DisplayName : FText::FromName(Upgrade.RequiredUpgrade));
	}
	return FText::GetEmpty();
}

bool UWEProgressionComponent::GrantUpgrade(FName Id)
{
	const FWEUpgradeDefinition* Upgrade = Catalog ? Catalog->Find(Id) : nullptr;
	if (!GetOwner()->HasAuthority() || !Upgrade || !GetBlockReason(*Upgrade).IsEmpty())
	{
		return false;
	}
	OwnedUpgrades.Add(Id);
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Upgrade bought: %s"), *Id.ToString());
	OnRep_Progression();
	return true;
}

void UWEProgressionComponent::SetOwnedUpgrades(const TArray<FName>& Ids)
{
	if (GetOwner()->HasAuthority())
	{
		OwnedUpgrades = Ids;
		OnRep_Progression();
	}
}

int32 UWEProgressionComponent::GetBenchExtraSlots() const
{
	int32 Total = 0;
	for (const FName& Id : OwnedUpgrades)
	{
		const FWEUpgradeDefinition* U = Catalog ? Catalog->Find(Id) : nullptr;
		Total += U ? U->BenchExtraSlots : 0;
	}
	return Total;
}

float UWEProgressionComponent::GetInventoryExtraKg() const
{
	float Total = 0.f;
	for (const FName& Id : OwnedUpgrades)
	{
		const FWEUpgradeDefinition* U = Catalog ? Catalog->Find(Id) : nullptr;
		Total += U ? U->InventoryExtraKg : 0.f;
	}
	return Total;
}

float UWEProgressionComponent::GetWorkSpeedMultiplier() const
{
	float Total = 1.f;
	for (const FName& Id : OwnedUpgrades)
	{
		const FWEUpgradeDefinition* U = Catalog ? Catalog->Find(Id) : nullptr;
		Total *= U ? U->WorkSpeedMultiplier : 1.f;
	}
	return Total;
}

float UWEProgressionComponent::GetPowerCapacityBonusKw() const
{
	float Bonus = 0.f;
	for (const FName& Id : OwnedUpgrades)
	{
		const FWEUpgradeDefinition* Upgrade = Catalog ? Catalog->Find(Id) : nullptr;
		Bonus += Upgrade ? Upgrade->PowerCapacityKw : 0.f;
	}
	return Bonus;
}

float UWEProgressionComponent::GetLightMultiplier() const
{
	float Total = 1.f;
	for (const FName& Id : OwnedUpgrades)
	{
		const FWEUpgradeDefinition* U = Catalog ? Catalog->Find(Id) : nullptr;
		Total *= U ? U->LightMultiplier : 1.f;
	}
	return Total;
}

void UWEProgressionComponent::ApplyEffects()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Furniture behind an upgrade, and storage sizes.
	for (TActorIterator<AWEPlaceholderProp> It(World); It; ++It)
	{
		const FName Required = It->GetRequiredUpgrade();
		It->SetUnlocked(Required.IsNone() || HasUpgrade(Required));
		It->SetStorageBonusSlots(It->IsA<AWEWorkbench>() ? GetBenchExtraSlots() : 0);
		It->RefreshFromUpgrades();
	}

	// Player inventories.
	for (TActorIterator<AWEPlayerState> It(World); It; ++It)
	{
		It->ApplyInventoryBonus(GetInventoryExtraKg());
	}

	// Workshop lights (tagged in the map). The original brightness is cached so repeated calls never compound.
	const float LightFactor = GetLightMultiplier();
	for (TActorIterator<ALight> It(World); It; ++It)
	{
		ULightComponent* Light = It->GetLightComponent();
		if (!Light || !It->Tags.Contains(WEProgression::WorkshopLightTag))
		{
			continue;
		}
		const float Base = BaseLightIntensity.FindOrAdd(Light, Light->Intensity);
		Light->SetIntensity(Base * LightFactor);
	}
}

// --- Guide ----------------------------------------------------------------------------------------

FText UWEProgressionComponent::GetGuideStepText(int32 Step)
{
	switch (Step)
	{
	case 0: return LOCTEXT("Guide0", "Nimm ein Werkzeug von der Werkbank, z. B. den Schraubendreher [E].");
	case 1: return LOCTEXT("Guide1", "Trag den alten Computer zur Werkbank und leg ihn darauf [E].");
	case 2: return LOCTEXT("Guide2", "Zerlege den Computer Schritt für Schritt [E auf den Computer].");
	case 3: return LOCTEXT("Guide3", "Hol die Teile aus der Werkbank und verkaufe sie am Online-Marktplatz (blauer PC-Tisch).");
	case 4: return LOCTEXT("Guide4", "Investiere: kauf neues Material, ein Werkzeug oder ein Werkstatt-Upgrade.");
	case 5: return LOCTEXT("Guide5", "Speichere dein Unternehmen [F5].");
	default: return FText::GetEmpty();
	}
}

FText UWEProgressionComponent::GetGuideText() const
{
	if (!IsGuideFinished())
	{
		return FText::Format(LOCTEXT("GuideFmt", "Erste Schritte ({0}/{1}): {2}"), FText::AsNumber(GuideStep + 1), FText::AsNumber(NumGuideSteps), GetGuideStepText(GuideStep));
	}
	// Long-term motivation: point at the cheapest upgrade that can be bought next.
	const FWEUpgradeDefinition* Next = nullptr;
	if (Catalog)
	{
		for (const FWEUpgradeDefinition& Upgrade : Catalog->Upgrades)
		{
			if (GetBlockReason(Upgrade).IsEmpty() && (!Next || Upgrade.Cost < Next->Cost))
			{
				Next = &Upgrade;
			}
		}
	}
	if (Next)
	{
		return FText::Format(LOCTEXT("NextGoal", "Nächstes Ziel: {0} ({1})"), Next->DisplayName, FText::FromString(Next->Cost.ToDisplayString()));
	}
	return LOCTEXT("AllDone", "Alle Werkstatt-Upgrades gekauft – mehr kommt mit der großen Werkstatt.");
}

void UWEProgressionComponent::NotifyEvent(EWEProgressEvent Event)
{
	if (!GetOwner()->HasAuthority() || IsGuideFinished())
	{
		return;
	}
	static const EWEProgressEvent Expected[NumGuideSteps] = {
		EWEProgressEvent::ToolStored, EWEProgressEvent::ItemPlacedOnBench, EWEProgressEvent::DisassemblyFinished,
		EWEProgressEvent::ItemSold, EWEProgressEvent::PurchaseMade, EWEProgressEvent::GameSaved };
	if (Expected[GuideStep] == Event)
	{
		++GuideStep;
		UE_LOG(LogWorkshopEmpire, Log, TEXT("Guide step %d/%d done"), GuideStep, NumGuideSteps);
		OnRep_Progression();
	}
}

void UWEProgressionComponent::SetGuideStep(int32 Step)
{
	if (GetOwner()->HasAuthority())
	{
		GuideStep = FMath::Clamp(Step, 0, NumGuideSteps);
		OnRep_Progression();
	}
}

#undef LOCTEXT_NAMESPACE
