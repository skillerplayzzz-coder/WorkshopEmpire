#include "UI/WETradeWidget.h"
#include "World/WEDayNightComponent.h"
#include "Economy/WEClassifiedsComponent.h"
#include "Components/UniformGridPanel.h"
#include "Workshop/WEBenchmark.h"
#include "WELog.h"
#include "Blueprint/WidgetLayoutLibrary.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Economy/WEAuctionComponent.h"
#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WEMarketComponent.h"
#include "Economy/WETraderDefinition.h"
#include "Economy/WETraderTerminal.h"
#include "EngineUtils.h"
#include "Framework/WEPlayerController.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Logistics/WEForklift.h"
#include "Logistics/WEVehicle.h"
#include "Machines/WEPowerGridComponent.h"
#include "Player/WECharacter.h"
#include "Progression/WEMilestoneComponent.h"
#include "Progression/WEProgressionComponent.h"
#include "Progression/WETutorialComponent.h"
#include "Progression/WEUpgradeDefinition.h"
#include "Staff/WEEmployee.h"
#include "Staff/WEStaffComponent.h"
#include "Workshop/WEWorkshopStyleComponent.h"

#define LOCTEXT_NAMESPACE "WETrade"

using namespace WEUI;

namespace WEOs
{
	const FLinearColor Taskbar = Col::Hex(0x0D1218, 0.96f);
	const FLinearColor TitleBar = Col::Hex(0x34506E);
	const FLinearColor TitleBarActive = Col::Hex(0x3E6E9E);
	const FLinearColor WindowBody = Col::Hex(0x15191E, 0.985f);
	constexpr int32 NumApps = static_cast<int32>(UWETradeWidget::EApp::Count);
}

// =================================================================================================================
// Row
// =================================================================================================================

void UWETradeRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	Frame = Box(WidgetTree, Col::Raised, FMargin(10.f, 7.f), Col::Line, 2.f);
	WidgetTree->RootWidget = Frame;
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Frame->SetContent(Row);

	UOverlay* PicBox = WidgetTree->ConstructWidget<UOverlay>();
	Picture = WidgetTree->ConstructWidget<UImage>();
	Picture->SetDesiredSizeOverride(FVector2D(42.f, 42.f));
	PicBox->AddChildToOverlay(Picture);
	SwatchBox = Box(WidgetTree, FLinearColor::White, FMargin(0.f), Col::LineStrong, 3.f);
	SwatchBox->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* S = PicBox->AddChildToOverlay(Sized(WidgetTree, SwatchBox, 42.f, 42.f)))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Center);
	}
	PictureBox = Sized(WidgetTree, PicBox, 46.f, 46.f);
	AddH(Row, PictureBox, FMargin(0.f, 0.f, 10.f, 0.f));

	UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>();
	TitleText = Text(WidgetTree, FText::GetEmpty(), 13, Col::Text, EFont::Strong);
	DetailText = Text(WidgetTree, FText::GetEmpty(), 11, Col::TextDim);
	AddV(Texts, TitleText);
	AddV(Texts, DetailText, FMargin(0.f, 1.f, 0.f, 0.f));
	Chips = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddV(Texts, Chips, FMargin(0.f, 3.f, 0.f, 0.f));
	AddH(Row, Texts, FMargin(), true);

	PriceText = Text(WidgetTree, FText::GetEmpty(), 15, Col::Money, EFont::Head);
	AddH(Row, PriceText, FMargin(10.f, 0.f));
	Button1 = Button(WidgetTree, FText::FromString(TEXT("·")), EButton::Primary, NAME_None, 12, &Button1Text);
	AddH(Row, Button1, FMargin(4.f, 0.f, 0.f, 0.f));
	Button2 = Button(WidgetTree, FText::FromString(TEXT("·")), EButton::Secondary, NAME_None, 12, &Button2Text);
	AddH(Row, Button2, FMargin(4.f, 0.f, 0.f, 0.f));
	Button1->OnClicked.AddDynamic(this, &UWETradeRowWidget::HandleButton1);
	Button2->OnClicked.AddDynamic(this, &UWETradeRowWidget::HandleButton2);
}

void UWETradeRowWidget::Configure(UWETradeWidget* InOwner, EWETradeRowKind InKind, const FText& Title, const FText& Detail, const FText& Price,
	const FText& InButton1, bool bButton1Enabled, const FText& InButton2, bool bButton2Enabled)
{
	Owner = InOwner;
	Kind = InKind;
	const float Wrap = WrapOverride > 0.f ? WrapOverride : 430.f;
	TitleText->SetWrapTextAt(Wrap);
	DetailText->SetWrapTextAt(Wrap);
	TitleText->SetWrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping);
	DetailText->SetWrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping);
	TitleText->SetText(Title);
	DetailText->SetText(Detail);
	DetailText->SetVisibility(Detail.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	PriceText->SetText(Price);
	Button1Text->SetText(Upper(InButton1));
	Button1->SetVisibility(InButton1.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	Button1->SetIsEnabled(bButton1Enabled);
	Button2Text->SetText(Upper(InButton2));
	Button2->SetVisibility(InButton2.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	Button2->SetIsEnabled(bButton2Enabled);
	Chips->ClearChildren();
	Chips->SetVisibility(ESlateVisibility::Collapsed);
	if (Kind == EWETradeRowKind::Info)
	{
		Frame->SetBrush(Brush(Col::WithAlpha(Col::Raised, 0.55f), Col::WithAlpha(Col::Line, 0.6f), 1.f, 2.f));
		TitleText->SetFont(Font(EFont::Strong, 12));
		PictureBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UWETradeRowWidget::SetPicture(UTexture2D* ItemTexture, FName LineIcon, const FLinearColor& Tint)
{
	PictureBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	UTexture2D* Tex = ItemTexture ? ItemTexture : IconTexture(LineIcon);
	Picture->SetVisibility(Tex ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (Tex)
	{
		Picture->SetBrushFromTexture(Tex);
		Picture->SetDesiredSizeOverride(ItemTexture ? FVector2D(46.f, 46.f) : FVector2D(28.f, 28.f));
		Picture->SetColorAndOpacity(ItemTexture ? FLinearColor::White : Tint);
	}
}

void UWETradeRowWidget::SetSwatch(const FLinearColor& Color)
{
	PictureBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	Picture->SetVisibility(ESlateVisibility::Collapsed);
	SwatchBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	SwatchBox->SetBrush(Brush(Color, Col::LineStrong, 1.f, 3.f));
}

void UWETradeRowWidget::AddChip(const FText& Label, const FLinearColor& Color)
{
	AddH(Chips, WEUI::Chip(WidgetTree, Label, Color), FMargin(0.f, 0.f, 4.f, 0.f));
	Chips->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UWETradeRowWidget::SetButtonKinds(EButton First, EButton Second)
{
	SetButtonKind(Button1, First, Button1Text);
	SetButtonKind(Button2, Second, Button2Text);
}

void UWETradeRowWidget::SetHighlight(const FLinearColor& Outline)
{
	Frame->SetBrush(Brush(Col::Raised, Outline, 1.f, 2.f));
}

void UWETradeRowWidget::HandleButton1()
{
	if (Owner)
	{
		Owner->HandleRowAction(this, 1);
	}
}

void UWETradeRowWidget::HandleButton2()
{
	if (Owner)
	{
		Owner->HandleRowAction(this, 2);
	}
}

// =================================================================================================================
// Desktop
// =================================================================================================================

FText UWETradeWidget::AppTitle(EApp App)
{
	switch (App)
	{
	case EApp::Market:   return LOCTEXT("AppMarket", "Marktplatz");
	case EApp::Auctions: return LOCTEXT("AppAuctions", "Auktionen");
	case EApp::Staff:    return LOCTEXT("AppStaff", "Personal & Logistik");
	case EApp::Company:  return LOCTEXT("AppCompany", "Unternehmen");
	case EApp::Orders:   return LOCTEXT("AppOrders", "Aufträge");
	case EApp::Finance:  return LOCTEXT("AppFinance", "Finanzen");
	case EApp::Style:    return LOCTEXT("AppStyle", "Gestaltung");
	case EApp::Help:     return LOCTEXT("AppHelp", "Hilfe");
	case EApp::Classifieds: return LOCTEXT("AppClassifieds", "Kleinanzeigen");
	default:             return FText::GetEmpty();
	}
}

FName UWETradeWidget::AppIcon(EApp App)
{
	switch (App)
	{
	case EApp::Market:   return TEXT("Cart");
	case EApp::Auctions: return TEXT("Gavel");
	case EApp::Staff:    return TEXT("People");
	case EApp::Company:  return TEXT("Factory");
	case EApp::Orders:   return TEXT("Clipboard");
	case EApp::Finance:  return TEXT("Chart");
	case EApp::Style:    return TEXT("Roller");
	case EApp::Help:     return TEXT("Bulb");
	case EApp::Classifieds: return TEXT("Sell");
	default:             return TEXT("Window");
	}
}

FLinearColor UWETradeWidget::AppColor(EApp App)
{
	switch (App)
	{
	case EApp::Market:   return Col::Hex(0xE09A24);
	case EApp::Auctions: return Col::Hex(0x8E5BC7);
	case EApp::Staff:    return Col::Hex(0x3F9C63);
	case EApp::Company:  return Col::Hex(0x5B7590);
	case EApp::Orders:   return Col::Hex(0x3A86C8);
	case EApp::Finance:  return Col::Hex(0x2E9D9A);
	case EApp::Style:    return Col::Hex(0xC9573D);
	case EApp::Help:     return Col::Hex(0x7A8086);
	case EApp::Classifieds: return Col::Hex(0x2F8F5B);
	default:             return Col::OsBlue;
	}
}

void UWETradeWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	BuildDesktop();
}

void UWETradeWidget::BuildDesktop()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;
	DesktopRoot = Root;
	auto Fill = [Root](UWidget* W)
	{
		UOverlaySlot* S = Root->AddChildToOverlay(W);
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Fill);
		return S;
	};
	// Wallpaper and a faint logo.
	UImage* Wallpaper = WidgetTree->ConstructWidget<UImage>();
	if (UTexture2D* Tex = UITexture(TEXT("T_FerritWallpaper")))
	{
		Wallpaper->SetBrushFromTexture(Tex);
	}
	else
	{
		Wallpaper->SetColorAndOpacity(Col::OsBlueDark);
	}
	Fill(Wallpaper);
	UImage* Logo = WidgetTree->ConstructWidget<UImage>();
	if (UTexture2D* Tex = UITexture(TEXT("T_FerritLogo")))
	{
		Logo->SetBrushFromTexture(Tex);
	}
	Logo->SetDesiredSizeOverride(FVector2D(220.f, 220.f));
	Logo->SetColorAndOpacity(Col::WithAlpha(FLinearColor::White, 0.05f));
	if (UOverlaySlot* S = Root->AddChildToOverlay(Logo))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Center);
	}

	// Desktop: icons on the left, windows above them; taskbar at the bottom.
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Fill(Column);
	Desktop = WidgetTree->ConstructWidget<UCanvasPanel>();
	AddV(Column, Desktop, FMargin(), HAlign_Fill, true);

	// Phase 19: nine apps - the icons stand in two columns (five per column, like a real desktop).
	UUniformGridPanel* Icons = WidgetTree->ConstructWidget<UUniformGridPanel>();
	Icons->SetSlotPadding(FMargin(0.f, 0.f, 6.f, 6.f));
	int32 IconIndex = 0;
	const TArray<EApp> Order = { EApp::Market, EApp::Orders, EApp::Classifieds, EApp::Auctions, EApp::Staff, EApp::Finance, EApp::Company, EApp::Style, EApp::Help };
	for (const EApp App : Order)
	{
		UButton* Tile = WidgetTree->ConstructWidget<UButton>();
		Tile->SetStyle(ButtonStyle(EButton::Tile));
		UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>();
		UBorder* Badge = Box(WidgetTree, AppColor(App), FMargin(9.f), Col::WithAlpha(FLinearColor::White, 0.25f), 6.f);
		Badge->SetContent(Icon(WidgetTree, AppIcon(App), 30.f, FLinearColor::White));
		AddV(Inner, Badge, FMargin(), HAlign_Center);
		UTextBlock* Label = Text(WidgetTree, AppTitle(App), 11, FLinearColor::White, EFont::Strong, 108.f);
		Label->SetJustification(ETextJustify::Center);
		Label->SetShadowOffset(FVector2D(1.f, 1.f));
		Label->SetShadowColorAndOpacity(Col::WithAlpha(FLinearColor::Black, 0.8f));
		AddV(Inner, Label, FMargin(0.f, 5.f, 0.f, 0.f), HAlign_Center);
		Tile->SetContent(Inner);
		Icons->AddChildToUniformGrid(Sized(WidgetTree, Tile, 118.f, 0.f), IconIndex % 5, IconIndex / 5);
		++IconIndex;
		switch (App)
		{
		case EApp::Market:   Tile->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp0); break;
		case EApp::Auctions: Tile->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp1); break;
		case EApp::Staff:    Tile->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp2); break;
		case EApp::Company:  Tile->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp3); break;
		case EApp::Orders:   Tile->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp4); break;
		case EApp::Finance:  Tile->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp5); break;
		case EApp::Style:    Tile->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp6); break;
		case EApp::Help:     Tile->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp7); break; case EApp::Classifieds: Tile->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp8); break;
		default: break;
		}
	}
	if (UCanvasPanelSlot* S = Desktop->AddChildToCanvas(Icons))
	{
		S->SetAutoSize(true);
		S->SetPosition(FVector2D(18.f, 18.f));
		S->SetZOrder(0);
	}
	BuildTaskbar();
	AddV(Column, TaskbarWidget, FMargin(), HAlign_Fill);
	BuildStartMenu();
	BuildDialog();
	if (UOverlaySlot* S = Root->AddChildToOverlay(StartMenu))
	{
		S->SetHorizontalAlignment(HAlign_Left);
		S->SetVerticalAlignment(VAlign_Bottom);
		S->SetPadding(FMargin(6.f, 0.f, 0.f, 52.f));
	}
	Fill(DialogLayer);
}

void UWETradeWidget::BuildTaskbar()
{
	UBorder* Bar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Taskbar"));
	Bar->SetBrush(Brush(WEOs::Taskbar, Col::WithAlpha(FLinearColor::White, 0.08f), 1.f, 0.f));
	TaskbarWidget = Bar;
	Bar->SetPadding(FMargin(6.f, 4.f));
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Bar->SetContent(Row);
	UButton* Start = WidgetTree->ConstructWidget<UButton>();
	Start->SetStyle(ButtonStyle(EButton::Icon));
	UHorizontalBox* StartInner = WidgetTree->ConstructWidget<UHorizontalBox>();
	UImage* Logo = WidgetTree->ConstructWidget<UImage>();
	if (UTexture2D* Tex = UITexture(TEXT("T_FerritLogo")))
	{
		Logo->SetBrushFromTexture(Tex);
	}
	Logo->SetDesiredSizeOverride(FVector2D(24.f, 24.f));
	Logo->SetColorAndOpacity(Col::Accent);
	AddH(StartInner, Logo, FMargin(0.f, 0.f, 7.f, 0.f));
	AddH(StartInner, Text(WidgetTree, LOCTEXT("Start", "Ferrit"), 14, FLinearColor::White, EFont::Head));
	Start->SetContent(StartInner);
	Start->OnClicked.AddDynamic(this, &UWETradeWidget::HandleStart);
	AddH(Row, Start, FMargin(0.f, 0.f, 8.f, 0.f));
	AddH(Row, Divider(WidgetTree, Col::WithAlpha(FLinearColor::White, 0.12f), 1.f, true), FMargin(0.f, 4.f, 8.f, 4.f), false, VAlign_Fill);
	TaskButtons = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddH(Row, TaskButtons, FMargin(), true);

	auto Tray = [this, Row](FName IconName, const FLinearColor& Tint, TObjectPtr<UTextBlock>& OutText)
	{
		AddH(Row, Icon(WidgetTree, IconName, 16.f, Tint), FMargin(14.f, 0.f, 6.f, 0.f));
		OutText = Text(WidgetTree, FText::GetEmpty(), 13, FLinearColor::White, EFont::Strong);
		AddH(Row, OutText);
	};
	Tray(TEXT("Euro"), Col::Money, TrayBalance);
	Tray(TEXT("Clipboard"), Col::Info, TrayOrders);
	Tray(TEXT("Clock"), Col::TextDim, TrayClock);
	UButton* Logout = Button(WidgetTree, LOCTEXT("Logout", "Abmelden"), EButton::Ghost, TEXT("Exit"), 12);
	Logout->OnClicked.AddDynamic(this, &UWETradeWidget::HandleLogout);
	AddH(Row, Logout, FMargin(16.f, 0.f, 0.f, 0.f));
}

