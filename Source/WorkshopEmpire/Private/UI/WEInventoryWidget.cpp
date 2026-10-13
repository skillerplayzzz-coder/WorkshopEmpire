#include "UI/WEInventoryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/WEPlayerController.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "Styling/CoreStyle.h"
#include "UI/WEInventorySlotWidget.h"

#define LOCTEXT_NAMESPACE "WEInventoryWidget"

namespace WEInvStyle
{
	UTextBlock* MakeText(UWidgetTree* Tree, int32 Size, const FLinearColor& Color, const char* Typeface = "Regular")
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
		Text->SetFont(FCoreStyle::GetDefaultFontStyle(Typeface, Size));
		Text->SetColorAndOpacity(FSlateColor(Color));
		return Text;
	}
}

void UWEInventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	// Full-screen dim background (also the drop target for "put into the world").
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.45f));
	Dim->SetHorizontalAlignment(HAlign_Center);
	Dim->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Dim;

	UVerticalBox* Outer = WidgetTree->ConstructWidget<UVerticalBox>();
	Dim->SetContent(Outer);

	Columns_Box = WidgetTree->ConstructWidget<UHorizontalBox>();
	Outer->AddChildToVerticalBox(Columns_Box);

	UTextBlock* Help = WEInvStyle::MakeText(WidgetTree, 11, FLinearColor(0.75f, 0.75f, 0.75f));
	Help->SetText(LOCTEXT("Help", "Ziehen: verschieben  ·  Shift+Ziehen: halber Stapel  ·  Doppelklick: umlagern  ·  Rechtsklick: ablegen (Shift: 1 Stück)  ·  Aus dem Fenster ziehen: ablegen  ·  Tab/Esc: schließen"));
	if (UVerticalBoxSlot* HelpSlot = Outer->AddChildToVerticalBox(Help))
	{
		HelpSlot->SetHorizontalAlignment(HAlign_Center);
		HelpSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	}
}

void UWEInventoryWidget::BuildPanel(FWEInventoryPanel& Panel, UWEInventoryComponent* Inventory, bool bIsPlayer)
{
	Panel.Inventory = Inventory;
	Panel.Slots.Reset();

	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
	Frame->SetBrushColor(FLinearColor(0.025f, 0.027f, 0.03f, 0.92f));
	Frame->SetPadding(FMargin(14.f));
	if (UHorizontalBoxSlot* FrameSlot = Columns_Box->AddChildToHorizontalBox(Frame))
	{
		FrameSlot->SetPadding(FMargin(10.f, 0.f));
		FrameSlot->SetVerticalAlignment(VAlign_Top);
	}

	Panel.Root = WidgetTree->ConstructWidget<UVerticalBox>();
	Frame->SetContent(Panel.Root);

	Panel.Title = WEInvStyle::MakeText(WidgetTree, 18, FLinearColor(1.f, 0.78f, 0.35f), "Bold");
	Panel.Root->AddChildToVerticalBox(Panel.Title);

	Panel.Summary = WEInvStyle::MakeText(WidgetTree, 11, FLinearColor(0.8f, 0.85f, 0.9f));
	if (UVerticalBoxSlot* SummarySlot = Panel.Root->AddChildToVerticalBox(Panel.Summary))
	{
		SummarySlot->SetPadding(FMargin(0.f, 2.f, 0.f, 8.f));
	}

	Panel.Grid = WidgetTree->ConstructWidget<UUniformGridPanel>();
	Panel.Grid->SetSlotPadding(FMargin(2.f));
	Panel.Root->AddChildToVerticalBox(Panel.Grid);

	const int32 NumSlots = Inventory ? Inventory->GetNumSlots() : 0;
	for (int32 i = 0; i < NumSlots; ++i)
	{
		UWEInventorySlotWidget* SlotWidget = CreateWidget<UWEInventorySlotWidget>(GetOwningPlayer(), UWEInventorySlotWidget::StaticClass());
		SlotWidget->Init(Inventory, i, this);
		Panel.Grid->AddChildToUniformGrid(SlotWidget, i / Columns, i % Columns);
		Panel.Slots.Add(SlotWidget);
	}

	UButton* SortButton = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* SortLabel = WEInvStyle::MakeText(WidgetTree, 12, FLinearColor(0.05f, 0.05f, 0.05f));
	SortLabel->SetText(LOCTEXT("Sort", "Sortieren"));
	SortButton->SetContent(SortLabel);
	if (bIsPlayer)
	{
		SortButton->OnClicked.AddDynamic(this, &UWEInventoryWidget::HandleSortPlayer);
	}
	else
	{
		SortButton->OnClicked.AddDynamic(this, &UWEInventoryWidget::HandleSortContainer);
	}
	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
	if (!bIsPlayer)
	{
		UButton* TakeAllButton = WidgetTree->ConstructWidget<UButton>();
		UTextBlock* TakeAllLabel = WEInvStyle::MakeText(WidgetTree, 12, FLinearColor(0.05f, 0.05f, 0.05f));
		TakeAllLabel->SetText(LOCTEXT("TakeAll", "Alles nehmen"));
		TakeAllButton->SetContent(TakeAllLabel);
		TakeAllButton->OnClicked.AddDynamic(this, &UWEInventoryWidget::HandleTakeAll);
		if (UHorizontalBoxSlot* TakeSlot = Buttons->AddChildToHorizontalBox(TakeAllButton))
		{
			TakeSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
		}
	}
	Buttons->AddChildToHorizontalBox(SortButton);
	if (UVerticalBoxSlot* ButtonSlot = Panel.Root->AddChildToVerticalBox(Buttons))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Right);
		ButtonSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}

	RefreshPanel(Panel);
}

