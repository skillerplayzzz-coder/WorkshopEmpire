#include "Progression/WEProgressionComponent.h"
#include "Progression/WEMilestoneComponent.h"
#include "World/WEPlaceholderProp.h"
#include "NavigationSystem.h"
#include "Workshop/WEFurnitureDefinition.h"
#include "Workshop/WEWorkshopLayoutComponent.h"
#include "Economy/WECompanyComponent.h"
#include "Framework/WEPlayerController.h"

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
#include "Economy/WETraderTerminal.h"
#include "Items/WEPickupItem.h"
#include "Machines/WEPowerGridComponent.h"
#include "Player/WECharacter.h"
#include "Staff/WEEmployee.h"
#include "Staff/WEStaffComponent.h"
#include "World/WEBuildArea.h"
#include "Workshop/WEWorkshopStyleComponent.h"
#include "Progression/WETutorialComponent.h"

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
	const int32 TierBefore = GetCompanyTier();
	OwnedUpgrades.Add(Id);
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Upgrade bought: %s"), *Id.ToString());
	OnRep_Progression();
	if (GetCompanyTier() > TierBefore)
	{
		// A new company tier: everybody hears about it.
		const FText Message = FText::Format(NSLOCTEXT("WEProgression", "TierUp", "Neue Unternehmensstufe: {0}!"), GetTierName(GetCompanyTier()));
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (AWEPlayerController* PC = Cast<AWEPlayerController>(It->Get()))
			{
				PC->ClientShowMessage(Message);
			}
		}
		UE_LOG(LogWorkshopEmpire, Log, TEXT("Company tier %d"), GetCompanyTier());
		if (UWEMilestoneComponent* Milestones = UWEMilestoneComponent::Get(this))
		{
			Milestones->CheckMilestones(); // tier milestones right away, not at the next periodic check
		}
	}
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

int32 UWEProgressionComponent::GetStaffSlotBonus() const
{
	int32 Total = 0;
	for (const FName& Id : OwnedUpgrades)
	{
		const FWEUpgradeDefinition* U = Catalog ? Catalog->Find(Id) : nullptr;
		Total += U ? U->StaffSlots : 0;
	}
	return Total;
}

FName UWEProgressionComponent::MakeUpgradeTag(FName UpgradeId)
{
	return FName(*FString::Printf(TEXT("WEUpgrade.%s"), *UpgradeId.ToString()));
}

FText UWEProgressionComponent::GetTierName(int32 Tier)
{
	switch (Tier)
	{
	case 1: return NSLOCTEXT("WEProgression", "Tier1", "Kleine Garage");
	case 2: return NSLOCTEXT("WEProgression", "Tier2", "Große Werkstatt");
	case 3: return NSLOCTEXT("WEProgression", "Tier3", "Gewerbehalle");
	case 4: return NSLOCTEXT("WEProgression", "Tier4", "Fabrik");
	case 5: return NSLOCTEXT("WEProgression", "Tier5", "Industrieunternehmen");
	default: return FText::GetEmpty();
	}
}

int32 UWEProgressionComponent::GetCompanyTier() const
{
	int32 Tier = 1;
	for (const FName& Id : OwnedUpgrades)
	{
		const FWEUpgradeDefinition* U = Catalog ? Catalog->Find(Id) : nullptr;
		Tier = FMath::Max(Tier, U ? U->CompanyTier : 0);
	}
	if (Tier >= 4 && GetCompanyValue().GetCents() >= IndustrialCompanyValueCents)
	{
		Tier = MaxTier;
	}
	return Tier;
}

FWEMoney UWEProgressionComponent::GetCompanyValue() const
{
	int64 Cents = 0;
	if (const UWECompanyComponent* Company = UWECompanyComponent::Get(this))
	{
		Cents += Company->GetBalance().GetCents();
	}
	// Equipment and buildings at half their price (what a buyer would pay).
	for (const FName& Id : OwnedUpgrades)
	{
		const FWEUpgradeDefinition* U = Catalog ? Catalog->Find(Id) : nullptr;
		Cents += U ? U->Cost.GetCents() / 2 : 0;
	}
	if (const UWEWorkshopLayoutComponent* Layout = UWEWorkshopLayoutComponent::Get(this))
	{
		for (TActorIterator<AWEPlaceholderProp> It(GetWorld()); It; ++It)
		{
			const FWEFurnitureDefinition* Definition = It->IsPlayerPlaced() ? Layout->FindDefinition(It->GetFurnitureId()) : nullptr;
			Cents += Definition ? Definition->Cost.GetCents() / 2 : 0;
		}
	}
	return FWEMoney::FromCents(Cents);
}

