#include "UI/WEHUDWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Economy/WECompanyComponent.h"
#include "EngineUtils.h"
#include "Framework/WEPlayerController.h"
#include "Interaction/WEInteractionComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Machines/WEMachine.h"
#include "Machines/WEMachineDefinition.h"
#include "Machines/WEPowerGridComponent.h"
#include "Player/WECharacter.h"
#include "Progression/WEProgressionComponent.h"
#include "Progression/WETutorialComponent.h"
#include "UI/WEUIKit.h"
#include "Workshop/WEBuildComponent.h"
#include "Workshop/WEFurnitureDefinition.h"
#include "Workshop/WEWorkbench.h"
#include "Workshop/WEWorkshopLayoutComponent.h"

#define LOCTEXT_NAMESPACE "WEHUD"

using namespace WEUI;

namespace WEHUDStyle
{
	FProgressBarStyle BarStyle(const FLinearColor& Fill)
	{
		FProgressBarStyle S;
		S.SetBackgroundImage(Brush(Col::Hex(0x0B0C0E, 0.9f), Col::Line, 1.f, 1.f));
		S.SetFillImage(Brush(Fill, FLinearColor::Transparent, 0.f, 1.f));
		S.SetMarqueeImage(Brush(Fill, FLinearColor::Transparent, 0.f, 1.f));
		return S;
	}

	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Child, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position)
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Child);
		S->SetAnchors(Anchors);
		S->SetAlignment(Alignment);
		S->SetPosition(Position);
		S->SetAutoSize(true);
		return S;
	}

	/** Card: dark translucent body with a thin outline and an accent strip on the left. */
	UBorder* Card(UWidgetTree* Tree, UWidget* Content, const FLinearColor& Strip = Col::Accent, const FMargin& Padding = FMargin(14.f, 10.f))
	{
		UBorder* Frame = Box(Tree, Col::Hex(0x101215, 0.84f), FMargin(0.f), Col::WithAlpha(Col::LineStrong, 0.6f), 2.f);
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>();
		Frame->SetContent(Row);
		if (Strip.A > 0.f)
		{
			AddH(Row, Sized(Tree, Box(Tree, Strip, FMargin(0.f), FLinearColor::Transparent, 0.f), 3.f), FMargin(), false, VAlign_Fill);
		}
		UBorder* Inner = Box(Tree, FLinearColor::Transparent, Padding, FLinearColor::Transparent, 0.f);
		Inner->SetContent(Content);
		AddH(Row, Inner, FMargin(), true, VAlign_Fill);
		return Frame;
	}
}

using namespace WEHUDStyle;

FText UWEHUDWidget::FormatPrompt(const FText& KeyText, const FText& ActionText)
{
	if (KeyText.IsEmpty())
	{
		return ActionText;
	}
	return FText::Format(LOCTEXT("PromptFmt", "[{0}] {1}"), KeyText, ActionText);
}

void UWEHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return; // already built (e.g. by a UMG subclass)
	}
	Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Canvas;

	BuildCenter();
	BuildStatusCard();
	BuildObjectiveCard();
	BuildBottom();
	BuildExamineCard();
	BuildBuildPanel();

	// Notifications: a stack in the upper centre, newest at the bottom.
	ToastBox = WidgetTree->ConstructWidget<UVerticalBox>();
	Place(Canvas, ToastBox, FAnchors(0.5f, 0.f), FVector2D(0.5f, 0.f), FVector2D(0.f, 84.f));
	for (int32 i = 0; i < MaxToasts; ++i)
	{
		FToast T;
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		T.IconImage = Icon(WidgetTree, TEXT("Info"), 18.f, Col::Accent);
		AddH(Row, T.IconImage, FMargin(0.f, 0.f, 10.f, 0.f));
		T.Label = Text(WidgetTree, FText::GetEmpty(), 14, Col::Text, EFont::Strong, 560.f);
		AddH(Row, T.Label);
		T.Frame = Box(WidgetTree, Col::Hex(0x101215, 0.9f), FMargin(14.f, 8.f), Col::WithAlpha(Col::LineStrong, 0.7f), 2.f);
		T.Frame->SetContent(Row);
		T.Frame->SetVisibility(ESlateVisibility::Collapsed);
		AddV(ToastBox, T.Frame, FMargin(0.f, 0.f, 0.f, 6.f), HAlign_Center);
		ToastWidgets.Add(T.Frame);
		Toasts.Add(T);
	}

	// World markers (tutorial).
	for (int32 i = 0; i < MaxMarkers; ++i)
	{
		UVerticalBox* Marker = WidgetTree->ConstructWidget<UVerticalBox>();
		UTextBlock* Label = Text(WidgetTree, FText::GetEmpty(), 12, Col::TextOnAccent, EFont::Head);
		UBorder* Tag = Box(WidgetTree, Col::Accent, FMargin(8.f, 2.f), FLinearColor::Transparent, 2.f);
		Tag->SetContent(Label);
		AddV(Marker, Tag, FMargin(), HAlign_Center);
		AddV(Marker, Icon(WidgetTree, TEXT("ChevronDown"), 26.f, Col::Accent), FMargin(0.f, -2.f, 0.f, 0.f), HAlign_Center);
		UCanvasPanelSlot* S = Place(Canvas, Marker, FAnchors(0.f, 0.f), FVector2D(0.5f, 1.f), FVector2D::ZeroVector);
		S->SetZOrder(-1);
		Marker->SetVisibility(ESlateVisibility::Collapsed);
		MarkerWidgets.Add(Marker);
		MarkerLabels.Add(Label);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UWEHUDWidget::BuildCenter()
{
	// Crosshair: a small dot that turns amber when something can be used.
	Crosshair = WidgetTree->ConstructWidget<UImage>();
	Crosshair->SetBrush(Brush(FLinearColor::White, Col::WithAlpha(FLinearColor::Black, 0.6f), 1.f, 3.f));
	Crosshair->SetColorAndOpacity(Col::WithAlpha(FLinearColor::White, 0.85f));
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Crosshair))
	{
		S->SetAnchors(FAnchors(0.5f, 0.5f));
		S->SetAlignment(FVector2D(0.5f, 0.5f));
		S->SetSize(FVector2D(6.f, 6.f));
	}

	// Prompts: key cap + action, centred below the crosshair.
	UVerticalBox* PromptBox = WidgetTree->ConstructWidget<UVerticalBox>();
	for (int32 i = 0; i < MaxPromptLines; ++i)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		UTextBlock* KeyLabel = nullptr;
		UWidget* Cap = KeyCap(WidgetTree, FText::FromString(TEXT("E")), 13);
		if (UBorder* CapBorder = Cast<UBorder>(Cap))
		{
			KeyLabel = Cast<UTextBlock>(CapBorder->GetContent());
		}
		AddH(Row, Cap, FMargin(0.f, 0.f, 8.f, 0.f));
		UTextBlock* Action = Text(WidgetTree, FText::GetEmpty(), 16, Col::Text, EFont::Strong);
		Action->SetShadowOffset(FVector2D(1.f, 1.f));
		Action->SetShadowColorAndOpacity(Col::WithAlpha(FLinearColor::Black, 0.85f));
		AddH(Row, Action);
		AddV(PromptBox, Row, FMargin(0.f, 0.f, 0.f, 5.f), HAlign_Center);
		Row->SetVisibility(ESlateVisibility::Collapsed);
		PromptRows.Add(Row);
		PromptKeys.Add(KeyLabel);
		PromptCaps.Add(Cap);
		PromptTexts.Add(Action);
	}
	Place(Canvas, PromptBox, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.f), FVector2D(0.f, 34.f));
}

