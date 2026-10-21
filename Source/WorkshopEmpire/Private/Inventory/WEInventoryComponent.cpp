#include "Inventory/WEInventoryComponent.h"

#include "GameFramework/PlayerState.h"
#include "Interaction/WEInteractionComponent.h"
#include "Items/WEItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Player/WECharacter.h"

// --- Fast array callbacks (clients) --------------------------------------------------------------

void FWEInventoryEntry::PostReplicatedAdd(const FWEInventoryList& InArraySerializer)
{
	if (InArraySerializer.Owner) { InArraySerializer.Owner->NotifyChanged(); }
}

void FWEInventoryEntry::PostReplicatedChange(const FWEInventoryList& InArraySerializer)
{
	if (InArraySerializer.Owner) { InArraySerializer.Owner->NotifyChanged(); }
}

void FWEInventoryEntry::PreReplicatedRemove(const FWEInventoryList& InArraySerializer)
{
	// The entry is still in the list here: tell the listeners once the update is through (PostReplicatedReceive).
	InArraySerializer.bRemovedThisUpdate = true;
}

void FWEInventoryList::PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters& Parameters)
{
	if (bRemovedThisUpdate)
	{
		bRemovedThisUpdate = false;
		if (Owner) { Owner->NotifyChanged(); }
	}
}

// --- Component -------------------------------------------------------------------------------------

UWEInventoryComponent::UWEInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
	SetIsReplicatedByDefault(true);
	Inventory.Owner = this;
}

void UWEInventoryComponent::InitializeComponent()
{
	Super::InitializeComponent();
	Inventory.Owner = this;
}

void UWEInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWEInventoryComponent, Inventory);
}

void UWEInventoryComponent::NotifyChanged()
{
	Inventory.Owner = this;
	OnInventoryChanged.Broadcast(this);
}

bool UWEInventoryComponent::HasAuthorityToModify() const
{
	const AActor* Owner = GetOwner();
	return !Owner || Owner->HasAuthority();
}

// --- Queries -----------------------------------------------------------------------------------------

float UWEInventoryComponent::UnitWeightOf(const FWEItemInstance& Item) const
{
	// Tool bag (upgrade): tools in a player inventory weigh nothing.
	if (bToolsWeightless && Item.Definition && Item.Definition->ToolType != EWEToolType::None)
	{
		return 0.f;
	}
	return Item.GetUnitWeightKg();
}

float UWEInventoryComponent::GetTotalWeightKg() const
{
	float Total = 0.f;
	for (const FWEInventoryEntry& Entry : Inventory.Entries)
	{
		Total += UnitWeightOf(Entry.Item) * Entry.Item.Quantity;
	}
	return Total;
}

const FWEItemInstance* UWEInventoryComponent::GetItemInSlot(int32 Slot) const
{
	for (const FWEInventoryEntry& Entry : Inventory.Entries)
	{
		if (Entry.SlotIndex == Slot)
		{
			return &Entry.Item;
		}
	}
	return nullptr;
}

FWEInventoryEntry* UWEInventoryComponent::FindEntry(int32 Slot)
{
	return Inventory.Entries.FindByPredicate([Slot](const FWEInventoryEntry& E) { return E.SlotIndex == Slot; });
}

int32 UWEInventoryComponent::FindFirstEmptySlot() const
{
	for (int32 Slot = 0; Slot < NumSlots; ++Slot)
	{
		if (!GetItemInSlot(Slot))
		{
			return Slot;
		}
	}
	return INDEX_NONE;
}

int32 UWEInventoryComponent::CountItems(const UWEItemDefinition* Definition) const
{
	int32 Count = 0;
	for (const FWEInventoryEntry& Entry : Inventory.Entries)
	{
		if (Entry.Item.Definition == Definition)
		{
			Count += Entry.Item.Quantity;
		}
	}
	return Count;
}

TArray<FWEInventorySlotData> UWEInventoryComponent::GetAllSlots() const
{
	TArray<FWEInventorySlotData> Result;
	for (const FWEInventoryEntry& Entry : Inventory.Entries)
	{
		Result.Add({ Entry.SlotIndex, Entry.Item });
	}
	Result.Sort([](const FWEInventorySlotData& A, const FWEInventorySlotData& B) { return A.SlotIndex < B.SlotIndex; });
	return Result;
}