FText UWEProgressionComponent::GetNextTierText() const
{
	const int32 Tier = GetCompanyTier();
	if (Tier >= MaxTier)
	{
		return FText::GetEmpty();
	}
	if (Tier == 4)
	{
		return FText::Format(NSLOCTEXT("WEProgression", "NextIndustrial", "Industrieunternehmen: Firmenwert {0} erreichen (jetzt {1})"),
			FText::FromString(FWEMoney::FromCents(IndustrialCompanyValueCents).ToDisplayString()), FText::FromString(GetCompanyValue().ToDisplayString()));
	}
	// The cheapest building upgrade that lifts to the next tier.
	const FWEUpgradeDefinition* Next = nullptr;
	// Iterate the catalog itself: "Catalog ? Catalog->Upgrades : TArray()" made a temporary copy, and Next pointed into it
	// after the loop (use after free; harmless in the editor, a hang/crash in the packaged game).
	if (!Catalog)
	{
		return FText::GetEmpty();
	}
	for (const FWEUpgradeDefinition& U : Catalog->Upgrades)
	{
		if (U.CompanyTier == Tier + 1 && (!Next || U.Cost < Next->Cost))
		{
			Next = &U;
		}
	}
	return Next ? FText::Format(NSLOCTEXT("WEProgression", "NextBuilding", "{0}: \"{1}\" kaufen ({2})"), GetTierName(Tier + 1), Next->DisplayName,
		FText::FromString(Next->Cost.ToDisplayString())) : FText::GetEmpty();
}

void UWEProgressionComponent::ApplyEffects()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Buildings, doors, build areas and lights behind an upgrade (map actors tagged "WEUpgrade.<Id>").
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		for (const FName& Tag : It->Tags)
		{
			const FString TagString = Tag.ToString();
			if (TagString.StartsWith(TEXT("WEUpgrade.")))
			{
				const bool bOwned = HasUpgrade(FName(*TagString.RightChop(10)));
				if (It->IsHidden() == bOwned)
				{
					It->SetActorHiddenInGame(!bOwned);
					It->SetActorEnableCollision(bOwned);
					if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
					{
						Nav->UpdateActorInNavOctree(**It);
					}
				}
				break;
			}
		}
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
	// Phase 17.5: the lighting chosen per building (Gestaltung) multiplies in and sets the light colour.
	const float LightFactor = GetLightMultiplier();
	const UWEWorkshopStyleComponent* Style = UWEWorkshopStyleComponent::Get(this);
	for (TActorIterator<ALight> It(World); It; ++It)
	{
		ULightComponent* Light = It->GetLightComponent();
		if (!Light || !It->Tags.Contains(WEProgression::WorkshopLightTag))
		{
			continue;
		}
		const float Base = BaseLightIntensity.FindOrAdd(Light, Light->Intensity);
		float StyleFactor = 1.f;
		FLinearColor StyleColor(1.f, 0.92f, 0.82f);
		if (Style)
		{
			Style->GetLighting(UWEWorkshopStyleComponent::GetZoneOf(*It), StyleFactor, StyleColor);
		}
		Light->SetIntensity(Base * LightFactor * StyleFactor);
		Light->SetLightColor(StyleColor);
	}
}

// --- Selling old buildings (Phase 17.5) ----------------------------------------------------------------

bool UWEProgressionComponent::CanOfferBuildingSale(FName Id) const
{
	const FWEUpgradeDefinition* Building = Catalog ? Catalog->Find(Id) : nullptr;
	if (!Building || Building->CompanyTier <= 0 || !HasUpgrade(Id))
	{
		return false;
	}
	// Only once something bigger is there: the old building is then no longer needed.
	for (const FName& Owned : OwnedUpgrades)
	{
		const FWEUpgradeDefinition* Other = Catalog->Find(Owned);
		if (Other && Other->CompanyTier > Building->CompanyTier)
		{
			return true;
		}
	}
	return false;
}

FWEMoney UWEProgressionComponent::GetBuildingSalePrice(FName Id) const
{
	const FWEUpgradeDefinition* Building = Catalog ? Catalog->Find(Id) : nullptr;
	return Building ? FWEMoney::FromCents(FMath::RoundToInt64(Building->Cost.GetCents() * BuildingResaleFactor)) : FWEMoney();
}

bool UWEProgressionComponent::IsInsideBuilding(FName Id, const FVector& Location) const
{
	const FName Tag = MakeUpgradeTag(Id);
	for (TActorIterator<AWEBuildArea> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Contains(Tag) && It->ContainsPoint2D(Location))
		{
			return true;
		}
	}
	return false;
}

