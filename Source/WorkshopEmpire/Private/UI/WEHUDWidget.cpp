#include "UI/WEHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Interaction/WEInteractionComponent.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "WEHUD"

FText UWEHUDWidget::FormatPrompt(const FText& KeyText, const FText& ActionText)
{
	if (KeyText.IsEmpty())
	{
		return ActionText;
	}
	return FText::Format(LOCTEXT("PromptFmt", "[{0}] {1}"), KeyText, ActionText);
}

UTextBlock* UWEHUDWidget::MakeText(int32 Size, const FLinearColor& Color, bool bWrap)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", Size);
	Text->SetFont(Font);
	Text->SetColorAndOpacity(FSlateColor(Color));
	Text->SetShadowOffset(FVector2D(1.f, 1.f));
	Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.75f));
	Text->SetAutoWrapText(bWrap);
	return Text;
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

	// Crosshair: small dot in the screen centre.
	Crosshair = WidgetTree->ConstructWidget<UImage>();
	Crosshair->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.8f));
	if (UCanvasPanelSlot* CSlot = Canvas->AddChildToCanvas(Crosshair))
	{
		CSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CSlot->SetSize(FVector2D(4.f, 4.f));
	}

	// Interaction hints below the crosshair.
	UVerticalBox* PromptBox = WidgetTree->ConstructWidget<UVerticalBox>();
	for (int32 i = 0; i < MaxPromptLines; ++i)
	{
		UTextBlock* Line = MakeText(18, FLinearColor::White);
		Line->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* LineSlot = PromptBox->AddChildToVerticalBox(Line))
		{
			LineSlot->SetHorizontalAlignment(HAlign_Center);
		}
		PromptLines.Add(Line);
	}
	if (UCanvasPanelSlot* CSlot = Canvas->AddChildToCanvas(PromptBox))
	{
		CSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CSlot->SetAlignment(FVector2D(0.5f, 0.f));
		CSlot->SetPosition(FVector2D(0.f, 36.f));
		CSlot->SetAutoSize(true);
	}

	// Held item, bottom centre.
	HeldText = MakeText(14, FLinearColor(0.85f, 0.85f, 0.85f));
	if (UCanvasPanelSlot* CSlot = Canvas->AddChildToCanvas(HeldText))
	{
		CSlot->SetAnchors(FAnchors(0.5f, 1.f));
		CSlot->SetAlignment(FVector2D(0.5f, 1.f));
		CSlot->SetPosition(FVector2D(0.f, -40.f));
		CSlot->SetAutoSize(true);
	}

	// Examine panel, right side.
	ExaminePanel = WidgetTree->ConstructWidget<UBorder>();
	ExaminePanel->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.025f, 0.78f));
	ExaminePanel->SetPadding(FMargin(18.f, 14.f));
	UVerticalBox* ExamineBox = WidgetTree->ConstructWidget<UVerticalBox>();
	ExamineTitle = MakeText(22, FLinearColor(1.f, 0.78f, 0.35f));
	ExamineDescription = MakeText(14, FLinearColor::White, true);
	ExamineDetails = MakeText(14, FLinearColor(0.75f, 0.85f, 0.95f), true);
	ExamineBox->AddChildToVerticalBox(ExamineTitle);
	if (UVerticalBoxSlot* DescSlot = ExamineBox->AddChildToVerticalBox(ExamineDescription))
	{
		DescSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 8.f));
	}
	ExamineBox->AddChildToVerticalBox(ExamineDetails);
	ExaminePanel->SetContent(ExamineBox);
	ExaminePanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* CSlot = Canvas->AddChildToCanvas(ExaminePanel))
	{
		CSlot->SetAnchors(FAnchors(1.f, 0.5f));
		CSlot->SetAlignment(FVector2D(1.f, 0.5f));
		CSlot->SetPosition(FVector2D(-48.f, 0.f));
		CSlot->SetSize(FVector2D(380.f, 220.f));
		CSlot->SetAutoSize(false);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

UWEInteractionComponent* UWEHUDWidget::GetInteraction() const
{
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	return Character ? Character->GetInteractionComponent() : nullptr;
}

void UWEHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const UWEInteractionComponent* Interaction = GetInteraction();
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());

	TArray<FWEInteractionPrompt> Prompts;
	if (Interaction)
	{
		Interaction->BuildPrompts(Prompts);
	}
	for (int32 i = 0; i < PromptLines.Num(); ++i)
	{
		if (Prompts.IsValidIndex(i))
		{
			PromptLines[i]->SetText(FormatPrompt(Prompts[i].KeyText, Prompts[i].ActionText));
			PromptLines[i]->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			PromptLines[i]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	const AWEPickupItem* Held = Character ? Character->GetHeldItem() : nullptr;
	HeldText->SetText(Held ? FText::Format(LOCTEXT("HeldFmt", "In der Hand: {0}"), Held->GetDisplayName()) : FText::GetEmpty());

	FWEExamineInfo Info;
	if (Interaction && Interaction->IsExamining() && Interaction->GetExamineInfo(Info))
	{
		ExamineTitle->SetText(Info.Title);
		ExamineDescription->SetText(Info.Description);
		ExamineDetails->SetText(FText::Join(FText::FromString(TEXT("\n")), Info.Details));
		ExaminePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		ExaminePanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

#undef LOCTEXT_NAMESPACE
