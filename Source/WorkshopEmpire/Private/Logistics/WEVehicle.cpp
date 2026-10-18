#include "Logistics/WEVehicle.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WETraderTerminal.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/WEPlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Net/UnrealNetwork.h"
#include "Player/WECharacter.h"
#include "Progression/WEMilestoneComponent.h"
#include "Progression/WEProgressionComponent.h"
#include "Staff/WEEmployee.h"
#include "Staff/WEStaffComponent.h"
#include "WELog.h"

#define LOCTEXT_NAMESPACE "WEVehicle"

AWEVehicle::AWEVehicle()
{
	PropType = EWEPlaceholderPropType::Vehicle;
	Size = FVector(460.f, 200.f, 230.f);
	bHasStorage = true;
	BasicStats.DisplayName = LOCTEXT("OldVan", "Alter Transporter");
	BasicStats.CargoSlots = 20;
	BasicStats.CargoMaxKg = 600.f;
	BasicStats.TourSpeed = 1.f;
	BasicStats.Tint = FLinearColor(0.8f, 0.78f, 0.7f);
	ImprovedStats.DisplayName = LOCTEXT("ModernVan", "Moderner Lieferwagen");
	ImprovedStats.CargoSlots = 30;
	ImprovedStats.CargoMaxKg = 1200.f;
	ImprovedStats.TourSpeed = 1.6f;
	ImprovedStats.Tint = FLinearColor(0.25f, 0.45f, 0.75f);
	TruckStats.DisplayName = LOCTEXT("Truck", "Lkw mit Ladekran");
	TruckStats.CargoSlots = 40;
	TruckStats.CargoMaxKg = 3000.f;
	TruckStats.TourSpeed = 2.4f;
	TruckStats.Tint = FLinearColor(0.85f, 0.55f, 0.12f);
	RequiredUpgrade = TEXT("OldVan");
}

void AWEVehicle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWEVehicle, bAway);
	DOREPLIFETIME(AWEVehicle, bImproved);
	DOREPLIFETIME(AWEVehicle, bTruck);
	DOREPLIFETIME(AWEVehicle, SelectedTour);
	DOREPLIFETIME(AWEVehicle, ActiveTour);
	DOREPLIFETIME(AWEVehicle, ReturnAt);
	DOREPLIFETIME(AWEVehicle, LastResult);
}

void AWEVehicle::BeginPlay()
{
	Super::BeginPlay();
	bVisualAway = bAway;
	bVisualStateKnown = true;
	if (HasAuthority() && SelectedTour.IsNone())
	{
		const UWEStaffComponent* Staff = UWEStaffComponent::Get(this);
		if (Staff && Staff->GetTourCatalog() && Staff->GetTourCatalog()->Tours.Num() > 0)
		{
			SelectedTour = Staff->GetTourCatalog()->Tours[0].Id;
		}
	}
	RefreshFromUpgrades();
}

const FWEVehicleStats& AWEVehicle::GetStats() const
{
	return bTruck ? TruckStats : bImproved ? ImprovedStats : BasicStats;
}

void AWEVehicle::RefreshFromUpgrades()
{
	const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	bImproved = Progression && !ImprovedUpgrade.IsNone() && Progression->HasUpgrade(ImprovedUpgrade);
	bTruck = Progression && !TruckUpgrade.IsNone() && Progression->HasUpgrade(TruckUpgrade);
	RebuildVisuals();
}

void AWEVehicle::RebuildVisuals()
{
	const FWEVehicleStats& Stats = GetStats();
	Tint = Stats.Tint;
	StorageName = FText::Format(LOCTEXT("Cargo", "{0} – Laderaum"), Stats.DisplayName);
	StorageSlots = Stats.CargoSlots;
	StorageMaxWeightKg = Stats.CargoMaxKg;
	Super::RebuildVisuals();
	GetStorage()->SetAcceptsBulky(true);
	RebuildVanModel();
}

void AWEVehicle::SetUnlocked(bool bInUnlocked)
{
	bUnlocked = bInUnlocked;
	ApplyVisibility();
}

void AWEVehicle::OnRep_Away()
{
	RebuildVisuals();
	ApplyVisibility();
}

