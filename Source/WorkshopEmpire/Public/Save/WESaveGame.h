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
};

/** Since save version 3: remaining stock of a market terminal (matched by actor name). */
USTRUCT()
struct WORKSHOPEMPIRE_API FWESavedStock
{
	GENERATED_BODY()

	UPROPERTY() FName ActorName;
	UPROPERTY() TArray<int32> Remaining;
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
	/** 1: Phase 2. 2: Phase 3 (disassembly progress, items on workbenches). 3: Phase 4 (company account, market, orders, stock). 4: Phase 5 (upgrades, guide). */
	static constexpr int32 CurrentVersion = 4;

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
};
