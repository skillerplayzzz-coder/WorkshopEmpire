#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WEInventoryWidget.generated.h"

class UHorizontalBox;
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
	UPROPERTY(Transient) TObjectPtr<UUniformGridPanel> Grid;
	UPROPERTY(Transient) TArray<TObjectPtr<UWEInventorySlotWidget>> Slots;
};

/**
 * Inventory screen: the player inventory, optionally next to a container (shelf, workbench).
 * Built in C++ (no widget asset). Only sends requests through AWEPlayerController; the server
 * decides, and the slots refresh when the replicated inventory changes.
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

	static constexpr int32 Columns = 5;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UFUNCTION()
	void HandleSortPlayer();

	UFUNCTION()
	void HandleSortContainer();

	void BuildPanel(FWEInventoryPanel& Panel, UWEInventoryComponent* Inventory, bool bIsPlayer);
	void RefreshPanel(FWEInventoryPanel& Panel);
	void HandleInventoryChanged(UWEInventoryComponent* Inventory);
	void Unbind();

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> Columns_Box;

	UPROPERTY(Transient)
	FWEInventoryPanel PlayerPanel;

	UPROPERTY(Transient)
	FWEInventoryPanel ContainerPanel;

	FDelegateHandle PlayerHandle;
	FDelegateHandle ContainerHandle;
};