void AWEVehicle::ApplyVisibility()
{
	// Collision follows the gameplay state right away; what you see may still be driving.
	SetActorEnableCollision(bUnlocked && !bAway);
	const bool bAnimate = bVisualStateKnown && bUnlocked && bAway != bVisualAway && GetNetMode() != NM_DedicatedServer && GetWorld() && HasActorBegunPlay();
	if (bAnimate)
	{
		StartDriveAnimation(bAway);
		return;
	}
	if (!GetWorld() || !GetWorld()->GetTimerManager().IsTimerActive(DriveTimer))
	{
		bVisualAway = bAway;
		SetActorHiddenInGame(!(bUnlocked && !bAway));
		if (Boxes && bDriveHomeValid) { Boxes->SetRelativeLocation(DriveHome); }
	}
	bVisualStateKnown = HasActorBegunPlay();
}

void AWEVehicle::StartDriveAnimation(bool bLeaving)
{
	// The body is the root: remember where the van parks (relative = world location for a root component).
	if (Boxes && !GetWorld()->GetTimerManager().IsTimerActive(DriveTimer))
	{
		DriveHome = Boxes->GetRelativeLocation();
		bDriveHomeValid = true;
	}
	bVisualAway = bLeaving;
	bDriveLeaving = bLeaving;
	DriveStart = GetWorld()->GetTimeSeconds();
	LastDriveOffset = bLeaving ? 0.f : DriveDistance;
	SetActorHiddenInGame(false);
	if (USoundBase* Engine = LoadObject<USoundBase>(nullptr, TEXT("/Game/WorkshopEmpire/Audio/SFX_VanDrive.SFX_VanDrive")))
	{
		UGameplayStatics::SpawnSoundAttached(Engine, Boxes, NAME_None, FVector(80.f, 0.f, 60.f), EAttachLocation::KeepRelativeOffset, true);
	}
	GetWorld()->GetTimerManager().SetTimer(DriveTimer, this, &AWEVehicle::DriveTick, 1.f / 60.f, true);
	DriveTick();
}

void AWEVehicle::DriveTick()
{
	const float T = FMath::Clamp((GetWorld()->GetTimeSeconds() - DriveStart) / DriveSeconds, 0.f, 1.f);
	// Leaving: speeds up along its nose; arriving: rolls in and brakes.
	const float Along = bDriveLeaving ? T * T : 1.f - (1.f - T) * (1.f - T);
	const float Offset = bDriveLeaving ? Along * DriveDistance : (1.f - Along) * DriveDistance;
	if (Boxes)
	{
		FVector Delta(Offset, 0.f, FMath::Sin(T * 40.f) * 0.6f);
		if (Boxes == GetRootComponent())
		{
			Delta = GetActorRotation().RotateVector(Delta); // along the nose of the van in world space
		}
		Boxes->SetRelativeLocation(DriveHome + Delta);
	}
	// Wheels roll with the distance driven.
	const float Rolled = Offset - LastDriveOffset;
	LastDriveOffset = Offset;
	for (USceneComponent* Pivot : WheelPivots)
	{
		if (Pivot && WheelRadius > 1.f)
		{
			Pivot->AddLocalRotation(FRotator(-FMath::RadiansToDegrees(Rolled / WheelRadius), 0.f, 0.f));
		}
	}
	if (T >= 1.f)
	{
		GetWorld()->GetTimerManager().ClearTimer(DriveTimer);
		if (Boxes) { Boxes->SetRelativeLocation(DriveHome); }
		SetActorHiddenInGame(!(bUnlocked && !bAway));
		bVisualAway = bAway;
	}
}

const FWETourDefinition* AWEVehicle::FindTour(FName Id) const
{
	const UWEStaffComponent* Staff = UWEStaffComponent::Get(this);
	return Staff && Staff->GetTourCatalog() ? Staff->GetTourCatalog()->Find(Id) : nullptr;
}

float AWEVehicle::GetTourSeconds(const FWETourDefinition& Tour) const
{
	return Tour.Minutes * 60.f / FMath::Max(0.1f, GetStats().TourSpeed);
}

float AWEVehicle::GetSecondsToReturn() const
{
	if (!bAway)
	{
		return 0.f;
	}
	if (HasAuthority())
	{
		return Remaining;
	}
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return FMath::Max(0.f, ReturnAt - (GameState ? GameState->GetServerWorldTimeSeconds() : 0.f));
}