int32 UWEInventoryComponent::GetAddableQuantity(const FWEItemInstance& Item) const
{
	if (!Item.IsValid() || (!Item.Definition->bFitsInInventory && !bAcceptsBulky))
	{
		return 0;
	}

	// Weight limit.
	const float UnitWeight = UnitWeightOf(Item);
	const float FreeWeight = MaxWeightKg - GetTotalWeightKg();
	const int32 ByWeight = UnitWeight > KINDA_SMALL_NUMBER ? FMath::FloorToInt32((FreeWeight + KINDA_SMALL_NUMBER) / UnitWeight) : Item.Quantity;
	if (ByWeight <= 0)
	{
		return 0;
	}

	// Space: free room in matching stacks + empty slots.
	const int32 MaxStack = Item.Definition->GetMaxStackSize();
	int32 BySpace = 0;
	for (const FWEInventoryEntry& Entry : Inventory.Entries)
	{
		if (Entry.Item.CanStackWith(Item))
		{
			BySpace += FMath::Max(0, MaxStack - Entry.Item.Quantity);
		}
	}
	const int32 EmptySlots = NumSlots - Inventory.Entries.Num();
	BySpace += FMath::Max(0, EmptySlots) * MaxStack;

	return FMath::Min3(Item.Quantity, ByWeight, BySpace);
}

// --- Mutations ---------------------------------------------------------------------------------------

void UWEInventoryComponent::SetSlot(int32 Slot, const FWEItemInstance& Item)
{
	if (FWEInventoryEntry* Entry = FindEntry(Slot))
	{
		Entry->Item = Item;
		Inventory.MarkItemDirty(*Entry);
	}
	else
	{
		FWEInventoryEntry& NewEntry = Inventory.Entries.AddDefaulted_GetRef();
		NewEntry.SlotIndex = Slot;
		NewEntry.Item = Item;
		Inventory.MarkItemDirty(NewEntry);
	}
}

void UWEInventoryComponent::ClearSlot(int32 Slot)
{
	const int32 Index = Inventory.Entries.IndexOfByPredicate([Slot](const FWEInventoryEntry& E) { return E.SlotIndex == Slot; });
	if (Index != INDEX_NONE)
	{
		Inventory.Entries.RemoveAt(Index);
		Inventory.MarkArrayDirty();
	}
}

EWEInventoryResult UWEInventoryComponent::TryAddItem(const FWEItemInstance& Item, int32& OutAdded)
{
	OutAdded = 0;
	if (!HasAuthorityToModify())
	{
		return EWEInventoryResult::NotAllowed;
	}
	if (!Item.IsValid() || (!Item.Definition->bFitsInInventory && !bAcceptsBulky))
	{
		return EWEInventoryResult::InvalidItem;
	}

	const int32 Addable = GetAddableQuantity(Item);
	if (Addable <= 0)
	{
		const bool bWeight = UnitWeightOf(Item) > MaxWeightKg - GetTotalWeightKg();
		return bWeight ? EWEInventoryResult::TooHeavy : EWEInventoryResult::NoSpace;
	}

	int32 Remaining = Addable;
	const int32 MaxStack = Item.Definition->GetMaxStackSize();

	// 1) Fill existing compatible stacks.
	for (FWEInventoryEntry& Entry : Inventory.Entries)
	{
		if (Remaining <= 0)
		{
			break;
		}
		if (Entry.Item.CanStackWith(Item) && Entry.Item.Quantity < MaxStack)
		{
			const int32 Moved = FMath::Min(Remaining, MaxStack - Entry.Item.Quantity);
			Entry.Item.Quantity += Moved;
			Remaining -= Moved;
			Inventory.MarkItemDirty(Entry);
		}
	}

	// 2) New stacks in empty slots. The first new stack keeps the incoming instance id.
	bool bKeptId = false;
	while (Remaining > 0)
	{
		const int32 Slot = FindFirstEmptySlot();
		if (Slot == INDEX_NONE)
		{
			break;
		}
		FWEItemInstance Stack = Item;
		Stack.Quantity = FMath::Min(Remaining, MaxStack);
		if (bKeptId || !Stack.InstanceId.IsValid())
		{
			Stack.InstanceId = FGuid::NewGuid();
		}
		bKeptId = true;
		SetSlot(Slot, Stack);
		Remaining -= Stack.Quantity;
	}

	OutAdded = Addable - Remaining;
	NotifyChanged();
	return OutAdded == Item.Quantity ? EWEInventoryResult::Success : EWEInventoryResult::PartiallyAdded;
}

EWEInventoryResult UWEInventoryComponent::RemoveFromSlot(int32 Slot, int32 Quantity, FWEItemInstance& OutRemoved)
{
	if (!HasAuthorityToModify())
	{
		return EWEInventoryResult::NotAllowed;
	}
	FWEInventoryEntry* Entry = FindEntry(Slot);
	if (!Entry || Quantity <= 0)
	{
		return EWEInventoryResult::InvalidSlot;
	}

	if (Quantity >= Entry->Item.Quantity)
	{
		OutRemoved = Entry->Item;
		ClearSlot(Slot);
	}
	else
	{
		OutRemoved = Entry->Item.SplitOff(Quantity);
		Inventory.MarkItemDirty(*Entry);
	}
	NotifyChanged();
	return EWEInventoryResult::Success;
}

