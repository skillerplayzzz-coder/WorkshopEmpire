#include "Save/WESaveSystem.h"
#include "Workshop/WEDisassemblyDefinition.h"
#include "Framework/WEGameMode.h"

#include "Economy/WEAuctionComponent.h"
#include "Machines/WEMachine.h"
#include "Machines/WEConveyor.h"
#include "Machines/WEProductionStorage.h"
#include "Staff/WEEmployee.h"
#include "Staff/WEStaffComponent.h"
#include "Logistics/WEVehicle.h"
#include "Logistics/WEForklift.h"
#include "Machines/WEPowerGridComponent.h"
#include "Workshop/WEWorkshopLayoutComponent.h"
#include "World/WEPlaceholderProp.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/FileManager.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Player/WECharacter.h"
#include "Save/WESaveGame.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "WELog.h"
#include "World/WEGarageDoor.h"
#include "Workshop/WEWorkbench.h"
#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WEMarketComponent.h"
#include "Economy/WETraderTerminal.h"
#include "Progression/WEMilestoneComponent.h"
#include "Progression/WEProgressionComponent.h"
#include "Progression/WETutorialComponent.h"
#include "Workshop/WEWorkshopStyleComponent.h"

#define LOCTEXT_NAMESPACE "WESaveSystem"

// --- Paths -------------------------------------------------------------------------------------------

bool FWESaveSystem::IsValidSlotName(const FString& Slot)
{
	if (Slot.IsEmpty() || Slot.Len() > 32)
	{
		return false;
	}
	for (const TCHAR C : Slot)
	{
		if (!FChar::IsAlnum(C) && C != TEXT('_') && C != TEXT('-'))
		{
			return false;
		}
	}
	return true;
}

FString FWESaveSystem::GetSaveDirectory()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), TEXT("WorkshopEmpire"));
}

FString FWESaveSystem::GetSlotPath(const FString& Slot)
{
	return FPaths::Combine(GetSaveDirectory(), Slot + TEXT(".wesave"));
}

// --- Item conversion -----------------------------------------------------------------------------

FWESavedItem FWESaveSystem::ToSaved(const FWEItemInstance& Item)
{
	FWESavedItem Saved;
	Saved.ItemId = Item.Definition ? Item.Definition->ItemId : NAME_None;
	Saved.InstanceId = Item.InstanceId;
	Saved.Quantity = Item.Quantity;
	Saved.Condition = Item.Condition;
	Saved.Quality = Item.Quality;
	Saved.DisassemblyStep = Item.DisassemblyStep;
	Saved.RepairFaults = Item.Repair.Faults;
	Saved.RepairRemoved = Item.Repair.Removed;
	Saved.RepairReplaced = Item.Repair.Replaced;
	Saved.RepairFlags = Item.Repair.Flags;
	Saved.Marks = Item.Marks;
	Saved.RemovedParts = Item.RemovedParts;
	return Saved;
}

bool FWESaveSystem::FromSaved(const FWESavedItem& Saved, FWEItemInstance& OutItem)
{
	UWEItemDefinition* Definition = UWEItemDefinition::FindById(Saved.ItemId);
	if (!Definition || Saved.Quantity <= 0)
	{
		UE_LOG(LogWorkshopEmpire, Warning, TEXT("Save: unknown or invalid item '%s' skipped"), *Saved.ItemId.ToString());
		return false;
	}
	OutItem.Definition = Definition;
	OutItem.InstanceId = Saved.InstanceId.IsValid() ? Saved.InstanceId : FGuid::NewGuid();
	OutItem.Quantity = FMath::Min(Saved.Quantity, Definition->GetMaxStackSize());
	OutItem.Condition = Saved.Condition;
	OutItem.Quality = FMath::Min<uint8>(Saved.Quality, 100);
	OutItem.DisassemblyStep = Saved.DisassemblyStep;
	OutItem.Repair.Faults = Saved.RepairFaults;
	OutItem.Repair.Removed = Saved.RepairRemoved;
	OutItem.Repair.Replaced = Saved.RepairReplaced;
	OutItem.Repair.Flags = Saved.RepairFlags;
	OutItem.Marks = Saved.Marks & (FWEItemInstance::Favorite | FWEItemInstance::Locked | FWEItemInstance::ReleasedForSale);
	OutItem.RemovedParts = Saved.RemovedParts;
	if (Definition->HasAssembly())
	{
		const int32 NumParts = Definition->Disassembly->NumSteps();
		if (OutItem.RemovedParts == 0 && OutItem.DisassemblyStep > 0)
		{
			// Saved before version 15 (steps in a fixed order): the first DisassemblyStep parts are out.
			for (int32 i = 0; i < FMath::Min<int32>(OutItem.DisassemblyStep, NumParts); ++i)
			{
				OutItem.RemovedParts |= (1 << i);
			}
		}
		OutItem.RemovedParts &= (1 << NumParts) - 1;
		OutItem.DisassemblyStep = static_cast<uint8>(FMath::CountBits(static_cast<uint32>(OutItem.RemovedParts)));
	}
	return true;
}

TArray<FWESavedSlot> FWESaveSystem::ToSavedSlots(const TArray<FWEInventorySlotData>& Slots)
{
	TArray<FWESavedSlot> Result;
	for (const FWEInventorySlotData& Slot : Slots)
	{
		Result.Add({ Slot.SlotIndex, ToSaved(Slot.Item) });
	}
	return Result;
}

