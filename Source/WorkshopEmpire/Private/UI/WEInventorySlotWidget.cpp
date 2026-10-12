#include "UI/WEInventorySlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Framework/WEPlayerController.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Styling/CoreStyle.h"
#include "UI/WEInventoryWidget.h"

#define LOCTEXT_NAMESPACE "WEInventorySlot"

namespace WESlotStyle
{
	const FLinearColor Empty(0.06f, 0.065f, 0.07f, 0.9f);
	const FLinearColor Highlight(0.35f, 0.30f, 0.12f, 0.95f);
	constexpr float Size = 76.f;
}

void UWEInventorySlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
	Box->SetWidthOverride(WESlotStyle::Size);
	Box->SetHeightOverride(WESlotStyle::Size);
	WidgetTree->RootWidget = Box;

	Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(WESlotStyle::Empty);
	Background->SetPadding(FMargin(4.f));
	Background->SetClipping(EWidgetClipping::ClipToBounds); // long names must not spill into the next slot
	Box->SetContent(Background);

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>();
	Background->SetContent(Overlay);

	NameText = WidgetTree->ConstructWidget<UTextBlock>();
	NameText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 8));
	NameText->SetAutoWrapText(true);
	NameText->SetWrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping);
	NameText->SetShadowOffset(FVector2D(1.f, 1.f));
	NameText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
	if (UOverlaySlot* OSlot = Overlay->AddChildToOverlay(NameText))
	{
		OSlot->SetHorizontalAlignment(HAlign_Fill);
		OSlot->SetVerticalAlignment(VAlign_Top);
	}

	QuantityText = WidgetTree->ConstructWidget<UTextBlock>();
	QuantityText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
	QuantityText->SetShadowOffset(FVector2D(1.f, 1.f));
	QuantityText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
	if (UOverlaySlot* OSlot = Overlay->AddChildToOverlay(QuantityText))
	{
		OSlot->SetHorizontalAlignment(HAlign_Right);
		OSlot->SetVerticalAlignment(VAlign_Bottom);
		OSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	ConditionStrip = WidgetTree->ConstructWidget<UBorder>();
	ConditionStrip->SetPadding(FMargin(0.f, 2.f));
	if (UOverlaySlot* OSlot = Overlay->AddChildToOverlay(ConditionStrip))
	{
		OSlot->SetHorizontalAlignment(HAlign_Fill);
		OSlot->SetVerticalAlignment(VAlign_Bottom);
	}
}

void UWEInventorySlotWidget::Init(UWEInventoryComponent* InInventory, int32 InSlot, UWEInventoryWidget* InOwner)
{
	Inventory = InInventory;
	SlotIndex = InSlot;
	OwnerWidget = InOwner;
	Refresh();
}

FText UWEInventorySlotWidget::BuildTooltip(const FWEItemInstance& Item)
{
	if (!Item.IsValid())
	{
		return FText::GetEmpty();
	}
	const UWEItemDefinition* Def = Item.Definition;
	return FText::Format(LOCTEXT("TooltipFmt", "{0}\n{1}\n\nKategorie: {2}\nZustand: {3} (Qualität {4} %)\nMenge: {5}\nGewicht: {6}\nSchätzwert: {7}"),
		Item.GetDisplayName(), Def->Description, WEItems::GetCategoryText(Def->Category),
		WEItems::GetConditionText(Item.Condition), FText::AsNumber(Item.Quality), FText::AsNumber(Item.Quantity),
		AWEPickupItem::FormatWeight(Item.GetTotalWeightKg()), FText::FromString(Item.GetEstimatedValue().ToDisplayString()));
}

