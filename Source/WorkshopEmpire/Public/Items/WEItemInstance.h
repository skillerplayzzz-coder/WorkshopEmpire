#pragma once

#include "CoreMinimal.h"
#include "Items/WEItemTypes.h"
#include "WEMoney.h"
#include "WEItemInstance.generated.h"

class UWEItemDefinition;

/**
 * Repair state of one device (§9.5). Bit i refers to fault i of the definition's UWERepairDefinition.
 * Faults are rolled once, on the server, when a device is first diagnosed or disassembled.
 */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWERepairState
{
	GENERATED_BODY()

	enum EFlags : uint8
	{
		FaultsRolled = 1 << 0,
		Diagnosed    = 1 << 1,
		Refurbished  = 1 << 2,
		CustomerItem = 1 << 3,
		/** Phase 19: a customer's PC for an upgrade order - parts may be swapped. */
		CustomerUpgrade = 1 << 5,
		/** Parts were put back into a device with a part layout: it needs a function test before it counts as working. */
		NeedsTest    = 1 << 4
	};

	/** Components that are actually broken (a dud spare part keeps its bit until the function test). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Repair")
	uint8 Faults = 0;

	/** Broken part taken out, waiting for a spare part. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Repair")
	uint8 Removed = 0;

	/** Spare part installed since the last function test. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Repair")
	uint8 Replaced = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Repair")
	uint8 Flags = 0;

	bool HasFlag(EFlags Flag) const { return (Flags & Flag) != 0; }
	void SetFlag(EFlags Flag) { Flags |= Flag; }
	static bool HasBit(uint8 Mask, int32 Index) { return Index >= 0 && Index < 8 && (Mask & (1 << Index)) != 0; }
	bool IsInProgress() const { return Removed != 0 || Replaced != 0; }

	bool operator==(const FWERepairState& Other) const
	{
		return Faults == Other.Faults && Removed == Other.Removed && Replaced == Other.Replaced && Flags == Other.Flags;
	}
};

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

	/**
	 * Devices with a part layout (UWEAssemblyDefinition, Phase 18a): bit i = part i is out. Parts can come out in any
	 * order their prerequisites allow; DisassemblyStep then counts the parts that are out.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 RemovedParts = 0;
	bool IsPartRemoved(int32 Part) const { return Part >= 0 && Part < 32 && (RemovedParts & (1 << Part)) != 0; }

	/**
	 * Parts put back into the device (reassembly): 4 bits per part, 0 = the device's own part, otherwise the condition + 1
	 * of the item that was installed. Decides the function test and what comes out when the part is removed again.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int64 InstalledParts = 0;
	static constexpr int32 MaxInstalledParts = 16;
	/** Condition (index) of the item installed as Part, or INDEX_NONE for the device's own part. */
	int32 GetInstalledCondition(int32 Part) const
	{
		return Part >= 0 && Part < MaxInstalledParts ? static_cast<int32>((InstalledParts >> (Part * 4)) & 0xF) - 1 : INDEX_NONE;
	}
	/** ConditionIndex INDEX_NONE: the device's own part again (bits cleared). */
	void SetInstalledCondition(int32 Part, int32 ConditionIndex)
	{
		if (Part >= 0 && Part < MaxInstalledParts)
		{
			InstalledParts &= ~(int64(0xF) << (Part * 4));
			InstalledParts |= int64(FMath::Clamp(ConditionIndex + 1, 0, 15)) << (Part * 4);
		}
	}
	/** Phase 19: which variant sits in each part place (2 bits per part, 0 = the device's normal part, see FWEAssemblyPart::Alternatives). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 PartVariants = 0;
	int32 GetPartVariant(int32 Part) const { return Part >= 0 && Part < MaxInstalledParts ? (PartVariants >> (Part * 2)) & 0x3 : 0; }
	void SetPartVariant(int32 Part, int32 Variant)
	{
		if (Part >= 0 && Part < MaxInstalledParts)
		{
			PartVariants &= ~(0x3 << (Part * 2));
			PartVariants |= (FMath::Clamp(Variant, 0, 3) << (Part * 2));
		}
	}
	/** Some installed part is broken (condition "Beschädigt" or worse). */
	bool HasBrokenInstalledPart() const;
	bool NeedsFunctionTest() const { return Repair.HasFlag(FWERepairState::NeedsTest); }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FWERepairState Repair;

	/** Phase 17.5: player marks against accidental sale (EMarks). Kept when the item moves; part of the save. */
	enum EMarks : uint8
	{
		Favorite    = 1 << 0,
		Locked      = 1 << 1,
		/** Personal equipment (tools) deliberately released for sale. */
		ReleasedForSale = 1 << 2
	};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	uint8 Marks = 0;

	bool HasMark(EMarks Mark) const { return (Marks & Mark) != 0; }
	bool IsFavorite() const { return HasMark(Favorite); }
	bool IsLocked() const { return HasMark(Locked); }

	/** Tools and carrying aids: the player's own equipment, not goods. */
	bool IsPersonalEquipment() const;

	/**
	 * Never sold or handed to a customer without a deliberate step: locked or favourite items, and
	 * personal equipment that was not released for sale. Customer items are protected separately.
	 */
	bool IsSaleProtected() const { return IsLocked() || IsFavorite() || (IsPersonalEquipment() && !HasMark(ReleasedForSale)); }

	/** Rare or valuable: the UI asks before selling it. */
	bool NeedsSaleConfirmation() const;

	/** Condition "Beschädigt" or worse: needs a repair before it works. */
	bool IsBroken() const { return Condition >= EWEItemCondition::Damaged; }

	/** Belongs to a customer (repair order): cannot be sold or disassembled. */
	bool IsCustomerItem() const { return Repair.HasFlag(FWERepairState::CustomerItem); }

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
			&& Condition == Other.Condition && Quality == Other.Quality && DisassemblyStep == Other.DisassemblyStep && Repair == Other.Repair
			&& Marks == Other.Marks && RemovedParts == Other.RemovedParts && InstalledParts == Other.InstalledParts && PartVariants == Other.PartVariants;
	}
};
