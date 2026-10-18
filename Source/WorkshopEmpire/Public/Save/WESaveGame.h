#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Items/WEItemTypes.h"
#include "Progression/WEMilestoneComponent.h"
#include "Staff/WEStaffTypes.h"
#include "Workshop/WEWorkshopStyleComponent.h"
#include "Player/WECharacterLook.h"
#include "WESaveGame.generated.h"

/** Item reference by stable ItemId (not by asset path), so assets can move without breaking saves. */
USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedItem
{
	GENERATED_BODY()

	UPROPERTY() FName ItemId;
	UPROPERTY() FGuid InstanceId;
	UPROPERTY() int32 Quantity = 1;
	UPROPERTY() EWEItemCondition Condition = EWEItemCondition::Used;
	UPROPERTY() uint8 Quality = 50;
	/** Since save version 2. Missing in version 1 files -> 0 (intact). */
	UPROPERTY() uint8 DisassemblyStep = 0;
	/** Since save version 5: repair state (FWERepairState). Older files -> nothing known yet. */
	UPROPERTY() uint8 RepairFaults = 0;
	UPROPERTY() uint8 RepairRemoved = 0;
	UPROPERTY() uint8 RepairReplaced = 0;
	UPROPERTY() uint8 RepairFlags = 0;
	/** Since save version 14: favourite / locked / released for sale (FWEItemInstance::EMarks). */
	UPROPERTY() uint8 Marks = 0;
	/** Since save version 15: parts already out of a device with a part layout. */
	UPROPERTY() int32 RemovedParts = 0;
};

USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedSlot
{
	GENERATED_BODY()

	UPROPERTY() int32 SlotIndex = INDEX_NONE;
	UPROPERTY() FWESavedItem Item;
};

USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedWorldItem
{
	GENERATED_BODY()

	UPROPERTY() FWESavedItem Item;
	UPROPERTY() FTransform Transform;
	/** Since save version 2: workbench the item lies on (actor name), or None. */
	UPROPERTY() FName PlacedOn;
};

/** Inventory of a placed actor (shelf, workbench), matched by the actor's stable name in the map. */
USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedContainer
{
	GENERATED_BODY()

	UPROPERTY() FName ActorName;
	UPROPERTY() TArray<FWESavedSlot> Slots;
	/** Since save version 8: which inventory of the actor (machines have input and output). None = the first one. */
	UPROPERTY() FName ComponentName;
};

/** Since save version 8: state of a machine (production progress is persistent, §13.5). */
USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedMachine
{
	GENERATED_BODY()

	UPROPERTY() FName ActorName;
	UPROPERTY() bool bOn = false;
	UPROPERTY() uint8 Maintenance = 100;
	UPROPERTY() int32 RecipeIndex = INDEX_NONE;
	UPROPERTY() float Progress = 0.f;
	UPROPERTY() float CycleSeconds = 0.f;
	UPROPERTY() TArray<FWESavedItem> Processing;
	/** Since version 9: recipe chosen with [T] (INDEX_NONE = automatic). */
	UPROPERTY() int32 SelectedRecipe = INDEX_NONE;
};

/** Since save version 9: a conveyor with the items lying on it. */
USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedConveyor
{
	GENERATED_BODY()

	UPROPERTY() FName ActorName;
	UPROPERTY() bool bOn = true;
	UPROPERTY() TArray<FWESavedItem> Items;
	/** Belt position (cm from the start) of each item. */
	UPROPERTY() TArray<float> Positions;
};

/** Since save version 10: an employee with his task (actors by name). */
USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedEmployee
{
	GENERATED_BODY()

	UPROPERTY() FName ActorName;
	UPROPERTY() FWEEmployeeProfile Profile;
	UPROPERTY() FVector Location = FVector::ZeroVector;
	UPROPERTY() float Yaw = 0.f;
	UPROPERTY() uint8 TaskKind = 0;
	UPROPERTY() FName SourceName;
	UPROPERTY() FName TargetName;
	UPROPERTY() FName FilterItemId;
	UPROPERTY() bool bPaused = false;
	UPROPERTY() bool bHasCarried = false;
	UPROPERTY() FWESavedItem Carried;
	UPROPERTY() float UnpaidSeconds = 0.f;
};