void UWETradeWidget::BuildStartMenu()
{
	UVerticalBox* Menu = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>();
	UImage* Logo = WidgetTree->ConstructWidget<UImage>();
	if (UTexture2D* Tex = UITexture(TEXT("T_FerritLogo")))
	{
		Logo->SetBrushFromTexture(Tex);
	}
	Logo->SetDesiredSizeOverride(FVector2D(34.f, 34.f));
	Logo->SetColorAndOpacity(Col::Accent);
	AddH(Head, Logo, FMargin(0.f, 0.f, 10.f, 0.f));
	UVerticalBox* HeadTexts = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(HeadTexts, Text(WidgetTree, LOCTEXT("OsName", "Ferrit OS"), 18, FLinearColor::White, EFont::Head));
	AddV(HeadTexts, Text(WidgetTree, LOCTEXT("OsSub", "Werkstattrechner · Version 3.4"), 11, Col::TextDim));
	AddH(Head, HeadTexts);
	AddV(Menu, Head, FMargin(10.f, 8.f, 10.f, 10.f));
	AddV(Menu, Divider(WidgetTree, Col::WithAlpha(FLinearColor::White, 0.1f)), FMargin(0.f, 0.f, 0.f, 6.f));
	for (int32 i = 0; i < WEOs::NumApps; ++i)
	{
		const EApp App = static_cast<EApp>(i);
		UButton* B = Button(WidgetTree, AppTitle(App), EButton::Ghost, AppIcon(App), 13);
		if (UHorizontalBox* Inner = Cast<UHorizontalBox>(B->GetContent()))
		{
			if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Inner->Slot))
			{
				ContentSlot->SetHorizontalAlignment(HAlign_Left);
			}
		}
		switch (i)
		{
		case 0: B->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp0); break;
		case 1: B->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp1); break;
		case 2: B->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp2); break;
		case 3: B->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp3); break;
		case 4: B->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp4); break;
		case 5: B->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp5); break;
		case 6: B->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp6); break;
		case 7: B->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp7); break; default: B->OnClicked.AddDynamic(this, &UWETradeWidget::HandleApp8); break;
		}
		AddV(Menu, B, FMargin(4.f, 0.f));
	}
	AddV(Menu, Divider(WidgetTree, Col::WithAlpha(FLinearColor::White, 0.1f)), FMargin(0.f, 6.f));
	UButton* Leave = Button(WidgetTree, LOCTEXT("ShutDown", "Abmelden und aufstehen"), EButton::Ghost, TEXT("Exit"), 13);
	Leave->OnClicked.AddDynamic(this, &UWETradeWidget::HandleLogout);
	AddV(Menu, Leave, FMargin(4.f, 0.f, 4.f, 4.f));
	UBorder* Frame = Box(WidgetTree, Col::Hex(0x111820, 0.98f), FMargin(6.f), Col::WithAlpha(FLinearColor::White, 0.12f), 3.f);
	Frame->SetContent(Sized(WidgetTree, Menu, 290.f));
	Frame->SetVisibility(ESlateVisibility::Collapsed);
	StartMenu = Frame;
}

void UWETradeWidget::BuildDialog()
{
	UBorder* Shade = Box(WidgetTree, Col::Hex(0x000000, 0.55f), FMargin(0.f), FLinearColor::Transparent, 0.f);
	Shade->SetHorizontalAlignment(HAlign_Center);
	Shade->SetVerticalAlignment(VAlign_Center);
	UVerticalBox* Box2 = WidgetTree->ConstructWidget<UVerticalBox>();
	UBorder* Title = Box(WidgetTree, WEOs::TitleBarActive, FMargin(14.f, 8.f), FLinearColor::Transparent, 0.f);
	UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddH(TitleRow, Icon(WidgetTree, TEXT("Warning"), 18.f, Col::Accent), FMargin(0.f, 0.f, 8.f, 0.f));
	DialogTitle = Text(WidgetTree, FText::GetEmpty(), 15, FLinearColor::White, EFont::Head);
	AddH(TitleRow, DialogTitle, FMargin(), true);
	Title->SetContent(TitleRow);
	AddV(Box2, Title);
	DialogMessage = Text(WidgetTree, FText::GetEmpty(), 13, Col::Text, EFont::Body, 470.f);
	AddV(Box2, DialogMessage, FMargin(18.f, 16.f, 18.f, 16.f));
	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
	UButton* No = Button(WidgetTree, LOCTEXT("Cancel", "Abbrechen"), EButton::Secondary, NAME_None, 13);
	No->OnClicked.AddDynamic(this, &UWETradeWidget::HandleConfirmNo);
	AddH(Buttons, No, FMargin(0.f, 0.f, 8.f, 0.f));
	DialogYes = Button(WidgetTree, LOCTEXT("Confirm", "Bestätigen"), EButton::Primary, NAME_None, 13, &DialogYesText);
	DialogYes->OnClicked.AddDynamic(this, &UWETradeWidget::HandleConfirmYes);
	AddH(Buttons, DialogYes);
	AddV(Box2, Buttons, FMargin(18.f, 0.f, 18.f, 16.f), HAlign_Right);
	UBorder* Frame = Box(WidgetTree, WEOs::WindowBody, FMargin(0.f), Col::WithAlpha(FLinearColor::White, 0.18f), 3.f);
	Frame->SetContent(Sized(WidgetTree, Box2, 520.f));
	Shade->SetContent(Frame);
	Shade->SetVisibility(ESlateVisibility::Collapsed);
	DialogLayer = Shade;
}

FVector2D UWETradeWidget::GetDesktopSize() const
{
	const FVector2D Size = Desktop ? Desktop->GetCachedGeometry().GetLocalSize() : FVector2D::ZeroVector;
	if (Size.X > 100.f)
	{
		return Size;
	}
	// Not laid out yet (first window): the viewport in UI units minus the taskbar.
	const float Scale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), 0.01f);
	const FVector2D View = UWidgetLayoutLibrary::GetViewportSize(this) / Scale;
	return View.X > 100.f ? FVector2D(View.X, View.Y - 60.f) : FVector2D(1920.f, 1020.f);
}

void UWETradeWidget::CreateWindow(EApp App)
{
	FAppWindow& W = Window(App);
	if (W.bBuilt)
	{
		return;
	}
	W.bBuilt = true;
	const int32 Index = static_cast<int32>(App);
	UVerticalBox* Outer = WidgetTree->ConstructWidget<UVerticalBox>();
	// Title bar: icon, title, minimise, close.
	// White brush tint: the bar colour comes from SetBrushColor (focus changes it), which multiplies the tint.
	W.TitleBar = Box(WidgetTree, FLinearColor::White, FMargin(10.f, 5.f, 4.f, 5.f), FLinearColor::Transparent, 0.f);
	W.TitleBar->SetBrushColor(WEOs::TitleBar);
	UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	UBorder* Badge = Box(WidgetTree, AppColor(App), FMargin(3.f), FLinearColor::Transparent, 3.f);
	Badge->SetContent(Icon(WidgetTree, AppIcon(App), 15.f, FLinearColor::White));
	AddH(TitleRow, Badge, FMargin(0.f, 0.f, 8.f, 0.f));
	AddH(TitleRow, Text(WidgetTree, AppTitle(App), 14, FLinearColor::White, EFont::Head), FMargin(), true);
	UButton* Min = Button(WidgetTree, FText::GetEmpty(), EButton::Icon, TEXT("Minus"), 11);
	UButton* Close = Button(WidgetTree, FText::GetEmpty(), EButton::Icon, TEXT("Close"), 11);
	switch (Index)
	{
	case 0: Min->OnClicked.AddDynamic(this, &UWETradeWidget::HandleMin0); Close->OnClicked.AddDynamic(this, &UWETradeWidget::HandleClose0); break;
	case 1: Min->OnClicked.AddDynamic(this, &UWETradeWidget::HandleMin1); Close->OnClicked.AddDynamic(this, &UWETradeWidget::HandleClose1); break;
	case 2: Min->OnClicked.AddDynamic(this, &UWETradeWidget::HandleMin2); Close->OnClicked.AddDynamic(this, &UWETradeWidget::HandleClose2); break;
	case 3: Min->OnClicked.AddDynamic(this, &UWETradeWidget::HandleMin3); Close->OnClicked.AddDynamic(this, &UWETradeWidget::HandleClose3); break;
	case 4: Min->OnClicked.AddDynamic(this, &UWETradeWidget::HandleMin4); Close->OnClicked.AddDynamic(this, &UWETradeWidget::HandleClose4); break;
	case 5: Min->OnClicked.AddDynamic(this, &UWETradeWidget::HandleMin5); Close->OnClicked.AddDynamic(this, &UWETradeWidget::HandleClose5); break;
	case 6: Min->OnClicked.AddDynamic(this, &UWETradeWidget::HandleMin6); Close->OnClicked.AddDynamic(this, &UWETradeWidget::HandleClose6); break;
	case 7: Min->OnClicked.AddDynamic(this, &UWETradeWidget::HandleMin7); Close->OnClicked.AddDynamic(this, &UWETradeWidget::HandleClose7); break; default: Min->OnClicked.AddDynamic(this, &UWETradeWidget::HandleMin8); Close->OnClicked.AddDynamic(this, &UWETradeWidget::HandleClose8); break;
	}
	AddH(TitleRow, Min, FMargin(0.f, 0.f, 2.f, 0.f));
	AddH(TitleRow, Close);
	W.TitleBar->SetContent(TitleRow);
	AddV(Outer, W.TitleBar);
	UBorder* BodyFrame = Box(WidgetTree, WEOs::WindowBody, FMargin(14.f, 12.f), FLinearColor::Transparent, 0.f);
	W.Body = WidgetTree->ConstructWidget<UVerticalBox>();
	BodyFrame->SetContent(W.Body);
	AddV(Outer, BodyFrame, FMargin(), HAlign_Fill, true);
	W.Frame = Box(WidgetTree, WEOs::WindowBody, FMargin(1.f), Col::WithAlpha(FLinearColor::White, 0.16f), 3.f);
	W.Frame->SetContent(Outer);

	// Size: most of the desktop right of the icons, a little cascaded.
	const FVector2D Desk = GetDesktopSize();
	W.Size = FVector2D(FMath::Clamp(Desk.X - 380.f, 640.f, 1560.f), FMath::Clamp(Desk.Y - 70.f, 420.f, 900.f));
	const float Cascade = (OpenedCount++ % 5) * 26.f;
	W.Position = FVector2D(FMath::Max(270.f, (Desk.X - W.Size.X) * 0.5f + 110.f) + Cascade, FMath::Max(12.f, (Desk.Y - W.Size.Y) * 0.5f) + Cascade * 0.6f);
	if (UCanvasPanelSlot* S = Desktop->AddChildToCanvas(W.Frame))
	{
		S->SetPosition(W.Position);
		S->SetSize(W.Size);
		S->SetZOrder(++TopZ);
	}
	WindowWidgets.Add(W.Frame);

	switch (App)
	{
	case EApp::Market:   BuildMarketApp(W.Body); break;
	case EApp::Auctions: BuildAuctionsApp(W.Body); break;
	case EApp::Staff:    BuildStaffApp(W.Body); break;
	case EApp::Company:  BuildCompanyApp(W.Body); break;
	case EApp::Orders:   BuildOrdersApp(W.Body); break;
	case EApp::Finance:  BuildFinanceApp(W.Body); break;
	case EApp::Style:    BuildStyleApp(W.Body); break;
	case EApp::Help:     BuildHelpApp(W.Body); break; case EApp::Classifieds: BuildClassifiedsApp(W.Body); break;
	default: break;
	}

	// Taskbar entry.
	W.TaskButton = Button(WidgetTree, AppTitle(App), EButton::Ghost, AppIcon(App), 12, &W.TaskLabel);
	switch (Index)
	{
	case 0: W.TaskButton->OnClicked.AddDynamic(this, &UWETradeWidget::HandleTask0); break;
	case 1: W.TaskButton->OnClicked.AddDynamic(this, &UWETradeWidget::HandleTask1); break;
	case 2: W.TaskButton->OnClicked.AddDynamic(this, &UWETradeWidget::HandleTask2); break;
	case 3: W.TaskButton->OnClicked.AddDynamic(this, &UWETradeWidget::HandleTask3); break;
	case 4: W.TaskButton->OnClicked.AddDynamic(this, &UWETradeWidget::HandleTask4); break;
	case 5: W.TaskButton->OnClicked.AddDynamic(this, &UWETradeWidget::HandleTask5); break;
	case 6: W.TaskButton->OnClicked.AddDynamic(this, &UWETradeWidget::HandleTask6); break;
	case 7: W.TaskButton->OnClicked.AddDynamic(this, &UWETradeWidget::HandleTask7); break; default: W.TaskButton->OnClicked.AddDynamic(this, &UWETradeWidget::HandleTask8); break;
	}
	AddH(TaskButtons, W.TaskButton, FMargin(0.f, 0.f, 4.f, 0.f));
}

void UWETradeWidget::OpenApp(EApp App)
{
	if (App >= EApp::Count)
	{
		return;
	}
	CreateWindow(App);
	FAppWindow& W = Window(App);
	const bool bWasOpen = W.bOpen && !W.bMinimized;
	W.bOpen = true;
	W.bMinimized = false;
	W.Frame->SetVisibility(ESlateVisibility::Visible);
	W.TaskButton->SetVisibility(ESlateVisibility::Visible);
	if (!bWasOpen)
	{
		W.Anim.Start();
		Play(this, ESound::Open, 0.45f);
	}
	FocusApp(App);
	if (StartMenu)
	{
		StartMenu->SetVisibility(ESlateVisibility::Collapsed);
	}
	LastSignature.Reset();
}

void UWETradeWidget::CloseApp(EApp App)
{
	FAppWindow& W = Window(App);
	if (!W.bBuilt || !W.bOpen)
	{
		return;
	}
	W.bOpen = false;
	W.Frame->SetVisibility(ESlateVisibility::Collapsed);
	W.TaskButton->SetVisibility(ESlateVisibility::Collapsed);
	Play(this, ESound::Close, 0.4f);
	// The topmost remaining window becomes active.
	for (int32 i = 0; i < WEOs::NumApps; ++i)
	{
		if (Windows[i].bOpen && !Windows[i].bMinimized)
		{
			FocusApp(static_cast<EApp>(i));
		}
	}
	UpdateTaskbar();
}

void UWETradeWidget::MinimizeApp(EApp App)
{
	FAppWindow& W = Window(App);
	if (!W.bOpen)
	{
		return;
	}
	W.bMinimized = true;
	W.Frame->SetVisibility(ESlateVisibility::Collapsed);
	Play(this, ESound::Toggle, 0.4f);
	UpdateTaskbar();
}

bool UWETradeWidget::IsAppOpen(EApp App) const
{
	const FAppWindow& W = Window(App);
	return W.bOpen && !W.bMinimized;
}

void UWETradeWidget::FocusApp(EApp App)
{
	ActiveApp = App;
	FAppWindow& W = Window(App);
	if (UCanvasPanelSlot* S = W.Frame ? Cast<UCanvasPanelSlot>(W.Frame->Slot) : nullptr)
	{
		S->SetZOrder(++TopZ);
	}
	for (int32 i = 0; i < WEOs::NumApps; ++i)
	{
		if (Windows[i].TitleBar)
		{
			Windows[i].TitleBar->SetBrushColor(i == static_cast<int32>(App) ? WEOs::TitleBarActive : WEOs::TitleBar);
		}
	}
	// Old page numbers (dev runs): the active app.
	switch (App)
	{
	case EApp::Market:   CurrentPage = 0; break;
	case EApp::Auctions: CurrentPage = 1; break;
	case EApp::Staff:    CurrentPage = 2; break;
	case EApp::Company:  CurrentPage = 3; break;
	default:             CurrentPage = static_cast<int32>(App); break;
	}
	UpdateTaskbar();
}

void UWETradeWidget::UpdateTaskbar()
{
	for (int32 i = 0; i < WEOs::NumApps; ++i)
	{
		FAppWindow& W = Windows[i];
		if (!W.TaskButton)
		{
			continue;
		}
		const bool bActive = W.bOpen && !W.bMinimized && static_cast<int32>(ActiveApp) == i;
		W.TaskButton->SetStyle(ButtonStyle(bActive ? EButton::TabActive : EButton::Ghost));
		if (W.TaskLabel)
		{
			W.TaskLabel->SetColorAndOpacity(FSlateColor(bActive ? FLinearColor::White : W.bMinimized ? Col::TextMuted : Col::TextDim));
		}
	}
}

void UWETradeWidget::TaskClicked(EApp App)
{
	FAppWindow& W = Window(App);
	if (W.bMinimized || ActiveApp != App)
	{
		OpenApp(App);
	}
	else
	{
		MinimizeApp(App);
	}
}

void UWETradeWidget::ShowPage(int32 Page)
{
	static const EApp Map[] = { EApp::Market, EApp::Auctions, EApp::Staff, EApp::Company, EApp::Orders, EApp::Finance, EApp::Style, EApp::Help, EApp::Classifieds };
	if (Page >= 0 && Page < UE_ARRAY_COUNT(Map))
	{
		OpenApp(Map[Page]);
	}
}