void UWEHUDWidget::BuildStatusCard()
{
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	TierText = Heading(WidgetTree, FText::GetEmpty(), 11, Col::TextDim);
	AddV(Box, TierText, FMargin(), HAlign_Right);
	UHorizontalBox* BalanceRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddH(BalanceRow, Icon(WidgetTree, TEXT("Euro"), 20.f, Col::Money), FMargin(0.f, 0.f, 8.f, 0.f));
	BalanceText = Text(WidgetTree, FText::GetEmpty(), 21, Col::Money, EFont::Head);
	AddH(BalanceRow, BalanceText);
	AddV(Box, BalanceRow, FMargin(0.f, 1.f, 0.f, 0.f), HAlign_Right);
	UHorizontalBox* Power = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddH(Power, Icon(WidgetTree, TEXT("Power"), 14.f, Col::Accent), FMargin(0.f, 0.f, 6.f, 0.f));
	PowerBar = WidgetTree->ConstructWidget<UProgressBar>();
	PowerBar->SetWidgetStyle(BarStyle(Col::Accent));
	AddH(Power, Sized(WidgetTree, PowerBar, 90.f, 6.f), FMargin(0.f, 0.f, 8.f, 0.f));
	PowerText = Text(WidgetTree, FText::GetEmpty(), 11, Col::TextDim, EFont::Strong);
	AddH(Power, PowerText);
	PowerRow = Power;
	AddV(Box, Power, FMargin(0.f, 5.f, 0.f, 0.f), HAlign_Right);
	StatusCard = Card(WidgetTree, Box, FLinearColor::Transparent, FMargin(14.f, 8.f, 14.f, 9.f));
	Place(Canvas, StatusCard, FAnchors(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-22.f, 18.f));
}

