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
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY() int32 SaveVersion = CurrentVersion;
	UPROPERTY() FString MapName;
	UPROPERTY() FDateTime SavedAtUtc;
	UPROPERTY() TArray<FWESavedPlayer> Players;
	UPROPERTY() TArray<FWESavedWorldItem> WorldItems;
	UPROPERTY() TArray<FWESavedContainer> Containers;
	UPROPERTY() TArray<FWESavedDoor> Doors;
};
