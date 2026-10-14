#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Items/WEItemTypes.h"
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

USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedPlayer
{
	GENERATED_BODY()

	/** "Host" for the listen-server player, otherwise the unique net id. */
	UPROPERTY() FString PlayerKey;
	UPROPERTY() FTransform PawnTransform;
	UPROPERTY() FRotator ControlRotation = FRotator::ZeroRotator;
	UPROPERTY() TArray<FWESavedSlot> Inventory;
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
	/** 1: Phase 2. 2: Phase 3 (disassembly progress, items on workbenches). 3: Phase 4 (company account, market, orders, stock). 4: Phase 5 (upgrades, guide). 5: Phase 6 (repair state, repair orders). 6: Phase 7 (rolled stock, auctions, order quality). */
	static constexpr int32 CurrentVersion = 6;

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
};