void UWETradeWidget::Setup(AWETraderTerminal* InTerminal)
{
	Terminal = InTerminal;
	LastSignature.Reset();
	bool bAnyOpen = false;
	for (const FAppWindow& W : Windows)
	{
		bAnyOpen |= W.bOpen && !W.bMinimized;
	}
	if (!bAnyOpen)
	{
		OpenApp(EApp::Market);
	}
	// The desktop fades in once the view has reached the monitor.
	SetRenderOpacity(0.f);
	FadeIn = -0.38f;
	Play(this, ESound::Confirm, 0.35f);
	Rebuild();
}

// --- Generic list helpers ---------------------------------------------------------------------------------------

UScrollBox* UWETradeWidget::MakeList(UVerticalBox* Parent, bool bFill)
{
	UBorder* Inset = Box(WidgetTree, Col::PanelDark, FMargin(6.f), Col::Line, 2.f);
	UScrollBox* List = WidgetTree->ConstructWidget<UScrollBox>();
	List->SetScrollbarThickness(FVector2D(6.f, 6.f));
	Inset->SetContent(List);
	AddV(Parent, Inset, FMargin(0.f, 0.f, 0.f, 0.f), HAlign_Fill, bFill);
	return List;
}

UWETradeRowWidget* UWETradeWidget::AddRow(UScrollBox* List, float Wrap)
{
	UWETradeRowWidget* Row = CreateWidget<UWETradeRowWidget>(GetOwningPlayer(), UWETradeRowWidget::StaticClass());
	Row->WrapOverride = Wrap;
	List->AddChild(Row);
	Row->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	return Row;
}

void UWETradeWidget::AddSection(UScrollBox* List, const FText& Title, FName IconName)
{
	UWidget* Header = SectionHeader(WidgetTree, Title, IconName, nullptr, 12);
	const bool bFirst = List->GetChildrenCount() == 0;
	if (UScrollBoxSlot* S = Cast<UScrollBoxSlot>(List->AddChild(Header)))
	{
		S->SetPadding(FMargin(2.f, bFirst ? 2.f : 10.f, 0.f, 6.f));
	}
}

void UWETradeWidget::AddInfo(UScrollBox* List, const FText& Title, const FText& Detail, const FText& Price, float Wrap)
{
	UWETradeRowWidget* Row = AddRow(List, Wrap);
	Row->Configure(this, EWETradeRowKind::Info, Title, Detail, Price, FText::GetEmpty(), false);
}

UWidgetSwitcher* UWETradeWidget::MakeTabs(UVerticalBox* Parent, const TArray<FText>& Labels, const TArray<FName>& Icons, TArray<TObjectPtr<UButton>>& OutButtons,
	TArray<TObjectPtr<UTextBlock>>& OutLabels, TArray<TObjectPtr<UImage>>& OutIcons)
{
	UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>();
	for (int32 i = 0; i < Labels.Num(); ++i)
	{
		UTextBlock* Label = nullptr;
		UImage* TabIcon = nullptr;
		UButton* B = Button(WidgetTree, Labels[i], EButton::Tab, Icons.IsValidIndex(i) ? Icons[i] : NAME_None, 13, &Label, &TabIcon);
		AddH(Tabs, B, FMargin(0.f, 0.f, 2.f, 0.f));
		OutButtons.Add(B);
		OutLabels.Add(Label);
		OutIcons.Add(TabIcon);
	}
	AddV(Parent, Tabs);
	AddV(Parent, Divider(WidgetTree, Col::LineStrong), FMargin(0.f, 0.f, 0.f, 10.f));
	UWidgetSwitcher* Switcher = WidgetTree->ConstructWidget<UWidgetSwitcher>();
	AddV(Parent, Switcher, FMargin(), HAlign_Fill, true);
	return Switcher;
}

void UWETradeWidget::SelectTab(UWidgetSwitcher* Switcher, TArray<TObjectPtr<UButton>>& Buttons, TArray<TObjectPtr<UTextBlock>>& Labels, TArray<TObjectPtr<UImage>>& Icons, int32 Index)
{
	if (Switcher)
	{
		Switcher->SetActiveWidgetIndex(Index);
	}
	for (int32 i = 0; i < Buttons.Num(); ++i)
	{
		SetButtonKind(Buttons[i], i == Index ? EButton::TabActive : EButton::Tab, Labels.IsValidIndex(i) ? Labels[i].Get() : nullptr, Icons.IsValidIndex(i) ? Icons[i].Get() : nullptr);
	}
	LastSignature.Reset();
}

// --- App: Marktplatz ---------------------------------------------------------------------------------------------

void UWETradeWidget::BuildMarketApp(UVerticalBox* Body)
{
	MarketPages = MakeTabs(Body, { LOCTEXT("TabSell", "Verkaufen"), LOCTEXT("TabBuy", "Kaufen"), LOCTEXT("TabShop", "Werkstatt-Shop") },
		{ TEXT("Tag"), TEXT("Cart"), TEXT("Wrench") }, MarketTabButtons, MarketTabLabels, MarketTabIcons);
	MarketTabButtons[0]->OnClicked.AddDynamic(this, &UWETradeWidget::HandleMarketTab0);
	MarketTabButtons[1]->OnClicked.AddDynamic(this, &UWETradeWidget::HandleMarketTab1);
	MarketTabButtons[2]->OnClicked.AddDynamic(this, &UWETradeWidget::HandleMarketTab2);

	UVerticalBox* SellPage = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(SellPage, Text(WidgetTree, LOCTEXT("SellHint", "Verkauft wird an den Händler, der am meisten zahlt. Werkzeuge, gesperrte Teile und Favoriten stehen unten und werden nie aus Versehen verkauft."),
		11, Col::TextDim, EFont::Body, 900.f), FMargin(0.f, 0.f, 0.f, 8.f));
	SellList = MakeList(SellPage);
	MarketPages->AddChild(SellPage);

	UVerticalBox* BuyPage = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBox* FilterRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddH(FilterRow, Text(WidgetTree, LOCTEXT("BuyHint", "Große Einkäufe kommen auf die Lieferpalette am Tor."), 11, Col::TextDim), FMargin(), true);
	UTextBlock* FilterLabel = nullptr;
	UButton* Filter = Button(WidgetTree, LOCTEXT("FilterAll", "Alle Händler"), EButton::Secondary, TEXT("Search"), 11, &FilterLabel);
	BuyFilterText = FilterLabel;
	Filter->OnClicked.AddDynamic(this, &UWETradeWidget::HandleBuyFilterNext);
	AddH(FilterRow, Filter);
	AddV(BuyPage, FilterRow, FMargin(0.f, 0.f, 0.f, 8.f));
	BuyList = MakeList(BuyPage);
	MarketPages->AddChild(BuyPage);

	UVerticalBox* ShopPage = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(ShopPage, Text(WidgetTree, LOCTEXT("ShopHint", "Verbesserungen und Gebäude für die ganze Firma. Gebäude bringen eine neue Unternehmensstufe."), 11, Col::TextDim, EFont::Body, 900.f),
		FMargin(0.f, 0.f, 0.f, 8.f));
	UpgradeList = MakeList(ShopPage);
	MarketPages->AddChild(ShopPage);
	SelectTab(MarketPages, MarketTabButtons, MarketTabLabels, MarketTabIcons, 0);
}

void UWETradeWidget::ShowMarketTab(int32 Tab)
{
	OpenApp(EApp::Market);
	MarketTab = FMath::Clamp(Tab, 0, 2);
	SelectTab(MarketPages, MarketTabButtons, MarketTabLabels, MarketTabIcons, MarketTab);
}

void UWETradeWidget::HandleMarketTab0() { ShowMarketTab(0); }
void UWETradeWidget::HandleMarketTab1() { ShowMarketTab(1); }
void UWETradeWidget::HandleMarketTab2() { ShowMarketTab(2); }

void UWETradeWidget::HandleBuyFilterNext()
{
	const int32 NumTraders = Terminal ? Terminal->GetTraders().Num() : 0;
	BuyFilter = (BuyFilter + 1) % (NumTraders + 1);
	const UWETraderDefinition* Trader = BuyFilter > 0 && Terminal && Terminal->GetTraders().IsValidIndex(BuyFilter - 1) ? Terminal->GetTraders()[BuyFilter - 1].Get() : nullptr;
	if (BuyFilterText)
	{
		BuyFilterText->SetText(Upper(Trader ? Trader->DisplayName : LOCTEXT("FilterAll2", "Alle Händler")));
	}
	LastSignature.Reset();
}

void UWETradeWidget::RebuildMarket(const FWEMoney& Balance)
{
	if (!SellList)
	{
		return;
	}
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	SellList->ClearChildren();
	// Carried item first.
	if (const AWEPickupItem* Held = Character ? Character->GetHeldItem() : nullptr)
	{
		AddSection(SellList, LOCTEXT("SecHeld", "In der Hand"), TEXT("Hand"));
		const FWEItemInstance& Item = Held->GetItem();
		const UWETraderDefinition* Trader = nullptr;
		const FWEMoney Price = Terminal->GetBestSellPrice(Item, &Trader);
		const bool bCustomer = Item.IsCustomerItem();
		const bool bProtected = Item.IsSaleProtected();
		UWETradeRowWidget* Row = AddRow(SellList);
		Row->SetPicture(ItemIcon(Item.Definition), CategoryIcon(static_cast<uint8>(Item.Definition->Category)), Col::TextDim);
		Row->Configure(this, EWETradeRowKind::SellHeld, Held->GetDisplayName(),
			bCustomer ? LOCTEXT("HeldCustomer", "Kundengerät – über den Reparaturauftrag abliefern")
			: bProtected ? AWETraderTerminal::ProtectedText(Item)
			: Trader ? FText::Format(LOCTEXT("SellTo", "an {0}"), Trader->DisplayName) : LOCTEXT("NoBuyerDetail", "Kein Händler interessiert"),
			Trader && !bCustomer && !bProtected ? MoneyText(Price) : FText::FromString(TEXT("—")),
			LOCTEXT("SellBtn", "Verkaufen"), Trader && !bCustomer && !bProtected);
		Row->AddChip(WEItems::GetConditionText(Item.Condition), WEItems::GetConditionColor(Item.Condition));
	}
	const UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr;
	TArray<FWEInventorySlotData> Goods, Kept;
	for (const FWEInventorySlotData& SlotData : Inventory ? Inventory->GetAllSlots() : TArray<FWEInventorySlotData>())
	{
		(SlotData.Item.IsSaleProtected() || SlotData.Item.IsCustomerItem() ? Kept : Goods).Add(SlotData);
	}
	auto AddSellRow = [this](const FWEInventorySlotData& SlotData, UWEInventoryComponent* Source)
	{
		FWEItemInstance One = SlotData.Item;
		One.Quantity = 1;
		const UWETraderDefinition* Trader = nullptr;
		const FWEMoney UnitPrice = Terminal->GetBestSellPrice(One, &Trader);
		const FWEMoney AllPrice = Terminal->GetBestSellPrice(SlotData.Item);
		const bool bStack = SlotData.Item.Quantity > 1;
		UWETradeRowWidget* Row = AddRow(SellList);
		Row->InventorySlot = SlotData.SlotIndex;
		Row->SourceInventory = Source;
		Row->SetPicture(ItemIcon(SlotData.Item.Definition), CategoryIcon(static_cast<uint8>(SlotData.Item.Definition->Category)), Col::TextDim);
		Row->Configure(this, EWETradeRowKind::Sell,
			bStack ? FText::Format(LOCTEXT("Stack", "{0}  ×{1}"), SlotData.Item.GetDisplayName(), FText::AsNumber(SlotData.Item.Quantity)) : SlotData.Item.GetDisplayName(),
			Trader ? FText::Format(LOCTEXT("SellTo2", "an {0}"), Trader->DisplayName) : LOCTEXT("NoBuyerDetail2", "Kein Händler interessiert"),
			Trader ? (bStack ? FText::Format(LOCTEXT("PerUnit", "{0} /Stk."), MoneyText(UnitPrice)) : MoneyText(UnitPrice)) : FText::FromString(TEXT("—")),
			bStack ? LOCTEXT("SellOne", "1 verkaufen") : LOCTEXT("SellBtn2", "Verkaufen"), Trader != nullptr,
			bStack ? FText::Format(LOCTEXT("SellAll", "Alle {0}"), MoneyText(AllPrice)) : FText::GetEmpty(), Trader != nullptr);
		Row->AddChip(WEItems::GetConditionText(SlotData.Item.Condition), WEItems::GetConditionColor(SlotData.Item.Condition));
		if (SlotData.Item.Definition->Rarity >= EWEItemRarity::Rare)
		{
			Row->AddChip(StaticEnum<EWEItemRarity>()->GetDisplayNameTextByValue(static_cast<int64>(SlotData.Item.Definition->Rarity)), RarityColor(static_cast<uint8>(SlotData.Item.Definition->Rarity)));
		}
		if (SlotData.Item.IsPersonalEquipment())
		{
			Row->AddChip(LOCTEXT("ChipReleased", "Ausrüstung freigegeben"), Col::Info);
		}
		if (SlotData.Item.NeedsSaleConfirmation())
		{
			Row->AddChip(LOCTEXT("ChipValuable", "Wertvoll – Rückfrage"), Col::Accent);
		}
	};
	AddSection(SellList, LOCTEXT("SecGoods", "Verkaufsware im Inventar"), TEXT("Tag"));
	for (const FWEInventorySlotData& SlotData : Goods)
	{
		AddSellRow(SlotData, nullptr);
	}
	// Phase 19: goods lying in the workshop's storages, sold straight from there.
	int32 StoredGoods = 0;
	for (UWEInventoryComponent* Storage : Terminal->GetWorkshopStorages())
	{
		TArray<FWEInventorySlotData> Stored;
		for (const FWEInventorySlotData& SlotData : Storage->GetAllSlots())
		{
			if (!SlotData.Item.IsSaleProtected() && !SlotData.Item.IsCustomerItem())
			{
				Stored.Add(SlotData);
			}
		}
		if (Stored.Num() == 0)
		{
			continue;
		}
		AddSection(SellList, FText::Format(LOCTEXT("SecStorage", "Im Lager: {0}"), Storage->GetDisplayName()), TEXT("Box"));
		for (const FWEInventorySlotData& SlotData : Stored)
		{
			AddSellRow(SlotData, Storage);
		}
		StoredGoods += Stored.Num();
	}
	if (Goods.Num() == 0 && StoredGoods == 0)
	{
		AddInfo(SellList, LOCTEXT("NothingToSell", "Nichts zu verkaufen"), LOCTEXT("NothingToSellDetail", "Zerlege Gegenstände an der Werkbank – was im Werkbanklager oder in einem Regal liegt, kann hier direkt verkauft werden."));
	}
	if (Kept.Num() > 0)
	{
		AddSection(SellList, LOCTEXT("SecKept", "Ausrüstung und geschützte Teile"), TEXT("Shield"));
		for (const FWEInventorySlotData& SlotData : Kept)
		{
			const FWEItemInstance& Item = SlotData.Item;
			UWETradeRowWidget* Row = AddRow(SellList);
			Row->InventorySlot = SlotData.SlotIndex;
			Row->SetPicture(ItemIcon(Item.Definition), CategoryIcon(static_cast<uint8>(Item.Definition->Category)), Col::TextDim);
			FText Button = FText::GetEmpty();
			if (Item.IsCustomerItem())
			{
				Row->MarkToClear = 0;
			}
			else if (Item.IsLocked())
			{
				Row->MarkToClear = FWEItemInstance::Locked;
				Button = LOCTEXT("BtnUnlock", "Entsperren");
			}
			else if (Item.IsFavorite())
			{
				Row->MarkToClear = FWEItemInstance::Favorite;
				Button = LOCTEXT("BtnUnfav", "Favorit lösen");
			}
			else
			{
				Row->MarkToClear = FWEItemInstance::ReleasedForSale; // set, not clear: release equipment
				Button = LOCTEXT("BtnRelease", "Freigeben");
			}
			Row->Configure(this, EWETradeRowKind::Protected, Item.Quantity > 1 ? FText::Format(LOCTEXT("Stack2", "{0}  ×{1}"), Item.GetDisplayName(), FText::AsNumber(Item.Quantity)) : Item.GetDisplayName(),
				Item.IsCustomerItem() ? LOCTEXT("KeptCustomer", "Kundengerät – über den Auftrag abliefern") : AWETraderTerminal::ProtectedText(Item),
				FText::GetEmpty(), Button, !Button.IsEmpty());
			Row->SetButtonKinds(EButton::Secondary, EButton::Secondary);
			Row->AddChip(Item.IsLocked() ? LOCTEXT("ChipLocked", "Gesperrt") : Item.IsFavorite() ? LOCTEXT("ChipFav", "Favorit") : Item.IsCustomerItem() ? LOCTEXT("ChipCustomer", "Kunde") : LOCTEXT("ChipEquip", "Ausrüstung"),
				Item.IsFavorite() ? Col::Accent : Col::Info);
		}
	}

	// Buy.
	BuyList->ClearChildren();
	const UWETraderDefinition* FilterTrader = BuyFilter > 0 && Terminal->GetTraders().IsValidIndex(BuyFilter - 1) ? Terminal->GetTraders()[BuyFilter - 1].Get() : nullptr;
	for (const FWEMarketOffer& Offer : Terminal->GetOffers())
	{
		if (FilterTrader && !Offer.TraderName.EqualTo(FilterTrader->DisplayName))
		{
			continue;
		}
		UWETradeRowWidget* Row = AddRow(BuyList);
		Row->OfferIndex = Offer.OfferIndex;
		Row->SetPicture(ItemIcon(Offer.Item.Definition), CategoryIcon(static_cast<uint8>(Offer.Item.Definition->Category)), Col::TextDim);
		Row->Configure(this, EWETradeRowKind::Buy, Offer.Item.GetDisplayName(),
			FText::Format(LOCTEXT("OfferDetail", "{0} · noch {1} Stück"), Offer.TraderName, FText::AsNumber(Offer.Remaining)),
			MoneyText(Offer.UnitPrice), LOCTEXT("BuyBtn", "Kaufen"), Offer.Remaining > 0 && Balance >= Offer.UnitPrice);
		Row->AddChip(WEItems::GetConditionText(Offer.Item.Condition), WEItems::GetConditionColor(Offer.Item.Condition));
		if (Offer.Item.Definition->Rarity >= EWEItemRarity::Rare)
		{
			Row->AddChip(LOCTEXT("RareTag", "Selten"), RarityColor(static_cast<uint8>(Offer.Item.Definition->Rarity)));
		}
		if (!Offer.Item.Definition->bFitsInInventory)
		{
			Row->AddChip(LOCTEXT("PalletTag", "Lieferpalette"), Col::TextDim);
		}
	}
	if (BuyList->GetChildrenCount() == 0)
	{
		AddInfo(BuyList, LOCTEXT("NoOffers", "Gerade nichts im Angebot"), LOCTEXT("NoOffersDetail", "Die Händler füllen ihr Lager regelmäßig auf."));
	}

	// Workshop shop.
	UpgradeList->ClearChildren();
	const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	if (Progression && Progression->GetCatalog())
	{
		for (const FWEUpgradeDefinition& Upgrade : Progression->GetCatalog()->Upgrades)
		{
			const bool bOwned = Progression->HasUpgrade(Upgrade.Id);
			const FText Blocked = Progression->GetBlockReason(Upgrade);
			UWETradeRowWidget* Row = AddRow(UpgradeList);
			Row->UpgradeId = Upgrade.Id;
			Row->SetPicture(nullptr, Upgrade.CompanyTier > 0 ? FName(TEXT("Building")) : FName(TEXT("Wrench")), bOwned ? Col::Good : Col::Accent);
			Row->Configure(this, EWETradeRowKind::Upgrade, Upgrade.DisplayName, bOwned ? Upgrade.Description : (Blocked.IsEmpty() ? Upgrade.Description : Blocked),
				bOwned ? FText::GetEmpty() : MoneyText(Upgrade.Cost), bOwned ? FText::GetEmpty() : LOCTEXT("BuyUpgrade", "Kaufen"), !bOwned && Blocked.IsEmpty() && Balance >= Upgrade.Cost);
			if (bOwned)
			{
				Row->AddChip(LOCTEXT("Owned", "Gekauft"), Col::Good);
			}
			if (Upgrade.CompanyTier > 0)
			{
				Row->AddChip(FText::Format(LOCTEXT("TierChip", "Stufe {0}"), FText::AsNumber(Upgrade.CompanyTier)), Col::Info);
			}
		}
	}
}

