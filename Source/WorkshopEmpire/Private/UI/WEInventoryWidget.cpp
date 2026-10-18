#include "UI/WEInventoryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/WEPlayerController.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "UI/WEInventorySlotWidget.h"

#define LOCTEXT_NAMESPACE "WEInventoryWidget"

using namespace WEUI;

namespace WEInvStyle
{
	FProgressBarStyle BarStyle()
	{
		FProgressBarStyle S;
		S.SetBackgroundImage(Brush(Col::PanelDark, Col::Line, 1.f, 1.f));
		S.SetFillImage(Brush(Col::Accent, FLinearColor::Transparent, 0.f, 1.f));
		S.SetMarqueeImage(Brush(Col::Accent, FLinearColor::Transparent, 0.f, 1.f));
		return S;
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
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = RootCanvas;

	// Full-screen backdrop (also the drop target for "put into the world").
	UBorder* Dim = Box(WidgetTree, Col::Backdrop, FMargin(0.f), FLinearColor::Transparent, 0.f);
	if (UCanvasPanelSlot* S = RootCanvas->AddChildToCanvas(Dim))
	{
		S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		S->SetOffsets(FMargin(0.f));
	}

	UVerticalBox* Outer = WidgetTree->ConstructWidget<UVerticalBox>();
	Content = Outer;
	if (UCanvasPanelSlot* S = RootCanvas->AddChildToCanvas(Outer))
	{
		S->SetAnchors(FAnchors(0.5f, 0.5f));
		S->SetAlignment(FVector2D(0.5f, 0.5f));
		S->SetAutoSize(true);
	}
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddV(Outer, Row);
	Columns_Box = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddH(Row, Columns_Box, FMargin(), false, VAlign_Top);

	// Details of the item under the mouse.
	UVerticalBox* Details = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(Details, SectionHeader(WidgetTree, LOCTEXT("Details", "Details"), TEXT("Info")));
	DetailEmpty = Text(WidgetTree, LOCTEXT("DetailsEmpty", "Mit der Maus auf einen Gegenstand zeigen."), 12, Col::TextMuted, EFont::Body, 250.f);
	AddV(Details, DetailEmpty, FMargin(0.f, 14.f, 0.f, 0.f));
	DetailImage = WidgetTree->ConstructWidget<UImage>();
	DetailImage->SetDesiredSizeOverride(FVector2D(128.f, 128.f));
	AddV(Details, DetailImage, FMargin(0.f, 10.f, 0.f, 0.f), HAlign_Center);
	DetailName = Text(WidgetTree, FText::GetEmpty(), 18, Col::Text, EFont::Head, 260.f);
	AddV(Details, DetailName, FMargin(0.f, 8.f, 0.f, 0.f));
	DetailChips = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddV(Details, DetailChips, FMargin(0.f, 6.f, 0.f, 0.f));
	DetailDescription = Text(WidgetTree, FText::GetEmpty(), 12, Col::TextDim, EFont::Body, 260.f);
	AddV(Details, DetailDescription, FMargin(0.f, 8.f, 0.f, 8.f));
	AddV(Details, Divider(WidgetTree));
	DetailRows = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(Details, DetailRows, FMargin(0.f, 8.f, 0.f, 0.f));
	DetailProtection = Text(WidgetTree, FText::GetEmpty(), 12, Col::Info, EFont::Strong, 260.f);
	AddV(Details, DetailProtection, FMargin(0.f, 10.f, 0.f, 0.f));
	UBorder* DetailsFrame = Box(WidgetTree, Col::Panel, FMargin(16.f, 14.f), Col::Line, 2.f);
	DetailsFrame->SetContent(Sized(WidgetTree, Details, 270.f, 0.f));
	DetailsCard = DetailsFrame;
	AddH(Row, DetailsFrame, FMargin(10.f, 0.f, 0.f, 0.f), false, VAlign_Fill);

	// Key hints.
	UHorizontalBox* Hints = WidgetTree->ConstructWidget<UHorizontalBox>();
	auto Hint = [this, Hints](const TCHAR* Key, const FText& What)
	{
		AddH(Hints, KeyCap(WidgetTree, FText::FromString(Key), 10), FMargin(0.f, 0.f, 5.f, 0.f));
		AddH(Hints, Text(WidgetTree, What, 11, Col::TextDim), FMargin(0.f, 0.f, 14.f, 0.f));
	};
	Hint(TEXT("Ziehen"), LOCTEXT("HDrag", "verschieben"));
	Hint(TEXT("Shift"), LOCTEXT("HShift", "halber Stapel"));
	Hint(TEXT("2× Klick"), LOCTEXT("HDouble", "umlagern"));
	Hint(TEXT("Rechtsklick"), LOCTEXT("HRight", "Menü: Favorit, Sperren, Ablegen"));
	Hint(TEXT("Q"), LOCTEXT("HQ", "ablegen"));
	Hint(TEXT("Tab"), LOCTEXT("HTab", "schließen"));
	AddV(Outer, Hints, FMargin(0.f, 12.f, 0.f, 0.f), HAlign_Center);

	// Context menu (right-click), positioned at the mouse.
	UVerticalBox* Menu = WidgetTree->ConstructWidget<UVerticalBox>();
	MenuTitle = Text(WidgetTree, FText::GetEmpty(), 12, Col::TextDim, EFont::Head, 220.f);
	AddV(Menu, MenuTitle, FMargin(8.f, 2.f, 8.f, 6.f));
	auto MenuButton = [this, Menu](const FText& Label, FName IconName, TObjectPtr<UTextBlock>* OutText = nullptr)
	{
		UTextBlock* LabelText = nullptr;
		UButton* B = Button(WidgetTree, Label, EButton::Ghost, IconName, 12, &LabelText);
		if (UHorizontalBox* Inner = Cast<UHorizontalBox>(B->GetContent()))
		{
			if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Inner->Slot))
			{
				ContentSlot->SetHorizontalAlignment(HAlign_Left);
			}
		}
		if (OutText)
		{
			*OutText = LabelText;
		}
		AddV(Menu, B, FMargin(0.f, 0.f, 0.f, 1.f));
		return B;
	};
	MenuButton(LOCTEXT("MenuDrop", "Ablegen"), TEXT("Download"))->OnClicked.AddDynamic(this, &UWEInventoryWidget::HandleMenuDrop);
	MenuDropOne = MenuButton(LOCTEXT("MenuDropOne", "1 Stück ablegen"), TEXT("Minus"));
	MenuDropOne->OnClicked.AddDynamic(this, &UWEInventoryWidget::HandleMenuDropOne);
	MenuButton(LOCTEXT("MenuFav", "Als Favorit markieren"), TEXT("Star"), &MenuFavoriteText)->OnClicked.AddDynamic(this, &UWEInventoryWidget::HandleMenuFavorite);
	MenuButton(LOCTEXT("MenuLock", "Sperren"), TEXT("Lock"), &MenuLockText)->OnClicked.AddDynamic(this, &UWEInventoryWidget::HandleMenuLock);
	MenuRelease = MenuButton(LOCTEXT("MenuRelease", "Zum Verkauf freigeben"), TEXT("Tag"), &MenuReleaseText);
	MenuRelease->OnClicked.AddDynamic(this, &UWEInventoryWidget::HandleMenuRelease);
	ContextMenu = Box(WidgetTree, Col::Hex(0x15171A, 0.98f), FMargin(4.f, 8.f), Col::LineStrong, 2.f);
	ContextMenu->SetContent(Sized(WidgetTree, Menu, 230.f));
	ContextMenu->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* S = RootCanvas->AddChildToCanvas(ContextMenu))
	{
		S->SetAutoSize(true);
		S->SetZOrder(10);
	}
	RefreshDetails();
}