AWEEmployee* AWEVehicle::FindDriver() const
{
	for (TActorIterator<AWEEmployee> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && It->GetTask().Kind == EWETaskKind::DriveVehicle && It->GetTask().Target == this)
		{
			return *It;
		}
	}
	return nullptr;
}

FText AWEVehicle::GetStartBlockReason() const
{
	const FWETourDefinition* Tour = FindTour(SelectedTour);
	if (bAway)
	{
		return LOCTEXT("Away", "Unterwegs");
	}
	if (!Tour)
	{
		return LOCTEXT("NoTour", "Keine Tour gewählt ([T])");
	}
	if (const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this); Progression && Progression->GetCompanyTier() < Tour->MinTier)
	{
		return FText::Format(LOCTEXT("NeedsTier", "Benötigt Stufe {0}: {1}"), FText::AsNumber(Tour->MinTier), UWEProgressionComponent::GetTierName(Tour->MinTier));
	}
	const AWEEmployee* Driver = FindDriver();
	if (!Driver)
	{
		return LOCTEXT("NoDriver", "Kein Fahrer – Fahrer einstellen und dem Wagen zuweisen");
	}
	if (!Driver->IsReadyToDrive())
	{
		return FText::Format(LOCTEXT("DriverNotReady", "{0} ist noch nicht am Wagen"), FText::FromString(Driver->GetProfile().Name));
	}
	if (Tour->Kind != EWETourKind::Pickup && GetStorage()->GetAllSlots().Num() == 0)
	{
		return LOCTEXT("Empty", "Laderaum leer");
	}
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	const int64 Cost = Tour->FuelCost.GetCents() + (Tour->Kind == EWETourKind::Pickup ? Tour->PickupPrice.GetCents() : 0);
	if (!Company || !Company->CanAfford(FWEMoney::FromCents(Cost)))
	{
		return FText::Format(LOCTEXT("NoMoney", "Nicht genug Geld ({0})"), FText::FromString(FWEMoney::FromCents(Cost).ToDisplayString()));
	}
	return FText::GetEmpty();
}

bool AWEVehicle::CanLoad(const AWEPickupItem* Item, FText& OutReason) const
{
	if (!Item || !Item->GetItem().IsValid() || bAway || !bUnlocked)
	{
		return false;
	}
	if (Item->GetItem().IsCustomerItem())
	{
		OutReason = LOCTEXT("Customer", "Kundengerät – nicht einladen");
		return false;
	}
	if (GetStorage()->GetAddableQuantity(Item->GetItem()) < Item->GetItem().Quantity)
	{
		OutReason = LOCTEXT("Full", "Laderaum voll");
		return false;
	}
	return true;
}

bool AWEVehicle::LoadItem(AWEPickupItem* Item)
{
	FText Reason;
	if (!HasAuthority() || !CanLoad(Item, Reason))
	{
		return false;
	}
	int32 Added = 0;
	GetStorage()->TryAddItem(Item->GetItem(), Added);
	if (Added <= 0)
	{
		return false;
	}
	if (AWECharacter* Holder = Item->GetHolder())
	{
		Holder->ReleaseHeldItem();
	}
	UE_LOG(LogWENet, Log, TEXT("%s: %s loaded"), *GetName(), *Item->GetDisplayName().ToString());
	Item->Destroy();
	return true;
}

int32 AWEVehicle::UnloadBulky()
{
	if (!HasAuthority() || bAway)
	{
		return 0;
	}
	int32 Count = 0;
	const TArray<FWEInventorySlotData> Slots = GetStorage()->GetAllSlots();
	for (const FWEInventorySlotData& Slot : Slots)
	{
		if (!Slot.Item.Definition || Slot.Item.Definition->bFitsInInventory)
		{
			continue;
		}
		FWEItemInstance Taken;
		if (GetStorage()->RemoveFromSlot(Slot.SlotIndex, Slot.Item.Quantity, Taken) == EWEInventoryResult::Success)
		{
			// Behind the van (its back faces -X), side by side.
			const FVector At = GetActorTransform().TransformPosition(FVector(-Size.X * 0.5f - 90.f, -Size.Y * 0.4f + (Count % 4) * 60.f, 50.f));
			AWEPickupItem::SpawnWorldItem(GetWorld(), Taken, FTransform(GetActorRotation(), At));
			++Count;
		}
	}
	return Count;
}