// --- App: Kleinanzeigen (Phase 19) ----------------------------------------------------------------------------------

void UWETradeWidget::BuildClassifiedsApp(UVerticalBox* Body)
{
	ClassifiedPages = MakeTabs(Body, { LOCTEXT("TabAds", "Angebote"), LOCTEXT("TabOwnAds", "Meine Anzeigen") },
		{ TEXT("Search"), TEXT("Sell") }, ClassifiedTabButtons, ClassifiedTabLabels, ClassifiedTabIcons);
	ClassifiedTabButtons[0]->OnClicked.AddDynamic(this, &UWETradeWidget::HandleClassifiedsTab0);
	ClassifiedTabButtons[1]->OnClicked.AddDynamic(this, &UWETradeWidget::HandleClassifiedsTab1);
	UVerticalBox* AdsPage = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(AdsPage, Text(WidgetTree, LOCTEXT("AdsHint", "Gebrauchtes von privat: zum Angebotspreis kaufen oder weniger bieten – der Verkäufer nimmt an, macht ein Gegenangebot oder lehnt ab. Gekauftes kommt mit der Post auf die Lieferpalette."),
		11, Col::TextDim, EFont::Body, 900.f), FMargin(0.f, 0.f, 0.f, 8.f));
	AdsList = MakeList(AdsPage);
	ClassifiedPages->AddChild(AdsPage);
	UVerticalBox* OwnPage = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(OwnPage, Text(WidgetTree, LOCTEXT("OwnAdsHint", "Eigene Anzeigen: Interessenten melden sich mit Angeboten – je fairer der Preis, desto schneller. Inseriert wird aus dem Inventar oder den Lagern der Werkstatt."),
		11, Col::TextDim, EFont::Body, 900.f), FMargin(0.f, 0.f, 0.f, 8.f));
	OwnAdsList = MakeList(OwnPage);
	ClassifiedPages->AddChild(OwnPage);
	SelectTab(ClassifiedPages, ClassifiedTabButtons, ClassifiedTabLabels, ClassifiedTabIcons, 0);
}

void UWETradeWidget::HandleClassifiedsTab0() { SelectTab(ClassifiedPages, ClassifiedTabButtons, ClassifiedTabLabels, ClassifiedTabIcons, 0); }
void UWETradeWidget::HandleClassifiedsTab1() { SelectTab(ClassifiedPages, ClassifiedTabButtons, ClassifiedTabLabels, ClassifiedTabIcons, 1); }

void UWETradeWidget::RebuildClassifieds(const FWEMoney& Balance)
{
	const UWEClassifiedsComponent* Classifieds = UWEClassifiedsComponent::Get(this);
	if (!AdsList || !OwnAdsList || !Classifieds)
	{
		return;
	}
	const float Now = Classifieds->GetServerTime();
	auto Round50 = [](double Cents) { return FMath::Max<int64>(50, FMath::RoundToInt64(Cents / 50.0) * 50); };

	// Offers from private sellers, and what is on its way.
	AdsList->ClearChildren();
	if (Classifieds->GetDeliveries().Num() > 0)
	{
		AddSection(AdsList, LOCTEXT("SecPost", "Unterwegs zur Lieferpalette"), TEXT("Truck"));
		for (const FWEClassifiedDelivery& D : Classifieds->GetDeliveries())
		{
			AddInfo(AdsList, D.Item.GetDisplayName(), FText::Format(LOCTEXT("PostEta", "kommt in etwa {0} s"), FText::AsNumber(FMath::Max(0, FMath::CeilToInt(D.ArrivesAt - Now)))));
		}
	}
	AddSection(AdsList, LOCTEXT("SecAds", "Aktuelle Anzeigen"), TEXT("Search"));
	for (const FWEClassified& Ad : Classifieds->GetAds())
	{
		UWETradeRowWidget* Row = AddRow(AdsList);
		Row->ContractId = Ad.Id;
		const bool bCounter = !Ad.Counter.IsZero();
		const bool bSilent = Ad.Offers >= UWEClassifiedsComponent::MaxOffersPerAd;
		Row->PriceA = Ad.Asking.GetCents();
		Row->PriceB = bCounter ? Ad.Counter.GetCents() : Round50(Ad.Asking.GetCents() * 0.75);
		Row->SetPicture(ItemIcon(Ad.Item.Definition), CategoryIcon(static_cast<uint8>(Ad.Item.Definition->Category)), Col::TextDim);
		Row->Configure(this, EWETradeRowKind::ClassifiedAd, Ad.Item.GetDisplayName(),
			FText::Format(LOCTEXT("AdDetail", "{0} – „{1}“"), FText::FromString(Ad.Seller), FText::FromString(Ad.Text)),
			MoneyText(Ad.Asking), FText::Format(LOCTEXT("AdBuy", "Kaufen {0}"), MoneyText(Ad.Asking)), Balance >= Ad.Asking,
			bSilent ? LOCTEXT("AdSilent", "Keine Antwort") : bCounter ? FText::Format(LOCTEXT("AdCounter", "Gegenangebot {0} annehmen"), MoneyText(Ad.Counter))
				: FText::Format(LOCTEXT("AdOffer", "{0} bieten"), MoneyText(FWEMoney::FromCents(Row->PriceB))),
			!bSilent && Balance.GetCents() >= Row->PriceB);
		Row->AddChip(WEItems::GetConditionText(Ad.Item.Condition), WEItems::GetConditionColor(Ad.Item.Condition));
		const FWEMoney Value = UWEMarketComponent::GetMarketValue(Ad.Item, UWEMarketComponent::Get(this));
		Row->AddChip(FText::Format(LOCTEXT("AdValue", "Marktwert {0}"), MoneyText(Value)), Ad.Asking <= Value ? Col::Good : Col::TextDim);
		if (bCounter)
		{
			Row->AddChip(LOCTEXT("AdCounterChip", "Gegenangebot"), Col::Accent);
		}
		Row->AddChip(FText::Format(LOCTEXT("AdMinutes", "noch {0} min"), FText::AsNumber(FMath::Max(0, FMath::CeilToInt((Ad.ExpiresAt - Now) / 60.f)))), Col::TextMuted);
	}
	if (Classifieds->GetAds().Num() == 0)
	{
		AddInfo(AdsList, LOCTEXT("NoAds", "Gerade keine Anzeigen"), LOCTEXT("NoAdsDetail", "Neue Anzeigen kommen laufend dazu."));
	}

	// Own ads, and what could be listed.
	OwnAdsList->ClearChildren();
	AddSection(OwnAdsList, LOCTEXT("SecOwn", "Meine Anzeigen"), TEXT("Sell"));
	for (const FWEOwnListing& L : Classifieds->GetListings())
	{
		UWETradeRowWidget* Row = AddRow(OwnAdsList);
		Row->ContractId = L.Id;
		Row->SetPicture(ItemIcon(L.Item.Definition), CategoryIcon(static_cast<uint8>(L.Item.Definition->Category)), Col::TextDim);
		Row->Configure(this, EWETradeRowKind::ClassifiedOwn,
			L.Item.Quantity > 1 ? FText::Format(LOCTEXT("OwnStack", "{0}  ×{1}"), L.Item.GetDisplayName(), FText::AsNumber(L.Item.Quantity)) : L.Item.GetDisplayName(),
			L.Offer.IsZero() ? LOCTEXT("OwnWaiting", "Noch kein Angebot – Interessenten melden sich hier.") : FText::Format(LOCTEXT("OwnOffer", "{0} bietet {1}"), FText::FromString(L.Buyer), MoneyText(L.Offer)),
			MoneyText(L.Price), L.Offer.IsZero() ? LOCTEXT("OwnNoOffer", "Warten …") : FText::Format(LOCTEXT("OwnAccept", "Für {0} verkaufen"), MoneyText(L.Offer)), !L.Offer.IsZero(),
			LOCTEXT("OwnWithdraw", "Zurückziehen"), true);
		if (!L.Offer.IsZero())
		{
			Row->AddChip(FText::Format(LOCTEXT("OwnShare", "{0} % vom Preis"), FText::AsNumber(FMath::RoundToInt(100.0 * L.Offer.GetCents() / FMath::Max<int64>(1, L.Price.GetCents())))), Col::Info);
		}
	}
	if (Classifieds->GetListings().Num() == 0)
	{
		AddInfo(OwnAdsList, LOCTEXT("NoOwn", "Noch keine eigenen Anzeigen"), LOCTEXT("NoOwnDetail", "Unten etwas auswählen und inserieren – zum Marktwert oder etwas darüber."));
	}
	if (Classifieds->GetListings().Num() >= UWEClassifiedsComponent::MaxListings)
	{
		return;
	}
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	TArray<UWEInventoryComponent*> Sources;
	Sources.Add(Character ? Character->GetPlayerInventory() : nullptr);
	if (Terminal)
	{
		Sources.Append(Terminal->GetWorkshopStorages());
	}
	AddSection(OwnAdsList, LOCTEXT("SecListable", "Inserieren"), TEXT("Plus"));
	for (UWEInventoryComponent* Source : Sources)
	{
		for (const FWEInventorySlotData& SlotData : Source ? Source->GetAllSlots() : TArray<FWEInventorySlotData>())
		{
			if (SlotData.Item.IsSaleProtected() || SlotData.Item.IsCustomerItem() || !SlotData.Item.Definition)
			{
				continue;
			}
			const int64 Value = FMath::Max<int64>(50, UWEMarketComponent::GetMarketValue(SlotData.Item, UWEMarketComponent::Get(this)).GetCents());
			UWETradeRowWidget* Row = AddRow(OwnAdsList);
			Row->InventorySlot = SlotData.SlotIndex;
			Row->SourceInventory = Source;
			Row->PriceA = Round50(Value * 1.15);
			Row->PriceB = Round50(Value * 0.9);
			Row->SetPicture(ItemIcon(SlotData.Item.Definition), CategoryIcon(static_cast<uint8>(SlotData.Item.Definition->Category)), Col::TextDim);
			Row->Configure(this, EWETradeRowKind::ClassifiedNew,
				SlotData.Item.Quantity > 1 ? FText::Format(LOCTEXT("NewStack", "{0}  ×{1}"), SlotData.Item.GetDisplayName(), FText::AsNumber(SlotData.Item.Quantity)) : SlotData.Item.GetDisplayName(),
				FText::Format(LOCTEXT("NewDetail", "{0} · Marktwert {1}"), Source->GetDisplayName(), MoneyText(FWEMoney::FromCents(Value))), FText::GetEmpty(),
				FText::Format(LOCTEXT("NewHigh", "Für {0} inserieren"), MoneyText(FWEMoney::FromCents(Row->PriceA))), true,
				FText::Format(LOCTEXT("NewQuick", "Schnell: {0}"), MoneyText(FWEMoney::FromCents(Row->PriceB))), true);
			Row->SetButtonKinds(EButton::Secondary, EButton::Secondary);
			Row->AddChip(WEItems::GetConditionText(SlotData.Item.Condition), WEItems::GetConditionColor(SlotData.Item.Condition));
		}
	}
}

// --- App: Aufträge -------------------------------------------------------------------------------------------------

void UWETradeWidget::BuildOrdersApp(UVerticalBox* Body)
{
	AddV(Body, Text(WidgetTree, LOCTEXT("OrdersHint", "Kundenaufträge annehmen und abliefern. Reparaturgeräte stehen nach dem Annehmen auf der Lieferpalette; große Mengen liefert der Firmenwagen (Tour \"Auslieferung\")."),
		11, Col::TextDim, EFont::Body, 900.f), FMargin(0.f, 0.f, 0.f, 8.f));
	ContractList = MakeList(Body);
}

