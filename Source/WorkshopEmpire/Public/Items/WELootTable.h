#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/WEItemInstance.h"
#include "WELootTable.generated.h"

class UWEItemDefinition;

/** One possible find: item, how often, how many, in which state. */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWELootEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UWEItemDefinition> Item;

	/** Relative chance per roll. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 MinQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 MaxQuantity = 1;

	/** Condition range (best .. worst), chosen evenly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	EWEItemCondition BestCondition = EWEItemCondition::Used;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	EWEItemCondition WorstCondition = EWEItemCondition::Defective;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0", ClampMax = "100"))
	uint8 MinQuality = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0", ClampMax = "100"))
	uint8 MaxQuality = 60;
};

/**
 * Weighted table of finds (§11): contents of return pallets, the scrap pile and rare finds.
 * Data-driven; rolling is done by the server with a seeded stream so results are reproducible.
 */
UCLASS(BlueprintType)
class WORKSHOPEMPIRE_API UWELootTable : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Rolls one entry (or an invalid instance if the table is empty). */
	FWEItemInstance RollOne(FRandomStream& Random) const;

	/** Rolls between MinRolls and MaxRolls entries. */
	TArray<FWEItemInstance> Roll(FRandomStream& Random) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TArray<FWELootEntry> Entries;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 MinRolls = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 MaxRolls = 5;
};