void AWEVehicle::CycleTour()
{
	const UWEStaffComponent* Staff = UWEStaffComponent::Get(this);
	const UWETourCatalog* Catalog = Staff ? Staff->GetTourCatalog() : nullptr;
	if (!HasAuthority() || !Catalog || Catalog->Tours.Num() == 0 || bAway)
	{
		return;
	}
	const int32 Index = Catalog->Tours.IndexOfByPredicate([this](const FWETourDefinition& T) { return T.Id == SelectedTour; });
	SelectedTour = Catalog->Tours[(Index + 1) % Catalog->Tours.Num()].Id;
	ForceNetUpdate();
}

bool AWEVehicle::StartTour(AWECharacter* By)
{
	const FText Reason = GetStartBlockReason();
	const FWETourDefinition* Tour = FindTour(SelectedTour);
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	AWEEmployee* Driver = FindDriver();
	if (!HasAuthority() || !Reason.IsEmpty() || !Tour || !Company || !Driver)
	{
		if (AWEPlayerController* PC = By ? Cast<AWEPlayerController>(By->GetController()) : nullptr; PC && !Reason.IsEmpty())
		{
			PC->ClientShowMessage(Reason);
		}
		return false;
	}
	const FString Who = By && By->GetPlayerState() ? By->GetPlayerState()->GetPlayerName() : FString();
	if (!Tour->FuelCost.IsZero() && !Company->TryWithdraw(Tour->FuelCost, FString::Printf(TEXT("Tankfüllung: %s"), *Tour->DisplayName.ToString()), Who))
	{
		return false;
	}
	if (Tour->Kind == EWETourKind::Pickup && !Tour->PickupPrice.IsZero()
		&& !Company->TryWithdraw(Tour->PickupPrice, FString::Printf(TEXT("Großeinkauf: %s"), *Tour->DisplayName.ToString()), Who))
	{
		Company->Deposit(Tour->FuelCost, TEXT("Rückbuchung Tankfüllung"), Who);
		return false;
	}
	ActiveTour = Tour->Id;
	Remaining = GetTourSeconds(*Tour);
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	ReturnAt = (GameState ? GameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds()) + Remaining;
	bAway = true;
	Driver->SetAway(true);
	ApplyVisibility();
	ForceNetUpdate();
	Notify(FText::Format(LOCTEXT("Departed", "{0} ist losgefahren: {1} (zurück in {2} min)"), FText::FromString(Driver->GetProfile().Name),
		Tour->DisplayName, FText::AsNumber(FMath::CeilToInt(Remaining / 60.f))));
	UE_LOG(LogWENet, Log, TEXT("%s: tour '%s' started (%.0f s)"), *GetName(), *Tour->DisplayName.ToString(), Remaining);
	return true;
}

void AWEVehicle::StepTour(float DeltaSeconds)
{
	if (!HasAuthority() || !bAway)
	{
		return;
	}
	Remaining -= DeltaSeconds;
	if (Remaining <= 0.f)
	{
		FinishTour();
	}
}

