#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WEMoney.h"
#include "WEUpgradeDefinition.generated.h"

/**
 * A workshop upgrade the company can buy (spec §4.3 visible progress, §5.5 invest). Data-driven:
 * every effect is a number here, the code only applies them. Effects stack over all owned upgrades.
 */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEUpgradeDefinition
{
	GENERATED_BODY()

	/** Stable id (saved). Never rename once shipped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	FWEMoney Cost;

	/** Must be owned before this one can be bought (None = no prerequisite). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	FName RequiredUpgrade;

	/** Extra slots for every workbench storage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade|Effects", meta = (ClampMin = "0"))
	int32 BenchExtraSlots = 0;

	/** Extra carrying capacity of every player inventory. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade|Effects", meta = (ClampMin = "0", Units = "kg"))
	float InventoryExtraKg = 0.f;

	/** Phase 19: extra slots in every player inventory (tool bag). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade|Effects", meta = (ClampMin = "0"))
	int32 InventoryExtraSlots = 0;

	/** Phase 19: tools weigh nothing in player inventories (tool bag). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade|Effects")
	bool bToolsWeightless = false;

	/** Disassembly speed factor (1.25 = 25 % faster). Multiplies with the tool speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade|Effects", meta = (ClampMin = "1"))
	float WorkSpeedMultiplier = 1.f;

	/** Brightness factor for workshop lights (actors tagged "WE_WorkshopLight"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade|Effects", meta = (ClampMin = "1"))
	float LightMultiplier = 1.f;

	/** Placed actors whose RequiredUpgrade equals this Id become visible/usable once bought. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade|Effects")
	bool bUnlocksActors = false;

	/** Extra power connection for machines (kW, Phase 9). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade|Effects", meta = (ClampMin = "0"))
	float PowerCapacityKw = 0.f;

	/** Buildings (Phase 12): owning this lifts the company to this tier (0 = no building). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade|Effects", meta = (ClampMin = "0", ClampMax = "5"))
	int32 CompanyTier = 0;

	/** More employees allowed (Phase 12). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade|Effects", meta = (ClampMin = "0"))
	int32 StaffSlots = 0;
};

UCLASS(BlueprintType)
class WORKSHOPEMPIRE_API UWEUpgradeCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	const FWEUpgradeDefinition* Find(FName Id) const
	{
		return Upgrades.FindByPredicate([Id](const FWEUpgradeDefinition& U) { return U.Id == Id; });
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	TArray<FWEUpgradeDefinition> Upgrades;
};
