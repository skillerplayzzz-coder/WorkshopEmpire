#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/WEItemTypes.h"
#include "WEDisassemblyDefinition.generated.h"

class UWEItemDefinition;

/** One thing a disassembly step yields. */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEDisassemblyOutput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Disassembly")
	TObjectPtr<UWEItemDefinition> Item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Disassembly", meta = (ClampMin = "0"))
	int32 MinQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Disassembly", meta = (ClampMin = "0"))
	int32 MaxQuantity = 1;

	/** Probability (0-1) that this output appears at all. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Disassembly", meta = (ClampMin = "0", ClampMax = "1"))
	float Chance = 1.f;

	/** Components inherit (and vary) the source condition/quality; raw materials do not (so they stack). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Disassembly")
	bool bInheritCondition = true;
};

/** One visible action of a disassembly sequence, e.g. "Gehäuseschrauben lösen". */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEDisassemblyStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Disassembly")
	FText ActionText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Disassembly")
	EWEToolType RequiredTool = EWEToolType::None;

	/** Duration with a basic tool (speed multiplier 1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Disassembly", meta = (ClampMin = "0.1", Units = "s"))
	float BaseSeconds = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Disassembly")
	TArray<FWEDisassemblyOutput> Outputs;
};

/**
 * Reusable, data-driven disassembly sequence (§9.2). Item definitions reference one; several item
 * types can share it. Steps are executed in order at a workbench; each step needs a tool type and
 * yields components and/or raw materials. Detail depth is chosen per item (one step to many).
 */
UCLASS(BlueprintType)
class WORKSHOPEMPIRE_API UWEDisassemblyDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	int32 NumSteps() const { return Steps.Num(); }
	const FWEDisassemblyStep* GetStep(int32 Index) const { return Steps.IsValidIndex(Index) ? &Steps[Index] : nullptr; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Disassembly")
	TArray<FWEDisassemblyStep> Steps;
};