void AWEVehicle::FinishTour()
{
	const FWETourDefinition* Tour = FindTour(ActiveTour);
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!HasAuthority() || !bAway)
	{
		return;
	}
	FString Result;
	if (Tour && Tour->Kind == EWETourKind::Sell && Company)
	{
		// Direct delivery: best trader price plus the tour bonus; what nobody buys comes back.
		const AWETraderTerminal* Terminal = nullptr;
		for (TActorIterator<AWETraderTerminal> It(GetWorld()); It && !Terminal; ++It)
		{
			Terminal = *It;
		}
		int64 Cents = 0;
		int32 Units = 0;
		const TArray<FWEInventorySlotData> Slots = GetStorage()->GetAllSlots();
		for (const FWEInventorySlotData& Slot : Slots)
		{
			const FWEMoney Price = Terminal ? Terminal->GetBestSellPrice(Slot.Item) : FWEMoney();
			if (Price.IsZero() || Slot.Item.IsCustomerItem() || Slot.Item.IsSaleProtected())
			{
				continue;
			}
			FWEItemInstance Sold;
			if (GetStorage()->RemoveFromSlot(Slot.SlotIndex, Slot.Item.Quantity, Sold) == EWEInventoryResult::Success)
			{
				Cents += FMath::RoundToInt64(Price.GetCents() * Tour->SellFactor);
				Units += Sold.Quantity;
			}
		}
		if (Cents > 0)
		{
			Company->Deposit(FWEMoney::FromCents(Cents), FString::Printf(TEXT("Verkaufsfahrt: %s (%d Stück)"), *Tour->DisplayName.ToString(), Units), FString());
		}
		Result = FString::Printf(TEXT("%s: %d Stück für %s verkauft"), *Tour->DisplayName.ToString(), Units, *FWEMoney::FromCents(Cents).ToDisplayString());
	}
	else if (Tour && Tour->Kind == EWETourKind::Deliver)
	{
		// Every accepted order whose goods are complete in the cargo gets one delivery.
		TArray<FString> Delivered;
		if (UWEContractComponent* Contracts = UWEContractComponent::Get(this))
		{
			const TArray<FWEContract> Open = Contracts->GetContracts();
			for (const FWEContract& Contract : Open)
			{
				if (Contract.State == EWEContractState::Accepted && Contract.Kind == EWEContractKind::Supply
					&& Contracts->DeliverFromStorage(Contract.Id, GetStorage(), FString()) == EWEContractResult::Success)
				{
					Delivered.Add(Contract.Title.ToString());
				}
			}
		}
		Result = Delivered.Num() > 0 ? FString::Printf(TEXT("%s: %s"), *Tour->DisplayName.ToString(), *FString::Join(Delivered, TEXT(", ")))
			: FString::Printf(TEXT("%s: nichts Passendes geladen"), *Tour->DisplayName.ToString());
	}
	else if (Tour && Tour->Kind == EWETourKind::Pickup)
	{
		int32 Units = 0;
		int32 Index = 0;
		for (const FWETourItem& Entry : Tour->PickupItems)
		{
			if (Entry.Chance < 1.f && FMath::FRand() > Entry.Chance)
			{
				continue; // a rare find that is not in this lot
			}
			for (int32 Left = Entry.Item ? Entry.Quantity : 0; Left > 0;)
			{
				FWEItemInstance Item = FWEItemInstance::Make(Entry.Item, FMath::Min(Left, Entry.Item->GetMaxStackSize()), Entry.Condition, static_cast<uint8>(Entry.Quality));
				Left -= Item.Quantity;
				Units += Item.Quantity;
				int32 Added = 0;
				GetStorage()->TryAddItem(Item, Added);
				if (Added < Item.Quantity)
				{
					// Cargo full: unloaded next to the van.
					Item.Quantity -= Added;
					const FVector At = GetActorTransform().TransformPosition(FVector(-Size.X * 0.5f - 60.f, -Size.Y * 0.4f + (Index++ % 5) * 50.f, 40.f));
					AWEPickupItem::SpawnWorldItem(GetWorld(), Item, FTransform(At));
				}
			}
		}
		Result = FString::Printf(TEXT("%s: %d Teile im Laderaum"), *Tour->DisplayName.ToString(), Units);
	}
	if (Tour)
	{
		if (UWEMilestoneComponent* Milestones = UWEMilestoneComponent::Get(this)) { Milestones->AddCount(EWEMilestoneKind::ToursCompleted); }
	}
	bAway = false;
	ActiveTour = NAME_None;
	Remaining = 0.f;
	ReturnAt = 0.f;
	LastResult = Result;
	ApplyVisibility();
	if (AWEEmployee* Driver = FindDriver())
	{
		Driver->SetAway(false, GetActorTransform().TransformPosition(FVector(0.f, Size.Y * 0.5f + 70.f, 0.f)));
	}
	ForceNetUpdate();
	Notify(FText::Format(LOCTEXT("Returned", "Der Wagen ist zurück – {0}"), FText::FromString(Result)));
	UE_LOG(LogWENet, Log, TEXT("%s: tour finished: %s"), *GetName(), *Result);
}