TArray<FWEInventorySlotData> FWESaveSystem::FromSavedSlots(const TArray<FWESavedSlot>& Slots)
{
	TArray<FWEInventorySlotData> Result;
	for (const FWESavedSlot& Slot : Slots)
	{
		FWEItemInstance Item;
		if (FromSaved(Slot.Item, Item))
		{
			Result.Add({ Slot.SlotIndex, Item });
		}
	}
	return Result;
}

// --- Capture / apply -------------------------------------------------------------------------------

FString FWESaveSystem::GetPlayerKey(const APlayerController* PC)
{
	if (!PC)
	{
		return FString();
	}
	if (PC->IsLocalController())
	{
		return TEXT("Host");
	}
	const APlayerState* PS = PC->PlayerState;
	if (PS && PS->GetUniqueId().IsValid())
	{
		return PS->GetUniqueId().ToString();
	}
	return PS ? PS->GetPlayerName() : PC->GetName();
}

FWESavedPlayer FWESaveSystem::CapturePlayer(APlayerController* PC)
{
	FWESavedPlayer Player;
	const AWECharacter* Character = PC ? Cast<AWECharacter>(PC->GetPawn()) : nullptr;
	if (!Character)
	{
		return Player;
	}
	Player.PlayerKey = GetPlayerKey(PC);
	Player.PlayerName = PC->PlayerState ? PC->PlayerState->GetPlayerName() : FString();
	Player.PawnTransform = Character->GetActorTransform();
	Player.ControlRotation = PC->GetControlRotation();
	Player.bHasLook = Character->HasLook();
	Player.Look = Character->GetLook();
	if (const UWEInventoryComponent* Inventory = Character->GetPlayerInventory())
	{
		Player.Inventory = ToSavedSlots(Inventory->GetAllSlots());
	}
	return Player;
}

void FWESaveSystem::AddDepartedPlayers(UWESaveGame* Save, const TArray<FWESavedPlayer>& Departed)
{
	for (const FWESavedPlayer& Player : Departed)
	{
		if (Save && !FindPlayer(Save->Players, Player.PlayerKey, FString()))
		{
			Save->Players.Add(Player);
		}
	}
}