TArray<FText> UWEProgressionComponent::GetBuildingSaleBlockers(FName Id) const
{
	TArray<FText> Blockers;
	const FWEUpgradeDefinition* Building = Catalog ? Catalog->Find(Id) : nullptr;
	UWorld* World = GetWorld();
	if (!Building || !World || !CanOfferBuildingSale(Id))
	{
		Blockers.Add(LOCTEXT("SaleNotPossible", "Dieses Gebäude kann nicht verkauft werden"));
		return Blockers;
	}
	int32 Items = 0;
	for (TActorIterator<AWEPickupItem> It(World); It; ++It)
	{
		Items += !It->GetHolder() && IsInsideBuilding(Id, It->GetActorLocation()) ? 1 : 0;
	}
	int32 Furniture = 0;
	int32 FilledStorages = 0;
	for (TActorIterator<AWEPlaceholderProp> It(World); It; ++It)
	{
		if (!IsInsideBuilding(Id, It->GetActorLocation()))
		{
			continue;
		}
		if (It->IsPlayerPlaced())
		{
			++Furniture;
		}
		else if (const UWEInventoryComponent* Storage = It->FindComponentByClass<UWEInventoryComponent>(); Storage && Storage->GetAllSlots().Num() > 0)
		{
			++FilledStorages;
		}
	}
	int32 Workers = 0;
	if (const UWEStaffComponent* Staff = UWEStaffComponent::Get(this))
	{
		for (const AWEEmployee* Employee : Staff->GetEmployees())
		{
			const FWEEmployeeTask& Task = Employee->GetTask();
			Workers += IsInsideBuilding(Id, Employee->GetActorLocation())
				|| (Task.Source && IsInsideBuilding(Id, Task.Source->GetActorLocation()))
				|| (Task.Target && IsInsideBuilding(Id, Task.Target->GetActorLocation())) ? 1 : 0;
		}
		if (Staff->GetEmployees().Num() > Staff->GetMaxEmployees() - Building->StaffSlots)
		{
			Blockers.Add(FText::Format(LOCTEXT("SaleStaffCap", "Zu viele Mitarbeiter: ohne das Gebäude sind nur {0} erlaubt"),
				FText::AsNumber(FMath::Max(0, Staff->GetMaxEmployees() - Building->StaffSlots))));
		}
	}
	if (Items > 0)
	{
		Blockers.Add(FText::Format(LOCTEXT("SaleItems", "{0} Gegenstände liegen noch darin"), FText::AsNumber(Items)));
	}
	if (Furniture > 0)
	{
		Blockers.Add(FText::Format(LOCTEXT("SaleFurniture", "{0} Einrichtung/Maschinen stehen noch darin – im Baumodus umstellen oder verkaufen"), FText::AsNumber(Furniture)));
	}
	if (FilledStorages > 0)
	{
		Blockers.Add(FText::Format(LOCTEXT("SaleStorages", "{0} Lager darin sind nicht leer"), FText::AsNumber(FilledStorages)));
	}
	if (Workers > 0)
	{
		Blockers.Add(FText::Format(LOCTEXT("SaleWorkers", "{0} Mitarbeiter arbeiten noch darin"), FText::AsNumber(Workers)));
	}
	if (const UWEPowerGridComponent* Grid = UWEPowerGridComponent::Get(this); Grid && Building->PowerCapacityKw > 0.f
		&& Grid->GetLoadKw() > Grid->GetCapacityKw() - Building->PowerCapacityKw + KINDA_SMALL_NUMBER)
	{
		Blockers.Add(FText::Format(LOCTEXT("SalePower", "Zu viel Strom belegt: ohne das Gebäude {0} kW, belegt {1} kW"),
			FText::AsNumber(FMath::RoundToInt(Grid->GetCapacityKw() - Building->PowerCapacityKw)), FText::AsNumber(FMath::RoundToInt(Grid->GetLoadKw() * 10.f) / 10.f)));
	}
	return Blockers;
}