void UWEInventoryWidget::BuildPanel(FWEInventoryPanel& Panel, UWEInventoryComponent* Inventory, bool bIsPlayer)
{
	Panel.Inventory = Inventory;
	Panel.Slots.Reset();

	UBorder* Frame = Box(WidgetTree, Col::Panel, FMargin(16.f, 14.f), Col::Line, 2.f);
	AddH(Columns_Box, Frame, FMargin(0.f, 0.f, 10.f, 0.f), false, VAlign_Fill);
	Panel.Root = WidgetTree->ConstructWidget<UVerticalBox>();
	Frame->SetContent(Panel.Root);

	UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddH(Head, Sized(WidgetTree, Box(WidgetTree, Col::Accent, FMargin(0.f), FLinearColor::Transparent, 0.f), 3.f, 20.f), FMargin(0.f, 0.f, 8.f, 0.f));
	AddH(Head, Icon(WidgetTree, bIsPlayer ? FName(TEXT("Person")) : FName(TEXT("Box")), 18.f, Col::TextDim), FMargin(0.f, 0.f, 8.f, 0.f));
	Panel.Title = Heading(WidgetTree, FText::GetEmpty(), 15, Col::Text);
	AddH(Head, Panel.Title, FMargin(), true);
	AddV(Panel.Root, Head);

	Panel.Summary = Text(WidgetTree, FText::GetEmpty(), 11, Col::TextDim, EFont::Strong);
	AddV(Panel.Root, Panel.Summary, FMargin(0.f, 8.f, 0.f, 4.f));
	Panel.WeightBar = WidgetTree->ConstructWidget<UProgressBar>();
	Panel.WeightBar->SetWidgetStyle(WEInvStyle::BarStyle());
	AddV(Panel.Root, Sized(WidgetTree, Panel.WeightBar, 0.f, 4.f), FMargin(0.f, 0.f, 0.f, 10.f));

	Panel.Grid = WidgetTree->ConstructWidget<UUniformGridPanel>();
	Panel.Grid->SetSlotPadding(FMargin(2.f));
	AddV(Panel.Root, Panel.Grid);
	const int32 NumSlots = Inventory ? Inventory->GetNumSlots() : 0;
	for (int32 i = 0; i < NumSlots; ++i)
	{
		UWEInventorySlotWidget* SlotWidget = CreateWidget<UWEInventorySlotWidget>(GetOwningPlayer(), UWEInventorySlotWidget::StaticClass());
		SlotWidget->Init(Inventory, i, this);
		Panel.Grid->AddChildToUniformGrid(SlotWidget, i / Columns, i % Columns);
		Panel.Slots.Add(SlotWidget);
	}

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
	if (!bIsPlayer)
	{
		UButton* TakeAll = Button(WidgetTree, LOCTEXT("TakeAll", "Alles nehmen"), EButton::Primary, TEXT("Download"), 12);
		TakeAll->OnClicked.AddDynamic(this, &UWEInventoryWidget::HandleTakeAll);
		AddH(Buttons, TakeAll, FMargin(0.f, 0.f, 6.f, 0.f));
	}
	UButton* Sort = Button(WidgetTree, LOCTEXT("Sort", "Sortieren"), EButton::Secondary, TEXT("Grip"), 12);
	Sort->OnClicked.AddDynamic(this, bIsPlayer ? &UWEInventoryWidget::HandleSortPlayer : &UWEInventoryWidget::HandleSortContainer);
	AddH(Buttons, Sort);
	AddV(Panel.Root, Buttons, FMargin(0.f, 12.f, 0.f, 0.f), HAlign_Right);

	RefreshPanel(Panel);
}