UWESaveGame* FWESaveSystem::Capture(UWorld* World)
{
	UWESaveGame* Save = NewObject<UWESaveGame>();
	if (!World)
	{
		return Save;
	}
	Save->MapName = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());
	Save->SavedAtUtc = FDateTime::UtcNow();

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && Cast<AWECharacter>(PC->GetPawn()))
		{
			Save->Players.Add(CapturePlayer(PC));
		}
	}

	// Players who left the session keep their place and inventory in the save (Phase 13).
	if (const AWEGameMode* GameMode = World->GetAuthGameMode<AWEGameMode>())
	{
		AddDepartedPlayers(Save, GameMode->GetDepartedPlayers());
	}

	for (TActorIterator<AWEPickupItem> It(World); It; ++It)
	{
		AWEPickupItem* Item = *It;
		if (!IsValid(Item) || !Item->GetItem().IsValid())
		{
			continue;
		}
		FWESavedWorldItem& Saved = Save->WorldItems.AddDefaulted_GetRef();
		Saved.Item = ToSaved(Item->GetItem());
		// Held items are stored where the holder would put them down.
		Saved.Transform = Item->GetHolder() ? Item->GetHolder()->ComputeDropTransform(Item) : Item->GetActorTransform();
		Saved.Transform.SetScale3D(FVector::OneVector);
		Saved.PlacedOn = Item->GetPlacedOn() ? Item->GetPlacedOn()->GetFName() : NAME_None;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->IsA<APlayerState>())
		{
			continue; // player inventories are saved with the player
		}
		TArray<UWEInventoryComponent*> Inventories;
		Actor->GetComponents(Inventories);
		for (const UWEInventoryComponent* Inventory : Inventories)
		{
			FWESavedContainer& Container = Save->Containers.AddDefaulted_GetRef();
			Container.ActorName = Actor->GetFName();
			Container.ComponentName = Inventory->GetFName();
			Container.Slots = ToSavedSlots(Inventory->GetAllSlots());
		}
	}

	// Machines and power (version 8).
	for (TActorIterator<AWEMachine> It(World); It; ++It)
	{
		FWESavedMachine& Saved = Save->Machines.AddDefaulted_GetRef();
		Saved.ActorName = It->GetFName();
		Saved.bOn = It->IsOn();
		Saved.Maintenance = It->GetStatus().Maintenance;
		Saved.RecipeIndex = It->GetStatus().RecipeIndex;
		Saved.Progress = It->GetProgress();
		Saved.CycleSeconds = It->GetStatus().CycleSeconds;
		Saved.SelectedRecipe = It->GetSelectedRecipe();
		for (const FWEItemInstance& Item : It->GetProcessing())
		{
			Saved.Processing.Add(ToSaved(Item));
		}
	}
	// Conveyors and material stores (version 9).
	for (TActorIterator<AWEConveyor> It(World); It; ++It)
	{
		FWESavedConveyor& Saved = Save->Conveyors.AddDefaulted_GetRef();
		Saved.ActorName = It->GetFName();
		Saved.bOn = It->IsOn();
		for (const FWEConveyorItem& Entry : It->GetItems())
		{
			Saved.Items.Add(ToSaved(Entry.Item));
			Saved.Positions.Add(Entry.Position);
		}
	}
	for (TActorIterator<AWEProductionStorage> It(World); It; ++It)
	{
		Save->StoreOutputs.Add({ It->GetFName(), It->IsOutputOn(), It->GetOutputFilter() ? It->GetOutputFilter()->ItemId : NAME_None });
	}
	// Staff and vans (version 10).
	if (const UWEStaffComponent* Staff = UWEStaffComponent::Get(World))
	{
		Save->bHasStaff = true;
		Save->Applicants = Staff->GetApplicants();
		Save->OwedWagesCents = Staff->GetOwedWages().GetCents();
		for (const AWEEmployee* Employee : Staff->GetEmployees())
		{
			FWESavedEmployee& Saved = Save->Employees.AddDefaulted_GetRef();
			Saved.ActorName = Employee->GetFName();
			Saved.Profile = Employee->GetProfile();
			Saved.Location = Employee->GetActorLocation();
			Saved.Yaw = Employee->GetActorRotation().Yaw;
			Saved.TaskKind = static_cast<uint8>(Employee->GetTask().Kind);
			Saved.SourceName = Employee->GetTask().Source ? Employee->GetTask().Source->GetFName() : NAME_None;
			Saved.TargetName = Employee->GetTask().Target ? Employee->GetTask().Target->GetFName() : NAME_None;
			Saved.FilterItemId = Employee->GetTask().Filter ? Employee->GetTask().Filter->ItemId : NAME_None;
			Saved.bPaused = Employee->IsPaused();
			Saved.bHasCarried = Employee->GetCarried().IsValid();
			if (Saved.bHasCarried)
			{
				Saved.Carried = ToSaved(Employee->GetCarried());
			}
			Saved.UnpaidSeconds = Employee->GetUnpaidSeconds();
		}
	}
	for (TActorIterator<AWEVehicle> It(World); It; ++It)
	{
		Save->Vehicles.Add({ It->GetFName(), It->GetSelectedTour(), It->GetActiveTour(), It->GetRemainingSeconds() });
	}
	if (const UWECompanyComponent* Books = UWECompanyComponent::Get(World))
	{
		Save->BookTotals = Books->GetCategoryTotals();
	}
	if (const UWEMilestoneComponent* Milestones = UWEMilestoneComponent::Get(World))
	{
		Save->bHasMilestones = true;
		Save->CompanyStats = Milestones->GetStats();
		Save->CompletedMilestones = Milestones->GetCompleted();
	}
	if (const UWEPowerGridComponent* Grid = UWEPowerGridComponent::Get(World))
	{
		Save->PowerUnbilledKWh = Grid->GetUnbilledKWh();
	}

	for (TActorIterator<AWEGarageDoor> It(World); It; ++It)
	{
		Save->Doors.Add({ It->GetFName(), It->IsOpen() });
	}

	// Economy (version 3).
	if (const UWECompanyComponent* Company = UWECompanyComponent::Get(World))
	{
		Save->CompanyBalanceCents = Company->GetBalance().GetCents();
	}
	if (const UWEMarketComponent* Market = UWEMarketComponent::Get(World))
	{
		Save->MarketDemand = Market->GetAllDemand();
	}
	if (const UWEContractComponent* Contracts = UWEContractComponent::Get(World))
	{
		const float Now = Contracts->GetServerTime();
		for (const FWEContract& Contract : Contracts->GetContracts())
		{
			FWESavedContract& Saved = Save->Contracts.AddDefaulted_GetRef();
			Saved.Id = Contract.Id;
			Saved.Title = Contract.Title.ToString();
			Saved.ItemId = Contract.Item ? Contract.Item->ItemId : NAME_None;
			Saved.Quantity = Contract.Quantity;
			Saved.MinCondition = Contract.MinCondition;
			Saved.RewardCents = Contract.Reward.GetCents();
			Saved.bAccepted = Contract.State == EWEContractState::Accepted;
			Saved.RemainingSeconds = FMath::Max(0.f, Contract.ExpiresAt - Now);
			Saved.DeliverySeconds = Contract.DeliverySeconds;
			Saved.AcceptedBy = Contract.AcceptedBy;
			Saved.Kind = static_cast<uint8>(Contract.Kind);
			Saved.CustomerCondition = Contract.CustomerCondition;
			Saved.CustomerItemId = Contract.CustomerItemId;
			Saved.bIndustrial = Contract.bIndustrial;
			Saved.Rounds = Contract.Rounds;
			Saved.RoundsDone = Contract.RoundsDone;
			Saved.MinQuality = Contract.MinQuality;
		}
	}
	for (TActorIterator<AWETraderTerminal> It(World); It; ++It)
	{
		FWESavedStock& Stock = Save->Stocks.AddDefaulted_GetRef();
		Stock.ActorName = It->GetFName();
		for (const FWEStockLine& Line : It->GetStockLines())
		{
			FWESavedStockLine& Saved = Stock.Lines.AddDefaulted_GetRef();
			Saved.TraderIndex = Line.TraderIndex;
			Saved.EntryIndex = Line.EntryIndex;
			Saved.Item = ToSaved(Line.Item);
			Saved.Remaining = Line.Remaining;
		}
	}

	// Furniture (version 7): every placed piece, so containers can be matched by name after loading.
	Save->bHasFurniture = true;
	for (TActorIterator<AWEPlaceholderProp> It(World); It; ++It)
	{
		Save->Furniture.Add({ It->GetFName(), It->GetFurnitureId(), It->GetActorTransform() });
	}

	// Auctions (version 6).
	if (const UWEAuctionComponent* Auctions = UWEAuctionComponent::Get(World))
	{
		Save->bHasAuctions = true;
		const float Now = Auctions->GetServerTime();
		for (const FWEAuctionLot& Lot : Auctions->GetLots())
		{
			FWESavedAuctionLot& Saved = Save->Auctions.AddDefaulted_GetRef();
			Saved.Id = Lot.Id;
			Saved.Title = Lot.Title.ToString();
			for (const FWEItemInstance& Item : Lot.Items)
			{
				Saved.Items.Add(ToSaved(Item));
			}
			Saved.EstimatedCents = Lot.EstimatedValue.GetCents();
			Saved.CurrentBidCents = Lot.CurrentBid.GetCents();
			Saved.NumBids = Lot.NumBids;
			Saved.bCompanyLeads = Lot.bCompanyLeads;
			Saved.LeaderName = Lot.LeaderName;
			Saved.RemainingSeconds = FMath::Max(0.f, Lot.EndsAt - Now);
			Saved.BidderMaxCents = Auctions->GetBidderMaxCents(Lot.Id);
		}
	}

	// Progression (version 4).
	if (const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(World))
	{
		Save->bHasProgression = true;
		Save->OwnedUpgrades = Progression->GetOwnedUpgrades();
		Save->GuideStep = Progression->GetGuideStep();
	}
	// Workshop look and tutorial (version 14).
	if (const UWEWorkshopStyleComponent* Style = UWEWorkshopStyleComponent::Get(World))
	{
		Save->bHasStyle = true;
		Save->ZoneStyles = Style->GetAllStyles();
	}
	if (const UWETutorialComponent* Tutorial = UWETutorialComponent::Get(World))
	{
		Save->TutorialState = static_cast<uint8>(Tutorial->GetState());
		Save->TutorialStep = Tutorial->GetStep();
	}
	return Save;
}

