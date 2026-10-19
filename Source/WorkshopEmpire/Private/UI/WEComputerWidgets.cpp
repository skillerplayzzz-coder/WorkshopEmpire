#include "UI/WEComputerWidgets.h"
#include "World/WEDayNightComponent.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Economy/WETraderTerminal.h"
#include "UI/WEUIKit.h"

#define LOCTEXT_NAMESPACE "WEComputer"

using namespace WEUI;

void UWEDesktopIdleWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;
	UImage* Wallpaper = WidgetTree->ConstructWidget<UImage>();
	if (UTexture2D* Tex = UITexture(TEXT("T_FerritWallpaper")))
	{
		Wallpaper->SetBrushFromTexture(Tex);
	}
	else
	{
		Wallpaper->SetColorAndOpacity(Col::OsBlueDark);
	}
	if (UOverlaySlot* S = Root->AddChildToOverlay(Wallpaper))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Fill);
	}
	UVerticalBox* Center = WidgetTree->ConstructWidget<UVerticalBox>();
	UImage* Logo = WidgetTree->ConstructWidget<UImage>();
	if (UTexture2D* Tex = UITexture(TEXT("T_FerritLogo")))
	{
		Logo->SetBrushFromTexture(Tex);
	}
	Logo->SetDesiredSizeOverride(FVector2D(110.f, 110.f));
	Logo->SetColorAndOpacity(Col::WithAlpha(FLinearColor::White, 0.85f));
	AddV(Center, Logo, FMargin(0.f, 0.f, 0.f, 18.f), HAlign_Center);
	ClockText = Text(WidgetTree, FText::GetEmpty(), 96, Col::Hex(0xEAF2FF), EFont::Light);
	AddV(Center, ClockText, FMargin(), HAlign_Center);
	DateText = Text(WidgetTree, FText::GetEmpty(), 26, Col::Hex(0xA9C2DE), EFont::Body);
	AddV(Center, DateText, FMargin(0.f, 0.f, 0.f, 26.f), HAlign_Center);
	StatusText = Text(WidgetTree, FText::GetEmpty(), 24, Col::Hex(0xD8E6F5), EFont::Strong);
	AddV(Center, StatusText, FMargin(), HAlign_Center);
	if (UOverlaySlot* S = Root->AddChildToOverlay(Center))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Center);
	}
	UTextBlock* Brand = Text(WidgetTree, LOCTEXT("Brand", "FERRIT OS  ·  Werkstattrechner"), 18, Col::Hex(0x7F98B5), EFont::Head);
	if (UOverlaySlot* S = Root->AddChildToOverlay(Brand))
	{
		S->SetHorizontalAlignment(HAlign_Left);
		S->SetVerticalAlignment(VAlign_Bottom);
		S->SetPadding(FMargin(28.f, 0.f, 0.f, 22.f));
	}
}

void UWEDesktopIdleWidget::SetTerminal(AWETraderTerminal* InTerminal)
{
	Terminal = InTerminal;
}

void UWEDesktopIdleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Accumulated += InDeltaTime;
	if (Accumulated < 0.5f)
	{
		return;
	}
	Accumulated = 0.f;
	// Phase 19: the lock screen shows the time of the game world.
	if (const UWEDayNightComponent* Clock = UWEDayNightComponent::Get(this))
	{
		ClockText->SetText(Clock->GetClockText());
		DateText->SetText(FText::Format(LOCTEXT("GameDay", "Tag {0} · {1}"), FText::AsNumber(Clock->GetDay()), Clock->GetOpeningText()));
	}
	const FString User = Terminal.IsValid() ? Terminal->UserName : FString();
	StatusText->SetText(bLocalUse ? LOCTEXT("InUseMe", "Angemeldet")
		: !User.IsEmpty() ? FText::Format(LOCTEXT("InUse", "In Benutzung: {0}"), FText::FromString(User))
		: LOCTEXT("Locked", "Gesperrt – am Schreibtisch [E] drücken"));
}

#undef LOCTEXT_NAMESPACE
