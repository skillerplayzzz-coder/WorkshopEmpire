#include "UI/WEInventorySlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Framework/WEPlayerController.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "UI/WEInventoryWidget.h"
#include "UI/WEUIKit.h"

#define LOCTEXT_NAMESPACE "WEInventorySlot"

using namespace WEUI;

void UWEInventorySlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(false);
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(TileSize);
	Size->SetHeightOverride(TileSize);
	WidgetTree->RootWidget = Size;

	Background = Box(WidgetTree, Col::PanelDark, FMargin(0.f), Col::Line, 2.f);
	Background->SetClipping(EWidgetClipping::ClipToBounds);
	Size->SetContent(Background);

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>();
	Background->SetContent(Overlay);
	auto Add = [Overlay](UWidget* W, EHorizontalAlignment H, EVerticalAlignment V, const FMargin& Pad)
	{
		UOverlaySlot* S = Overlay->AddChildToOverlay(W);
		S->SetHorizontalAlignment(H);
		S->SetVerticalAlignment(V);
		S->SetPadding(Pad);
	};

	// Picture of the item (or a category icon), centred a little above the name.
	ItemImage = WidgetTree->ConstructWidget<UImage>();
	ItemImage->SetDesiredSizeOverride(FVector2D(66.f, 66.f));
	// A fixed box: the picture always fills 66 x 66 (the image's own size override was not reliable).
	Add(Sized(WidgetTree, ItemImage, 66.f, 66.f), HAlign_Center, VAlign_Center, FMargin(0.f, 0.f, 0.f, 10.f));
	FallbackIcon = Icon(WidgetTree, TEXT("Box"), 30.f, Col::TextMuted);
	Add(FallbackIcon, HAlign_Center, VAlign_Center, FMargin(0.f, 0.f, 0.f, 14.f));

	NameText = Text(WidgetTree, FText::GetEmpty(), 8, Col::TextDim, EFont::Head);
	NameText->SetJustification(ETextJustify::Center);
	NameText->SetClipping(EWidgetClipping::ClipToBounds);
	NameText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	Add(NameText, HAlign_Fill, VAlign_Bottom, FMargin(3.f, 0.f, 3.f, 5.f));

	QuantityText = Text(WidgetTree, FText::GetEmpty(), 11, Col::Text, EFont::Head);
	QuantityText->SetShadowOffset(FVector2D(1.f, 1.f));
	QuantityText->SetShadowColorAndOpacity(Col::WithAlpha(FLinearColor::Black, 0.9f));
	Add(QuantityText, HAlign_Right, VAlign_Top, FMargin(0.f, 3.f, 5.f, 0.f));

	FavoriteBadge = Icon(WidgetTree, TEXT("Star"), 13.f, Col::Accent);
	Add(FavoriteBadge, HAlign_Left, VAlign_Top, FMargin(4.f, 4.f, 0.f, 0.f));
	LockBadge = Icon(WidgetTree, TEXT("Lock"), 13.f, Col::Text);
	Add(LockBadge, HAlign_Left, VAlign_Top, FMargin(4.f, 20.f, 0.f, 0.f));

	ConditionStrip = Box(WidgetTree, FLinearColor::White, FMargin(0.f, 1.5f), FLinearColor::Transparent, 0.f); // colour via SetBrushColor
	ConditionStrip->SetBrushColor(FLinearColor::Transparent);
	Add(ConditionStrip, HAlign_Fill, VAlign_Bottom, FMargin(0.f));
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

void UWEInventorySlotWidget::ApplyFrame()
{
	const FWEItemInstance* Item = Inventory ? Inventory->GetItemInSlot(SlotIndex) : nullptr;
	FLinearColor Fill = Item ? Col::Raised : Col::PanelDark;
	FLinearColor Outline = Col::Line;
	if (Item && Item->Definition && Item->Definition->Rarity >= EWEItemRarity::Rare)
	{
		Outline = Col::WithAlpha(RarityColor(static_cast<uint8>(Item->Definition->Rarity)), 0.85f);
	}
	if (bHovered && Item)
	{
		Fill = Col::RaisedHover;
		Outline = Col::LineStrong;
	}
	if (bDropTarget)
	{
		Fill = Col::WithAlpha(Col::Accent, 0.22f);
		Outline = Col::Accent;
	}
	Background->SetBrush(Brush(Fill, Outline, 1.f, 2.f));
}