void UWETradeWidget::RebuildOrders()
{
	if (!ContractList)
	{
		return;
	}
	ContractList->ClearChildren();
	const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	if (!Contracts)
	{
		return;
	}
	const float Now = Contracts->GetServerTime();
	// Phase 19: what customers think of the workshop.
	if (Contracts->GetRatingCount() > 0)
	{
		AddInfo(ContractList, FText::Format(LOCTEXT("RatingTitle", "Kundenbewertung: {0} von 5 Sternen"), FText::AsNumber(FMath::RoundToFloat(Contracts->GetAverageRating() * 10.f) / 10.f)),
			FText::Format(LOCTEXT("RatingDetail", "{0} Bewertungen · 4 Sterne bringen 10 %, 5 Sterne 25 % mehr Lohn"), FText::AsNumber(Contracts->GetRatingCount())));
	}
	for (const bool bAcceptedPass : { true, false })
	{
		bool bHeader = false;
		for (const FWEContract& Contract : Contracts->GetContracts())
		{
			const bool bAccepted = Contract.State == EWEContractState::Accepted;
			if (bAccepted != bAcceptedPass)
			{
				continue;
			}
			if (!bHeader)
			{
				AddSection(ContractList, bAccepted ? LOCTEXT("SecRunning", "Laufende Aufträge") : LOCTEXT("SecOffers", "Neue Anfragen"), bAccepted ? FName(TEXT("Clock")) : FName(TEXT("Mail")));
				bHeader = true;
			}
			const int32 Have = UWEContractComponent::CountDeliverable(Contract, Character);
			const int32 Minutes = FMath::Max(0, FMath::CeilToInt((Contract.ExpiresAt - Now) / 60.f));
			UWETradeRowWidget* Row = AddRow(ContractList);
			Row->ContractId = Contract.Id;
			Row->bContractAccepted = bAccepted;
			Row->SetPicture(ItemIcon(Contract.Item), Contract.Kind == EWEContractKind::Supply ? FName(TEXT("Box")) : FName(TEXT("Repair")), Col::TextDim);
			const FText ItemName = Contract.Item ? Contract.Item->DisplayName : FText::GetEmpty();
			FText Detail;
			if (Contract.Kind == EWEContractKind::BuildPc)
			{
				Detail = FText::Format(LOCTEXT("BuildPcDetail", "Funktionierenden PC zusammenbauen, getestet, Benchmark mindestens {0} Punkte – in der Hand abliefern"), WEBenchmark::PointsText(Contract.MinBenchmark));
			}
			else if (Contract.Kind == EWEContractKind::UpgradePc)
			{
				Detail = FText::Format(bAccepted ? LOCTEXT("UpgradePcAccepted", "Kunden-PC steht auf der Palette: Teile tauschen, testen, Benchmark mindestens {0} Punkte. Altteile bleiben bei dir.")
					: LOCTEXT("UpgradePcOpen", "PC aufrüsten auf mindestens {0} Punkte (Altteile bleiben bei dir)"), WEBenchmark::PointsText(Contract.MinBenchmark));
			}
			else if (Contract.Kind == EWEContractKind::Repair)
			{
				Detail = FText::Format(bAccepted ? LOCTEXT("RepairContractAccepted", "Reparieren: {0} – liegt auf der Palette, repariert in der Hand abliefern") : LOCTEXT("RepairContractOpen", "Reparieren: {0}"),
					FText::Format(LOCTEXT("RepairItem", "{0}, {1}"), ItemName, WEItems::GetConditionText(Contract.CustomerCondition)));
			}
			else
			{
				Detail = FText::Format(LOCTEXT("ContractDetail", "{0}× {1}, mindestens {2}{3}"), FText::AsNumber(Contract.Quantity), ItemName,
					WEItems::GetConditionText(Contract.MinCondition),
					Contract.Rounds > 1 ? FText::Format(LOCTEXT("RoundsTag", " · Liefervertrag {0}/{1}, je Lieferung"), FText::AsNumber(Contract.RoundsDone + 1), FText::AsNumber(Contract.Rounds)) : FText::GetEmpty());
			}
			Row->Configure(this, EWETradeRowKind::Contract, Contract.Title, Detail, MoneyText(Contract.Reward),
				bAccepted ? FText::Format(LOCTEXT("Deliver", "Abliefern {0}/{1}"), FText::AsNumber(FMath::Min(Have, Contract.Quantity)), FText::AsNumber(Contract.Quantity)) : LOCTEXT("Accept", "Annehmen"),
				bAccepted ? Have >= Contract.Quantity : true);
			Row->AddChip(FText::Format(LOCTEXT("MinutesChip", "noch {0} min"), FText::AsNumber(Minutes)), Minutes <= 3 ? Col::Bad : Col::TextDim);
			if (bAccepted)
			{
				Row->AddChip(FText::Format(LOCTEXT("AcceptedBy", "von {0}"), FText::FromString(Contract.AcceptedBy)), Col::Info);
			}
			if (Contract.bIndustrial)
			{
				Row->AddChip(LOCTEXT("IndustrialTag", "Industrie"), Col::Accent);
			}
			if (Contract.IsPcOrder())
			{
				Row->AddChip(FText::Format(LOCTEXT("BenchChip", "≥ {0} Punkte"), WEBenchmark::PointsText(Contract.MinBenchmark)), Col::Info);
				if (const AWEPickupItem* Held = Character ? Character->GetHeldItem() : nullptr; bAccepted && Held && Held->GetItem().Definition == Contract.Item)
				{
					const int32 Score = WEBenchmark::Score(Held->GetItem());
					Row->AddChip(Score > 0 ? FText::Format(LOCTEXT("HeldScore", "In der Hand: {0}"), FText::AsNumber(Score)) : LOCTEXT("HeldUntested", "In der Hand: nicht getestet"),
						Score >= Contract.MinBenchmark ? Col::Good : Col::Bad);
				}
			}
			if (!Contract.GetPenalty().IsZero() && bAccepted)
			{
				Row->AddChip(FText::Format(LOCTEXT("PenaltyChip", "Strafe {0}"), MoneyText(Contract.GetPenalty())), Col::Bad);
			}
		}
	}
	if (ContractList->GetChildrenCount() == 0)
	{
		AddInfo(ContractList, LOCTEXT("NoContracts", "Gerade keine Aufträge"), LOCTEXT("NoContractsDetail", "Neue Anfragen kommen laufend herein."));
	}
}

// --- App: Auktionen ------------------------------------------------------------------------------------------------

void UWETradeWidget::BuildAuctionsApp(UVerticalBox* Body)
{
	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
	UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(Left, SectionHeader(WidgetTree, LOCTEXT("Auctions", "Laufende Auktionen"), TEXT("Gavel"), nullptr, 13), FMargin(0.f, 0.f, 0.f, 8.f));
	AuctionList = MakeList(Left);
	AddH(Columns, Left, FMargin(0.f, 0.f, 10.f, 0.f), true, VAlign_Fill);
	UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(Right, SectionHeader(WidgetTree, LOCTEXT("Sources", "Weitere Quellen"), TEXT("Search"), nullptr, 13), FMargin(0.f, 0.f, 0.f, 8.f));
	SourcesList = MakeList(Right);
	AddH(Columns, Sized(WidgetTree, Right, 360.f), FMargin(), false, VAlign_Fill);
	AddV(Body, Columns, FMargin(), HAlign_Fill, true);
}

void UWETradeWidget::RebuildAuctions(const FWEMoney& Balance)
{
	if (!AuctionList)
	{
		return;
	}
	AuctionList->ClearChildren();
	SourcesList->ClearChildren();
	const UWEAuctionComponent* Auctions = UWEAuctionComponent::Get(this);
	if (Auctions)
	{
		const float Now = Auctions->GetServerTime();
		for (const FWEAuctionLot& Lot : Auctions->GetLots())
		{
			TArray<FString> Parts;
			for (const FWEItemInstance& Item : Lot.Items)
			{
				Parts.Add(FString::Printf(TEXT("%s%s (%s)"), Item.Quantity > 1 ? *FString::Printf(TEXT("%d× "), Item.Quantity) : TEXT(""),
					*Item.GetDisplayName().ToString(), *WEItems::GetConditionText(Item.Condition).ToString()));
			}
			const int32 Seconds = FMath::Max(0, FMath::CeilToInt(Lot.EndsAt - Now));
			const FWEMoney Next = UWEAuctionComponent::GetNextBid(Lot);
			UWETradeRowWidget* Row = AddRow(AuctionList, 380.f);
			Row->LotId = Lot.Id;
			Row->SetPicture(Lot.Items.Num() > 0 ? ItemIcon(Lot.Items[0].Definition) : nullptr, TEXT("Gavel"), Col::TextDim);
			Row->Configure(this, EWETradeRowKind::Auction, Lot.Title,
				FText::Format(LOCTEXT("LotDetail", "{0}\nSchätzwert {1}"), FText::FromString(FString::Join(Parts, TEXT(", "))), MoneyText(Lot.EstimatedValue)),
				Lot.NumBids == 0 ? FText::Format(LOCTEXT("StartPrice", "ab {0}"), MoneyText(Lot.CurrentBid)) : MoneyText(Lot.CurrentBid),
				Lot.bCompanyLeads ? LOCTEXT("Leading", "Führend") : FText::Format(LOCTEXT("BidBtn", "Bieten {0}"), MoneyText(Next)),
				!Lot.bCompanyLeads && Balance >= Next);
			Row->AddChip(FText::Format(LOCTEXT("LotTime", "{0}:{1}"), FText::AsNumber(Seconds / 60), FText::FromString(FString::Printf(TEXT("%02d"), Seconds % 60))), Seconds < 30 ? Col::Bad : Col::TextDim);
			Row->AddChip(Lot.NumBids == 0 ? LOCTEXT("NoBids", "kein Gebot") : Lot.bCompanyLeads ? FText::Format(LOCTEXT("YouLead", "ihr führt ({0})"), FText::FromString(Lot.LeaderName))
				: FText::Format(LOCTEXT("OtherLeads", "führt: {0}"), FText::FromString(Lot.LeaderName)), Lot.bCompanyLeads ? Col::Good : Col::Info);
			if (Lot.bCompanyLeads)
			{
				Row->SetHighlight(Col::Good);
			}
		}
	}
	if (AuctionList->GetChildrenCount() == 0)
	{
		AddInfo(AuctionList, LOCTEXT("NoAuctions", "Gerade keine Auktionen"), LOCTEXT("NoAuctionsDetail", "Neue Posten kommen laufend herein."));
	}
	AddInfo(SourcesList, LOCTEXT("SrcRules", "So funktionieren Auktionen"),
		LOCTEXT("SrcRulesDetail", "Ein Mitbieter hat ein geheimes Limit. Bezahlt wird erst beim Zuschlag; reicht das Geld dann nicht, verfällt er. Gebote kurz vor Schluss verlängern um 20 s. Gewonnenes steht auf der Lieferpalette."), FText::GetEmpty(), 300.f);
	AddInfo(SourcesList, LOCTEXT("SrcScrap", "Schrotthaufen hinter dem Tor"),
		LOCTEXT("SrcScrapDetail", "Was dort liegt, ist gratis. Alle paar Minuten kommt neuer Schrott – manchmal ist etwas Wertvolles dabei."), FText::GetEmpty(), 300.f);
	AddInfo(SourcesList, LOCTEXT("SrcPallets", "Retourenpaletten"),
		LOCTEXT("SrcPalletsDetail", "Im Marktplatz unter Kaufen. Unbekannter Inhalt, auf der Lieferpalette mit [Q] öffnen."), FText::GetEmpty(), 300.f);
	if (Auctions)
	{
		for (const FString& Result : Auctions->GetRecentResults())
		{
			AddInfo(SourcesList, LOCTEXT("SrcResult", "Auktionsergebnis"), FText::FromString(Result), FText::GetEmpty(), 300.f);
		}
	}
}

// --- App: Personal & Logistik --------------------------------------------------------------------------------------

void UWETradeWidget::BuildStaffApp(UVerticalBox* Body)
{
	StaffPages = MakeTabs(Body, { LOCTEXT("Applicants", "Bewerber"), LOCTEXT("Staff", "Mitarbeiter"), LOCTEXT("Logistics", "Fahrzeuge & Löhne") },
		{ TEXT("Mail"), TEXT("People"), TEXT("Truck") }, StaffTabButtons, StaffTabLabels, StaffTabIcons);
	StaffTabButtons[0]->OnClicked.AddDynamic(this, &UWETradeWidget::HandleStaffTab0);
	StaffTabButtons[1]->OnClicked.AddDynamic(this, &UWETradeWidget::HandleStaffTab1);
	StaffTabButtons[2]->OnClicked.AddDynamic(this, &UWETradeWidget::HandleStaffTab2);
	for (TObjectPtr<UScrollBox>* List : { &ApplicantList, &StaffList, &LogisticsList })
	{
		UVerticalBox* Page = WidgetTree->ConstructWidget<UVerticalBox>();
		*List = MakeList(Page);
		StaffPages->AddChild(Page);
	}
	SelectTab(StaffPages, StaffTabButtons, StaffTabLabels, StaffTabIcons, 0);
}

void UWETradeWidget::HandleStaffTab0() { SelectTab(StaffPages, StaffTabButtons, StaffTabLabels, StaffTabIcons, 0); }
void UWETradeWidget::HandleStaffTab1() { SelectTab(StaffPages, StaffTabButtons, StaffTabLabels, StaffTabIcons, 1); }
void UWETradeWidget::HandleStaffTab2() { SelectTab(StaffPages, StaffTabButtons, StaffTabLabels, StaffTabIcons, 2); }

void UWETradeWidget::RebuildStaff(const FWEMoney& Balance)
{
	if (!ApplicantList)
	{
		return;
	}
	ApplicantList->ClearChildren();
	StaffList->ClearChildren();
	LogisticsList->ClearChildren();
	const UWEStaffComponent* Staff = UWEStaffComponent::Get(this);
	if (!Staff)
	{
		return;
	}
	const TArray<AWEEmployee*> Employees = Staff->GetEmployees();
	const int32 MaxEmployees = Staff->GetMaxEmployees();
	const TArray<FWEEmployeeProfile>& Applicants = Staff->GetApplicants();
	AddSection(ApplicantList, FText::Format(LOCTEXT("ApplicantsHead", "Bewerbungen · {0} von {1} Plätzen belegt"), FText::AsNumber(Employees.Num()), FText::AsNumber(MaxEmployees)), TEXT("Mail"));
	for (int32 i = 0; i < Applicants.Num(); ++i)
	{
		const FWEEmployeeProfile& A = Applicants[i];
		UWETradeRowWidget* Row = AddRow(ApplicantList);
		Row->OfferIndex = i;
		Row->SetPicture(nullptr, TEXT("Person"), Col::Info);
		Row->Configure(this, EWETradeRowKind::Applicant, FText::Format(LOCTEXT("ApplicantTitle", "{0} – {1}"), FText::FromString(A.Name), AWEEmployee::GetRoleText(A.Role)),
			Staff->GetRoleDescription(A.Role), FText::Format(LOCTEXT("PerMinute", "{0}/min"), MoneyText(FWEMoney::FromCents(A.WageCentsPerMinute))),
			LOCTEXT("Hire", "Einstellen"), Employees.Num() < MaxEmployees);
		Row->AddChip(FText::Format(LOCTEXT("SkillChip", "Erfahrung {0}/3"), FText::AsNumber(A.Skill)), A.Skill >= 3 ? Col::Good : Col::TextDim);
	}
	for (AWEEmployee* Employee : Employees)
	{
		UWETradeRowWidget* Row = AddRow(StaffList);
		Row->Employee = Employee;
		Row->bEmployeePaused = Employee->IsPaused();
		Row->SetPicture(nullptr, TEXT("Person"), Employee->IsPaused() ? Col::TextMuted : Col::Good);
		Row->Configure(this, EWETradeRowKind::Employee, FText::Format(LOCTEXT("EmployeeTitle", "{0} – {1}"), FText::FromString(Employee->GetProfile().Name), Employee->GetRoleText()),
			FText::Format(LOCTEXT("EmployeeDetail", "{0}\n{1}"), Employee->GetTaskText(), Employee->GetStateText()), Employee->GetWageText(),
			Employee->IsPaused() ? LOCTEXT("ResumeBtn", "Arbeiten") : LOCTEXT("PauseBtn", "Pause"), Employee->GetState() != EWEEmployeeState::Away,
			LOCTEXT("FireBtn", "Entlassen"), Employee->GetState() != EWEEmployeeState::Away);
		Row->SetButtonKinds(EButton::Secondary, EButton::Danger);
	}
	if (Employees.Num() == 0)
	{
		AddInfo(StaffList, LOCTEXT("NoStaff", "Noch keine Mitarbeiter"),
			LOCTEXT("NoStaffDetail", "Neue Mitarbeiter warten am Tor. Aufgaben verteilt ihr in der Werkstatt: [E] auf den Mitarbeiter, dann Quelle und Ziel anschauen und [E]."));
	}
	const int32 PayrollSeconds = FMath::CeilToInt(Staff->GetSecondsToPayroll());
	AddInfo(LogisticsList, LOCTEXT("Wages", "Löhne"), FText::Format(LOCTEXT("WagesDetail", "{0} pro Minute · nächste Abrechnung in {1}:{2}{3}"),
		MoneyText(Staff->GetWagesPerMinute()), FText::AsNumber(PayrollSeconds / 60), FText::FromString(FString::Printf(TEXT("%02d"), PayrollSeconds % 60)),
		Staff->GetOwedWages().IsZero() ? FText::GetEmpty() : FText::Format(LOCTEXT("Owed", "\nAusstehend: {0} – ohne Lohn wird nicht gearbeitet"), MoneyText(Staff->GetOwedWages()))));
	for (TActorIterator<AWEVehicle> It(GetWorld()); It; ++It)
	{
		if (!It->IsUnlocked())
		{
			AddInfo(LogisticsList, LOCTEXT("VanLocked", "Firmenwagen"), LOCTEXT("VanLockedDetail", "Noch keiner. Im Werkstatt-Shop: Alter Transporter."));
			continue;
		}
		const FWETourDefinition* Tour = It->FindTour(It->IsAway() ? It->GetActiveTour() : It->GetSelectedTour());
		const int32 Seconds = FMath::CeilToInt(It->GetSecondsToReturn());
		const AWEEmployee* Driver = It->FindDriver();
		AddInfo(LogisticsList, It->GetStats().DisplayName, It->IsAway()
			? FText::Format(LOCTEXT("VanAway", "Unterwegs: {0} · zurück in {1}:{2}"), Tour ? Tour->DisplayName : FText::GetEmpty(),
				FText::AsNumber(Seconds / 60), FText::FromString(FString::Printf(TEXT("%02d"), Seconds % 60)))
			: FText::Format(LOCTEXT("VanParked", "Steht vor dem Tor · Fahrer: {0} · gewählte Tour: {1}{2}"),
				Driver ? FText::FromString(Driver->GetProfile().Name) : LOCTEXT("NoDriver", "keiner"), Tour ? Tour->DisplayName : FText::GetEmpty(),
				It->GetLastResult().IsEmpty() ? FText::GetEmpty() : FText::Format(LOCTEXT("VanLast", "\nLetzte Tour: {0}"), FText::FromString(It->GetLastResult()))));
	}
	bool bForklift = false;
	for (TActorIterator<AWEForklift> It(GetWorld()); It; ++It)
	{
		bForklift |= It->IsUnlocked();
	}
	AddInfo(LogisticsList, LOCTEXT("Forklift", "Gabelstapler"), bForklift ? LOCTEXT("ForkliftOwned", "In der Werkstatt: [E] aufsteigen, hebt bis 1.500 kg, [X] absteigen.")
		: LOCTEXT("ForkliftLocked", "Noch keiner. Im Werkstatt-Shop: Gabelstapler."));
}