void AWEVehicle::RestoreTour(FName InSelectedTour, FName InActiveTour, float RemainingSeconds)
{
	if (!HasAuthority())
	{
		return;
	}
	if (!InSelectedTour.IsNone())
	{
		SelectedTour = InSelectedTour;
	}
	ActiveTour = InActiveTour;
	bAway = !InActiveTour.IsNone();
	Remaining = bAway ? FMath::Max(0.f, RemainingSeconds) : 0.f;
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	ReturnAt = bAway ? (GameState ? GameState->GetServerWorldTimeSeconds() : 0.f) + Remaining : 0.f;
	ApplyVisibility();
	ForceNetUpdate();
}

void AWEVehicle::Notify(const FText& Message) const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(It->Get()))
		{
			PC->ClientShowMessage(Message);
		}
	}
}

// --- Interaction ---------------------------------------------------------------------------------------

FText AWEVehicle::GetInteractionText(const AWECharacter* Interactor) const
{
	return FText::Format(LOCTEXT("OpenCargo", "Laderaum öffnen ({0})"), GetStats().DisplayName);
}

bool AWEVehicle::CanInteract(const AWECharacter* Interactor, FText& OutReason) const
{
	return Interactor && bUnlocked && !bAway;
}

bool AWEVehicle::GetSecondaryInteraction(const AWECharacter* Interactor, FText& OutText) const
{
	if (!Interactor || !bUnlocked || bAway || Interactor->GetHeldItem())
	{
		return false;
	}
	int32 Bulky = 0;
	for (const FWEInventorySlotData& Slot : GetStorage()->GetAllSlots())
	{
		Bulky += Slot.Item.Definition && !Slot.Item.Definition->bFitsInInventory ? Slot.Item.Quantity : 0;
	}
	if (Bulky == 0)
	{
		return false;
	}
	OutText = FText::Format(LOCTEXT("Unload", "Sperriges ausladen ({0})"), FText::AsNumber(Bulky));
	return true;
}

void AWEVehicle::InteractSecondary(AWECharacter* Interactor)
{
	UnloadBulky();
}

bool AWEVehicle::GetAlternativeInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const
{
	const FWETourDefinition* Tour = FindTour(SelectedTour);
	if (!Interactor || !bUnlocked || bAway || !Tour)
	{
		return false;
	}
	OutReason = GetStartBlockReason();
	if (!OutReason.IsEmpty())
	{
		OutReason = FText::Format(LOCTEXT("TourBlocked", "Tour: {0}"), OutReason);
		return false;
	}
	OutText = FText::Format(LOCTEXT("StartTour", "Tour starten: {0}"), Tour->DisplayName);
	return true;
}

void AWEVehicle::InteractAlternative(AWECharacter* Interactor)
{
	StartTour(Interactor);
}

bool AWEVehicle::GetSettingInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const
{
	const FWETourDefinition* Tour = FindTour(SelectedTour);
	if (!Interactor || !bUnlocked || bAway)
	{
		return false;
	}
	OutText = FText::Format(LOCTEXT("ChooseTour", "Tour: {0}"), Tour ? Tour->DisplayName : LOCTEXT("None", "–"));
	return true;
}

void AWEVehicle::InteractSetting(AWECharacter* Interactor)
{
	CycleTour();
}