void UWEHUDWidget::BuildObjectiveCard()
{
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	// Phase 18d: compact - small label, the task in one or two lines, a thin progress line.
	ObjectiveLabel = Heading(WidgetTree, FText::GetEmpty(), 9, Col::Accent);
	AddV(Box, ObjectiveLabel);
	ObjectiveText = Text(WidgetTree, FText::GetEmpty(), 12, Col::Text, EFont::Body, 270.f);
	AddV(Box, ObjectiveText, FMargin(0.f, 2.f, 0.f, 0.f));
	ObjectiveProgress = WidgetTree->ConstructWidget<UProgressBar>();
	ObjectiveProgress->SetWidgetStyle(BarStyle(Col::Accent));
	AddV(Box, Sized(WidgetTree, ObjectiveProgress, 0.f, 2.f), FMargin(0.f, 6.f, 0.f, 0.f));
	ObjectiveHint = Text(WidgetTree, FText::GetEmpty(), 9, Col::TextMuted, EFont::Body);
	AddV(Box, ObjectiveHint, FMargin(0.f, 4.f, 0.f, 0.f));
	ObjectiveCard = Card(WidgetTree, Box, Col::Accent, FMargin(10.f, 7.f));
	ObjectiveCard->SetRenderOpacity(0.92f);
	Place(Canvas, Sized(WidgetTree, ObjectiveCard, 296.f), FAnchors(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(18.f, 14.f));
}

void UWEHUDWidget::BuildBottom()
{
	// Work progress (bench step, machine), above the held item.
	UVerticalBox* WorkBox = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBox* WorkHead = WidgetTree->ConstructWidget<UHorizontalBox>();
	WorkText = Text(WidgetTree, FText::GetEmpty(), 13, Col::Text, EFont::Strong);
	AddH(WorkHead, WorkText, FMargin(), true);
	WorkPercent = Text(WidgetTree, FText::GetEmpty(), 13, Col::Accent, EFont::Head);
	AddH(WorkHead, WorkPercent, FMargin(12.f, 0.f, 0.f, 0.f));
	AddV(WorkBox, WorkHead);
	WorkProgress = WidgetTree->ConstructWidget<UProgressBar>();
	WorkProgress->SetWidgetStyle(BarStyle(Col::Accent));
	AddV(WorkBox, Sized(WidgetTree, WorkProgress, 0.f, 6.f), FMargin(0.f, 6.f, 0.f, 0.f));
	WorkCard = Card(WidgetTree, Sized(WidgetTree, WorkBox, 380.f), FLinearColor::Transparent, FMargin(14.f, 8.f));
	WorkCard->SetVisibility(ESlateVisibility::Collapsed);
	Place(Canvas, WorkCard, FAnchors(0.5f, 1.f), FVector2D(0.5f, 1.f), FVector2D(0.f, -112.f));

	// Held item, bottom centre.
	UHorizontalBox* Held = WidgetTree->ConstructWidget<UHorizontalBox>();
	HeldIcon = WidgetTree->ConstructWidget<UImage>();
	HeldIcon->SetDesiredSizeOverride(FVector2D(44.f, 44.f));
	AddH(Held, HeldIcon, FMargin(0.f, 0.f, 12.f, 0.f));
	UVerticalBox* HeldTexts = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(HeldTexts, Heading(WidgetTree, LOCTEXT("InHand", "In der Hand"), 10, Col::TextMuted));
	HeldName = Text(WidgetTree, FText::GetEmpty(), 15, Col::Text, EFont::Strong);
	AddV(HeldTexts, HeldName);
	HeldDetail = Text(WidgetTree, FText::GetEmpty(), 11, Col::TextDim);
	AddV(HeldTexts, HeldDetail);
	AddH(Held, HeldTexts);
	HeldCard = Card(WidgetTree, Held, Col::Info, FMargin(12.f, 8.f, 16.f, 8.f));
	HeldCard->SetVisibility(ESlateVisibility::Collapsed);
	Place(Canvas, HeldCard, FAnchors(0.5f, 1.f), FVector2D(0.5f, 1.f), FVector2D(0.f, -26.f));
}

void UWEHUDWidget::BuildExamineCard()
{
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>();
	ExamineIcon = WidgetTree->ConstructWidget<UImage>();
	ExamineIcon->SetDesiredSizeOverride(FVector2D(56.f, 56.f));
	AddH(Head, ExamineIcon, FMargin(0.f, 0.f, 12.f, 0.f));
	ExamineTitle = Text(WidgetTree, FText::GetEmpty(), 19, Col::Text, EFont::Head, 290.f);
	AddH(Head, ExamineTitle, FMargin(), true);
	AddV(Box, Head);
	ExamineDescription = Text(WidgetTree, FText::GetEmpty(), 13, Col::TextDim, EFont::Body, 360.f);
	AddV(Box, ExamineDescription, FMargin(0.f, 8.f, 0.f, 10.f));
	AddV(Box, Divider(WidgetTree));
	ExamineDetails = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(Box, ExamineDetails, FMargin(0.f, 8.f, 0.f, 0.f));
	ExamineCard = Card(WidgetTree, Sized(WidgetTree, Box, 360.f), Col::Info, FMargin(16.f, 12.f));
	ExamineCard->SetVisibility(ESlateVisibility::Collapsed);
	Place(Canvas, ExamineCard, FAnchors(1.f, 0.5f), FVector2D(1.f, 0.5f), FVector2D(-40.f, 0.f));
}

void UWEHUDWidget::BuildBuildPanel()
{
	// Phase 18d: compact build menu - category tabs, the pieces of the chosen category with picture, price and a
	// one-line description, then the status of the preview and the keys.
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(Box, SectionHeader(WidgetTree, LOCTEXT("BuildTitle", "Baumodus"), TEXT("Building"), nullptr, 13));
	UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>();
	for (int32 c = 0; c < WEFurniture::NumCategories; ++c)
	{
		UImage* CatIcon = Icon(WidgetTree, WEFurniture::GetCategoryIcon(static_cast<EWEFurnitureCategory>(c)), 18.f, Col::TextDim);
		UBorder* Tab = WEUI::Box(WidgetTree, Col::WithAlpha(Col::PanelDark, 0.7f), FMargin(10.f, 6.f), Col::Line, 3.f);
		Tab->SetContent(CatIcon);
		Tab->SetHorizontalAlignment(HAlign_Center);
		AddH(Tabs, Tab, FMargin(0.f, 0.f, 4.f, 0.f), true);
		BuildCatFrames.Add(Tab);
		BuildCatIcons.Add(CatIcon);
	}
	AddV(Box, Tabs, FMargin(0.f, 10.f, 0.f, 0.f));
	UHorizontalBox* CatLine = WidgetTree->ConstructWidget<UHorizontalBox>();
	BuildCategoryLabel = Text(WidgetTree, FText::GetEmpty(), 12, Col::Accent, EFont::Strong);
	AddH(CatLine, BuildCategoryLabel, FMargin(), true);
	AddH(CatLine, KeyCap(WidgetTree, FText::FromString(TEXT("T")), 10), FMargin(6.f, 0.f, 4.f, 0.f));
	AddH(CatLine, Text(WidgetTree, LOCTEXT("NextCat", "nächste Kategorie"), 10, Col::TextMuted));
	AddV(Box, CatLine, FMargin(2.f, 8.f, 0.f, 4.f));
	BuildList = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(Box, BuildList, FMargin(0.f, 2.f, 0.f, 0.f));
	for (int32 i = 0; i < BuildRows; ++i)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		UImage* Picture = WidgetTree->ConstructWidget<UImage>();
		Picture->SetDesiredSizeOverride(FVector2D(40.f, 40.f));
		UBorder* PictureFrame = WEUI::Box(WidgetTree, Col::WithAlpha(Col::PanelDark, 0.85f), FMargin(2.f), Col::Line, 3.f);
		PictureFrame->SetContent(Sized(WidgetTree, Picture, 40.f, 40.f));
		AddH(Row, PictureFrame, FMargin(0.f, 0.f, 10.f, 0.f));
		UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>();
		UTextBlock* Name = Text(WidgetTree, FText::GetEmpty(), 13, Col::Text, EFont::Strong);
		AddV(Lines, Name);
		UTextBlock* Desc = Text(WidgetTree, FText::GetEmpty(), 10, Col::TextMuted, EFont::Body);
		Desc->SetClipping(EWidgetClipping::ClipToBounds);
		AddV(Lines, Sized(WidgetTree, Desc, 230.f), FMargin(0.f, 1.f, 0.f, 0.f));
		AddH(Row, Lines, FMargin(), true);
		UTextBlock* Price = Text(WidgetTree, FText::GetEmpty(), 12, Col::Money, EFont::Strong);
		AddH(Row, Price, FMargin(8.f, 0.f, 0.f, 0.f));
		UBorder* Frame = WEUI::Box(WidgetTree, FLinearColor::Transparent, FMargin(6.f, 4.f), FLinearColor::Transparent, 3.f);
		Frame->SetContent(Row);
		AddV(BuildList, Frame, FMargin(0.f, 0.f, 0.f, 2.f));
		BuildRowFrames.Add(Frame);
		BuildRowNames.Add(Name);
		BuildRowPrices.Add(Price);
		BuildRowImages.Add(Picture);
		BuildRowDescs.Add(Desc);
	}
	BuildMoreText = Text(WidgetTree, FText::GetEmpty(), 10, Col::TextMuted);
	AddV(Box, BuildMoreText, FMargin(6.f, 0.f, 0.f, 0.f));
	BuildStatusText = Text(WidgetTree, FText::GetEmpty(), 12, Col::Accent, EFont::Strong, 340.f);
	AddV(Box, BuildStatusText, FMargin(0.f, 8.f, 0.f, 6.f));
	AddV(Box, Divider(WidgetTree));
	// Key hints wrap onto a second line instead of running out of the card.
	UWrapBox* Keys = WidgetTree->ConstructWidget<UWrapBox>();
	Keys->SetInnerSlotPadding(FVector2D(0.f, 4.f));
	auto Key = [this, Keys](const TCHAR* Cap, const FText& What)
	{
		UHorizontalBox* Pair = WidgetTree->ConstructWidget<UHorizontalBox>();
		AddH(Pair, KeyCap(WidgetTree, FText::FromString(Cap), 10), FMargin(0.f, 0.f, 4.f, 0.f));
		AddH(Pair, Text(WidgetTree, What, 10, Col::TextDim), FMargin(0.f, 0.f, 10.f, 0.f));
		Keys->AddChildToWrapBox(Pair);
	};
	Key(TEXT("Rad"), LOCTEXT("KeyWheel", "Auswahl"));
	Key(TEXT("E"), LOCTEXT("KeyPlace", "Setzen"));
	Key(TEXT("R"), LOCTEXT("KeyRot", "90°"));
	Key(TEXT("Q"), LOCTEXT("KeyRot15", "15°"));
	Key(TEXT("X"), LOCTEXT("KeyRemove", "Abbauen"));
	Key(TEXT("B"), LOCTEXT("KeyLeave", "Beenden"));
	AddV(Box, Keys, FMargin(0.f, 8.f, 0.f, 0.f));
	BuildCard = Card(WidgetTree, Sized(WidgetTree, Box, 360.f), Col::Accent, FMargin(12.f, 10.f));
	BuildCard->SetVisibility(ESlateVisibility::Collapsed);
	Place(Canvas, BuildCard, FAnchors(0.f, 0.5f), FVector2D(0.f, 0.5f), FVector2D(18.f, 30.f));
}
// --- Messages -----------------------------------------------------------------------------------------------