// --- App: Finanzen --------------------------------------------------------------------------------------------------

void UWETradeWidget::BuildFinanceApp(UVerticalBox* Body)
{
	UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddH(Head, Icon(WidgetTree, TEXT("Euro"), 36.f, Col::Money), FMargin(0.f, 0.f, 12.f, 0.f));
	UVerticalBox* HeadTexts = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(HeadTexts, Heading(WidgetTree, LOCTEXT("Account", "Firmenkonto"), 11, Col::TextDim));
	FinanceBalance = Text(WidgetTree, FText::GetEmpty(), 30, Col::Money, EFont::Head);
	AddV(HeadTexts, FinanceBalance);
	AddH(Head, HeadTexts);
	AddV(Body, Head, FMargin(0.f, 0.f, 0.f, 12.f));
	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
	UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(Left, SectionHeader(WidgetTree, LOCTEXT("Books", "Einnahmen und Ausgaben"), TEXT("Chart"), nullptr, 13), FMargin(0.f, 0.f, 0.f, 8.f));
	BooksList = MakeList(Left);
	AddH(Columns, Left, FMargin(0.f, 0.f, 10.f, 0.f), true, VAlign_Fill);
	UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(Right, SectionHeader(WidgetTree, LOCTEXT("History", "Letzte Buchungen"), TEXT("Clock"), nullptr, 13), FMargin(0.f, 0.f, 0.f, 8.f));
	HistoryList = MakeList(Right);
	AddH(Columns, Right, FMargin(), true, VAlign_Fill);
	AddV(Body, Columns, FMargin(), HAlign_Fill, true);
}

void UWETradeWidget::RebuildFinance()
{
	if (!BooksList)
	{
		return;
	}
	BooksList->ClearChildren();
	HistoryList->ClearChildren();
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!Company)
	{
		return;
	}
	FinanceBalance->SetText(MoneyText(Company->GetBalance()));
	int64 HourSum = 0;
	int64 TotalSum = 0;
	for (int32 i = 0; i < static_cast<int32>(EWEBookingCategory::Count); ++i)
	{
		const EWEBookingCategory Category = static_cast<EWEBookingCategory>(i);
		const int64 Hour = Company->GetCategoryLastHourCents(Category);
		const int64 Total = Company->GetCategoryTotalCents(Category);
		HourSum += Hour;
		TotalSum += Total;
		if (Hour == 0 && Total == 0)
		{
			continue;
		}
		UWETradeRowWidget* Row = AddRow(BooksList, 260.f);
		Row->SetPicture(nullptr, Total >= 0 ? FName(TEXT("Plus")) : FName(TEXT("Minus")), Total >= 0 ? Col::Good : Col::Bad);
		Row->Configure(this, EWETradeRowKind::Info, UWECompanyComponent::GetCategoryName(Category),
			FText::Format(LOCTEXT("BookDetail", "letzte Stunde {0}"), MoneyText(FWEMoney::FromCents(Hour))), MoneyText(FWEMoney::FromCents(Total)), FText::GetEmpty(), false);
	}
	UWETradeRowWidget* Result = AddRow(BooksList, 260.f);
	Result->Configure(this, EWETradeRowKind::Info, LOCTEXT("BookResult", "Ergebnis"),
		FText::Format(LOCTEXT("BookResultDetail", "letzte Stunde {0}"), MoneyText(FWEMoney::FromCents(HourSum))), MoneyText(FWEMoney::FromCents(TotalSum)), FText::GetEmpty(), false);
	Result->SetHighlight(TotalSum >= 0 ? Col::Good : Col::Bad);
	int32 Shown = 0;
	for (const FWETransactionRecord& Record : Company->GetRecentTransactions())
	{
		if (Shown++ >= 30)
		{
			break;
		}
		UWETradeRowWidget* Row = AddRow(HistoryList, 300.f);
		Row->SetPicture(nullptr, Record.Amount.IsNegative() ? FName(TEXT("Minus")) : FName(TEXT("Plus")), Record.Amount.IsNegative() ? Col::Bad : Col::Good);
		Row->Configure(this, EWETradeRowKind::Info, FText::FromString(Record.Description), FText::FromString(Record.PlayerName),
			FText::FromString(FString::Printf(TEXT("%s%s"), Record.Amount.IsNegative() ? TEXT("") : TEXT("+"), *Record.Amount.ToDisplayString())), FText::GetEmpty(), false);
	}
	if (Shown == 0)
	{
		AddInfo(HistoryList, LOCTEXT("NoBookings", "Noch keine Buchungen"), FText::GetEmpty());
	}
}

// --- App: Unternehmen -----------------------------------------------------------------------------------------------

void UWETradeWidget::BuildCompanyApp(UVerticalBox* Body)
{
	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
	UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(Left, SectionHeader(WidgetTree, LOCTEXT("CompanyTier", "Unternehmen"), TEXT("Factory"), nullptr, 13), FMargin(0.f, 0.f, 0.f, 8.f));
	TierList = MakeList(Left);
	AddV(Left, SectionHeader(WidgetTree, LOCTEXT("Buildings", "Gebäude"), TEXT("Building"), nullptr, 13), FMargin(0.f, 10.f, 0.f, 8.f));
	BuildingList = MakeList(Left);
	AddH(Columns, Left, FMargin(0.f, 0.f, 10.f, 0.f), true, VAlign_Fill);
	UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(Right, SectionHeader(WidgetTree, LOCTEXT("Milestones", "Meilensteine"), TEXT("Medal"), nullptr, 13), FMargin(0.f, 0.f, 0.f, 8.f));
	MilestoneList = MakeList(Right);
	AddV(Right, SectionHeader(WidgetTree, LOCTEXT("OrdersOverview", "Laufende Aufträge"), TEXT("Clipboard"), nullptr, 13), FMargin(0.f, 10.f, 0.f, 8.f));
	ContractsOverview = MakeList(Right);
	AddH(Columns, Right, FMargin(), true, VAlign_Fill);
	AddV(Body, Columns, FMargin(), HAlign_Fill, true);
}

void UWETradeWidget::RebuildCompany()
{
	if (!TierList)
	{
		return;
	}
	TierList->ClearChildren();
	BuildingList->ClearChildren();
	MilestoneList->ClearChildren();
	ContractsOverview->ClearChildren();
	const float Wrap = 300.f;
	const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	const UWEStaffComponent* Staff = UWEStaffComponent::Get(this);
	if (Progression)
	{
		const int32 Tier = Progression->GetCompanyTier();
		AddInfo(TierList, FText::Format(LOCTEXT("TierNow", "Stufe {0}: {1}"), FText::AsNumber(Tier), UWEProgressionComponent::GetTierName(Tier)),
			LOCTEXT("TierWhat", "Größere Gebäude bringen Platz, Strom, mehr Mitarbeiter, neue Maschinen und Industrieaufträge."), FText::GetEmpty(), Wrap);
		const FText Next = Progression->GetNextTierText();
		AddInfo(TierList, LOCTEXT("TierNext", "Nächste Stufe"), Next.IsEmpty() ? LOCTEXT("TierTop", "Höchste Stufe erreicht") : Next, FText::GetEmpty(), Wrap);
		AddInfo(TierList, LOCTEXT("Value", "Firmenwert"), LOCTEXT("ValueDetail", "Konto + halber Kaufpreis von Einrichtung, Maschinen, Gebäuden und Fahrzeugen"), MoneyText(Progression->GetCompanyValue()), Wrap);
	}
	if (const UWEPowerGridComponent* Grid = UWEPowerGridComponent::Get(this))
	{
		AddInfo(TierList, LOCTEXT("Power", "Stromanschluss"), FText::Format(LOCTEXT("PowerDetail", "{0} von {1} kW belegt"),
			FText::AsNumber(FMath::RoundToInt(Grid->GetLoadKw() * 10.f) / 10.f), FText::AsNumber(FMath::RoundToInt(Grid->GetCapacityKw()))), FText::GetEmpty(), Wrap);
	}
	if (Staff)
	{
		AddInfo(TierList, LOCTEXT("StaffCap", "Personal"), FText::Format(LOCTEXT("StaffCapDetail", "{0} von {1} Mitarbeitern"),
			FText::AsNumber(Staff->GetEmployees().Num()), FText::AsNumber(Staff->GetMaxEmployees())), FText::GetEmpty(), Wrap);
	}

	// Buildings: owned ones, with sale (Phase 17.5) once something bigger is there.
	if (Progression && Progression->GetCatalog())
	{
		AddInfo(BuildingList, LOCTEXT("Garage", "Garage (Firmensitz)"), LOCTEXT("GarageDetail", "Das erste Gebäude – bleibt immer."), FText::GetEmpty(), Wrap);
		for (const FWEUpgradeDefinition& Upgrade : Progression->GetCatalog()->Upgrades)
		{
			if (Upgrade.CompanyTier <= 0 || !Progression->HasUpgrade(Upgrade.Id))
			{
				continue;
			}
			UWETradeRowWidget* Row = AddRow(BuildingList, Wrap);
			Row->UpgradeId = Upgrade.Id;
			Row->SetPicture(nullptr, TEXT("Building"), Col::Info);
			if (!Progression->CanOfferBuildingSale(Upgrade.Id))
			{
				Row->Configure(this, EWETradeRowKind::Building, Upgrade.DisplayName, LOCTEXT("BuildingInUse", "Das größte Gebäude – kann nicht verkauft werden."), FText::GetEmpty(), FText::GetEmpty(), false);
				continue;
			}
			const TArray<FText> Blockers = Progression->GetBuildingSaleBlockers(Upgrade.Id);
			const FText Detail = Blockers.Num() > 0
				? FText::Format(LOCTEXT("BuildingBlocked", "Noch nicht verkaufbar: {0}"), FText::Join(FText::FromString(TEXT("; ")), Blockers))
				: FText::Join(FText::FromString(TEXT(" · ")), Progression->GetBuildingSaleEffects(Upgrade.Id));
			Row->Configure(this, EWETradeRowKind::Building, Upgrade.DisplayName, Detail, MoneyText(Progression->GetBuildingSalePrice(Upgrade.Id)),
				LOCTEXT("ClearBuilding", "Räumen"), Blockers.Num() > 0, LOCTEXT("SellBuilding", "Verkaufen"), Blockers.Num() == 0);
			Row->SetButtonKinds(EButton::Secondary, EButton::Danger);
			Row->AddChip(Blockers.Num() > 0 ? LOCTEXT("ChipBlocked", "belegt") : LOCTEXT("ChipFree", "leer"), Blockers.Num() > 0 ? Col::Bad : Col::Good);
		}
	}

	if (const UWEMilestoneComponent* Milestones = UWEMilestoneComponent::Get(this); Milestones && Milestones->GetCatalog())
	{
		const TArray<FWEMilestoneDefinition>& All = Milestones->GetCatalog()->Milestones;
		AddInfo(MilestoneList, FText::Format(LOCTEXT("MilestonesTitle", "{0} von {1} erreicht"), FText::AsNumber(Milestones->GetCompleted().Num()), FText::AsNumber(All.Num())),
			LOCTEXT("MilestonesDetail", "Ziele für die ganze Firma – jedes bringt eine Prämie aufs Firmenkonto."), FText::GetEmpty(), Wrap);
		TArray<const FWEMilestoneDefinition*> Open;
		for (const FWEMilestoneDefinition& M : All)
		{
			if (!Milestones->IsCompleted(M.Id))
			{
				Open.Add(&M);
			}
		}
		Open.StableSort([Milestones](const FWEMilestoneDefinition& A, const FWEMilestoneDefinition& B)
		{
			return static_cast<double>(Milestones->GetProgress(A.Kind)) / A.Target > static_cast<double>(Milestones->GetProgress(B.Kind)) / B.Target;
		});
		for (int32 i = 0; i < Open.Num() && i < 8; ++i)
		{
			UWETradeRowWidget* Row = AddRow(MilestoneList, Wrap);
			Row->SetPicture(nullptr, TEXT("Medal"), Col::TextMuted);
			Row->Configure(this, EWETradeRowKind::Info, Open[i]->Title, FText::Format(LOCTEXT("MilestoneOpen", "{0}  ·  {1}"), Open[i]->Description, Milestones->GetProgressText(*Open[i])),
				Open[i]->Reward.IsZero() ? FText::GetEmpty() : MoneyText(Open[i]->Reward), FText::GetEmpty(), false);
		}
		for (const FWEMilestoneDefinition& M : All)
		{
			if (Milestones->IsCompleted(M.Id))
			{
				UWETradeRowWidget* Row = AddRow(MilestoneList, Wrap);
				Row->SetPicture(nullptr, TEXT("Check"), Col::Good);
				Row->Configure(this, EWETradeRowKind::Info, M.Title, M.Description, FText::GetEmpty(), FText::GetEmpty(), false);
			}
		}
	}
	if (const UWEContractComponent* Contracts = UWEContractComponent::Get(this))
	{
		const float Now = Contracts->GetServerTime();
		for (const FWEContract& C : Contracts->GetContracts())
		{
			if (C.State != EWEContractState::Accepted)
			{
				continue;
			}
			const int32 Minutes = FMath::Max(0, FMath::CeilToInt((C.ExpiresAt - Now) / 60.f));
			AddInfo(ContractsOverview, C.Title, FText::Format(LOCTEXT("OverviewDetail", "{0}× {1} · noch {2} min{3}"), FText::AsNumber(C.Quantity),
				C.Item ? C.Item->DisplayName : FText::GetEmpty(), FText::AsNumber(Minutes),
				C.Rounds > 1 ? FText::Format(LOCTEXT("OverviewRounds", " · Lieferung {0} von {1}"), FText::AsNumber(C.RoundsDone + 1), FText::AsNumber(C.Rounds)) : FText::GetEmpty()),
				MoneyText(C.Reward), Wrap);
		}
		if (ContractsOverview->GetChildrenCount() == 0)
		{
			AddInfo(ContractsOverview, LOCTEXT("NoOrders", "Keine laufenden Aufträge"), LOCTEXT("NoOrdersDetail", "Aufträge im Programm \"Aufträge\" annehmen."), FText::GetEmpty(), Wrap);
		}
	}
}

// --- App: Gestaltung ---------------------------------------------------------------------------------------------------

void UWETradeWidget::BuildStyleApp(UVerticalBox* Body)
{
	UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddH(Head, Text(WidgetTree, LOCTEXT("StyleBuilding", "Gebäude:"), 13, Col::TextDim, EFont::Strong), FMargin(0.f, 0.f, 8.f, 0.f));
	UTextBlock* ZoneLabel = nullptr;
	UButton* Zone = Button(WidgetTree, LOCTEXT("GarageZone", "Garage"), EButton::Secondary, TEXT("ChevronRight"), 12, &ZoneLabel);
	StyleZoneText = ZoneLabel;
	Zone->OnClicked.AddDynamic(this, &UWETradeWidget::HandleStyleZoneNext);
	AddH(Head, Zone, FMargin(), false);
	AddH(Head, WidgetTree->ConstructWidget<USpacer>(), FMargin(), true);
	AddH(Head, Text(WidgetTree, LOCTEXT("DecoHint", "Deko und Einrichtung kaufst du im Baumodus [B]."), 11, Col::TextDim));
	AddV(Body, Head, FMargin(0.f, 0.f, 0.f, 10.f));
	UHorizontalBox* Cats = WidgetTree->ConstructWidget<UHorizontalBox>();
	const FName CatIcons[] = { TEXT("Paint"), TEXT("Floor"), TEXT("Lamp"), TEXT("Door") };
	for (int32 i = 0; i < 4; ++i)
	{
		UTextBlock* Label = nullptr;
		UImage* TabIcon = nullptr;
		UButton* B = Button(WidgetTree, UWEWorkshopStyleComponent::GetCategoryName(static_cast<EWEStyleCategory>(i)), EButton::Tab, CatIcons[i], 13, &Label, &TabIcon);
		AddH(Cats, B, FMargin(0.f, 0.f, 2.f, 0.f));
		StyleCatButtons.Add(B);
		StyleCatLabels.Add(Label);
		StyleCatIcons.Add(TabIcon);
	}
	StyleCatButtons[0]->OnClicked.AddDynamic(this, &UWETradeWidget::HandleStyleCat0);
	StyleCatButtons[1]->OnClicked.AddDynamic(this, &UWETradeWidget::HandleStyleCat1);
	StyleCatButtons[2]->OnClicked.AddDynamic(this, &UWETradeWidget::HandleStyleCat2);
	StyleCatButtons[3]->OnClicked.AddDynamic(this, &UWETradeWidget::HandleStyleCat3);
	AddV(Body, Cats);
	AddV(Body, Divider(WidgetTree, Col::LineStrong), FMargin(0.f, 0.f, 0.f, 10.f));
	StyleList = MakeList(Body);
	SelectTab(nullptr, StyleCatButtons, StyleCatLabels, StyleCatIcons, 0);
}

