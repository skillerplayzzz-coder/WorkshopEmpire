#pragma once

#include "CoreMinimal.h"
#include "Items/WEItemTypes.h"
#include "WEMoney.h"
#include "WEItemInstance.generated.h"

class UWEItemDefinition;

/**
 * One concrete item (or stack of identical items): definition + per-copy state.
 * Small and replication-friendly; used in inventories, world items and save games.
 */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEItemInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UWEItemDefinition> Definition;

	/** Unique per instance (stack). Stays the same when the item moves between world and inventory. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	FGuid InstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EWEItemCondition Condition = EWEItemCondition::Used;

	/** 0-100, finer than condition; affects value and later repair results. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0", ClampMax = "100"))
	uint8 Quality = 50;

	/** Number of disassembly steps already done (0 = intact). Kept when the item leaves the workbench. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	uint8 DisassemblyStep = 0;

	bool IsPartiallyDisassembled() const { return DisassemblyStep > 0; }

	static FWEItemInstance Make(UWEItemDefinition* InDefinition, int32 InQuantity = 1, EWEItemCondition InCondition = EWEItemCondition::Used, uint8 InQuality = 50);

	bool IsValid() const { return Definition != nullptr && Quantity > 0; }

	/** Same definition and condition, and the definition is stackable. */
	bool CanStackWith(const FWEItemInstance& Other) const;

	float GetUnitWeightKg() const;
	float GetTotalWeightKg() const { return GetUnitWeightKg() * Quantity; }

	/** Rough value estimate (base value x condition x quality x quantity). Real pricing comes in Phase 4. */
	FWEMoney GetEstimatedValue() const;

	FText GetDisplayName() const;

	/** Splits off Count units into a new instance with a fresh id; this keeps the rest. */
	FWEItemInstance SplitOff(int32 Count);

	bool operator==(const FWEItemInstance& Other) const
	{
		return Definition == Other.Definition && InstanceId == Other.InstanceId && Quantity == Other.Quantity
			&& Condition == Other.Condition && Quality == Other.Quality && DisassemblyStep == Other.DisassemblyStep;
	}
};
