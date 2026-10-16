#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/WEItemTypes.h"
#include "WEMachineDefinition.generated.h"

class UWEItemDefinition;

/** One output of a machine recipe. */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEMachineOutput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine")
	TObjectPtr<UWEItemDefinition> Item;

	/** Fixed amount per cycle (MinQuantity..MaxQuantity), used when PerInputKg is 0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine", meta = (ClampMin = "0"))
	int32 MinQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine", meta = (ClampMin = "0"))
	int32 MaxQuantity = 1;

	/** Probability (0-1) that this output appears in a cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine", meta = (ClampMin = "0", ClampMax = "1"))
	float Chance = 1.f;

	/** > 0: amount = input weight (kg) x this, rounded (e.g. the shredder makes 0.9 units per kg). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine", meta = (ClampMin = "0"))
	float PerInputKg = 0.f;

	/** Condition and quality of the product (materials "Gebraucht"/50 like workbench materials; new products "Neu"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine")
	EWEItemCondition Condition = EWEItemCondition::Used;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine", meta = (ClampMin = "0", ClampMax = "100"))
	int32 Quality = 50;
};

/**
 * Production recipe (§13.5 "Datengetriebene Produktionsrezepte"). The input is either a specific item
 * (InputItem x InputQuantity) or any one item of the listed categories (shredder).
 */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEMachineRecipe
{
	GENERATED_BODY()

	/** E.g. "Elektronik schreddern". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine")
	TObjectPtr<UWEItemDefinition> InputItem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine", meta = (ClampMin = "1"))
	int32 InputQuantity = 1;

	/** Used when InputItem is empty: one item of any of these categories per cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine")
	TArray<EWEItemCategory> InputCategories;

	/** Optional second ingredient (production recipes, Phase 10), e.g. plastic for a cable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine")
	TObjectPtr<UWEItemDefinition> SecondInputItem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine", meta = (ClampMin = "1"))
	int32 SecondInputQuantity = 1;

	/** True if Item is an ingredient of this recipe. */
	bool UsesItem(const UWEItemDefinition* Item) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine")
	TArray<FWEMachineOutput> Outputs;

	/** Cycle time = BaseSeconds + SecondsPerKg x input weight (at full efficiency). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine", meta = (ClampMin = "0.1", Units = "s"))
	float BaseSeconds = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine", meta = (ClampMin = "0", Units = "s"))
	float SecondsPerKg = 0.f;
};

/**
 * A machine type (§13.1/13.2): power, recipes, storage, wear. Placed through the build mode as
 * furniture (FWEFurnitureDefinition::Machine). Data asset per type in /Game/WorkshopEmpire/Data/Machines.
 */
UCLASS(BlueprintType)
class WORKSHOPEMPIRE_API UWEMachineDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine")
	FText DisplayName;

	/** Power while a cycle runs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine", meta = (ClampMin = "0"))
	float PowerKw = 5.f;

	/** Power while switched on but waiting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine", meta = (ClampMin = "0"))
	float IdlePowerKw = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine")
	TArray<FWEMachineRecipe> Recipes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine", meta = (ClampMin = "1"))
	int32 InputSlots = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine", meta = (Units = "kg"))
	float InputMaxWeightKg = 150.f;

	/** Bulky devices (carried) can be put in (shredder). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine")
	bool bAcceptsBulky = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine", meta = (ClampMin = "1"))
	int32 OutputSlots = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine", meta = (Units = "kg"))
	float OutputMaxWeightKg = 200.f;

	/** Maintenance points (of 100) lost per cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine", meta = (ClampMin = "0", ClampMax = "100"))
	float WearPerCycle = 2.f;

	/** Below this maintenance level the machine works slower (efficiency x0.7). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine", meta = (ClampMin = "0", ClampMax = "100"))
	float WornThreshold = 30.f;

	/** Consumed by [X] "Warten": back to 100 %. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Machine")
	TObjectPtr<UWEItemDefinition> MaintenanceItem;

	/** True if Item can be processed by any recipe (any quantity). */
	bool AcceptsItem(const UWEItemDefinition* Item) const;
};