void UWEHUDWidget::ShowMessage(const FText& Message, float Seconds)
{
	if (Toasts.Num() == 0 || Message.IsEmpty())
	{
		return;
	}
	const FString S = Message.ToString();
	// Same message again: refresh it instead of stacking duplicates.
	for (FToast& T : Toasts)
	{
		if (T.bUsed && T.Label->GetText().ToString() == S)
		{
			T.Age = FMath::Min(T.Age, 0.25f);
			T.Life = Seconds;
			return;
		}
	}
	// Free slot or the oldest one; the list shifts so that the newest is at the bottom.
	int32 Free = Toasts.IndexOfByPredicate([](const FToast& Candidate) { return !Candidate.bUsed; });
	if (Free == INDEX_NONE)
	{
		Free = 0;
		for (int32 i = 1; i < Toasts.Num(); ++i)
		{
			Free = Toasts[i].Age > Toasts[Free].Age ? i : Free;
		}
	}
	FToast& T = Toasts[Free];
	FName IconName = TEXT("Info");
	FLinearColor Color = Col::Accent;
	ESound Sound = ESound::Notify;
	if (S.StartsWith(TEXT("+")))
	{
		IconName = TEXT("Euro"); Color = Col::Money; Sound = ESound::Cash;
	}
	else if (S.StartsWith(TEXT("-")))
	{
		IconName = TEXT("Cart"); Color = Col::Info; Sound = ESound::Click;
	}
	else if (S.StartsWith(TEXT("✓")))
	{
		IconName = TEXT("Check"); Color = Col::Good; Sound = ESound::Confirm;
	}
	else if (S.Contains(TEXT("nicht")) || S.Contains(TEXT("fehlt")) || S.Contains(TEXT("Kein")) || S.Contains(TEXT("zu weit")) || S.Contains(TEXT("fehlgeschlagen"))
		|| S.Contains(TEXT("Geschützt")) || S.Contains(TEXT("gesperrt")) || S.Contains(TEXT("voll")) || S.Contains(TEXT("Zu ")))
	{
		IconName = TEXT("Warning"); Color = Col::Bad; Sound = ESound::Error;
	}
	if (UTexture2D* Tex = IconTexture(IconName))
	{
		T.IconImage->SetBrushFromTexture(Tex);
		T.IconImage->SetDesiredSizeOverride(FVector2D(18.f, 18.f));
	}
	T.IconImage->SetColorAndOpacity(Color);
	T.Frame->SetBrush(Brush(Col::Hex(0x101215, 0.92f), Col::WithAlpha(Color, 0.75f), 1.f, 2.f));
	T.Label->SetText(Message);
	T.Age = 0.f;
	T.Life = Seconds;
	T.bUsed = true;
	T.Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
	// Re-order: move to the bottom of the stack.
	ToastBox->RemoveChild(T.Frame);
	AddV(ToastBox, T.Frame, FMargin(0.f, 0.f, 0.f, 6.f), HAlign_Center);
	Play(this, Sound, 0.45f);
}