void UWEInventoryWidget::RefreshPanel(FWEInventoryPanel& Panel)
{
	if (!Panel.Inventory || !Panel.Title)
	{
		return;
	}
	Panel.Title->SetText(Upper(Panel.Inventory->GetDisplayName()));
	Panel.Summary->SetText(FText::Format(LOCTEXT("Summary", "{0} / {1}   ·   {2} / {3} Plätze"),
		AWEPickupItem::FormatWeight(Panel.Inventory->GetTotalWeightKg()), AWEPickupItem::FormatWeight(Panel.Inventory->GetMaxWeightKg()),
		FText::AsNumber(Panel.Inventory->GetAllSlots().Num()), FText::AsNumber(Panel.Inventory->GetNumSlots())));
	const float Fill = Panel.Inventory->GetMaxWeightKg() > 0.f ? Panel.Inventory->GetTotalWeightKg() / Panel.Inventory->GetMaxWeightKg() : 0.f;
	Panel.WeightBar->SetPercent(Fill);
	Panel.WeightBar->SetFillColorAndOpacity(Fill > 0.9f ? Col::Bad : FLinearColor::White);
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
	DetailInventory.Reset();
	DetailSlot = INDEX_NONE;
	CloseContextMenu();

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
	RefreshDetails();
	OpenAnim.Start();
	Play(this, ESound::Open, 0.5f);
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
	RefreshDetails();
}