void FWESaveSystem::Apply(UWorld* World, const UWESaveGame* Save)
{
	if (!World || !Save)
	{
		return;
	}

	// Nobody keeps driving a forklift through a load (it is restored where it was saved).
	for (TActorIterator<AWEForklift> It(World); It; ++It)
	{
		It->Dismount();
	}

	// 0) Furniture first: items lying on benches and container contents refer to furniture by name.
	if (Save->bHasFurniture)
	{
		TSet<FName> SavedNames;
		for (const FWESavedFurniture& Saved : Save->Furniture)
		{
			SavedNames.Add(Saved.ActorName);
		}
		TArray<AWEPlaceholderProp*> ToRemove;
		for (TActorIterator<AWEPlaceholderProp> It(World); It; ++It)
		{
			if (It->IsPlayerPlaced() && !SavedNames.Contains(It->GetFName()))
			{
				ToRemove.Add(*It);
			}
		}
		for (AWEPlaceholderProp* Prop : ToRemove)
		{
			if (AWEWorkbench* Bench = Cast<AWEWorkbench>(Prop))
			{
				Bench->ResetForLoad();
			}
			Prop->Destroy();
		}
		UWEWorkshopLayoutComponent* Layout = UWEWorkshopLayoutComponent::Get(World);
		for (const FWESavedFurniture& Saved : Save->Furniture)
		{
			AWEPlaceholderProp* Existing = nullptr;
			for (TActorIterator<AWEPlaceholderProp> It(World); It && !Existing; ++It)
			{
				Existing = It->GetFName() == Saved.ActorName ? *It : nullptr;
			}
			if (Existing)
			{
				Existing->SetActorTransform(Saved.Transform, false, nullptr, ETeleportType::TeleportPhysics);
			}
			else if (!Saved.FurnitureId.IsNone() && Layout)
			{
				Layout->SpawnFurniture(Saved.FurnitureId, Saved.Transform, Saved.ActorName);
			}
		}
	}

	// 1) Free all hands and workbenches, then replace every world item with the saved ones.
	for (TActorIterator<AWEWorkbench> It(World); It; ++It)
	{
		It->ResetForLoad();
	}
	for (TActorIterator<AWECharacter> It(World); It; ++It)
	{
		It->ReleaseHeldItem();
	}
	TArray<AWEPickupItem*> ToDestroy;
	for (TActorIterator<AWEPickupItem> It(World); It; ++It)
	{
		ToDestroy.Add(*It);
	}
	for (AWEPickupItem* Item : ToDestroy)
	{
		Item->Destroy();
	}
	for (const FWESavedWorldItem& Saved : Save->WorldItems)
	{
		FWEItemInstance Item;
		if (FromSaved(Saved.Item, Item))
		{
			AWEPickupItem* Spawned = AWEPickupItem::SpawnWorldItem(World, Item, Saved.Transform);
			if (Spawned && !Saved.PlacedOn.IsNone())
			{
				for (TActorIterator<AWEWorkbench> Bench(World); Bench; ++Bench)
				{
					if (Bench->GetFName() == Saved.PlacedOn)
					{
						Bench->PlaceItem(Spawned);
					}
				}
			}
		}
	}

	// 2) Upgrades first (they change storage sizes), then containers and doors, matched by actor name.
	if (Save->bHasProgression)
	{
		if (UWEProgressionComponent* Progression = UWEProgressionComponent::Get(World))
		{
			Progression->SetOwnedUpgrades(Save->OwnedUpgrades);
			Progression->SetGuideStep(Save->GuideStep);
		}
	}
	// Version 14: look of the buildings and the tutorial (older saves: original look, tutorial off).
	if (UWEWorkshopStyleComponent* Style = UWEWorkshopStyleComponent::Get(World))
	{
		Style->SetAllStyles(Save->bHasStyle ? Save->ZoneStyles : TArray<FWEZoneStyle>());
	}
	if (UWETutorialComponent* Tutorial = UWETutorialComponent::Get(World))
	{
		const uint8 State = Save->SaveVersion >= 14 ? Save->TutorialState : 0;
		Tutorial->SetState(State <= static_cast<uint8>(EWETutorialState::Completed) ? static_cast<EWETutorialState>(State) : EWETutorialState::Off, Save->TutorialStep);
	}
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		TArray<UWEInventoryComponent*> Inventories;
		Actor->GetComponents(Inventories);
		for (int32 InventoryIndex = 0; InventoryIndex < Inventories.Num() && !Actor->IsA<APlayerState>(); ++InventoryIndex)
		{
			UWEInventoryComponent* Inventory = Inventories[InventoryIndex];
			// Older saves have one container per actor without a component name: that is the first inventory.
			const FWESavedContainer* Saved = Save->Containers.FindByPredicate([Actor, Inventory, InventoryIndex](const FWESavedContainer& C)
			{
				return C.ActorName == Actor->GetFName() && (C.ComponentName == Inventory->GetFName() || (C.ComponentName.IsNone() && InventoryIndex == 0));
			});
			Inventory->SetAllSlots(Saved ? FromSavedSlots(Saved->Slots) : TArray<FWEInventorySlotData>());
		}
		if (AWEGarageDoor* Door = Cast<AWEGarageDoor>(Actor))
		{
			if (const FWESavedDoor* Saved = Save->Doors.FindByPredicate([Door](const FWESavedDoor& D) { return D.ActorName == Door->GetFName(); }))
			{
				Door->SetOpen(Saved->bOpen);
			}
		}
		if (AWETraderTerminal* Terminal = Cast<AWETraderTerminal>(Actor))
		{
			if (const FWESavedStock* Saved = Save->Stocks.FindByPredicate([Terminal](const FWESavedStock& S) { return S.ActorName == Terminal->GetFName(); }))
			{
				// Older saves only have counts for fixed offers: keep the current (rolled) stock then.
				TArray<FWEStockLine> Lines;
				for (const FWESavedStockLine& SavedLine : Saved->Lines)
				{
					FWEStockLine& Line = Lines.AddDefaulted_GetRef();
					Line.TraderIndex = SavedLine.TraderIndex;
					Line.EntryIndex = SavedLine.EntryIndex;
					Line.Remaining = SavedLine.Remaining;
					if (!FromSaved(SavedLine.Item, Line.Item))
					{
						Lines.Pop();
					}
				}
				if (Lines.Num() > 0)
				{
					Terminal->SetStockLines(Lines);
				}
			}
		}
	}

	// Machines (version 8): state, maintenance and the running cycle continue where they were.
	for (TActorIterator<AWEMachine> It(World); It; ++It)
	{
		const FWESavedMachine* Saved = Save->Machines.FindByPredicate([&It](const FWESavedMachine& M) { return M.ActorName == It->GetFName(); });
		TArray<FWEItemInstance> Processing;
		if (Saved)
		{
			for (const FWESavedItem& SavedItem : Saved->Processing)
			{
				FWEItemInstance Item;
				if (FromSaved(SavedItem, Item))
				{
					Processing.Add(Item);
				}
			}
		}
		It->RestoreState(Saved && Saved->bOn, Saved ? Saved->Maintenance : 100, Saved ? Saved->RecipeIndex : INDEX_NONE,
			Saved ? Saved->Progress : 0.f, Saved ? Saved->CycleSeconds : 0.f, Processing, Saved ? Saved->SelectedRecipe : INDEX_NONE);
	}
	// Conveyors and material stores (version 9): items stay where they lay on the belt.
	for (TActorIterator<AWEConveyor> It(World); It; ++It)
	{
		const FWESavedConveyor* Saved = Save->Conveyors.FindByPredicate([&It](const FWESavedConveyor& C) { return C.ActorName == It->GetFName(); });
		TArray<FWEConveyorItem> Items;
		for (int32 i = 0; Saved && i < Saved->Items.Num(); ++i)
		{
			FWEConveyorItem Entry;
			if (FromSaved(Saved->Items[i], Entry.Item))
			{
				Entry.Position = Saved->Positions.IsValidIndex(i) ? Saved->Positions[i] : 0.f;
				Items.Add(Entry);
			}
		}
		It->RestoreState(!Saved || Saved->bOn, Items);
	}
	for (TActorIterator<AWEProductionStorage> It(World); It; ++It)
	{
		const FWESavedStoreOutput* Saved = Save->StoreOutputs.FindByPredicate([&It](const FWESavedStoreOutput& S) { return S.ActorName == It->GetFName(); });
		It->SetOutput(!Saved || Saved->bOutputOn, Saved && !Saved->FilterItemId.IsNone() ? UWEItemDefinition::FindById(Saved->FilterItemId) : nullptr);
	}

	// Staff and vans (version 10): employees are replaced by the saved ones (tasks refer to furniture by name).
	if (UWEStaffComponent* Staff = UWEStaffComponent::Get(World); Staff && Save->bHasStaff)
	{
		for (AWEEmployee* Employee : Staff->GetEmployees())
		{
			Employee->Destroy();
		}
		auto FindActor = [World](FName Name) -> AActor*
		{
			if (Name.IsNone())
			{
				return nullptr;
			}
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (It->GetFName() == Name && IsValid(*It) && !It->IsActorBeingDestroyed())
				{
					return *It;
				}
			}
			return nullptr;
		};
		for (const FWESavedEmployee& Saved : Save->Employees)
		{
			AWEEmployee* Employee = Staff->SpawnEmployee(Saved.Profile, FTransform(FRotator(0.f, Saved.Yaw, 0.f), Saved.Location), Saved.ActorName);
			if (!Employee)
			{
				continue;
			}
			FWEEmployeeTask Task;
			Task.Kind = static_cast<EWETaskKind>(Saved.TaskKind);
			Task.Source = FindActor(Saved.SourceName);
			Task.Target = FindActor(Saved.TargetName);
			Task.Filter = Saved.FilterItemId.IsNone() ? nullptr : UWEItemDefinition::FindById(Saved.FilterItemId);
			if (Task.Kind != EWETaskKind::None)
			{
				Employee->AssignTask(Task);
			}
			FWEItemInstance Carried;
			if (Saved.bHasCarried && FromSaved(Saved.Carried, Carried))
			{
				Employee->RestoreCarried(Carried);
			}
			Employee->SetPaused(Saved.bPaused);
		}
		Staff->RestoreState(Save->Applicants, FWEMoney::FromCents(Save->OwedWagesCents));
	}
	for (TActorIterator<AWEVehicle> It(World); It; ++It)
	{
		const FWESavedVehicle* Saved = Save->Vehicles.FindByPredicate([&It](const FWESavedVehicle& V) { return V.ActorName == It->GetFName(); });
		It->RestoreTour(Saved ? Saved->SelectedTour : NAME_None, Saved ? Saved->ActiveTour : NAME_None, Saved ? Saved->RemainingSeconds : 0.f);
		if (It->IsAway())
		{
			if (AWEEmployee* Driver = It->FindDriver())
			{
				Driver->SetAway(true);
			}
		}
	}
	if (UWEPowerGridComponent* Grid = UWEPowerGridComponent::Get(World); Grid && Save->SaveVersion >= 8)
	{
		Grid->SetUnbilledKWh(Save->PowerUnbilledKWh);
	}

	// Auctions (version 6; older saves keep the current lots).
	if (Save->bHasAuctions)
	{
		if (UWEAuctionComponent* Auctions = UWEAuctionComponent::Get(World))
		{
			const float Now = Auctions->GetServerTime();
			TArray<FWEAuctionLot> Lots;
			TArray<int64> Limits;
			for (const FWESavedAuctionLot& Saved : Save->Auctions)
			{
				FWEAuctionLot Lot;
				Lot.Id = Saved.Id;
				Lot.Title = FText::FromString(Saved.Title);
				for (const FWESavedItem& SavedItem : Saved.Items)
				{
					FWEItemInstance Item;
					if (FromSaved(SavedItem, Item))
					{
						Lot.Items.Add(Item);
					}
				}
				if (Lot.Items.Num() == 0)
				{
					continue;
				}
				Lot.EstimatedValue = FWEMoney::FromCents(Saved.EstimatedCents);
				Lot.CurrentBid = FWEMoney::FromCents(Saved.CurrentBidCents);
				Lot.NumBids = Saved.NumBids;
				Lot.bCompanyLeads = Saved.bCompanyLeads;
				Lot.LeaderName = Saved.LeaderName;
				Lot.EndsAt = Now + Saved.RemainingSeconds;
				Lots.Add(Lot);
				Limits.Add(Saved.BidderMaxCents);
			}
			Auctions->SetLots(Lots, Limits);
		}
	}

	// Economy (version 3; older saves keep the current account and market).
	if (Save->CompanyBalanceCents >= 0)
	{
		if (UWECompanyComponent* Company = UWECompanyComponent::Get(World))
		{
			Company->SetBalance(FWEMoney::FromCents(Save->CompanyBalanceCents));
			Company->SetCategoryTotals(Save->BookTotals);
		}
		// Since version 13. Older saves start counting now (milestones already met are paid on the next check).
		if (UWEMilestoneComponent* Milestones = UWEMilestoneComponent::Get(World); Milestones && Save->bHasMilestones)
		{
			Milestones->SetState(Save->CompanyStats, Save->CompletedMilestones);
		}
		if (UWEMarketComponent* Market = UWEMarketComponent::Get(World); Market && Save->MarketDemand.Num() > 0)
		{
			Market->SetAllDemand(Save->MarketDemand);
		}
		if (UWEContractComponent* Contracts = UWEContractComponent::Get(World))
		{
			const float Now = Contracts->GetServerTime();
			TArray<FWEContract> Restored;
			for (const FWESavedContract& Saved : Save->Contracts)
			{
				FWEContract Contract;
				Contract.Id = Saved.Id;
				Contract.Title = FText::FromString(Saved.Title);
				Contract.Item = UWEItemDefinition::FindById(Saved.ItemId);
				Contract.Quantity = Saved.Quantity;
				Contract.MinCondition = Saved.MinCondition;
				Contract.Reward = FWEMoney::FromCents(Saved.RewardCents);
				Contract.State = Saved.bAccepted ? EWEContractState::Accepted : EWEContractState::Offered;
				Contract.ExpiresAt = Now + Saved.RemainingSeconds;
				Contract.DeliverySeconds = Saved.DeliverySeconds;
				Contract.AcceptedBy = Saved.AcceptedBy;
				Contract.Kind = Saved.Kind == 1 ? EWEContractKind::Repair : EWEContractKind::Supply;
				Contract.CustomerCondition = Saved.CustomerCondition;
				Contract.CustomerItemId = Saved.CustomerItemId;
			Contract.bIndustrial = Saved.bIndustrial;
			Contract.Rounds = FMath::Max(1, Saved.Rounds);
			Contract.RoundsDone = Saved.RoundsDone;
				Contract.MinQuality = Saved.MinQuality;
				if (Contract.Item)
				{
					Restored.Add(Contract);
				}
			}
			Contracts->SetContracts(Restored);
		}
	}

	// 3) Players currently connected.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ApplyPlayer(It->Get(), Save);
	}
}