void UWETradeWidget::HandleStyleCat0() { StyleCategory = 0; SelectTab(nullptr, StyleCatButtons, StyleCatLabels, StyleCatIcons, 0); }
void UWETradeWidget::HandleStyleCat1() { StyleCategory = 1; SelectTab(nullptr, StyleCatButtons, StyleCatLabels, StyleCatIcons, 1); }
void UWETradeWidget::HandleStyleCat2() { StyleCategory = 2; SelectTab(nullptr, StyleCatButtons, StyleCatLabels, StyleCatIcons, 2); }
void UWETradeWidget::HandleStyleCat3() { StyleCategory = 3; SelectTab(nullptr, StyleCatButtons, StyleCatLabels, StyleCatIcons, 3); }

void UWETradeWidget::HandleStyleZoneNext()
{
	const UWEWorkshopStyleComponent* Style = UWEWorkshopStyleComponent::Get(this);
	const int32 Count = Style ? Style->GetOwnedZones().Num() : 1;
	StyleZoneIndex = (StyleZoneIndex + 1) % FMath::Max(1, Count);
	LastSignature.Reset();
}

void UWETradeWidget::RebuildStyle(const FWEMoney& Balance)
{
	if (!StyleList)
	{
		return;
	}
	StyleList->ClearChildren();
	const UWEWorkshopStyleComponent* Style = UWEWorkshopStyleComponent::Get(this);
	if (!Style)
	{
		return;
	}
	const TArray<FName> Zones = Style->GetOwnedZones();
	StyleZoneIndex = FMath::Clamp(StyleZoneIndex, 0, Zones.Num() - 1);
	const FName Zone = Zones[StyleZoneIndex];
	StyleZoneText->SetText(Upper(UWEWorkshopStyleComponent::GetZoneName(Zone)));
	const EWEStyleCategory Category = static_cast<EWEStyleCategory>(StyleCategory);
	const FWEZoneStyle Current = Style->GetZoneStyle(Zone);
	const TArray<FWEStyleOption>& Options = UWEWorkshopStyleComponent::GetOptions(Category);
	for (int32 i = 0; i < Options.Num(); ++i)
	{
		const FWEStyleOption& O = Options[i];
		const bool bCurrent = Current.Get(Category) == i;
		const FText Lock = Style->GetLockReason(Zone, Category, i);
		const FWEMoney Price = Style->GetPrice(Zone, Category, i);
		UWETradeRowWidget* Row = AddRow(StyleList);
		Row->StyleZone = Zone;
		Row->StyleCategory = static_cast<uint8>(Category);
		Row->OfferIndex = i;
		Row->SetSwatch(O.Swatch);
		Row->Configure(this, EWETradeRowKind::Style, O.Name,
			bCurrent ? LOCTEXT("StyleCurrent", "Aktuell gewählt") : !Lock.IsEmpty() ? Lock : Category == EWEStyleCategory::Lighting
				? FText::Format(LOCTEXT("LightDetail", "Helligkeit {0} %"), FText::AsNumber(FMath::RoundToInt(O.LightFactor * 100.f))) : FText::GetEmpty(),
			Price.IsZero() ? LOCTEXT("Free", "gratis") : MoneyText(Price), bCurrent ? FText::GetEmpty() : LOCTEXT("ApplyStyle", "Anwenden"),
			!bCurrent && Lock.IsEmpty() && Balance >= Price);
		if (bCurrent)
		{
			Row->SetHighlight(Col::Accent);
			Row->AddChip(LOCTEXT("ChipCurrent", "aktiv"), Col::Accent);
		}
		else if (!Lock.IsEmpty())
		{
			Row->AddChip(LOCTEXT("ChipLockedStyle", "gesperrt"), Col::TextMuted);
		}
	}
}

// --- App: Hilfe --------------------------------------------------------------------------------------------------------

void UWETradeWidget::BuildHelpApp(UVerticalBox* Body)
{
	HelpList = MakeList(Body);
}

void UWETradeWidget::RebuildHelp()
{
	if (!HelpList)
	{
		return;
	}
	HelpList->ClearChildren();
	AddSection(HelpList, LOCTEXT("HelpTutorial", "Tutorial"), TEXT("Bulb"));
	const UWETutorialComponent* Tutorial = UWETutorialComponent::Get(this);
	const EWETutorialState State = Tutorial ? Tutorial->GetState() : EWETutorialState::Off;
	UWETradeRowWidget* Row = AddRow(HelpList);
	Row->SetPicture(nullptr, TEXT("Bulb"), Col::Accent);
	FText Detail;
	switch (State)
	{
	case EWETutorialState::Active:    Detail = FText::Format(LOCTEXT("TutActive", "Läuft – Schritt {0} von {1}"), FText::AsNumber(Tutorial->GetStep() + 1), FText::AsNumber(UWETutorialComponent::NumSteps())); break;
	case EWETutorialState::Completed: Detail = LOCTEXT("TutDone", "Abgeschlossen"); break;
	case EWETutorialState::Skipped:   Detail = LOCTEXT("TutSkipped", "Übersprungen"); break;
	default:                          Detail = LOCTEXT("TutOff", "Nicht gestartet"); break;
	}
	Row->Configure(this, EWETradeRowKind::Tutorial, LOCTEXT("TutTitle", "Einführung in Werkstatt, Werkbank, Handel und Aufträge"), Detail, FText::GetEmpty(),
		State == EWETutorialState::Active ? LOCTEXT("TutRestart", "Neu starten") : LOCTEXT("TutStart", "Starten"), true,
		State == EWETutorialState::Active ? LOCTEXT("TutSkip", "Überspringen") : FText::GetEmpty(), State == EWETutorialState::Active);
	Row->SetButtonKinds(EButton::Primary, EButton::Secondary);

	AddSection(HelpList, LOCTEXT("HelpControls", "Steuerung"), TEXT("Keyboard"));
	const TPair<const TCHAR*, FText> Keys[] = {
		{ TEXT("W A S D"), LOCTEXT("KMove", "Laufen, Shift sprintet, Leertaste springt, Strg duckt") },
		{ TEXT("E"), LOCTEXT("KUse", "Aufheben, benutzen, Werkbank: Arbeitsansicht") },
		{ TEXT("Q"), LOCTEXT("KAlt", "Reparatur / Öffnen / zweite Aktion") },
		{ TEXT("R"), LOCTEXT("KR", "Vom Tisch nehmen, ablegen") },
		{ TEXT("F"), LOCTEXT("KExamine", "Untersuchen") },
		{ TEXT("Tab"), LOCTEXT("KInv", "Inventar") },
		{ TEXT("B"), LOCTEXT("KBuild", "Baumodus (Einrichtung, Maschinen, Deko)") },
		{ TEXT("F5"), LOCTEXT("KSave", "Speichern (Host)") },
		{ TEXT("Esc"), LOCTEXT("KMenu", "Menü, Einstellungen, Tutorial") },
	};
	for (const TPair<const TCHAR*, FText>& K : Keys)
	{
		UWETradeRowWidget* KeyRow = AddRow(HelpList);
		KeyRow->SetPicture(nullptr, TEXT("Keyboard"), Col::TextDim);
		KeyRow->Configure(this, EWETradeRowKind::Info, FText::FromString(K.Key), K.Value, FText::GetEmpty(), FText::GetEmpty(), false);
	}
	AddSection(HelpList, LOCTEXT("HelpAbout", "Über Ferrit OS"), TEXT("Info"));
	AddInfo(HelpList, LOCTEXT("About", "Ferrit OS 3.4 – Werkstattrechner"), LOCTEXT("AboutDetail", "Alle Programme arbeiten für die ganze Firma: Was hier gekauft oder angenommen wird, sehen alle Mitspieler sofort."));
}

// --- Signature / rebuild -----------------------------------------------------------------------------------------------

FString UWETradeWidget::BuildSignature() const
{
	TStringBuilder<2048> Sig;
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	Sig << (Company ? Company->GetBalance().GetCents() : -1) << TEXT("|") << (Company ? Company->GetRecentTransactions().Num() : 0) << TEXT("|");
	if (const UWEMilestoneComponent* Milestones = UWEMilestoneComponent::Get(this))
	{
		const FWECompanyStats& S = Milestones->GetStats();
		Sig << TEXT("ms") << Milestones->GetCompleted().Num() << TEXT(",") << S.DevicesDisassembled << TEXT(",") << S.DevicesRepaired << TEXT(",") << S.OrdersCompleted
			<< TEXT(",") << S.IndustrialOrders << TEXT(",") << S.MachineCycles << TEXT(",") << S.ToursCompleted << TEXT(",") << S.RareFinds.Num() << TEXT("|");
	}
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	if (const UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr)
	{
		for (const FWEInventorySlotData& SlotData : Inventory->GetAllSlots())
		{
			Sig << SlotData.SlotIndex << TEXT(":") << GetNameSafe(SlotData.Item.Definition) << TEXT("x") << SlotData.Item.Quantity
				<< TEXT("c") << static_cast<int32>(SlotData.Item.Condition) << TEXT("m") << SlotData.Item.Marks << TEXT(",");
		}
	}
	if (const UWEClassifiedsComponent* Classifieds = IsAppOpen(EApp::Classifieds) ? UWEClassifiedsComponent::Get(this) : nullptr)
	{
		const float ClassifiedNow = Classifieds->GetServerTime();
		for (const FWEClassified& Ad : Classifieds->GetAds())
		{
			Sig << TEXT("|ad:") << Ad.Id.A << Ad.Counter.GetCents() << TEXT(",") << Ad.Offers << TEXT(",") << FMath::CeilToInt((Ad.ExpiresAt - ClassifiedNow) / 60.f);
		}
		for (const FWEClassifiedDelivery& D : Classifieds->GetDeliveries())
		{
			Sig << TEXT("|dl:") << FMath::CeilToInt((D.ArrivesAt - ClassifiedNow) / 5.f);
		}
		for (const FWEOwnListing& L : Classifieds->GetListings())
		{
			Sig << TEXT("|ol:") << L.Id.A << L.Offer.GetCents();
		}
	}
	if (Terminal && (IsAppOpen(EApp::Market) || IsAppOpen(EApp::Classifieds)))
	{
		// Goods in the workshop storages are on the sell list too.
		for (const UWEInventoryComponent* Storage : Terminal->GetWorkshopStorages())
		{
			Sig << TEXT("|s");
			for (const FWEInventorySlotData& SlotData : Storage->GetAllSlots())
			{
				Sig << SlotData.SlotIndex << GetNameSafe(SlotData.Item.Definition) << SlotData.Item.Quantity << SlotData.Item.Marks << TEXT(",");
			}
		}
	}
	Sig << TEXT("|held:") << (Character && Character->GetHeldItem() ? *Character->GetHeldItem()->GetName() : TEXT("-"));
	if (const UWEContractComponent* Ratings = UWEContractComponent::Get(this))
	{
		Sig << TEXT("|rt:") << Ratings->GetRatingSum() << TEXT("/") << Ratings->GetRatingCount();
	}
	if (const AWEPickupItem* HeldPc = Character ? Character->GetHeldItem() : nullptr)
	{
		Sig << TEXT("|hb:") << WEBenchmark::Score(HeldPc->GetItem()) << HeldPc->GetItem().RemovedParts;
	}
	if (const AWEPickupItem* HeldItem = Character ? Character->GetHeldItem() : nullptr)
	{
		Sig << TEXT("c") << static_cast<int32>(HeldItem->GetItem().Condition) << TEXT("q") << HeldItem->GetItem().Quality;
	}
	if (Terminal)
	{
		Sig << TEXT("|stock:");
		for (const int32 Value : Terminal->GetStockRemaining())
		{
			Sig << Value << TEXT(",");
		}
	}
	if (const UWEMarketComponent* Market = UWEMarketComponent::Get(this))
	{
		for (const float D : Market->GetAllDemand())
		{
			Sig << FMath::RoundToInt(D * 100.f) << TEXT(",");
		}
	}
	if (const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this))
	{
		for (const FName& Id : Progression->GetOwnedUpgrades())
		{
			Sig << TEXT("|u:") << *Id.ToString();
		}
	}
	Sig << TEXT("|page:") << CurrentPage << TEXT(",") << MarketTab << TEXT(",") << BuyFilter << TEXT(",") << StyleZoneIndex << TEXT(",") << StyleCategory;
	if (const UWEAuctionComponent* Auctions = UWEAuctionComponent::Get(this))
	{
		const float AuctionNow = Auctions->GetServerTime();
		for (const FWEAuctionLot& Lot : Auctions->GetLots())
		{
			Sig << TEXT("|a:") << Lot.Id.ToString() << Lot.CurrentBid.GetCents() << (Lot.bCompanyLeads ? TEXT("L") : TEXT("-"));
			if (IsAppOpen(EApp::Auctions))
			{
				Sig << TEXT("s") << FMath::CeilToInt(Lot.EndsAt - AuctionNow); // countdown only while visible
			}
		}
		for (const FString& Result : Auctions->GetRecentResults())
		{
			Sig << TEXT("|r:") << Result;
		}
	}
	if (const UWEStaffComponent* Staff = UWEStaffComponent::Get(this))
	{
		for (const FWEEmployeeProfile& A : Staff->GetApplicants())
		{
			Sig << TEXT("|ap:") << A.Name;
		}
		for (const AWEEmployee* E : Staff->GetEmployees())
		{
			Sig << TEXT("|e:") << E->GetName() << static_cast<int32>(E->GetState()) << E->GetStateText().ToString() << E->GetTaskText().ToString();
		}
		Sig << TEXT("|ow:") << Staff->GetOwedWages().GetCents();
		if (IsAppOpen(EApp::Finance) || IsAppOpen(EApp::Company))
		{
			if (Company)
			{
				for (const int64 Cents : Company->GetCategoryTotals())
				{
					Sig << TEXT("|bk:") << Cents;
				}
			}
		}
		if (IsAppOpen(EApp::Staff))
		{
			Sig << TEXT("|pt:") << FMath::CeilToInt(Staff->GetSecondsToPayroll() / 10.f);
			for (TActorIterator<AWEVehicle> It(GetWorld()); It; ++It)
			{
				Sig << TEXT("|v:") << (It->IsAway() ? FMath::CeilToInt(It->GetSecondsToReturn() / 10.f) : -1) << It->GetLastResult() << (It->IsUnlocked() ? 1 : 0);
			}
		}
	}
	if (const UWEContractComponent* Contracts = UWEContractComponent::Get(this))
	{
		const float Now = Contracts->GetServerTime();
		for (const FWEContract& C : Contracts->GetContracts())
		{
			Sig << TEXT("|") << C.Id.ToString() << static_cast<int32>(C.State) << TEXT("m") << FMath::CeilToInt((C.ExpiresAt - Now) / 60.f);
		}
	}
	if (const UWEWorkshopStyleComponent* Style = UWEWorkshopStyleComponent::Get(this))
	{
		for (const FWEZoneStyle& Z : Style->GetAllStyles())
		{
			Sig << TEXT("|st:") << *Z.Zone.ToString() << Z.WallPaint << Z.Floor << Z.Lighting << Z.Door;
		}
	}
	if (const UWETutorialComponent* Tutorial = UWETutorialComponent::Get(this))
	{
		Sig << TEXT("|tu:") << static_cast<int32>(Tutorial->GetState()) << Tutorial->GetStep();
	}
	if (IsAppOpen(EApp::Company))
	{
		// Things lying in old buildings change whether they can be sold.
		if (const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this))
		{
			for (const FName& Id : Progression->GetOwnedUpgrades())
			{
				Sig << TEXT("|bs:") << Progression->GetBuildingSaleBlockers(Id).Num();
			}
		}
	}
	return FString(Sig.ToView());
}

void UWETradeWidget::Rebuild()
{
	if (!Terminal)
	{
		return;
	}
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	const FWEMoney Balance = Company ? Company->GetBalance() : FWEMoney();
	RebuildMarket(Balance);
	RebuildOrders();
	RebuildAuctions(Balance);
	RebuildStaff(Balance);
	RebuildFinance();
	RebuildCompany();
	RebuildStyle(Balance);
	RebuildHelp();
	RebuildClassifieds(Balance);
	if (TrayBalance)
	{
		TrayBalance->SetText(MoneyText(Balance));
	}
	if (const UWEContractComponent* Contracts = UWEContractComponent::Get(this); Contracts && TrayOrders)
	{
		int32 Running = 0;
		int32 Offers = 0;
		for (const FWEContract& C : Contracts->GetContracts())
		{
			(C.State == EWEContractState::Accepted ? Running : Offers)++;
		}
		TrayOrders->SetText(FText::Format(LOCTEXT("TrayOrders", "{0} laufend · {1} neu"), FText::AsNumber(Running), FText::AsNumber(Offers)));
	}
}