TArray<FText> UWEProgressionComponent::GetBuildingSaleEffects(FName Id) const
{
	TArray<FText> Effects;
	const FWEUpgradeDefinition* Building = Catalog ? Catalog->Find(Id) : nullptr;
	if (!Building)
	{
		return Effects;
	}
	Effects.Add(FText::Format(LOCTEXT("EffPrice", "Erlös {0} (gekauft für {1})"), FText::FromString(GetBuildingSalePrice(Id).ToDisplayString()),
		FText::FromString(Building->Cost.ToDisplayString())));
	if (Building->PowerCapacityKw > 0.f)
	{
		Effects.Add(FText::Format(LOCTEXT("EffPower", "Stromanschluss −{0} kW"), FText::AsNumber(FMath::RoundToInt(Building->PowerCapacityKw))));
	}
	if (Building->StaffSlots > 0)
	{
		Effects.Add(FText::Format(LOCTEXT("EffStaff", "Mitarbeiterplätze −{0}"), FText::AsNumber(Building->StaffSlots)));
	}
	int32 StorageSlots = 0;
	for (TActorIterator<AWEPlaceholderProp> It(GetWorld()); It; ++It)
	{
		if (!It->IsPlayerPlaced() && IsInsideBuilding(Id, It->GetActorLocation()))
		{
			if (const UWEInventoryComponent* Storage = It->FindComponentByClass<UWEInventoryComponent>())
			{
				StorageSlots += Storage->GetNumSlots();
			}
		}
	}
	if (StorageSlots > 0)
	{
		Effects.Add(FText::Format(LOCTEXT("EffStorage", "Lagerplätze −{0}"), FText::AsNumber(StorageSlots)));
	}
	Effects.Add(LOCTEXT("EffArea", "Die Fläche steht danach nicht mehr zum Einrichten zur Verfügung"));
	return Effects;
}

int32 UWEProgressionComponent::ClearBuildingForSale(FName Id)
{
	UWorld* World = GetWorld();
	if (!World || !GetOwner()->HasAuthority() || !CanOfferBuildingSale(Id))
	{
		return 0;
	}
	int32 Moved = 0;
	// Loose items: onto the delivery pallet (nothing is ever lost).
	TArray<AWEPickupItem*> Loose;
	for (TActorIterator<AWEPickupItem> It(World); It; ++It)
	{
		if (!It->GetHolder() && !It->GetPlacedOn() && IsInsideBuilding(Id, It->GetActorLocation()))
		{
			Loose.Add(*It);
		}
	}
	AWEDeliveryZone* Zone = AWEDeliveryZone::Find(World);
	for (AWEPickupItem* Item : Loose)
	{
		const FTransform Drop = Zone ? Zone->NextDropTransform() : FTransform(FVector(320.f, -220.f, 80.f));
		Item->TeleportTo(Drop.GetLocation(), Drop.Rotator(), false, true);
		Item->WakeForReplication();
		++Moved;
	}
	// Employees working there: task off, wait in the yard in front of the garage.
	if (UWEStaffComponent* Staff = UWEStaffComponent::Get(this))
	{
		int32 Index = 0;
		for (AWEEmployee* Employee : Staff->GetEmployees())
		{
			const FWEEmployeeTask& Task = Employee->GetTask();
			const bool bInside = IsInsideBuilding(Id, Employee->GetActorLocation());
			if (bInside || (Task.Source && IsInsideBuilding(Id, Task.Source->GetActorLocation())) || (Task.Target && IsInsideBuilding(Id, Task.Target->GetActorLocation())))
			{
				Employee->ClearTask();
				if (bInside)
				{
					Employee->TeleportTo(FVector(560.f, -60.f + 70.f * Index++, 100.f), FRotator(0.f, 180.f, 0.f), false, true);
				}
				++Moved;
			}
		}
	}
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Building %s cleared for sale: %d things moved"), *Id.ToString(), Moved);
	return Moved;
}

bool UWEProgressionComponent::RevokeBuilding(FName Id)
{
	if (!GetOwner()->HasAuthority() || GetBuildingSaleBlockers(Id).Num() > 0)
	{
		return false;
	}
	OwnedUpgrades.Remove(Id);
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Building sold: %s"), *Id.ToString());
	OnRep_Progression();
	return true;
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
	// Phase 17.5: the tutorial listens to the same gameplay events.
	if (UWETutorialComponent* Tutorial = GetOwner()->HasAuthority() ? UWETutorialComponent::Get(this) : nullptr)
	{
		switch (Event)
		{
		case EWEProgressEvent::ToolStored:        Tutorial->NotifyEvent(EWETutorialEvent::ToolStored); break;
		case EWEProgressEvent::ItemPlacedOnBench: Tutorial->NotifyEvent(EWETutorialEvent::ItemPlacedOnBench); break;
		case EWEProgressEvent::ItemSold:          Tutorial->NotifyEvent(EWETutorialEvent::ItemSold); break;
		case EWEProgressEvent::GameSaved:         Tutorial->NotifyEvent(EWETutorialEvent::GameSaved); break;
		default: break;
		}
	}
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
