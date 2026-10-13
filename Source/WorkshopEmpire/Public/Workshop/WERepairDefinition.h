#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/WEItemTypes.h"
#include "WERepairDefinition.generated.h"

class UWEItemDefinition;

/** One thing that can be broken in a device, and how it is fixed (exchange a part). */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWERepairFault
{
	GENERATED_BODY()

	/** Shown after the diagnosis, e.g. "Netzteil defekt". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair")
	FText DisplayName;

	/** E.g. "Defektes Netzteil ausbauen". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair")
	FText RemoveText;

	/** E.g. "Ersatz-Netzteil einbauen". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair")
	FText InstallText;

	/** Spare part that is consumed when installing (from the worker's inventory or the bench storage). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair")
	TObjectPtr<UWEItemDefinition> Part;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair")
	EWEToolType Tool = EWEToolType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair", meta = (ClampMin = "0.1", Units = "s"))
	float RemoveSeconds = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair", meta = (ClampMin = "0.1", Units = "s"))
	float InstallSeconds = 2.f;

	/** Relative chance that this is what is broken. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair", meta = (ClampMin = "0"))
	float Weight = 1.f;

	/** True: removing it puts the broken part (condition "Defekt") into the bench storage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair")
	bool bReturnsBrokenPart = true;
};

/**
 * Data-driven repair procedure of a device type (§9.5): what can be broken, which spare part and tool
 * fixes it, and how to diagnose, test and refurbish it. Referenced by UWEItemDefinition::Repair.
 *
 * Flow at the workbench: diagnose → for every fault: remove broken part, install spare part →
 * function test (a damaged spare part may fail) → optional refurbishing (quality up).
 */
UCLASS(BlueprintType)
class WORKSHOPEMPIRE_API UWERepairDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Max 8 faults (stored as a bit mask per item). */
	static constexpr int32 MaxFaults = 8;

	const FWERepairFault* GetFault(int32 Index) const { return Faults.IsValidIndex(Index) ? &Faults[Index] : nullptr; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair")
	TArray<FWERepairFault> Faults;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair")
	EWEToolType DiagnoseTool = EWEToolType::Multimeter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair", meta = (ClampMin = "0.1", Units = "s"))
	float DiagnoseSeconds = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair")
	EWEToolType TestTool = EWEToolType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair", meta = (ClampMin = "0.1", Units = "s"))
	float TestSeconds = 3.f;

	/** Chance (0-1) that a defective device has a second fault. Damaged devices always have exactly one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair", meta = (ClampMin = "0", ClampMax = "1"))
	float SecondFaultChance = 0.3f;

	/** Chance (0-1) that a spare part in condition "Beschädigt" does not work (found by the function test). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair", meta = (ClampMin = "0", ClampMax = "1"))
	float DamagedPartFailChance = 0.5f;

	/** Quality points gained by a passed function test. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair", meta = (ClampMin = "0", ClampMax = "100"))
	int32 RepairQualityGain = 15;

	/** Consumable for refurbishing (e.g. cleaning kit). None = refurbishing not possible. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair")
	TObjectPtr<UWEItemDefinition> RefurbishMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair", meta = (ClampMin = "0.1", Units = "s"))
	float RefurbishSeconds = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Repair", meta = (ClampMin = "0", ClampMax = "100"))
	int32 RefurbishQualityGain = 30;
};
