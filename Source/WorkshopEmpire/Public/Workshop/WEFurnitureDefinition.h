#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WEMoney.h"
#include "World/WEPlaceholderProp.h"
#include "WEFurnitureDefinition.generated.h"

/** One buyable, placeable piece of workshop furniture (§14 "Werkstatteinrichtung"). */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEFurnitureDefinition
{
	GENERATED_BODY()

	/** Stable id (saves). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture")
	FWEMoney Cost;

	/** True: a workbench (place items on it, disassemble, repair). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture")
	bool bWorkbench = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture")
	EWEPlaceholderPropType PropType = EWEPlaceholderPropType::Shelf;

	/** X = depth, Y = width, Z = height (cm). Origin = floor centre, front faces +X. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture")
	FVector Size = FVector(50.f, 180.f, 200.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture")
	FLinearColor Tint = FLinearColor(0.3f, 0.32f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture")
	bool bHasStorage = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture")
	FText StorageName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture", meta = (ClampMin = "1"))
	int32 StorageSlots = 15;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture", meta = (Units = "kg"))
	float StorageMaxWeightKg = 150.f;

	/** Phase 12: only available with this upgrade (e.g. the furnace needs the commercial hall). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture")
	FName RequiredUpgrade;

	/** Material store in a production line: also feeds conveyors (Phase 10). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture")
	bool bProductionStorage = false;

	/** Set for machines (shredder, sorting machine, press; Phase 9). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture")
	TObjectPtr<class UWEMachineDefinition> Machine;

	/** Workbenches: work speed of this bench (1 = standard, a big bench works faster). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture", meta = (ClampMin = "0.1"))
	float WorkSpeedMultiplier = 1.f;
};

/** All furniture the company can buy and place (one Data Asset: DA_FurnitureCatalog). */
UCLASS(BlueprintType)
class WORKSHOPEMPIRE_API UWEFurnitureCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	const FWEFurnitureDefinition* Find(FName Id) const
	{
		return Items.FindByPredicate([Id](const FWEFurnitureDefinition& D) { return D.Id == Id; });
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture")
	TArray<FWEFurnitureDefinition> Items;
};