/** Since save version 10: van tour state. */
USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedVehicle
{
	GENERATED_BODY()

	UPROPERTY() FName ActorName;
	UPROPERTY() FName SelectedTour;
	UPROPERTY() FName ActiveTour;
	UPROPERTY() float RemainingSeconds = 0.f;
};

/** Since save version 9: output setting of a material store. */
USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedStoreOutput
{
	GENERATED_BODY()

	UPROPERTY() FName ActorName;
	UPROPERTY() bool bOutputOn = true;
	/** ItemId of the only material that goes out (None = everything). */
	UPROPERTY() FName FilterItemId;
};

USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedDoor
{
	GENERATED_BODY()

	UPROPERTY() FName ActorName;
	UPROPERTY() bool bOpen = false;
};

/** Since save version 3: customer order. Deadlines are stored as remaining seconds. */
USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedContract
{
	GENERATED_BODY()

	UPROPERTY() FGuid Id;
	UPROPERTY() FString Title;
	UPROPERTY() FName ItemId;
	UPROPERTY() int32 Quantity = 1;
	UPROPERTY() EWEItemCondition MinCondition = EWEItemCondition::Damaged;
	UPROPERTY() int64 RewardCents = 0;
	UPROPERTY() bool bAccepted = false;
	UPROPERTY() float RemainingSeconds = 0.f;
	UPROPERTY() float DeliverySeconds = 1200.f;
	UPROPERTY() FString AcceptedBy;
	/** Since save version 5: 0 = supply, 1 = repair order (+ the customer's device id). */
	UPROPERTY() uint8 Kind = 0;
	UPROPERTY() EWEItemCondition CustomerCondition = EWEItemCondition::Defective;
	UPROPERTY() FGuid CustomerItemId;
	/** Since save version 6. */
	UPROPERTY() uint8 MinQuality = 0;
	/** Since save version 11: industrial order / supply contract progress. */
	UPROPERTY() bool bIndustrial = false;
	UPROPERTY() int32 Rounds = 1;
	UPROPERTY() int32 RoundsDone = 0;
};

/** Since save version 6: one rolled offer of a terminal. */
USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedStockLine
{
	GENERATED_BODY()

	UPROPERTY() int32 TraderIndex = INDEX_NONE;
	UPROPERTY() int32 EntryIndex = INDEX_NONE;
	UPROPERTY() FWESavedItem Item;
	UPROPERTY() int32 Remaining = 0;
};

/** Since save version 3: stock of a market terminal (matched by actor name). */
USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedStock
{
	GENERATED_BODY()

	UPROPERTY() FName ActorName;
	/** Versions 3-5: remaining units per fixed offer (no longer applied: offers rotate since version 6). */
	UPROPERTY() TArray<int32> Remaining;
	/** Since version 6. */
	UPROPERTY() TArray<FWESavedStockLine> Lines;
};

/** Since save version 6: a running auction. */
USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedAuctionLot
{
	GENERATED_BODY()

	UPROPERTY() FGuid Id;
	UPROPERTY() FString Title;
	UPROPERTY() TArray<FWESavedItem> Items;
	UPROPERTY() int64 EstimatedCents = 0;
	UPROPERTY() int64 CurrentBidCents = 0;
	UPROPERTY() int32 NumBids = 0;
	UPROPERTY() bool bCompanyLeads = false;
	UPROPERTY() FString LeaderName;
	UPROPERTY() float RemainingSeconds = 0.f;
	/** The rival's hidden limit (the save file belongs to the host). */
	UPROPERTY() int64 BidderMaxCents = 0;
};

/** Since save version 7: one piece of furniture (map furniture: moved position; bought: id + position). */
USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedFurniture
{
	GENERATED_BODY()

	UPROPERTY() FName ActorName;
	/** None = part of the map. */
	UPROPERTY() FName FurnitureId;
	UPROPERTY() FTransform Transform;
};

USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedPlayer
{
	GENERATED_BODY()

	/** "Host" for the listen-server player, otherwise the unique net id. */
	UPROPERTY() FString PlayerKey;
	UPROPERTY() FTransform PawnTransform;
	UPROPERTY() FRotator ControlRotation = FRotator::ZeroRotator;
	UPROPERTY() TArray<FWESavedSlot> Inventory;
	/** Version 12: display name, to recognise a rejoining LAN player whose net id changed. */
	UPROPERTY() FString PlayerName;
	/** Version 15: the look from the character creator. */
	UPROPERTY() bool bHasLook = false;
	UPROPERTY() FWECharacterLook Look;
};

/**
 * Versioned save game of one company/world (spec §18). Owned and written by the host/server only.
 * Bump CurrentVersion when the layout changes and add a migration in FWESaveSystem::Migrate.
 */
UCLASS()
class WORKSHOPEMPIRE_API UWESaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** 1: Phase 2. 2: Phase 3 (disassembly progress, items on workbenches). 3: Phase 4 (company account, market, orders, stock). 4: Phase 5 (upgrades, guide). 5: Phase 6 (repair state, repair orders). 6: Phase 7 (rolled stock, auctions, order quality). 7: Phase 8 (furniture). 8: Phase 9 (machines, power, several inventories per actor). 9-11: Phases 10-12. 12: Phase 13 (player names; players who left the session are kept). 13: Phase 16 (company statistics, milestones). 14: Phase 17.5 (item marks, workshop look, tutorial, sold buildings via upgrades). 15: Phase 18a (parts out of devices, character, van cargo). */
	static constexpr int32 CurrentVersion = 15;

	UPROPERTY() int32 SaveVersion = CurrentVersion;
	UPROPERTY() FString MapName;
	UPROPERTY() FDateTime SavedAtUtc;
	UPROPERTY() TArray<FWESavedPlayer> Players;
	UPROPERTY() TArray<FWESavedWorldItem> WorldItems;
	UPROPERTY() TArray<FWESavedContainer> Containers;
	UPROPERTY() TArray<FWESavedDoor> Doors;

	// Since version 3. Older saves leave these at their defaults: the account keeps its current value.
	UPROPERTY() int64 CompanyBalanceCents = -1;
	UPROPERTY() TArray<float> MarketDemand;
	UPROPERTY() TArray<FWESavedContract> Contracts;
	UPROPERTY() TArray<FWESavedStock> Stocks;

	// Since version 4. Older saves: no upgrades, guide at its current step (bHasProgression false).
	UPROPERTY() bool bHasProgression = false;
	UPROPERTY() TArray<FName> OwnedUpgrades;
	UPROPERTY() int32 GuideStep = 0;

	// Since version 6.
	UPROPERTY() bool bHasAuctions = false;
	UPROPERTY() TArray<FWESavedAuctionLot> Auctions;

	// Since version 7. Older saves keep the map layout.
	UPROPERTY() bool bHasFurniture = false;
	UPROPERTY() TArray<FWESavedFurniture> Furniture;

	// Since version 8.
	UPROPERTY() TArray<FWESavedMachine> Machines;
	UPROPERTY() float PowerUnbilledKWh = 0.f;

	// Since version 9 (material flow).
	UPROPERTY() TArray<FWESavedConveyor> Conveyors;
	UPROPERTY() TArray<FWESavedStoreOutput> StoreOutputs;

	// Since version 10 (staff and logistics).
	UPROPERTY() bool bHasStaff = false;
	UPROPERTY() TArray<FWESavedEmployee> Employees;
	UPROPERTY() TArray<FWEEmployeeProfile> Applicants;
	UPROPERTY() int64 OwedWagesCents = 0;
	UPROPERTY() TArray<FWESavedVehicle> Vehicles;

	// Since version 11 (company books).
	UPROPERTY() TArray<int64> BookTotals;

	// Since version 13 (long-term goals).
	UPROPERTY() bool bHasMilestones = false;
	UPROPERTY() FWECompanyStats CompanyStats;
	UPROPERTY() TArray<FName> CompletedMilestones;

	// Since version 14 (Phase 17.5). Older saves: original look, no tutorial (state Off).
	UPROPERTY() bool bHasStyle = false;
	UPROPERTY() TArray<FWEZoneStyle> ZoneStyles;
	UPROPERTY() uint8 TutorialState = 0;
	UPROPERTY() int32 TutorialStep = 0;
};