void FWESaveSystem::ApplyPlayer(APlayerController* PC, const UWESaveGame* Save)
{
	AWECharacter* Character = PC ? Cast<AWECharacter>(PC->GetPawn()) : nullptr;
	if (!Character || !Save)
	{
		return;
	}
	const FWESavedPlayer* Saved = FindPlayer(Save->Players, GetPlayerKey(PC), PC->PlayerState ? PC->PlayerState->GetPlayerName() : FString());
	if (!Saved)
	{
		return;
	}
	ApplySavedPlayer(PC, *Saved);
}

const FWESavedPlayer* FWESaveSystem::FindPlayer(const TArray<FWESavedPlayer>& Players, const FString& Key, const FString& Name)
{
	if (const FWESavedPlayer* ByKey = Players.FindByPredicate([&Key](const FWESavedPlayer& P) { return !Key.IsEmpty() && P.PlayerKey == Key; }))
	{
		return ByKey;
	}
	// LAN ids change between game starts: fall back to the name, but only if it is unambiguous.
	if (Name.IsEmpty() || Key == TEXT("Host"))
	{
		return nullptr;
	}
	const FWESavedPlayer* Match = nullptr;
	for (const FWESavedPlayer& P : Players)
	{
		if (P.PlayerKey != TEXT("Host") && P.PlayerName == Name)
		{
			if (Match)
			{
				return nullptr;
			}
			Match = &P;
		}
	}
	return Match;
}

