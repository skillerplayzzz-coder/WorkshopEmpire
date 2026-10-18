#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/UserWidget.h"
#include "WEInventorySlotWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UWEInventoryComponent;
class UWEInventoryWidget;
struct FWEItemInstance;

/** Payload while dragging an inventory stack. */
UCLASS()
class WORKSHOPEMPIRE_API UWEItemDragOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UWEInventoryComponent> SourceInventory;

	int32 SourceSlot = INDEX_NONE;

	/** Units to move; 0 = whole stack. */
	int32 Quantity = 0;
};

/**
 * One inventory slot (Phase 17.5 look): picture of the item, short name, stack count, condition strip and
 * badges for favourite / locked / protected equipment. Left-drag moves (Shift: half stack), double-click moves
 * to the other open inventory, right-click opens the context menu (drop, favourite, lock, release), Q drops.
 * Hovering shows the item in the details panel of the inventory screen.
 */
UCLASS()
class WORKSHOPEMPIRE_API UWEInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Init(UWEInventoryComponent* InInventory, int32 InSlot, UWEInventoryWidget* InOwner);
	void Refresh();

	UWEInventoryComponent* GetInventory() const { return Inventory; }
	int32 GetSlotIndex() const { return SlotIndex; }

	/** Tooltip text for an item (public for tests). */
	static FText BuildTooltip(const FWEItemInstance& Item);

	static constexpr float TileSize = 78.f;

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	void SetHighlight(bool bOn);
	void ApplyFrame();

	UPROPERTY(Transient) TObjectPtr<UWEInventoryComponent> Inventory;
	UPROPERTY(Transient) TObjectPtr<UWEInventoryWidget> OwnerWidget;

	int32 SlotIndex = INDEX_NONE;
	bool bShiftOnPress = false;
	bool bHovered = false;
	bool bDropTarget = false;

	UPROPERTY(Transient) TObjectPtr<UBorder> Background;
	UPROPERTY(Transient) TObjectPtr<UBorder> ConditionStrip;
	UPROPERTY(Transient) TObjectPtr<UImage> ItemImage;
	UPROPERTY(Transient) TObjectPtr<UImage> FallbackIcon;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> QuantityText;
	UPROPERTY(Transient) TObjectPtr<UImage> FavoriteBadge;
	UPROPERTY(Transient) TObjectPtr<UImage> LockBadge;
};