UWEInventoryComponent* UWEInventoryWidget::GetOtherInventory(const UWEInventoryComponent* Inventory) const
{
	if (!ContainerPanel.Inventory)
	{
		return nullptr;
	}
	return Inventory == PlayerPanel.Inventory ? ContainerPanel.Inventory.Get() : PlayerPanel.Inventory.Get();
}

// --- Details ----------------------------------------------------------------------------------------------

void UWEInventoryWidget::ShowDetails(UWEInventorySlotWidget* SlotWidget)
{
	if (!SlotWidget || !SlotWidget->GetInventory() || !SlotWidget->GetInventory()->GetItemInSlot(SlotWidget->GetSlotIndex()))
	{
		return; // keep showing the last item when the mouse crosses empty slots
	}
	DetailInventory = SlotWidget->GetInventory();
	DetailSlot = SlotWidget->GetSlotIndex();
	RefreshDetails();
}

void UWEInventoryWidget::RefreshDetails()
{
	const FWEItemInstance* Item = DetailInventory.IsValid() ? DetailInventory->GetItemInSlot(DetailSlot) : nullptr;
	const ESlateVisibility On = ESlateVisibility::HitTestInvisible;
	const ESlateVisibility Off = ESlateVisibility::Collapsed;
	DetailEmpty->SetVisibility(Item ? Off : On);
	for (UWidget* W : { static_cast<UWidget*>(DetailImage), static_cast<UWidget*>(DetailName), static_cast<UWidget*>(DetailChips),
		static_cast<UWidget*>(DetailDescription), static_cast<UWidget*>(DetailRows), static_cast<UWidget*>(DetailProtection) })
	{
		W->SetVisibility(Item ? On : Off);
	}
	if (!Item || !Item->Definition)
	{
		return;
	}
	const UWEItemDefinition* Def = Item->Definition;
	if (UTexture2D* Tex = ItemIcon(Def))
	{
		DetailImage->SetBrushFromTexture(Tex);
		DetailImage->SetColorAndOpacity(FLinearColor::White);
	}
	else if (UTexture2D* Fallback = IconTexture(CategoryIcon(static_cast<uint8>(Def->Category))))
	{
		DetailImage->SetBrushFromTexture(Fallback);
		DetailImage->SetColorAndOpacity(Col::TextMuted);
	}
	DetailImage->SetDesiredSizeOverride(FVector2D(128.f, 128.f));
	DetailName->SetText(Item->GetDisplayName());
	DetailChips->ClearChildren();
	AddH(DetailChips, Chip(WidgetTree, WEItems::GetCategoryText(Def->Category), Col::TextDim), FMargin(0.f, 0.f, 4.f, 0.f));
	if (Def->Rarity != EWEItemRarity::Common)
	{
		AddH(DetailChips, Chip(WidgetTree, StaticEnum<EWEItemRarity>()->GetDisplayNameTextByValue(static_cast<int64>(Def->Rarity)), RarityColor(static_cast<uint8>(Def->Rarity))), FMargin(0.f, 0.f, 4.f, 0.f));
	}
	AddH(DetailChips, Chip(WidgetTree, WEItems::GetConditionText(Item->Condition), WEItems::GetConditionColor(Item->Condition)));
	DetailDescription->SetText(Def->Description);

	DetailRows->ClearChildren();
	auto RowLine = [this](const FText& Label, const FText& Value, const FLinearColor& Color = Col::Text)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		AddH(Row, Text(WidgetTree, Label, 12, Col::TextMuted), FMargin(0.f, 0.f, 10.f, 0.f));
		// Long values (tool line) wrap right-aligned instead of running over the label.
		UTextBlock* ValueText = Text(WidgetTree, Value, 12, Color, EFont::Strong, 175.f);
		ValueText->SetJustification(ETextJustify::Right);
		AddH(Row, ValueText, FMargin(), true);
		AddV(DetailRows, Row, FMargin(0.f, 0.f, 0.f, 3.f));
	};
	RowLine(LOCTEXT("RowQuality", "Qualität"), FText::Format(LOCTEXT("Pct", "{0} %"), FText::AsNumber(Item->Quality)));
	RowLine(LOCTEXT("RowQty", "Menge"), FText::AsNumber(Item->Quantity));
	RowLine(LOCTEXT("RowWeight", "Gewicht"), AWEPickupItem::FormatWeight(Item->GetTotalWeightKg()));
	RowLine(LOCTEXT("RowValue", "Schätzwert"), MoneyText(Item->GetEstimatedValue()), Col::Money);
	if (Def->ToolType != EWEToolType::None)
	{
		RowLine(LOCTEXT("RowTool", "Werkzeug"), FText::Format(LOCTEXT("ToolSpeed", "{0} · Tempo ×{1}"), WEItems::GetToolText(Def->ToolType),
			FText::AsNumber(Def->ToolSpeedMultiplier)));
	}
	if (Def->CanBeDisassembled() || Def->CanBeRepaired())
	{
		RowLine(LOCTEXT("RowWork", "Werkbank"), Def->CanBeRepaired() && Def->CanBeDisassembled() ? LOCTEXT("WorkBoth", "zerlegen oder reparieren")
			: Def->CanBeRepaired() ? LOCTEXT("WorkRepair", "reparieren") : LOCTEXT("WorkDis", "zerlegen"));
	}
	FText Protection;
	FLinearColor ProtColor = Col::Info;
	if (Item->IsCustomerItem())
	{
		Protection = LOCTEXT("ProtCustomer", "Kundengerät – über den Auftrag zurückgeben");
	}
	else if (Item->IsLocked())
	{
		Protection = LOCTEXT("ProtLocked", "Gesperrt – wird weder verkauft noch an Kunden geliefert");
		ProtColor = Col::Text;
	}
	else if (Item->IsFavorite())
	{
		Protection = LOCTEXT("ProtFav", "Favorit – vor Verkauf geschützt");
		ProtColor = Col::Accent;
	}
	else if (Item->IsPersonalEquipment())
	{
		Protection = Item->HasMark(FWEItemInstance::ReleasedForSale) ? LOCTEXT("ProtReleased", "Ausrüstung – zum Verkauf freigegeben")
			: LOCTEXT("ProtEquip", "Persönliche Ausrüstung – nicht in der Verkaufsliste");
	}
	else if (Item->NeedsSaleConfirmation())
	{
		Protection = LOCTEXT("ProtValuable", "Wertvoll – beim Verkauf wird nachgefragt");
		ProtColor = Col::Accent;
	}
	DetailProtection->SetText(Protection);
	DetailProtection->SetColorAndOpacity(FSlateColor(ProtColor));
	DetailProtection->SetVisibility(Protection.IsEmpty() ? Off : On);
}