// --- Tick / input ---------------------------------------------------------------------------------------------------------

void UWETradeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer());
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	if (!Terminal || !Terminal->IsWithinUseRange(Character))
	{
		if (PC)
		{
			PC->CloseTrade();
		}
		return;
	}
	// Fade in after the camera reached the monitor.
	if (FadeIn < 1.f)
	{
		FadeIn = FMath::Min(1.f, FadeIn + InDeltaTime / 0.25f);
		SetRenderOpacity(FMath::Clamp(FadeIn, 0.f, 1.f));
	}
	for (FAppWindow& W : Windows)
	{
		if (W.bOpen && !W.bMinimized && W.Anim.IsRunning())
		{
			W.Anim.Tick(W.Frame, InDeltaTime);
		}
	}
	ClockAccumulated += InDeltaTime;
	if (ClockAccumulated >= 1.f && TrayClock)
	{
		ClockAccumulated = 0.f;
		// Phase 19: the time of the game world, not of the real computer.
		if (const UWEDayNightComponent* Clock = UWEDayNightComponent::Get(this))
		{
			TrayClock->SetText(FText::Format(LOCTEXT("TrayClock", "Tag {0} · {1}"), FText::AsNumber(Clock->GetDay()), Clock->GetClockText()));
		}
	}
	// Rebuild only when something visible changed (keeps buttons stable under the mouse).
	const FString Signature = BuildSignature();
	if (Signature != LastSignature)
	{
		LastSignature = Signature;
		Rebuild();
		ForceLayoutPrepass();
	}
}

FReply UWETradeWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape && IsConfirmOpen())
	{
		HandleConfirmNo();
		return FReply::Handled();
	}
	if (Key == EKeys::Escape && StartMenu && StartMenu->GetVisibility() == ESlateVisibility::Visible)
	{
		StartMenu->SetVisibility(ESlateVisibility::Collapsed);
		return FReply::Handled();
	}
	if (Key == EKeys::Escape || Key == EKeys::Tab || Key == EKeys::E)
	{
		HandleLogout();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UWETradeWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (StartMenu && StartMenu->GetVisibility() == ESlateVisibility::Visible && !StartMenu->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		StartMenu->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || IsConfirmOpen())
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	// The topmost open window under the mouse: its title bar starts dragging; anywhere else focuses it.
	int32 Best = INDEX_NONE;
	int32 BestZ = -1;
	for (int32 i = 0; i < WEOs::NumApps; ++i)
	{
		const FAppWindow& W = Windows[i];
		const UCanvasPanelSlot* S = W.Frame ? Cast<UCanvasPanelSlot>(W.Frame->Slot) : nullptr;
		if (W.bOpen && !W.bMinimized && S && W.Frame->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()) && S->GetZOrder() > BestZ)
		{
			Best = i;
			BestZ = S->GetZOrder();
		}
	}
	if (Best == INDEX_NONE)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	FocusApp(static_cast<EApp>(Best));
	FAppWindow& W = Windows[Best];
	if (W.TitleBar && W.TitleBar->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		DragApp = Best;
		const FVector2D Local = Desktop->GetCachedGeometry().AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		DragOffset = Local - W.Position;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return FReply::Handled();
}

FReply UWETradeWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (DragApp != INDEX_NONE && Windows[DragApp].Frame)
	{
		FAppWindow& W = Windows[DragApp];
		const FVector2D Desk = GetDesktopSize();
		const FVector2D Local = Desktop->GetCachedGeometry().AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		W.Position.X = FMath::Clamp(Local.X - DragOffset.X, -W.Size.X + 120.f, Desk.X - 120.f);
		W.Position.Y = FMath::Clamp(Local.Y - DragOffset.Y, 0.f, Desk.Y - 40.f);
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(W.Frame->Slot))
		{
			S->SetPosition(W.Position);
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UWETradeWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (DragApp != INDEX_NONE)
	{
		DragApp = INDEX_NONE;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

// --- Dialog / taskbar handlers ------------------------------------------------------------------------------------------

void UWETradeWidget::ShowConfirm(const FText& Title, const FText& Message, const FText& ConfirmLabel, TFunction<void()> OnConfirm, bool bDanger)
{
	PendingConfirm = MoveTemp(OnConfirm);
	DialogTitle->SetText(Upper(Title));
	DialogMessage->SetText(Message);
	DialogYesText->SetText(Upper(ConfirmLabel));
	SetButtonKind(DialogYes, bDanger ? EButton::Danger : EButton::Primary, DialogYesText);
	DialogLayer->SetVisibility(ESlateVisibility::Visible);
	Play(this, ESound::Notify, 0.5f);
}

bool UWETradeWidget::IsConfirmOpen() const
{
	return DialogLayer && DialogLayer->GetVisibility() == ESlateVisibility::Visible;
}

void UWETradeWidget::HandleConfirmYes()
{
	DialogLayer->SetVisibility(ESlateVisibility::Collapsed);
	if (PendingConfirm)
	{
		TFunction<void()> Action = MoveTemp(PendingConfirm);
		PendingConfirm = nullptr;
		Action();
	}
}

void UWETradeWidget::HandleConfirmNo()
{
	DialogLayer->SetVisibility(ESlateVisibility::Collapsed);
	PendingConfirm = nullptr;
	Play(this, ESound::Close, 0.4f);
}

void UWETradeWidget::HandleStart()
{
	if (StartMenu)
	{
		StartMenu->SetVisibility(StartMenu->GetVisibility() == ESlateVisibility::Visible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void UWETradeWidget::HandleLogout()
{
	if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
	{
		Play(this, ESound::Close, 0.5f);
		PC->CloseTrade();
	}
}

void UWETradeWidget::HandleApp0() { OpenApp(EApp::Market); }
void UWETradeWidget::HandleApp1() { OpenApp(EApp::Auctions); }
void UWETradeWidget::HandleApp2() { OpenApp(EApp::Staff); }
void UWETradeWidget::HandleApp3() { OpenApp(EApp::Company); }
void UWETradeWidget::HandleApp4() { OpenApp(EApp::Orders); }
void UWETradeWidget::HandleApp5() { OpenApp(EApp::Finance); }
void UWETradeWidget::HandleApp6() { OpenApp(EApp::Style); }
void UWETradeWidget::HandleApp7() { OpenApp(EApp::Help); }
void UWETradeWidget::HandleApp8() { OpenApp(EApp::Classifieds); }
void UWETradeWidget::HandleTask0() { TaskClicked(EApp::Market); }
void UWETradeWidget::HandleTask1() { TaskClicked(EApp::Auctions); }
void UWETradeWidget::HandleTask2() { TaskClicked(EApp::Staff); }
void UWETradeWidget::HandleTask3() { TaskClicked(EApp::Company); }
void UWETradeWidget::HandleTask4() { TaskClicked(EApp::Orders); }
void UWETradeWidget::HandleTask5() { TaskClicked(EApp::Finance); }
void UWETradeWidget::HandleTask6() { TaskClicked(EApp::Style); }
void UWETradeWidget::HandleTask7() { TaskClicked(EApp::Help); }
void UWETradeWidget::HandleTask8() { TaskClicked(EApp::Classifieds); }
void UWETradeWidget::HandleClose0() { CloseApp(EApp::Market); }
void UWETradeWidget::HandleClose1() { CloseApp(EApp::Auctions); }
void UWETradeWidget::HandleClose2() { CloseApp(EApp::Staff); }
void UWETradeWidget::HandleClose3() { CloseApp(EApp::Company); }
void UWETradeWidget::HandleClose4() { CloseApp(EApp::Orders); }
void UWETradeWidget::HandleClose5() { CloseApp(EApp::Finance); }
void UWETradeWidget::HandleClose6() { CloseApp(EApp::Style); }
void UWETradeWidget::HandleClose7() { CloseApp(EApp::Help); }
void UWETradeWidget::HandleClose8() { CloseApp(EApp::Classifieds); }
void UWETradeWidget::HandleMin0() { MinimizeApp(EApp::Market); }
void UWETradeWidget::HandleMin1() { MinimizeApp(EApp::Auctions); }
void UWETradeWidget::HandleMin2() { MinimizeApp(EApp::Staff); }
void UWETradeWidget::HandleMin3() { MinimizeApp(EApp::Company); }
void UWETradeWidget::HandleMin4() { MinimizeApp(EApp::Orders); }
void UWETradeWidget::HandleMin5() { MinimizeApp(EApp::Finance); }
void UWETradeWidget::HandleMin6() { MinimizeApp(EApp::Style); }
void UWETradeWidget::HandleMin7() { MinimizeApp(EApp::Help); }
void UWETradeWidget::HandleMin8() { MinimizeApp(EApp::Classifieds); }

// --- Row actions ----------------------------------------------------------------------------------------------------------

void UWETradeWidget::HandleRowAction(const UWETradeRowWidget* Row, int32 ButtonIndex)
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer());
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	if (!PC || !Row || !Terminal)
	{
		return;
	}
	UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr;
	switch (Row->Kind)
	{
	case EWETradeRowKind::Sell:
	{
		UWEInventoryComponent* From = Row->SourceInventory.IsValid() ? Row->SourceInventory.Get() : Inventory;
		const FWEItemInstance* Item = From ? From->GetItemInSlot(Row->InventorySlot) : nullptr;
		const int32 Quantity = ButtonIndex == 1 ? 1 : 0;
		const int32 SellSlot = Row->InventorySlot;
		TWeakObjectPtr<AWETraderTerminal> WeakTerminal = Terminal.Get();
		auto Sell = [PC, WeakTerminal, From, SellSlot, Quantity]() { if (WeakTerminal.IsValid()) { PC->RequestSell(WeakTerminal.Get(), From, SellSlot, Quantity); } };
		if (Item && Item->NeedsSaleConfirmation())
		{
			FWEItemInstance Selling = *Item;
			Selling.Quantity = Quantity > 0 ? 1 : Item->Quantity;
			ShowConfirm(LOCTEXT("ConfirmSellTitle", "Wertvollen Gegenstand verkaufen?"),
				FText::Format(LOCTEXT("ConfirmSellText", "{0} ist selten oder wertvoll (Schätzwert {1}). Wirklich für {2} verkaufen?\n\nTipp: Mit Rechtsklick im Inventar kannst du solche Teile sperren oder als Favorit markieren."),
					Item->GetDisplayName(), MoneyText(Selling.GetEstimatedValue()), MoneyText(Terminal->GetBestSellPrice(Selling))),
				LOCTEXT("ConfirmSellBtn", "Verkaufen"), Sell);
		}
		else
		{
			Sell();
		}
		break;
	}
	case EWETradeRowKind::SellHeld:
	{
		const AWEPickupItem* Held = Character ? Character->GetHeldItem() : nullptr;
		TWeakObjectPtr<AWETraderTerminal> WeakTerminal = Terminal.Get();
		auto Sell = [PC, WeakTerminal]() { if (WeakTerminal.IsValid()) { PC->RequestSellHeld(WeakTerminal.Get()); } };
		if (Held && Held->GetItem().NeedsSaleConfirmation())
		{
			ShowConfirm(LOCTEXT("ConfirmSellTitle2", "Wertvollen Gegenstand verkaufen?"),
				FText::Format(LOCTEXT("ConfirmSellText2", "{0} ist selten oder wertvoll. Wirklich für {1} verkaufen?"), Held->GetDisplayName(), MoneyText(Terminal->GetBestSellPrice(Held->GetItem()))),
				LOCTEXT("ConfirmSellBtn2", "Verkaufen"), Sell);
		}
		else
		{
			Sell();
		}
		break;
	}
	case EWETradeRowKind::Protected:
		if (Inventory && Row->MarkToClear != 0)
		{
			// Equipment: release (set the mark); locked / favourite: clear the mark.
			const bool bSet = Row->MarkToClear == FWEItemInstance::ReleasedForSale;
			PC->RequestSetItemMarks(Inventory, Row->InventorySlot, Row->MarkToClear, bSet);
		}
		break;
	case EWETradeRowKind::Buy:
		PC->RequestBuy(Terminal, Row->OfferIndex, 1);
		break;
	case EWETradeRowKind::Upgrade:
		PC->RequestBuyUpgrade(Terminal, Row->UpgradeId);
		break;
	case EWETradeRowKind::Auction:
		PC->RequestBid(Terminal, Row->LotId);
		break;
	case EWETradeRowKind::Applicant:
		PC->RequestHire(Terminal, Row->OfferIndex);
		break;
	case EWETradeRowKind::Employee:
		if (AWEEmployee* Employee = Row->Employee.Get())
		{
			if (ButtonIndex == 2)
			{
				TWeakObjectPtr<AWEEmployee> WeakEmployee = Employee;
				TWeakObjectPtr<AWETraderTerminal> WeakTerminal = Terminal.Get();
				ShowConfirm(LOCTEXT("ConfirmFireTitle", "Mitarbeiter entlassen?"),
					FText::Format(LOCTEXT("ConfirmFireText", "{0} wirklich entlassen? Der offene Lohn wird abgerechnet."), FText::FromString(Employee->GetProfile().Name)),
					LOCTEXT("ConfirmFireBtn", "Entlassen"), [PC, WeakEmployee, WeakTerminal]()
					{
						if (WeakEmployee.IsValid() && WeakTerminal.IsValid())
						{
							PC->RequestStaffAction(WeakTerminal.Get(), WeakEmployee.Get(), AWEPlayerController::StaffFire);
						}
					}, true);
			}
			else
			{
				PC->RequestStaffAction(Terminal, Employee, Row->bEmployeePaused ? AWEPlayerController::StaffResume : AWEPlayerController::StaffPause);
			}
		}
		break;
	case EWETradeRowKind::Contract:
		if (Row->bContractAccepted)
		{
			PC->RequestDeliverContract(Terminal, Row->ContractId);
		}
		else
		{
			PC->RequestAcceptContract(Terminal, Row->ContractId);
		}
		break;
	case EWETradeRowKind::Building:
	{
		const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
		const FName Id = Row->UpgradeId;
		if (ButtonIndex == 1)
		{
			PC->RequestClearBuilding(Terminal, Id);
		}
		else if (Progression)
		{
			const FWEUpgradeDefinition* Building = Progression->GetCatalog() ? Progression->GetCatalog()->Find(Id) : nullptr;
			TWeakObjectPtr<AWETraderTerminal> WeakTerminal = Terminal.Get();
			ShowConfirm(LOCTEXT("ConfirmBuildingTitle", "Gebäude verkaufen?"),
				FText::Format(LOCTEXT("ConfirmBuildingText", "{0} verkaufen? Das lässt sich nur durch einen Neukauf zum vollen Preis rückgängig machen.\n\n{1}"),
					Building ? Building->DisplayName : FText::FromName(Id), FText::Join(FText::FromString(TEXT("\n")), Progression->GetBuildingSaleEffects(Id))),
				LOCTEXT("ConfirmBuildingBtn", "Verkaufen"), [PC, WeakTerminal, Id]() { if (WeakTerminal.IsValid()) { PC->RequestSellBuilding(WeakTerminal.Get(), Id); } }, true);
		}
		break;
	}
	case EWETradeRowKind::Style:
		PC->RequestApplyStyle(Terminal, Row->StyleZone, Row->StyleCategory, Row->OfferIndex);
		break;
	case EWETradeRowKind::ClassifiedAd:
		if (ButtonIndex == 1)
		{
			PC->RequestClassifiedOffer(Row->ContractId, Row->PriceA);
		}
		else if (const UWEClassifiedsComponent* Classifieds = UWEClassifiedsComponent::Get(this))
		{
			const FWEClassified* Ad = Classifieds->GetAds().FindByPredicate([Row](const FWEClassified& A) { return A.Id == Row->ContractId; });
			if (Ad && !Ad->Counter.IsZero())
			{
				PC->RequestClassifiedAcceptCounter(Row->ContractId);
			}
			else
			{
				PC->RequestClassifiedOffer(Row->ContractId, Row->PriceB);
			}
		}
		break;
	case EWETradeRowKind::ClassifiedNew:
		PC->RequestClassifiedList(Row->SourceInventory.Get(), Row->InventorySlot, ButtonIndex == 1 ? Row->PriceA : Row->PriceB);
		break;
	case EWETradeRowKind::ClassifiedOwn:
		if (ButtonIndex == 1)
		{
			PC->RequestClassifiedAcceptBuyer(Row->ContractId);
		}
		else
		{
			PC->RequestClassifiedWithdraw(Row->ContractId);
		}
		break;
	case EWETradeRowKind::Tutorial:
		PC->RequestTutorial(ButtonIndex == 1 ? 0 : 1);
		if (ButtonIndex == 1)
		{
			HandleLogout(); // the tutorial starts in the workshop
		}
		break;
	default:
		break;
	}
}

#undef LOCTEXT_NAMESPACE
