#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/WEUIKit.h"
#include "WEInventoryWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UHorizontalBox;
class UImage;
class UProgressBar;
class UTextBlock;
class UUniformGridPanel;
class UVerticalBox;
class UWEInventoryComponent;
class UWEInventorySlotWidget;

/** UI state of one inventory column (player or container). */
USTRUCT()
struct FWEInventoryPanel
{
	GENERATED_BODY()

	UPROPERTY(Transient) TObjectPtr<UWEInventoryComponent> Inventory;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Root;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Summary;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> WeightBar;
	UPROPERTY(Transient) TObjectPtr<UUniformGridPanel> Grid;
	UPROPERTY(Transient) TArray<TObjectPtr<UWEInventorySlotWidget>> Slots;
	/** Phase 19: search field (dims what does not match) and the order the sort button uses. */
	UPROPERTY(Transient) TObjectPtr<class UEditableTextBox> Search;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SortLabel;
	FString Filter;
	uint8 SortMode = 0;
};

/**
 * Inventory screen (Phase 17.5 look): the player inventory, optionally next to a container (shelf, workbench),
 * and a details panel for the item under the mouse. Right-click opens a context menu (drop, favourite, lock,
 * release equipment for sale). Built in C++ (no widget asset). Only sends requests through
 * AWEPlayerController; the server decides, and the slots refresh when the replicated inventory changes.
 * Dropping a dragged stack outside both panels puts it into the world.
 */
UCLASS()
class WORKSHOPEMPIRE_API UWEInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UWEInventoryComponent* PlayerInventory, UWEInventoryComponent* Container);

	/** The inventory shown in the other column (for double-click transfer), or nullptr. */
	UWEInventoryComponent* GetOtherInventory(const UWEInventoryComponent* Inventory) const;

	/** Details panel shows the item in this slot (hover). */
	void ShowDetails(UWEInventorySlotWidget* SlotWidget);
	/** Right-click menu for the stack in this slot. */
	void OpenContextMenu(UWEInventorySlotWidget* SlotWidget, const FVector2D& ScreenPosition);
	void CloseContextMenu();

	/** Dev runs / tests: details and context menu for a slot of the player inventory. */
	void DevShowPlayerSlot(int32 SlotIndex, bool bOpenMenu);

	static constexpr int32 Columns = 5;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UFUNCTION() void HandleSortPlayer();
	UFUNCTION() void HandleSortContainer();
	UFUNCTION() void HandleTakeAll();
	UFUNCTION() void HandleSearchPlayer(const FText& Value);
	UFUNCTION() void HandleSearchContainer(const FText& Value);
	UFUNCTION() void HandleSortModePlayer();
	UFUNCTION() void HandleSortModeContainer();
	void ApplyFilter(FWEInventoryPanel& Panel);
	static FText SortModeText(uint8 Mode);
	UFUNCTION() void HandleMenuDrop();
	UFUNCTION() void HandleMenuTakeInHand();
	UFUNCTION() void HandleMenuDropOne();
	UFUNCTION() void HandleMenuFavorite();
	UFUNCTION() void HandleMenuLock();
	UFUNCTION() void HandleMenuRelease();

	void BuildPanel(FWEInventoryPanel& Panel, UWEInventoryComponent* Inventory, bool bIsPlayer);
	void RefreshPanel(FWEInventoryPanel& Panel);
	void RefreshDetails();
	void HandleInventoryChanged(UWEInventoryComponent* Inventory);
	void Unbind();
	const struct FWEItemInstance* GetMenuItem() const;

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> RootCanvas;
	UPROPERTY(Transient) TObjectPtr<UWidget> Content;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> Columns_Box;
	UPROPERTY(Transient) FWEInventoryPanel PlayerPanel;
	UPROPERTY(Transient) FWEInventoryPanel ContainerPanel;

	// Details panel
	UPROPERTY(Transient) TObjectPtr<UWidget> DetailsCard;
	UPROPERTY(Transient) TObjectPtr<UImage> DetailImage;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailName;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> DetailChips;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailDescription;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> DetailRows;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailProtection;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailEmpty;
	TWeakObjectPtr<UWEInventoryComponent> DetailInventory;
	int32 DetailSlot = INDEX_NONE;

	// Context menu
	UPROPERTY(Transient) TObjectPtr<UBorder> ContextMenu;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MenuTitle;
	UPROPERTY(Transient) TObjectPtr<UButton> MenuDropOne;
	UPROPERTY(Transient) TObjectPtr<UButton> MenuTakeInHand;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MenuFavoriteText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MenuLockText;
	UPROPERTY(Transient) TObjectPtr<UButton> MenuRelease;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MenuReleaseText;
	TWeakObjectPtr<UWEInventoryComponent> MenuInventory;
	int32 MenuSlot = INDEX_NONE;

	WEUI::FOpenAnim OpenAnim;
	FDelegateHandle PlayerHandle;
	FDelegateHandle ContainerHandle;
};
