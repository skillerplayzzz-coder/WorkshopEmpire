#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/WEItemInstance.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "WEInventoryComponent.generated.h"

class UWEInventoryComponent;
class AWECharacter;

/** One occupied slot. Empty slots have no entry. */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FWEItemInstance Item;

	void PostReplicatedAdd(const struct FWEInventoryList& InArraySerializer);
	void PostReplicatedChange(const struct FWEInventoryList& InArraySerializer);
	void PreReplicatedRemove(const struct FWEInventoryList& InArraySerializer);
};

/** Delta-replicated list of slots: only changed entries go over the network. */
USTRUCT()
struct WORKSHOPEMPIRE_API FWEInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FWEInventoryEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UWEInventoryComponent> Owner;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FWEInventoryEntry, FWEInventoryList>(Entries, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FWEInventoryList> : public TStructOpsTypeTraitsBase2<FWEInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};

/** Slot + item, used for saving and for snapshots. */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEInventorySlotData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Inventory")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Inventory")
	FWEItemInstance Item;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FWEOnInventoryChanged, UWEInventoryComponent*);

UENUM()
enum class EWEInventoryResult : uint8
{
	Success,
	PartiallyAdded,
	NoSpace,
	TooHeavy,
	InvalidItem,
	InvalidSlot,
	NotAllowed
};

/**
 * Slot-based item storage used for the player inventory, shelves, workbenches, vehicles, machines.
 *
 * Authority: every mutating function runs on the server only (returns NotAllowed elsewhere).
 * Clients send requests through AWEPlayerController, which validates access and calls these.
 * Rules: one item stack per slot; stacks merge only for same definition + condition; total weight
 * and slot count are limited. Items that do not fit in inventories (bulky) are rejected.
 */
UCLASS(ClassGroup = (WorkshopEmpire), meta = (BlueprintSpawnableComponent))
class WORKSHOPEMPIRE_API UWEInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void InitializeComponent() override;

	// --- Queries (any machine) -----------------------------------------------------------------

	int32 GetNumSlots() const { return NumSlots; }
	float GetMaxWeightKg() const { return MaxWeightKg; }
	float GetTotalWeightKg() const;
	const FText& GetDisplayName() const { return DisplayName; }

	/** Setup (constructors / before play). */
	void Configure(const FText& InDisplayName, int32 InNumSlots, float InMaxWeightKg)
	{
		DisplayName = InDisplayName;
		NumSlots = FMath::Clamp(InNumSlots, 1, 200);
		MaxWeightKg = FMath::Max(0.f, InMaxWeightKg);
	}

	/** Item in Slot, or nullptr if empty / out of range. */
	const FWEItemInstance* GetItemInSlot(int32 Slot) const;
	int32 FindFirstEmptySlot() const;
	int32 CountItems(const UWEItemDefinition* Definition) const;
	TArray<FWEInventorySlotData> GetAllSlots() const;

	/** How many units of Item could be added right now (stacking, free slots, weight). */
	int32 GetAddableQuantity(const FWEItemInstance& Item) const;

	// --- Mutations (server only) --------------------------------------------------------------

	/** Adds as much of Item as fits. OutAdded = units added. */
	EWEInventoryResult TryAddItem(const FWEItemInstance& Item, int32& OutAdded);

	/** Removes up to Quantity from Slot. OutRemoved gets the removed part (same instance id if the whole stack). */
	EWEInventoryResult RemoveFromSlot(int32 Slot, int32 Quantity, FWEItemInstance& OutRemoved);

	/**
	 * Moves Quantity (<= 0 means whole stack) from FromSlot to Target/ToSlot.
	 * Empty target: move. Compatible stack: merge (as much as fits). Different item: swap (whole stacks only).
	 */
	EWEInventoryResult MoveItem(int32 FromSlot, UWEInventoryComponent* Target, int32 ToSlot, int32 Quantity = 0);

	/** Merges stacks and orders by category, name, condition. */
	void SortItems();

	/** Replaces all content (loading). Invalid entries are skipped. */
	void SetAllSlots(const TArray<FWEInventorySlotData>& Slots);
	void ClearAll();

	/** True if the given character may use this inventory (its own, or a container within reach). */
	bool CanBeAccessedBy(const AWECharacter* Character) const;

	FWEOnInventoryChanged OnInventoryChanged;

	/** Called by the fast array callbacks on clients. */
	void NotifyChanged();

protected:
	FWEInventoryEntry* FindEntry(int32 Slot);
	void SetSlot(int32 Slot, const FWEItemInstance& Item);
	void ClearSlot(int32 Slot);
	bool HasAuthorityToModify() const;

	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = "1", ClampMax = "200"))
	int32 NumSlots = 20;

	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = "0", Units = "kg"))
	float MaxWeightKg = 25.f;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	FText DisplayName;

	/** Containers: max distance (cm) for access by a player. */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	float AccessRange = 325.f;

	UPROPERTY(Replicated)
	FWEInventoryList Inventory;

	friend struct FWEInventoryEntry;
};