void FWESaveSystem::ApplySavedPlayer(APlayerController* PC, const FWESavedPlayer& Saved)
{
	AWECharacter* Character = PC ? Cast<AWECharacter>(PC->GetPawn()) : nullptr;
	if (!Character)
	{
		return;
	}
	Character->TeleportTo(Saved.PawnTransform.GetLocation(), Saved.PawnTransform.Rotator(), false, true);
	PC->ClientSetRotation(Saved.ControlRotation);
	if (Saved.bHasLook)
	{
		Character->ApplySavedLook(Saved.Look);
	}
	if (UWEInventoryComponent* Inventory = Character->GetPlayerInventory())
	{
		Inventory->SetAllSlots(FromSavedSlots(Saved.Inventory));
	}
}

// --- Files -----------------------------------------------------------------------------------------

TArray<FWESaveSlotInfo> FWESaveSystem::ListSlots()
{
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(GetSaveDirectory(), TEXT("*.wesave")), true, false);
	TArray<FWESaveSlotInfo> Result;
	for (const FString& File : Files)
	{
		FWESaveSlotInfo& Info = Result.AddDefaulted_GetRef();
		Info.Slot = FPaths::GetBaseFilename(File);
		FText Error;
		if (const UWESaveGame* Save = IsValidSlotName(Info.Slot) ? ReadFromFile(GetSlotPath(Info.Slot), Error) : nullptr)
		{
			Info.bReadable = true;
			Info.SavedAtUtc = Save->SavedAtUtc;
			Info.BalanceCents = Save->CompanyBalanceCents;
			Info.Version = Save->SaveVersion;
		}
	}
	Result.Sort([](const FWESaveSlotInfo& A, const FWESaveSlotInfo& B) { return A.SavedAtUtc > B.SavedAtUtc; });
	return Result;
}