bool AWEVehicle::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	const FWEVehicleStats& Stats = GetStats();
	const FWETourDefinition* Tour = FindTour(SelectedTour);
	const AWEEmployee* Driver = FindDriver();
	OutInfo.Title = Stats.DisplayName;
	OutInfo.Description = LOCTEXT("Desc", "Firmenwagen für Touren: Ware direkt beim Händler verkaufen (besserer Preis) oder Großeinkäufe abholen. Braucht einen Fahrer. [E] Laderaum, [T] Tour wählen, [Q] losfahren.");
	OutInfo.Details.Reset();
	FNumberFormattingOptions NoGrouping; // "3000 kg" - the engine culture would write "3,000"
	NoGrouping.SetUseGrouping(false);
	OutInfo.Details.Add(FText::Format(LOCTEXT("ExCargo", "Laderaum: {0} / {1} Plätze, {2} / {3} kg"), FText::AsNumber(GetStorage()->GetAllSlots().Num()),
		FText::AsNumber(GetStorage()->GetNumSlots()), FText::AsNumber(FMath::RoundToInt(GetStorage()->GetTotalWeightKg()), &NoGrouping),
		FText::AsNumber(FMath::RoundToInt(GetStorage()->GetMaxWeightKg()), &NoGrouping)));
	OutInfo.Details.Add(FText::Format(LOCTEXT("ExDriver", "Fahrer: {0}"), Driver ? FText::FromString(Driver->GetProfile().Name) : LOCTEXT("NoDriverShort", "keiner")));
	if (Tour)
	{
		OutInfo.Details.Add(FText::Format(LOCTEXT("ExTour", "Tour: {0} – {1} min, Sprit {2}{3}"), Tour->DisplayName, FText::AsNumber(FMath::CeilToInt(GetTourSeconds(*Tour) / 60.f)),
			FText::FromString(Tour->FuelCost.ToDisplayString()),
			Tour->Kind == EWETourKind::Pickup ? FText::Format(LOCTEXT("ExPrice", ", Einkauf {0}"), FText::FromString(Tour->PickupPrice.ToDisplayString())) : FText::GetEmpty()));
		OutInfo.Details.Add(Tour->Description);
	}
	if (!LastResult.IsEmpty())
	{
		OutInfo.Details.Add(FText::Format(LOCTEXT("ExLast", "Letzte Tour: {0}"), FText::FromString(LastResult)));
	}
	return true;
}

void AWEVehicle::RebuildVanModel()
{
	// 0 = old van, 1 = modern van, 2 = truck (the modern body, a size bigger).
	const int32 Kind = bTruck ? 2 : bImproved ? 1 : 0;
	if (Kind == VanModelKind && VanParts.Num() > 0)
	{
		Boxes->SetVisibility(false);
		return;
	}
	for (UStaticMeshComponent* Part : VanParts) { if (Part) { Part->DestroyComponent(); } }
	for (USceneComponent* Pivot : WheelPivots) { if (Pivot) { Pivot->DestroyComponent(); } }
	VanParts.Reset();
	WheelPivots.Reset();
	VanModelKind = Kind;
	UStaticMesh* Body = LoadObject<UStaticMesh>(nullptr, Kind == 0 ? TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_VanOld.SM_Gen_VanOld")
		: TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_VanModern.SM_Gen_VanModern"), nullptr, LOAD_Quiet | LOAD_NoWarn);
	UStaticMesh* Wheel = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_VanWheel.SM_Gen_VanWheel"), nullptr, LOAD_Quiet | LOAD_NoWarn);
	if (!Body || !Wheel)
	{
		Boxes->SetVisibility(true); // models not built: the box van stays
		return;
	}
	Boxes->SetVisibility(false);
	// The model is 540 cm long with its nose at +X; wheel centres 165 cm before/behind the middle, 24 cm above
	// the body's lowest point; the wheel model (radius 41 cm) stands on its own origin.
	const float Scale = Size.X / 540.f * (Kind == 2 ? 1.15f : 1.f);
	WheelRadius = 41.f * Scale;
	auto Make = [this](UStaticMesh* Mesh, USceneComponent* Parent, const FTransform& Relative)
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		Part->SetStaticMesh(Mesh);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetupAttachment(Parent);
		Part->SetRelativeTransform(Relative);
		Part->RegisterComponent();
		VanParts.Add(Part);
		return Part;
	};
	Make(Body, Boxes, FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, (41.f - 24.f) * Scale), FVector(Scale)));
	for (const float X : { -165.f, 165.f })
	{
		for (const float Y : { -1.f, 1.f })
		{
			USceneComponent* Pivot = NewObject<USceneComponent>(this, NAME_None, RF_Transient);
			Pivot->SetupAttachment(Boxes); // moves with the body when it drives
			Pivot->SetRelativeLocation(FVector(X * Scale, Y * 87.f * Scale, WheelRadius));
			Pivot->RegisterComponent();
			WheelPivots.Add(Pivot);
			// Hub cap outwards: the model's cap faces -Y.
			Make(Wheel, Pivot, FTransform(FRotator(0.f, Y > 0.f ? 180.f : 0.f, 0.f), FVector(0.f, 0.f, -WheelRadius), FVector(Scale)));
		}
	}
}
#undef LOCTEXT_NAMESPACE