EWEInventoryResult UWEInventoryComponent::MoveItem(int32 FromSlot, UWEInventoryComponent* Target, int32 ToSlot, int32 Quantity)
{
	if (!HasAuthorityToModify() || !Target || !Target->HasAuthorityToModify())
	{
		return EWEInventoryResult::NotAllowed;
	}
	FWEInventoryEntry* Source = FindEntry(FromSlot);
	if (!Source || ToSlot < 0 || ToSlot >= Target->NumSlots || (Target == this && FromSlot == ToSlot))
	{
		return EWEInventoryResult::InvalidSlot;
	}

	const FWEItemInstance SourceItem = Source->Item;
	const int32 Amount = (Quantity <= 0) ? SourceItem.Quantity : FMath::Min(Quantity, SourceItem.Quantity);
	const float AmountWeight = Target->UnitWeightOf(SourceItem) * Amount;
	const bool bCrossInventory = Target != this;
	const FWEItemInstance* TargetItem = Target->GetItemInSlot(ToSlot);

	if (!TargetItem)
	{
		// Move (or split) into an empty slot.
		if (bCrossInventory && Target->GetTotalWeightKg() + AmountWeight > Target->MaxWeightKg + KINDA_SMALL_NUMBER)
		{
			return EWEInventoryResult::TooHeavy;
		}
		FWEItemInstance Removed;
		RemoveFromSlot(FromSlot, Amount, Removed);
		Target->SetSlot(ToSlot, Removed);
		Target->NotifyChanged();
		return EWEInventoryResult::Success;
	}

	if (TargetItem->CanStackWith(SourceItem))
	{
		// Merge as much as fits into the target stack.
		const int32 Room = SourceItem.Definition->GetMaxStackSize() - TargetItem->Quantity;
		int32 Moved = FMath::Min(Amount, Room);
		if (bCrossInventory && Target->UnitWeightOf(SourceItem) > KINDA_SMALL_NUMBER)
		{
			const float Free = Target->MaxWeightKg - Target->GetTotalWeightKg();
			Moved = FMath::Min(Moved, FMath::FloorToInt32((Free + KINDA_SMALL_NUMBER) / Target->UnitWeightOf(SourceItem)));
		}
		if (Moved <= 0)
		{
			return Room <= 0 ? EWEInventoryResult::NoSpace : EWEInventoryResult::TooHeavy;
		}
		FWEItemInstance Removed;
		RemoveFromSlot(FromSlot, Moved, Removed);
		FWEInventoryEntry* TargetEntry = Target->FindEntry(ToSlot);
		TargetEntry->Item.Quantity += Moved;
		Target->Inventory.MarkItemDirty(*TargetEntry);
		Target->NotifyChanged();
		return Moved == Amount ? EWEInventoryResult::Success : EWEInventoryResult::PartiallyAdded;
	}

	// Swap whole stacks.
	if (Amount != SourceItem.Quantity)
	{
		return EWEInventoryResult::InvalidSlot;
	}
	const FWEItemInstance Other = *TargetItem;
	if (bCrossInventory)
	{
		const float TargetAfter = Target->GetTotalWeightKg() - Target->UnitWeightOf(Other) * Other.Quantity + Target->UnitWeightOf(SourceItem) * SourceItem.Quantity;
		const float SourceAfter = GetTotalWeightKg() - UnitWeightOf(SourceItem) * SourceItem.Quantity + UnitWeightOf(Other) * Other.Quantity;
		if (TargetAfter > Target->MaxWeightKg + KINDA_SMALL_NUMBER || SourceAfter > MaxWeightKg + KINDA_SMALL_NUMBER)
		{
			return EWEInventoryResult::TooHeavy;
		}
	}
	Target->SetSlot(ToSlot, SourceItem);
	SetSlot(FromSlot, Other);
	NotifyChanged();
	if (bCrossInventory)
	{
		Target->NotifyChanged();
	}
	return EWEInventoryResult::Success;
}

int32 UWEInventoryComponent::TransferAllTo(UWEInventoryComponent* Target)
{
	if (!HasAuthorityToModify() || !Target || Target == this || !Target->HasAuthorityToModify())
	{
		return 0;
	}
	int32 Moved = 0;
	for (const FWEInventorySlotData& Slot : GetAllSlots())
	{
		const int32 Movable = Target->GetAddableQuantity(Slot.Item);
		if (Movable <= 0)
		{
			continue;
		}
		FWEItemInstance Removed;
		RemoveFromSlot(Slot.SlotIndex, Movable, Removed);
		int32 Added = 0;
		Target->TryAddItem(Removed, Added);
		if (Added < Removed.Quantity)
		{
			// Never lose items: put back what did not fit.
			FWEItemInstance Rest = Removed;
			Rest.Quantity -= Added;
			int32 Back = 0;
			TryAddItem(Rest, Back);
		}
		Moved += Added;
	}
	return Moved;
}