bool FWESaveSystem::WriteToFile(const UWESaveGame* Save, const FString& Path, FText& OutError)
{
	TArray<uint8> Payload;
	if (!Save || !UGameplayStatics::SaveGameToMemory(const_cast<UWESaveGame*>(Save), Payload))
	{
		OutError = LOCTEXT("SerializeFailed", "Spielstand konnte nicht erstellt werden.");
		return false;
	}

	TArray<uint8> FileBytes;
	FMemoryWriter Writer(FileBytes);
	uint32 Magic = FileMagic;
	uint32 Format = FileFormatVersion;
	uint32 Size = Payload.Num();
	uint32 Crc = FCrc::MemCrc32(Payload.GetData(), Payload.Num());
	Writer << Magic << Format << Size << Crc;
	Writer.Serialize(Payload.GetData(), Payload.Num());

	IFileManager& FM = IFileManager::Get();
	FM.MakeDirectory(*FPaths::GetPath(Path), true);
	const FString TempPath = Path + TEXT(".tmp");
	const FString BackupPath = Path + TEXT(".bak");

	if (!FFileHelper::SaveArrayToFile(FileBytes, *TempPath))
	{
		OutError = LOCTEXT("WriteFailed", "Spielstand konnte nicht geschrieben werden.");
		return false;
	}
	if (FM.FileExists(*Path) && !FM.Move(*BackupPath, *Path, /*Replace*/ true))
	{
		UE_LOG(LogWorkshopEmpire, Warning, TEXT("Save: could not create backup %s"), *BackupPath);
	}
	if (!FM.Move(*Path, *TempPath, /*Replace*/ true))
	{
		OutError = LOCTEXT("MoveFailed", "Spielstand konnte nicht abgeschlossen werden.");
		return false;
	}
	return true;
}

