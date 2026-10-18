#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/WEItemTypes.h"
#include "WEMoney.h"
#include "World/WESurface.h"
#include "WEItemDefinition.generated.h"

class UStaticMesh;
class UTexture2D;
class UWEDisassemblyDefinition;
class UWEAssemblyDefinition;
class UWERepairDefinition;
class UWELootTable;

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

	/** Step-by-step disassembly at the workbench. Empty = cannot be disassembled. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Processing")
	TObjectPtr<UWEDisassemblyDefinition> Disassembly;

	bool CanBeDisassembled() const;

	/**
	 * Phase 18a: real part layout (case, mainboard, cards ...): part i is disassembly step i, but parts come out in any
	 * order their prerequisites allow, and the workbench shows the device with its parts. Optional.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Processing")
	TObjectPtr<UWEAssemblyDefinition> Assembly;

	/** Assembly is set and matches the disassembly steps one to one. */
	bool HasAssembly() const;

	/** Repair procedure at the workbench (faults, spare parts, test). Needs bRepairable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Processing", meta = (EditCondition = "bRepairable"))
	TObjectPtr<UWERepairDefinition> Repair;

	bool CanBeRepaired() const;

	/** Sealed container with unknown contents (e.g. return pallet): [Q] opens it and spawns the finds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Processing")
	TObjectPtr<UWELootTable> Contents;

	bool IsOpenable() const { return Contents != nullptr; }

	/**
	 * Extra value factor while the item is broken (condition "Beschädigt" or worse). Complex devices
	 * are worth far less broken than working, so that repairing pays off. 1 = no extra loss.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Processing", meta = (ClampMin = "0.05", ClampMax = "1"))
	float BrokenValueMultiplier = 1.f;

	/** If this item is a tool: which kind (None = not a tool). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Tool")
	EWEToolType ToolType = EWEToolType::None;

	/** Work speed with this tool (1 = basic, e.g. 2.5 for a cordless drill). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Tool", meta = (ClampMin = "0.1", EditCondition = "ToolType != EWEToolType::None"))
	float ToolSpeedMultiplier = 1.f;

	/** Carrying aid (e.g. folding hand truck): while in the inventory, the hands carry this much more (kg). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Tool", meta = (ClampMin = "0", Units = "kg"))
	float CarryBonusKg = 0.f;

	/** Raw material content (for recycling machines later and as information). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Processing")
	TArray<FWEMaterialAmount> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual")
	TObjectPtr<UStaticMesh> WorldMesh;

	/** Scale applied to WorldMesh (placeholder shapes are 100 cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual")
	FVector WorldScale = FVector(0.3f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual")
	FLinearColor Tint = FLinearColor(0.6f, 0.6f, 0.6f);

	/** Phase 15: material of the placeholder shape (metal, wood, ...) while there is no modelled mesh. A modelled WorldMesh keeps its own materials. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual")
	EWESurface Surface = EWESurface::Placeholder;

	/** Inventory icon (Vorschaubild). Optional; the UI shows a coloured tile until icons exist. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual")
	TObjectPtr<UTexture2D> Icon;
};
