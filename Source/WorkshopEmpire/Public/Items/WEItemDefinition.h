#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/WEItemTypes.h"
#include "WEMoney.h"
#include "WEItemDefinition.generated.h"

class UStaticMesh;
class UTexture2D;

/**
 * Static, data-driven description of an item type (spec §7). One Data Asset per item type in
 * /Game/WorkshopEmpire/Data/Items. Adding an item needs no C++ change.
 *
 * Per-copy state (condition, quality, stack size, unique id) lives in FWEItemInstance.
 * Registered with the Asset Manager as primary asset type "WEItem"; saves reference items by ItemId.
 */
UCLASS(BlueprintType)
class WORKSHOPEMPIRE_API UWEItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType PrimaryAssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Finds a definition by ItemId (loaded objects first, then the Asset Manager). */
	static UWEItemDefinition* FindById(FName ItemId);

	int32 GetMaxStackSize() const { return bStackable ? FMath::Max(1, MaxStackSize) : 1; }

	/** Unique, stable id used in saves and recipes. Never rename once shipped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EWEItemCategory Category = EWEItemCategory::Electronics;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EWEItemRarity Rarity = EWEItemRarity::Common;

	/** Weight of one unit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0.001", Units = "kg"))
	float WeightKg = 1.f;

	/** Reference value in "Neu" condition; buy/sell prices derive from it (Phase 4). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FWEMoney BaseValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Inventory")
	bool bStackable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Inventory", meta = (ClampMin = "1", EditCondition = "bStackable"))
	int32 MaxStackSize = 1;

	/** True: [E] puts it into the inventory. False: bulky, carried in the hands only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Inventory")
	bool bFitsInInventory = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Processing")
	bool bRepairable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Processing")
	bool bDisassemblable = false;

	/** What recycling/disassembly yields (Phase 3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Processing")
	TArray<FWEMaterialAmount> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual")
	TObjectPtr<UStaticMesh> WorldMesh;

	/** Scale applied to WorldMesh (placeholder shapes are 100 cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual")
	FVector WorldScale = FVector(0.3f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual")
	FLinearColor Tint = FLinearColor(0.6f, 0.6f, 0.6f);

	/** Inventory icon (Vorschaubild). Optional; the UI shows a coloured tile until icons exist. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual")
	TObjectPtr<UTexture2D> Icon;
};