UWESaveGame* FWESaveSystem::ReadSingleFile(const FString& Path, FText& OutError)
{
	TArray<uint8> FileBytes;
	if (!FFileHelper::LoadFileToArray(FileBytes, *Path, FILEREAD_Silent))
	{
		OutError = LOCTEXT("NotFound", "Spielstand nicht gefunden.");
		return nullptr;
	}
	constexpr int32 HeaderSize = 4 * sizeof(uint32);
	if (FileBytes.Num() < HeaderSize)
	{
		OutError = LOCTEXT("Corrupt", "Spielstand ist beschädigt.");
		return nullptr;
	}

	FMemoryReader Reader(FileBytes);
	uint32 Magic = 0, Format = 0, Size = 0, Crc = 0;
	Reader << Magic << Format << Size << Crc;
	if (Magic != FileMagic || Format > FileFormatVersion || Size != static_cast<uint32>(FileBytes.Num() - HeaderSize)
		|| FCrc::MemCrc32(FileBytes.GetData() + HeaderSize, Size) != Crc)
	{
		OutError = LOCTEXT("Corrupt", "Spielstand ist beschädigt.");
		return nullptr;
	}

	TArray<uint8> Payload(FileBytes.GetData() + HeaderSize, Size);
	UWESaveGame* Save = Cast<UWESaveGame>(UGameplayStatics::LoadGameFromMemory(Payload));
	if (!Save)
	{
		OutError = LOCTEXT("Corrupt", "Spielstand ist beschädigt.");
		return nullptr;
	}
	if (Save->SaveVersion > UWESaveGame::CurrentVersion)
	{
		OutError = LOCTEXT("TooNew", "Spielstand stammt aus einer neueren Spielversion.");
		return nullptr;
	}
	// Future: migrate older SaveVersion values here.
	return Save;
}

UWESaveGame* FWESaveSystem::ReadFromFile(const FString& Path, FText& OutError, bool* bOutUsedBackup)
{
	if (bOutUsedBackup)
	{
		*bOutUsedBackup = false;
	}
	FText MainError;
	if (UWESaveGame* Save = ReadSingleFile(Path, MainError))
	{
		return Save;
	}
	FText BackupError;
	if (UWESaveGame* Save = ReadSingleFile(Path + TEXT(".bak"), BackupError))
	{
		UE_LOG(LogWorkshopEmpire, Warning, TEXT("Save: %s unreadable (%s), loaded backup instead"), *Path, *MainError.ToString());
		if (bOutUsedBackup)
		{
			*bOutUsedBackup = true;
		}
		return Save;
	}
	OutError = MainError;
	return nullptr;
}

bool FWESaveSystem::SaveWorld(UWorld* World, const FString& Slot, FText& OutError)
{
	if (!IsValidSlotName(Slot))
	{
		OutError = LOCTEXT("BadSlot", "Ungültiger Name für den Spielstand.");
		return false;
	}
	if (!World || World->GetNetMode() == NM_Client)
	{
		OutError = LOCTEXT("HostOnly", "Nur der Host kann speichern.");
		return false;
	}
	const UWESaveGame* Save = Capture(World);
	const bool bOk = WriteToFile(Save, GetSlotPath(Slot), OutError);
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Save '%s': %s (%d players, %d world items, %d containers)"), *Slot,
		bOk ? TEXT("ok") : *OutError.ToString(), Save->Players.Num(), Save->WorldItems.Num(), Save->Containers.Num());
	return bOk;
}

bool FWESaveSystem::LoadWorld(UWorld* World, const FString& Slot, FText& OutError, UWESaveGame** OutSave)
{
	if (!IsValidSlotName(Slot))
	{
		OutError = LOCTEXT("BadSlot", "Ungültiger Name für den Spielstand.");
		return false;
	}
	if (!World || World->GetNetMode() == NM_Client)
	{
		OutError = LOCTEXT("HostOnlyLoad", "Nur der Host kann laden.");
		return false;
	}
	bool bUsedBackup = false;
	UWESaveGame* Save = ReadFromFile(GetSlotPath(Slot), OutError, &bUsedBackup);
	if (!Save)
	{
		return false;
	}
	if (Save->MapName != UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()))
	{
		OutError = LOCTEXT("OtherMap", "Spielstand gehört zu einer anderen Karte.");
		return false;
	}
	Apply(World, Save);
	if (OutSave)
	{
		*OutSave = Save;
	}
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Load '%s': ok%s (%d players, %d world items, %d containers)"), *Slot,
		bUsedBackup ? TEXT(" (from backup)") : TEXT(""), Save->Players.Num(), Save->WorldItems.Num(), Save->Containers.Num());
	return true;
}

#undef LOCTEXT_NAMESPACE