void UWEInventorySlotWidget::Refresh()
{
	if (!Background)
	{
		return;
	}
	const FWEItemInstance* Item = Inventory ? Inventory->GetItemInSlot(SlotIndex) : nullptr;
	ApplyFrame();
	if (!Item || !Item->IsValid())
	{
		ItemImage->SetVisibility(ESlateVisibility::Collapsed);
		FallbackIcon->SetVisibility(ESlateVisibility::Collapsed);
		NameText->SetText(FText::GetEmpty());
		QuantityText->SetText(FText::GetEmpty());
		ConditionStrip->SetBrushColor(FLinearColor::Transparent);
		FavoriteBadge->SetVisibility(ESlateVisibility::Collapsed);
		LockBadge->SetVisibility(ESlateVisibility::Collapsed);
		SetToolTipText(FText::GetEmpty());
		return;
	}
	if (UTexture2D* Tex = ItemIcon(Item->Definition))
	{
		ItemImage->SetBrushFromTexture(Tex);
		ItemImage->SetDesiredSizeOverride(FVector2D(66.f, 66.f));
		ItemImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		FallbackIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		ItemImage->SetVisibility(ESlateVisibility::Collapsed);
		if (UTexture2D* Tex2 = IconTexture(CategoryIcon(static_cast<uint8>(Item->Definition->Category))))
		{
			FallbackIcon->SetBrushFromTexture(Tex2);
			FallbackIcon->SetDesiredSizeOverride(FVector2D(30.f, 30.f));
		}
		FallbackIcon->SetColorAndOpacity(Item->Definition->Tint * 0.6f + FLinearColor(0.35f, 0.35f, 0.35f, 1.f));
		FallbackIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	// Phase 19: long names end in "…" instead of being cut off at both edges (the full name is in the details).
	FString Short = Item->GetDisplayName().ToString();
	if (Short.Len() > 12)
	{
		Short = Short.Left(11).TrimEnd() + TEXT("…");
	}
	NameText->SetText(FText::FromString(Short));
	QuantityText->SetText(Item->Quantity > 1 ? FText::Format(LOCTEXT("Qty", "{0}×"), FText::AsNumber(Item->Quantity)) : FText::GetEmpty());
	ConditionStrip->SetBrushColor(WEItems::GetConditionColor(Item->Condition));
	FavoriteBadge->SetVisibility(Item->IsFavorite() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	// Locked: lock; equipment that is protected by default: a shield.
	const bool bLock = Item->IsLocked();
	const bool bShield = !bLock && Item->IsPersonalEquipment() && !Item->HasMark(FWEItemInstance::ReleasedForSale);
	LockBadge->SetVisibility(bLock || bShield ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (UTexture2D* Badge = IconTexture(bLock ? FName(TEXT("Lock")) : FName(TEXT("Shield"))))
	{
		LockBadge->SetBrushFromTexture(Badge);
		LockBadge->SetDesiredSizeOverride(FVector2D(13.f, 13.f));
	}
	LockBadge->SetColorAndOpacity(bLock ? Col::Text : Col::Info);
	SetToolTipText(FText::GetEmpty()); // details panel instead of a tooltip
}

void UWEInventorySlotWidget::SetHighlight(bool bOn)
{
	bDropTarget = bOn;
	ApplyFrame();
}

void UWEInventorySlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	bHovered = true;
	ApplyFrame();
	if (OwnerWidget)
	{
		OwnerWidget->ShowDetails(this);
	}
}

void UWEInventorySlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bHovered = false;
	ApplyFrame();
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
		if (OwnerWidget)
		{
			OwnerWidget->OpenContextMenu(this, InMouseEvent.GetScreenSpacePosition());
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply UWEInventorySlotWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UWEInventorySlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	UWEInventoryComponent* Other = OwnerWidget ? OwnerWidget->GetOtherInventory(Inventory) : nullptr;
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer());
	if (Other && PC && Inventory && Inventory->GetItemInSlot(SlotIndex))
	{
		PC->RequestQuickTransfer(Inventory, SlotIndex, Other);
		Play(this, ESound::Drop, 0.5f);
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
	Visual->SetRenderOpacity(0.85f);
	Visual->SetRenderTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(1.08f, 1.08f), FVector2D::ZeroVector, 3.f));
	Operation->DefaultDragVisual = Visual;
	Operation->Pivot = EDragPivot::CenterCenter;
	OutOperation = Operation;
	Play(this, ESound::Click, 0.4f);
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
	Play(this, ESound::Drop, 0.5f);
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