void UWEInventoryWidget::RefreshPanel(FWEInventoryPanel& Panel)
{
	if (!Panel.Inventory || !Panel.Title)
	{
		return;
	}
	Panel.Title->SetText(Panel.Inventory->GetDisplayName());
	Panel.Summary->SetText(FText::Format(LOCTEXT("Summary", "Gewicht {0} / {1}   ·   Plätze {2} / {3}"),
		AWEPickupItem::FormatWeight(Panel.Inventory->GetTotalWeightKg()), AWEPickupItem::FormatWeight(Panel.Inventory->GetMaxWeightKg()),
		FText::AsNumber(Panel.Inventory->GetAllSlots().Num()), FText::AsNumber(Panel.Inventory->GetNumSlots())));
	for (UWEInventorySlotWidget* SlotWidget : Panel.Slots)
	{
		SlotWidget->Refresh();
	}
}

void UWEInventoryWidget::Setup(UWEInventoryComponent* PlayerInventory, UWEInventoryComponent* Container)
{
	Unbind();
	Columns_Box->ClearChildren();
	PlayerPanel = FWEInventoryPanel();
	ContainerPanel = FWEInventoryPanel();

	BuildPanel(PlayerPanel, PlayerInventory, true);
	if (PlayerInventory)
	{
		PlayerHandle = PlayerInventory->OnInventoryChanged.AddUObject(this, &UWEInventoryWidget::HandleInventoryChanged);
	}
	if (Container && Container != PlayerInventory)
	{
		BuildPanel(ContainerPanel, Container, false);
		ContainerHandle = Container->OnInventoryChanged.AddUObject(this, &UWEInventoryWidget::HandleInventoryChanged);
	}
}

void UWEInventoryWidget::Unbind()
{
	if (PlayerPanel.Inventory)
	{
		PlayerPanel.Inventory->OnInventoryChanged.Remove(PlayerHandle);
	}
	if (ContainerPanel.Inventory)
	{
		ContainerPanel.Inventory->OnInventoryChanged.Remove(ContainerHandle);
	}
	PlayerHandle.Reset();
	ContainerHandle.Reset();
}

void UWEInventoryWidget::NativeDestruct()
{
	Unbind();
	Super::NativeDestruct();
}

void UWEInventoryWidget::HandleInventoryChanged(UWEInventoryComponent* Inventory)
{
	if (Inventory == PlayerPanel.Inventory)
	{
		RefreshPanel(PlayerPanel);
	}
	if (Inventory == ContainerPanel.Inventory)
	{
		RefreshPanel(ContainerPanel);
	}
}

UWEInventoryComponent* UWEInventoryWidget::GetOtherInventory(const UWEInventoryComponent* Inventory) const
{
	if (!ContainerPanel.Inventory)
	{
		return nullptr;
	}
	return Inventory == PlayerPanel.Inventory ? ContainerPanel.Inventory.Get() : PlayerPanel.Inventory.Get();
}

void UWEInventoryWidget::HandleSortPlayer()
{
	if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
	{
		PC->RequestSort(PlayerPanel.Inventory);
	}
}

void UWEInventoryWidget::HandleTakeAll()
{
	if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
	{
		PC->RequestTakeAll(ContainerPanel.Inventory);
	}
}

void UWEInventoryWidget::HandleSortContainer()
{
	if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
	{
		PC->RequestSort(ContainerPanel.Inventory);
	}
}

void UWEInventoryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Close the container view once the player walks away from it.
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	if (ContainerPanel.Inventory && !ContainerPanel.Inventory->CanBeAccessedBy(Character))
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
		{
			PC->CloseInventory();
		}
	}
}

FReply UWEInventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Tab || Key == EKeys::Escape || Key == EKeys::I)
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
		{
			PC->CloseInventory();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

bool UWEInventoryWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	// Reached only if no slot accepted the drop: put the stack into the world.
	const UWEItemDragOperation* Operation = Cast<UWEItemDragOperation>(InOperation);
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer());

	// Dropped between slots inside a panel: do nothing rather than throwing the item out.
	const FVector2D ScreenPos = InDragDropEvent.GetScreenSpacePosition();
	for (const FWEInventoryPanel* Panel : { &PlayerPanel, &ContainerPanel })
	{
		if (Panel->Root && Panel->Root->GetCachedGeometry().IsUnderLocation(ScreenPos))
		{
			return true;
		}
	}

	if (Operation && PC)
	{
		PC->RequestDropFromInventory(Operation->SourceInventory, Operation->SourceSlot, Operation->Quantity);
		return true;
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