void UWEInventorySlotWidget::Refresh()
{
	if (!Background)
	{
		return;
	}
	const FWEItemInstance* Item = Inventory ? Inventory->GetItemInSlot(SlotIndex) : nullptr;
	if (!Item || !Item->IsValid())
	{
		Background->SetBrushColor(WESlotStyle::Empty);
		NameText->SetText(FText::GetEmpty());
		QuantityText->SetText(FText::GetEmpty());
		ConditionStrip->SetBrushColor(FLinearColor::Transparent);
		SetToolTipText(FText::GetEmpty());
		return;
	}

	const FLinearColor Tint = Item->Definition->Tint;
	const FLinearColor Tile(Tint.R * 0.55f + 0.05f, Tint.G * 0.55f + 0.05f, Tint.B * 0.55f + 0.05f, 0.95f);
	Background->SetBrushColor(Tile);
	// Dark text on light tiles, light text on dark tiles.
	const bool bLightTile = Tile.GetLuminance() > 0.35f;
	const FSlateColor TextColor(bLightTile ? FLinearColor(0.05f, 0.05f, 0.05f) : FLinearColor::White);
	const FLinearColor Shadow = bLightTile ? FLinearColor::Transparent : FLinearColor(0.f, 0.f, 0.f, 0.8f);
	NameText->SetColorAndOpacity(TextColor);
	NameText->SetShadowColorAndOpacity(Shadow);
	QuantityText->SetColorAndOpacity(TextColor);
	QuantityText->SetShadowColorAndOpacity(Shadow);
	NameText->SetText(Item->GetDisplayName());
	QuantityText->SetText(Item->Quantity > 1 ? FText::AsNumber(Item->Quantity) : FText::GetEmpty());
	ConditionStrip->SetBrushColor(WEItems::GetConditionColor(Item->Condition));
	SetToolTipText(BuildTooltip(*Item));
}

void UWEInventorySlotWidget::SetHighlight(bool bOn)
{
	if (bOn)
	{
		Background->SetBrushColor(WESlotStyle::Highlight);
	}
	else
	{
		Refresh();
	}
}

FReply UWEInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FWEItemInstance* Item = Inventory ? Inventory->GetItemInSlot(SlotIndex) : nullptr;
	if (!Item)
	{
		return FReply::Unhandled();
	}
	bShiftOnPress = InMouseEvent.IsShiftDown();

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
		{
			PC->RequestDropFromInventory(Inventory, SlotIndex, bShiftOnPress ? 1 : 0);
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply UWEInventorySlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	UWEInventoryComponent* Other = OwnerWidget ? OwnerWidget->GetOtherInventory(Inventory) : nullptr;
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer());
	if (Other && PC && Inventory && Inventory->GetItemInSlot(SlotIndex))
	{
		PC->RequestQuickTransfer(Inventory, SlotIndex, Other);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void UWEInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	const FWEItemInstance* Item = Inventory ? Inventory->GetItemInSlot(SlotIndex) : nullptr;
	if (!Item)
	{
		return;
	}
	UWEItemDragOperation* Operation = NewObject<UWEItemDragOperation>(this);
	Operation->SourceInventory = Inventory;
	Operation->SourceSlot = SlotIndex;
	Operation->Quantity = (bShiftOnPress && Item->Quantity > 1) ? Item->Quantity / 2 : 0;

	UWEInventorySlotWidget* Visual = CreateWidget<UWEInventorySlotWidget>(GetOwningPlayer(), GetClass());
	Visual->Init(Inventory, SlotIndex, nullptr);
	Visual->SetRenderOpacity(0.8f);
	Operation->DefaultDragVisual = Visual;
	Operation->Pivot = EDragPivot::CenterCenter;
	OutOperation = Operation;
}

bool UWEInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	SetHighlight(false);
	const UWEItemDragOperation* Operation = Cast<UWEItemDragOperation>(InOperation);
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer());
	if (!Operation || !PC || !Inventory)
	{
		return false;
	}
	if (Operation->SourceInventory == Inventory && Operation->SourceSlot == SlotIndex)
	{
		return true; // dropped on itself
	}
	PC->RequestMoveItem(Operation->SourceInventory, Operation->SourceSlot, Inventory, SlotIndex, Operation->Quantity);
	return true;
}

void UWEInventorySlotWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (Cast<UWEItemDragOperation>(InOperation))
	{
		SetHighlight(true);
	}
}

void UWEInventorySlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	SetHighlight(false);
}

#undef LOCTEXT_NAMESPACE