void UWEInventoryComponent::SortItems(uint8 Mode)
{
	if (!HasAuthorityToModify())
	{
		return;
	}

	// Merge compatible stacks first.
	TArray<FWEItemInstance> Items;
	for (const FWEInventoryEntry& Entry : Inventory.Entries)
	{
		bool bMerged = false;
		for (FWEItemInstance& Existing : Items)
		{
			if (Existing.CanStackWith(Entry.Item))
			{
				const int32 Room = Existing.Definition->GetMaxStackSize() - Existing.Quantity;
				const int32 Moved = FMath::Min(Room, Entry.Item.Quantity);
				Existing.Quantity += Moved;
				if (Moved < Entry.Item.Quantity)
				{
					FWEItemInstance Rest = Entry.Item;
					Rest.Quantity -= Moved;
					Items.Add(Rest);
				}
				bMerged = true;
				break;
			}
		}
		if (!bMerged)
		{
			Items.Add(Entry.Item);
		}
	}

	Items.Sort([Mode](const FWEItemInstance& A, const FWEItemInstance& B)
	{
		// Phase 19: other orders - by name, by value (most valuable first), by weight (heaviest first).
		if (Mode == 1)
		{
			const int32 ByName = A.GetDisplayName().CompareTo(B.GetDisplayName());
			if (ByName != 0)
			{
				return ByName < 0;
			}
		}
		else if (Mode == 2 && A.GetEstimatedValue() != B.GetEstimatedValue())
		{
			return A.GetEstimatedValue() > B.GetEstimatedValue();
		}
		else if (Mode == 3 && !FMath::IsNearlyEqual(A.GetTotalWeightKg(), B.GetTotalWeightKg()))
		{
			return A.GetTotalWeightKg() > B.GetTotalWeightKg();
		}
		if (A.Definition->Category != B.Definition->Category)
		{
			return A.Definition->Category < B.Definition->Category;
		}
		const int32 NameCompare = A.GetDisplayName().CompareTo(B.GetDisplayName());
		if (NameCompare != 0)
		{
			return NameCompare < 0;
		}
		return A.Condition < B.Condition;
	});

	Inventory.Entries.Reset();
	for (int32 i = 0; i < Items.Num() && i < NumSlots; ++i)
	{
		FWEInventoryEntry& Entry = Inventory.Entries.AddDefaulted_GetRef();
		Entry.SlotIndex = i;
		Entry.Item = Items[i];
	}
	Inventory.MarkArrayDirty();
	NotifyChanged();
}

EWEInventoryResult UWEInventoryComponent::SetSlotMarks(int32 Slot, uint8 Mask, bool bSet)
{
	if (!HasAuthorityToModify())
	{
		return EWEInventoryResult::NotAllowed;
	}
	const FWEItemInstance* Item = GetItemInSlot(Slot);
	if (!Item)
	{
		return EWEInventoryResult::InvalidSlot;
	}
	FWEItemInstance Changed = *Item;
	Changed.Marks = bSet ? (Changed.Marks | Mask) : (Changed.Marks & ~Mask);
	if (Changed.Marks != Item->Marks)
	{
		SetSlot(Slot, Changed);
		NotifyChanged();
	}
	return EWEInventoryResult::Success;
}

void UWEInventoryComponent::SetAllSlots(const TArray<FWEInventorySlotData>& Slots)
{
	if (!HasAuthorityToModify())
	{
		return;
	}
	Inventory.Entries.Reset();
	for (const FWEInventorySlotData& Data : Slots)
	{
		if (Data.Item.IsValid() && Data.SlotIndex >= 0 && Data.SlotIndex < NumSlots && !GetItemInSlot(Data.SlotIndex))
		{
			FWEInventoryEntry& Entry = Inventory.Entries.AddDefaulted_GetRef();
			Entry.SlotIndex = Data.SlotIndex;
			Entry.Item = Data.Item;
		}
	}
	Inventory.MarkArrayDirty();
	NotifyChanged();
}

void UWEInventoryComponent::ClearAll()
{
	SetAllSlots({});
}

bool UWEInventoryComponent::CanBeAccessedBy(const AWECharacter* Character) const
{
	if (!Character)
	{
		return false;
	}
	const AActor* Owner = GetOwner();
	if (Owner == Character || Owner == Character->GetPlayerState() || Owner == Character->GetController())
	{
		return true;
	}
	// Player inventories (owned by a PlayerState) are private.
	if (Owner && Owner->IsA<APlayerState>())
	{
		return false;
	}
	return UWEInteractionComponent::IsWithinReach(Character, Owner, AccessRange);
}