// --- Context menu ---------------------------------------------------------------------------------------------

const FWEItemInstance* UWEInventoryWidget::GetMenuItem() const
{
	return MenuInventory.IsValid() ? MenuInventory->GetItemInSlot(MenuSlot) : nullptr;
}

void UWEInventoryWidget::OpenContextMenu(UWEInventorySlotWidget* SlotWidget, const FVector2D& ScreenPosition)
{
	const FWEItemInstance* Item = SlotWidget && SlotWidget->GetInventory() ? SlotWidget->GetInventory()->GetItemInSlot(SlotWidget->GetSlotIndex()) : nullptr;
	if (!Item)
	{
		return;
	}
	MenuInventory = SlotWidget->GetInventory();
	MenuSlot = SlotWidget->GetSlotIndex();
	MenuTitle->SetText(Upper(Item->GetDisplayName()));
	MenuDropOne->SetVisibility(Item->Quantity > 1 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	MenuFavoriteText->SetText(Item->IsFavorite() ? LOCTEXT("MenuUnfav", "Favorit entfernen") : LOCTEXT("MenuFav2", "Als Favorit markieren"));
	MenuLockText->SetText(Item->IsLocked() ? LOCTEXT("MenuUnlock", "Entsperren") : LOCTEXT("MenuLock2", "Sperren (nie verkaufen)"));
	MenuRelease->SetVisibility(Item->IsPersonalEquipment() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	MenuReleaseText->SetText(Item->HasMark(FWEItemInstance::ReleasedForSale) ? LOCTEXT("MenuKeep", "Wieder als Ausrüstung schützen") : LOCTEXT("MenuRelease2", "Zum Verkauf freigeben"));
	const FVector2D Local = RootCanvas->GetCachedGeometry().AbsoluteToLocal(ScreenPosition);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(ContextMenu->Slot))
	{
		S->SetPosition(Local + FVector2D(4.f, 4.f));
	}
	ContextMenu->SetVisibility(ESlateVisibility::Visible);
	Play(this, ESound::Click, 0.4f);
}

void UWEInventoryWidget::DevShowPlayerSlot(int32 SlotIndex, bool bOpenMenu)
{
	UWEInventorySlotWidget* SlotWidget = PlayerPanel.Slots.IsValidIndex(SlotIndex) ? PlayerPanel.Slots[SlotIndex].Get() : nullptr;
	if (!SlotWidget)
	{
		return;
	}
	ShowDetails(SlotWidget);
	if (bOpenMenu)
	{
		OpenContextMenu(SlotWidget, SlotWidget->GetCachedGeometry().GetAbsolutePositionAtCoordinates(FVector2D(0.8f, 0.8f)));
	}
}

void UWEInventoryWidget::CloseContextMenu()
{
	if (ContextMenu)
	{
		ContextMenu->SetVisibility(ESlateVisibility::Collapsed);
	}
	MenuInventory.Reset();
	MenuSlot = INDEX_NONE;
}

void UWEInventoryWidget::HandleMenuDrop()
{
	if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()); PC && GetMenuItem())
	{
		PC->RequestDropFromInventory(MenuInventory.Get(), MenuSlot, 0);
	}
	CloseContextMenu();
}