TArray<FString> UWEHUDWidget::GetMessageTexts() const
{
	TArray<FString> Result;
	for (const FToast& T : Toasts)
	{
		if (T.bUsed)
		{
			Result.Add(T.Label->GetText().ToString());
		}
	}
	return Result;
}

void UWEHUDWidget::UpdateToasts(float DeltaTime)
{
	for (FToast& T : Toasts)
	{
		if (!T.bUsed)
		{
			continue;
		}
		T.Age += DeltaTime;
		const float In = FMath::Clamp(T.Age / 0.18f, 0.f, 1.f);
		const float Out = FMath::Clamp((T.Life - T.Age) / 0.35f, 0.f, 1.f);
		const float Ease = 1.f - FMath::Pow(1.f - In, 3.f);
		T.Frame->SetRenderOpacity(FMath::Min(Ease, Out));
		T.Frame->SetRenderTranslation(FVector2D(0.f, -12.f * (1.f - Ease)));
		if (T.Age >= T.Life)
		{
			T.bUsed = false;
			T.Frame->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

// --- World markers ---------------------------------------------------------------------------------------

void UWEHUDWidget::SetWorldMarkers(const TArray<FWEWorldMarker>& InMarkers)
{
	Markers = InMarkers;
}

void UWEHUDWidget::UpdateMarkers()
{
	APlayerController* PC = GetOwningPlayer();
	const AWEPlayerController* WEPC = Cast<AWEPlayerController>(PC);
	const bool bHidden = bFocusMode || (WEPC && WEPC->IsMenuOpen());
	const float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D ViewSize = UWidgetLayoutLibrary::GetViewportSize(this) / FMath::Max(Scale, 0.01f);
	for (int32 i = 0; i < MarkerWidgets.Num(); ++i)
	{
		UWidget* Marker = MarkerWidgets[i];
		FVector2D Screen;
		const bool bShow = !bHidden && PC && Markers.IsValidIndex(i) && PC->ProjectWorldLocationToScreen(Markers[i].Location, Screen, true);
		if (!bShow)
		{
			Marker->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		Screen /= FMath::Max(Scale, 0.01f);
		Screen.X = FMath::Clamp(Screen.X, 60.f, ViewSize.X - 60.f);
		Screen.Y = FMath::Clamp(Screen.Y, 70.f, ViewSize.Y - 40.f);
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Marker->Slot))
		{
			S->SetPosition(Screen + FVector2D(0.f, -6.f * FMath::Abs(FMath::Sin(PulseTime * 3.f))));
		}
		MarkerLabels[i]->SetText(Upper(Markers[i].Label));
		Marker->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

// --- Per frame -------------------------------------------------------------------------------------------------

void UWEHUDWidget::SetFocusMode(bool bFocus)
{
	bFocusMode = bFocus;
}

UWEInteractionComponent* UWEHUDWidget::GetInteraction() const
{
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	return Character ? Character->GetInteractionComponent() : nullptr;
}

void UWEHUDWidget::UpdatePrompts()
{
	const UWEInteractionComponent* Interaction = GetInteraction();
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	const AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer());
	const bool bMenuOpen = PC && PC->IsMenuOpen();
	const bool bBuildMode = Character && Character->GetBuildComponent() && Character->GetBuildComponent()->IsActive();
	TArray<FWEInteractionPrompt> Prompts;
	if (Interaction && !bMenuOpen && !bBuildMode && !bFocusMode)
	{
		Interaction->BuildPrompts(Prompts);
	}
	bool bAction = false;
	for (int32 i = 0; i < PromptRows.Num(); ++i)
	{
		if (!Prompts.IsValidIndex(i))
		{
			PromptRows[i]->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		const bool bHasKey = !Prompts[i].KeyText.IsEmpty();
		bAction |= bHasKey;
		PromptCaps[i]->SetVisibility(bHasKey ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (PromptKeys[i])
		{
			PromptKeys[i]->SetText(Prompts[i].KeyText);
		}
		PromptTexts[i]->SetText(Prompts[i].ActionText);
		PromptTexts[i]->SetColorAndOpacity(FSlateColor(bHasKey ? Col::Text : Col::TextDim));
		PromptRows[i]->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	Crosshair->SetColorAndOpacity(bAction ? Col::Accent : Col::WithAlpha(FLinearColor::White, 0.8f));
	Crosshair->SetVisibility(bMenuOpen || bFocusMode ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Crosshair->Slot))
	{
		S->SetSize(bAction ? FVector2D(8.f, 8.f) : FVector2D(5.f, 5.f));
	}
}

void UWEHUDWidget::UpdateStatus()
{
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!Company || bFocusMode)
	{
		StatusCard->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	StatusCard->SetVisibility(ESlateVisibility::HitTestInvisible);
	BalanceText->SetText(MoneyText(Company->GetBalance()));
	const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	TierText->SetText(Progression ? Upper(FText::Format(LOCTEXT("TierFmt", "Stufe {0} · {1}"), FText::AsNumber(Progression->GetCompanyTier()),
		UWEProgressionComponent::GetTierName(Progression->GetCompanyTier()))) : FText::GetEmpty());
	const UWEPowerGridComponent* Grid = UWEPowerGridComponent::Get(this);
	if (Grid && Grid->HasMachines())
	{
		PowerRow->SetVisibility(ESlateVisibility::HitTestInvisible);
		const float Load = Grid->GetLoadKw();
		const float Capacity = FMath::Max(0.1f, Grid->GetCapacityKw());
		PowerBar->SetPercent(Load / Capacity);
		PowerBar->SetFillColorAndOpacity(Load / Capacity > 0.9f ? Col::Bad : FLinearColor::White);
		PowerText->SetText(FText::Format(LOCTEXT("PowerFmt", "{0} / {1} kW"), FText::AsNumber(FMath::RoundToInt(Load * 10.f) / 10.f), FText::AsNumber(FMath::RoundToInt(Capacity))));
	}
	else
	{
		PowerRow->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UWEHUDWidget::UpdateObjective()
{
	const UWETutorialComponent* Tutorial = UWETutorialComponent::Get(this);
	const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	TArray<FWEWorldMarker> NewMarkers;
	if (bFocusMode)
	{
		ObjectiveCard->SetVisibility(ESlateVisibility::Collapsed);
		SetWorldMarkers(NewMarkers);
		return;
	}
	if (const FWETutorialStep* Step = Tutorial ? Tutorial->GetCurrentStep() : nullptr)
	{
		ObjectiveLabel->SetText(Upper(FText::Format(LOCTEXT("TutLabel", "Tutorial {0}/{1} · {2}"), FText::AsNumber(Tutorial->GetStep() + 1),
			FText::AsNumber(UWETutorialComponent::NumSteps()), Step->Title)));
		ObjectiveText->SetText(FText::Format(LOCTEXT("TutText", "{0}  [{1}]"), Step->Text, Step->Key));
		ObjectiveProgress->SetPercent(static_cast<float>(Tutorial->GetStep()) / UWETutorialComponent::NumSteps());
		ObjectiveProgress->SetVisibility(ESlateVisibility::HitTestInvisible);
		ObjectiveHint->SetText(LOCTEXT("TutSkip", "[Esc] → Tutorial überspringen"));
		ObjectiveHint->SetVisibility(ESlateVisibility::HitTestInvisible);
		TArray<FVector> Locations;
		Tutorial->GetMarkerLocations(Locations);
		for (const FVector& L : Locations)
		{
			NewMarkers.Add({ L, Step->Title });
		}
		ObjectiveCard->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else if (Progression)
	{
		const bool bGuide = !Progression->IsGuideFinished();
		ObjectiveLabel->SetText(Upper(bGuide ? FText::Format(LOCTEXT("GuideLabel", "Erste Schritte {0}/{1}"), FText::AsNumber(Progression->GetGuideStep() + 1),
			FText::AsNumber(UWEProgressionComponent::NumGuideSteps)) : LOCTEXT("GoalLabel", "Nächstes Ziel")));
		ObjectiveText->SetText(bGuide ? UWEProgressionComponent::GetGuideStepText(Progression->GetGuideStep()) : Progression->GetGuideText());
		ObjectiveProgress->SetPercent(static_cast<float>(Progression->GetGuideStep()) / UWEProgressionComponent::NumGuideSteps);
		ObjectiveProgress->SetVisibility(bGuide ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		ObjectiveHint->SetVisibility(ESlateVisibility::Collapsed);
		ObjectiveCard->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		ObjectiveCard->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetWorldMarkers(NewMarkers);
}

void UWEHUDWidget::UpdateHeld()
{
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	const AWEPickupItem* Held = Character ? Character->GetHeldItem() : nullptr;
	if (!Held || bFocusMode)
	{
		HeldCard->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const FWEItemInstance& Item = Held->GetItem();
	HeldName->SetText(Held->GetDisplayName());
	HeldDetail->SetText(FText::Format(LOCTEXT("HeldDetail", "{0} · {1}"), WEItems::GetConditionText(Item.Condition), AWEPickupItem::FormatWeight(Item.GetTotalWeightKg())));
	UTexture2D* Tex = ItemIcon(Item.Definition);
	HeldIcon->SetVisibility(Tex ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (Tex)
	{
		HeldIcon->SetBrushFromTexture(Tex);
		HeldIcon->SetDesiredSizeOverride(FVector2D(44.f, 44.f));
	}
	HeldCard->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UWEHUDWidget::UpdateWorkProgress()
{
	// The bench this player works at, or a busy bench / machine the player is looking at.
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	const UWEInteractionComponent* Interaction = GetInteraction();
	const AWEWorkbench* Shown = nullptr;
	if (Character && GetWorld())
	{
		for (TActorIterator<AWEWorkbench> It(GetWorld()); It; ++It)
		{
			if (It->IsBusy() && It->GetWorker() == Character)
			{
				Shown = *It;
				break;
			}
		}
	}
	if (!Shown && Interaction)
	{
		const AWEPickupItem* Focused = Cast<AWEPickupItem>(Interaction->GetFocusedActor());
		const AWEWorkbench* Bench = Focused ? Focused->GetPlacedOn() : nullptr;
		Shown = Bench && Bench->IsBusy() ? Bench : nullptr;
	}
	if (bFocusMode)
	{
		WorkCard->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	if (!Shown)
	{
		const AWEMachine* Machine = Interaction ? Cast<AWEMachine>(Interaction->GetFocusedActor()) : nullptr;
		if (Machine && Machine->GetMachineDefinition())
		{
			const float MachineProgress = Machine->GetProgress();
			const bool bCycle = Machine->IsRunning() || Machine->GetProcessing().Num() > 0;
			WorkCard->SetVisibility(ESlateVisibility::HitTestInvisible);
			WorkProgress->SetPercent(bCycle ? MachineProgress : 0.f);
			WorkText->SetText(FText::Format(LOCTEXT("MachineFmt", "{0}: {1} · Wartung {2} %"), Machine->GetMachineDefinition()->DisplayName,
				Machine->GetStatusText(), FText::AsNumber(FMath::RoundToInt(Machine->GetMaintenance()))));
			WorkPercent->SetText(bCycle ? FText::Format(LOCTEXT("Pct", "{0} %"), FText::AsNumber(FMath::RoundToInt(MachineProgress * 100.f))) : FText::GetEmpty());
			return;
		}
		WorkCard->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const float Progress = Shown->GetProgress();
	WorkCard->SetVisibility(ESlateVisibility::HitTestInvisible);
	WorkProgress->SetPercent(Progress);
	WorkText->SetText(Shown->GetActiveJobText());
	WorkPercent->SetText(FText::Format(LOCTEXT("Pct2", "{0} %"), FText::AsNumber(FMath::RoundToInt(Progress * 100.f))));
}

void UWEHUDWidget::UpdateExamine()
{
	const UWEInteractionComponent* Interaction = GetInteraction();
	FWEExamineInfo Info;
	if (bFocusMode || !Interaction || !Interaction->IsExamining() || !Interaction->GetExamineInfo(Info))
	{
		ExamineCard->SetVisibility(ESlateVisibility::Collapsed);
		LastExamineTitle.Reset();
		return;
	}
	ExamineCard->SetVisibility(ESlateVisibility::HitTestInvisible);
	ExamineTitle->SetText(Info.Title);
	ExamineDescription->SetText(Info.Description);
	const AWEPickupItem* Item = Cast<AWEPickupItem>(Interaction->GetFocusedActor());
	UTexture2D* Tex = Item ? ItemIcon(Item->GetItem().Definition) : nullptr;
	ExamineIcon->SetVisibility(Tex ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (Tex)
	{
		ExamineIcon->SetBrushFromTexture(Tex);
		ExamineIcon->SetDesiredSizeOverride(FVector2D(56.f, 56.f));
	}
	// Details as "Label: Value" rows (label dim, value bright).
	FString Signature = Info.Title.ToString();
	for (const FText& D : Info.Details)
	{
		Signature += TEXT("|") + D.ToString();
	}
	if (Signature == LastExamineTitle)
	{
		return;
	}
	LastExamineTitle = Signature;
	ExamineDetails->ClearChildren();
	for (const FText& Detail : Info.Details)
	{
		FString Label, Value;
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		if (Detail.ToString().Split(TEXT(": "), &Label, &Value))
		{
			AddH(Row, Sized(WidgetTree, Text(WidgetTree, FText::FromString(Label), 12, Col::TextMuted, EFont::Body, 120.f), 125.f), FMargin(), false, VAlign_Top);
			AddH(Row, Text(WidgetTree, FText::FromString(Value), 12, Col::Text, EFont::Strong, 220.f), FMargin(), true, VAlign_Top);
		}
		else
		{
			AddH(Row, Text(WidgetTree, Detail, 12, Col::Text, EFont::Body, 340.f), FMargin(), true, VAlign_Top);
		}
		AddV(ExamineDetails, Row, FMargin(0.f, 0.f, 0.f, 3.f));
	}
}

void UWEHUDWidget::UpdateBuildPanel()
{
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	const UWEBuildComponent* Build = Character ? Character->GetBuildComponent() : nullptr;
	const UWEWorkshopLayoutComponent* Layout = UWEWorkshopLayoutComponent::Get(this);
	const UWEFurnitureCatalog* Catalog = Layout ? Layout->GetCatalog() : nullptr;
	if (!Build || !Build->IsActive() || !Catalog)
	{
		BuildCard->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	BuildCard->SetVisibility(ESlateVisibility::HitTestInvisible);
	BuildStatusText->SetText(Build->GetStatusText());
	const int32 Selected = Build->GetSelectedIndex(); // INDEX_NONE = edit
	const int32 Category = Build->GetSelectedCategory();
	for (int32 c = 0; c < BuildCatFrames.Num(); ++c)
	{
		const bool bOn = c == Category;
		BuildCatFrames[c]->SetBrush(Brush(bOn ? Col::WithAlpha(Col::Accent, 0.22f) : Col::WithAlpha(Col::PanelDark, 0.7f), bOn ? Col::Accent : Col::Line, 1.f, 3.f));
		BuildCatIcons[c]->SetColorAndOpacity(bOn ? Col::Accent : Col::TextDim);
	}
	BuildCategoryLabel->SetText(Category == INDEX_NONE ? LOCTEXT("EditCat", "Bearbeiten") : WEFurniture::GetCategoryName(static_cast<EWEFurnitureCategory>(Category)));

	// Rows: "Bearbeiten" alone in edit mode, otherwise the pieces of the selected category (scrolled to the selection).
	TArray<int32> Rows;
	if (Category == INDEX_NONE)
	{
		Rows.Add(INDEX_NONE);
	}
	else
	{
		for (const int32 i : Build->GetMenuOrder())
		{
			if (static_cast<int32>(WEFurniture::GetCategory(Catalog->Items[i])) == Category)
			{
				Rows.Add(i);
			}
		}
	}
	const int32 SelectedRow = FMath::Max(0, Rows.IndexOfByKey(Selected));
	const int32 First = FMath::Clamp(SelectedRow - BuildRows / 2, 0, FMath::Max(0, Rows.Num() - BuildRows));
	for (int32 r = 0; r < BuildRows; ++r)
	{
		const int32 RowIndex = First + r;
		if (!Rows.IsValidIndex(RowIndex))
		{
			BuildRowFrames[r]->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		BuildRowFrames[r]->SetVisibility(ESlateVisibility::HitTestInvisible);
		const int32 Index = Rows[RowIndex];
		const bool bSel = Index == Selected;
		BuildRowFrames[r]->SetBrush(Brush(bSel ? Col::WithAlpha(Col::Accent, 0.16f) : Col::WithAlpha(Col::Raised, 0.35f), bSel ? Col::Accent : FLinearColor::Transparent, 1.f, 3.f));
		if (Index == INDEX_NONE)
		{
			BuildRowNames[r]->SetText(LOCTEXT("EditRow", "Bearbeiten"));
			BuildRowDescs[r]->SetText(LOCTEXT("EditDesc", "Möbel anvisieren: [E] verschieben, [X] abbauen"));
			BuildRowPrices[r]->SetText(FText::GetEmpty());
			BuildRowImages[r]->SetBrushFromTexture(IconTexture(TEXT("Wrench")));
			BuildRowImages[r]->SetColorAndOpacity(Col::TextDim);
			continue;
		}
		const FWEFurnitureDefinition& D = Catalog->Items[Index];
		const FText Lock = Layout->GetLockReason(D);
		BuildRowNames[r]->SetText(D.DisplayName);
		BuildRowNames[r]->SetColorAndOpacity(FSlateColor(!Lock.IsEmpty() ? Col::TextMuted : bSel ? Col::Text : Col::TextDim));
		BuildRowDescs[r]->SetText(Lock.IsEmpty() ? D.Description : Lock);
		BuildRowDescs[r]->SetColorAndOpacity(FSlateColor(Lock.IsEmpty() ? Col::TextMuted : Col::Bad));
		BuildRowPrices[r]->SetText(MoneyText(D.Cost));
		BuildRowPrices[r]->SetColorAndOpacity(FSlateColor(Lock.IsEmpty() ? Col::Money : Col::TextMuted));
		if (UTexture2D* Picture = FurnitureIcon(D.Id))
		{
			BuildRowImages[r]->SetBrushFromTexture(Picture);
			BuildRowImages[r]->SetColorAndOpacity(Lock.IsEmpty() ? FLinearColor::White : FLinearColor(1.f, 1.f, 1.f, 0.4f));
		}
		else
		{
			BuildRowImages[r]->SetBrushFromTexture(IconTexture(WEFurniture::GetCategoryIcon(WEFurniture::GetCategory(D))));
			BuildRowImages[r]->SetColorAndOpacity(Col::TextDim);
		}
	}
	const int32 Hidden = Rows.Num() - BuildRows;
	BuildMoreText->SetText(Hidden > 0 ? FText::Format(LOCTEXT("More", "{0} / {1} – mit dem Mausrad weiter"), FText::AsNumber(SelectedRow + 1), FText::AsNumber(Rows.Num())) : FText::GetEmpty());
	BuildMoreText->SetVisibility(Hidden > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
void UWEHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	PulseTime += InDeltaTime;
	UpdatePrompts();
	UpdateStatus();
	UpdateObjective();
	UpdateWorkProgress();
	UpdateHeld();
	UpdateExamine();
	UpdateBuildPanel();
	UpdateToasts(InDeltaTime);
	UpdateMarkers();
}

#undef LOCTEXT_NAMESPACE