void UWEInventoryWidget::HandleMenuDropOne()
{
	if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()); PC && GetMenuItem())
	{
		PC->RequestDropFromInventory(MenuInventory.Get(), MenuSlot, 1);
	}
	CloseContextMenu();
}

void UWEInventoryWidget::HandleMenuFavorite()
{
	if (const FWEItemInstance* Item = GetMenuItem())
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
		{
			PC->RequestSetItemMarks(MenuInventory.Get(), MenuSlot, FWEItemInstance::Favorite, !Item->IsFavorite());
		}
	}
	CloseContextMenu();
}

void UWEInventoryWidget::HandleMenuLock()
{
	if (const FWEItemInstance* Item = GetMenuItem())
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
		{
			PC->RequestSetItemMarks(MenuInventory.Get(), MenuSlot, FWEItemInstance::Locked, !Item->IsLocked());
		}
	}
	CloseContextMenu();
}

void UWEInventoryWidget::HandleMenuRelease()
{
	if (const FWEItemInstance* Item = GetMenuItem())
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
		{
			PC->RequestSetItemMarks(MenuInventory.Get(), MenuSlot, FWEItemInstance::ReleasedForSale, !Item->HasMark(FWEItemInstance::ReleasedForSale));
		}
	}
	CloseContextMenu();
}

// --- Buttons / input --------------------------------------------------------------------------------------------

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
	OpenAnim.Tick(Content, InDeltaTime);

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
	if (Key == EKeys::Escape && ContextMenu->GetVisibility() == ESlateVisibility::Visible)
	{
		CloseContextMenu();
		return FReply::Handled();
	}
	if (Key == EKeys::Tab || Key == EKeys::Escape || Key == EKeys::I)
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
		{
			Play(this, ESound::Close, 0.5f);
			PC->CloseInventory();
		}
		return FReply::Handled();
	}
	if (Key == EKeys::Q && DetailInventory.IsValid() && DetailInventory->GetItemInSlot(DetailSlot))
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
		{
			PC->RequestDropFromInventory(DetailInventory.Get(), DetailSlot, InKeyEvent.IsShiftDown() ? 1 : 0);
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UWEInventoryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A click anywhere outside the menu closes it.
	if (ContextMenu && ContextMenu->GetVisibility() == ESlateVisibility::Visible && !ContextMenu->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		CloseContextMenu();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

bool UWEInventoryWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	// Reached only if no slot accepted the drop: put the stack into the world.
	const UWEItemDragOperation* Operation = Cast<UWEItemDragOperation>(InOperation);
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer());

	// Dropped between slots inside a panel or on the details: do nothing rather than throwing the item out.
	const FVector2D ScreenPos = InDragDropEvent.GetScreenSpacePosition();
	if (Content && Content->GetCachedGeometry().IsUnderLocation(ScreenPos))
	{
		return true;
	}
	if (Operation && PC)
	{
		PC->RequestDropFromInventory(Operation->SourceInventory, Operation->SourceSlot, Operation->Quantity);
		Play(this, ESound::Drop, 0.5f);
		return true;
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
