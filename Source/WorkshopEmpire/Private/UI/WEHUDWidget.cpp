#include "UI/WEHUDWidget.h"
#include "Machines/WEMachine.h"
#include "Machines/WEMachineDefinition.h"
#include "Machines/WEPowerGridComponent.h"
#include "Components/SizeBox.h"
#include "Workshop/WEBuildComponent.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Economy/WECompanyComponent.h"
#include "Progression/WEProgressionComponent.h"
#include "EngineUtils.h"
#include "Workshop/WEWorkbench.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/WEPlayerController.h"
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
	// Fixed width, height grows with the details (machines and belts list many lines).
	USizeBox* ExamineWidth = WidgetTree->ConstructWidget<USizeBox>();
	ExamineWidth->SetWidthOverride(380.f);
	ExamineWidth->SetContent(ExaminePanel);
	if (UCanvasPanelSlot* CSlot = Canvas->AddChildToCanvas(ExamineWidth))
	{
		CSlot->SetAnchors(FAnchors(1.f, 0.5f));
		CSlot->SetAlignment(FVector2D(1.f, 0.5f));
		CSlot->SetPosition(FVector2D(-48.f, 0.f));
		CSlot->SetAutoSize(true);
	}

	// Work progress bar, lower centre (above the held-item line).
	WorkBox = WidgetTree->ConstructWidget<UVerticalBox>();
	WorkText = MakeText(14, FLinearColor(1.f, 0.8f, 0.45f));
	if (UVerticalBoxSlot* TextSlot = WorkBox->AddChildToVerticalBox(WorkText))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Center);
	}
	WorkProgress = WidgetTree->ConstructWidget<UProgressBar>();
	WorkProgress->SetFillColorAndOpacity(FLinearColor(1.f, 0.55f, 0.1f));
	if (UVerticalBoxSlot* BarSlot = WorkBox->AddChildToVerticalBox(WorkProgress))
	{
		BarSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}
	WorkBox->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* CSlot = Canvas->AddChildToCanvas(WorkBox))
	{
		CSlot->SetAnchors(FAnchors(0.5f, 1.f));
		CSlot->SetAlignment(FVector2D(0.5f, 1.f));
		CSlot->SetPosition(FVector2D(0.f, -110.f));
		CSlot->SetSize(FVector2D(360.f, 44.f));
	}

	// Guide / next goal, top left.
	GuidePanel = WidgetTree->ConstructWidget<UBorder>();
	GuidePanel->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.025f, 0.6f));
	GuidePanel->SetPadding(FMargin(12.f, 8.f));
	GuideText = MakeText(14, FLinearColor(1.f, 0.88f, 0.6f), false);
	GuideText->SetWrapTextAt(406.f); // the panel grows with the text (long steps need three lines)
	GuidePanel->SetContent(GuideText);
	if (UCanvasPanelSlot* CSlot = Canvas->AddChildToCanvas(GuidePanel))
	{
		CSlot->SetAnchors(FAnchors(0.f, 0.f));
		CSlot->SetAlignment(FVector2D(0.f, 0.f));
		CSlot->SetPosition(FVector2D(24.f, 20.f));
		CSlot->SetAutoSize(true);
	}

	// Company balance, top right.
	BalanceText = MakeText(16, FLinearColor(0.55f, 0.95f, 0.6f));
	if (UCanvasPanelSlot* CSlot = Canvas->AddChildToCanvas(BalanceText))
	{
		CSlot->SetAnchors(FAnchors(1.f, 0.f));
		CSlot->SetAlignment(FVector2D(1.f, 0.f));
		CSlot->SetPosition(FVector2D(-28.f, 22.f));
		CSlot->SetAutoSize(true);
	}

	// Notification, upper centre.
	MessagePanel = WidgetTree->ConstructWidget<UBorder>();
	MessagePanel->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.025f, 0.75f));
	MessagePanel->SetPadding(FMargin(16.f, 8.f));
	MessageText = MakeText(16, FLinearColor(1.f, 0.85f, 0.45f));
	MessagePanel->SetContent(MessageText);
	MessagePanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* CSlot = Canvas->AddChildToCanvas(MessagePanel))
	{
		CSlot->SetAnchors(FAnchors(0.5f, 0.f));
		CSlot->SetAlignment(FVector2D(0.5f, 0.f));
		CSlot->SetPosition(FVector2D(0.f, 60.f));
		CSlot->SetAutoSize(true);
	}

	// Build mode panel, left (Phase 8).
	BuildPanel = WidgetTree->ConstructWidget<UBorder>();
	BuildPanel->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.025f, 0.75f));
	BuildPanel->SetPadding(FMargin(14.f, 10.f));
	UVerticalBox* BuildBox = WidgetTree->ConstructWidget<UVerticalBox>();
	UTextBlock* BuildTitle = MakeText(16, FLinearColor(1.f, 0.78f, 0.35f), false);
	BuildTitle->SetText(LOCTEXT("BuildTitle", "Baumodus"));
	BuildBox->AddChildToVerticalBox(BuildTitle);
	BuildMenuText = MakeText(13, FLinearColor(0.9f, 0.92f, 0.95f), false);
	BuildBox->AddChildToVerticalBox(BuildMenuText);
	BuildStatusText = MakeText(13, FLinearColor(1.f, 0.88f, 0.6f), false);
	BuildStatusText->SetWrapTextAt(372.f);
	if (UVerticalBoxSlot* S = BuildBox->AddChildToVerticalBox(BuildStatusText))
	{
		S->SetPadding(FMargin(0.f, 8.f, 0.f, 4.f));
	}
	UTextBlock* BuildHelp = MakeText(11, FLinearColor(0.65f, 0.68f, 0.72f), false);
	BuildHelp->SetWrapTextAt(372.f);
	BuildHelp->SetText(LOCTEXT("BuildHelp", "Mausrad: Auswahl · [E] bestätigen · [R] 90° · [Q] 15° drehen · [X] abbauen/abbrechen · [B] Baumodus beenden"));
	BuildBox->AddChildToVerticalBox(BuildHelp);
	// Fixed width: the status line changes length all the time; a fixed width keeps the layout stable.
	USizeBox* BuildWidth = WidgetTree->ConstructWidget<USizeBox>();
	BuildWidth->SetWidthOverride(372.f);
	BuildWidth->SetContent(BuildBox);
	BuildPanel->SetContent(BuildWidth);
	BuildPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* CSlot = Canvas->AddChildToCanvas(BuildPanel))
	{
		CSlot->SetAnchors(FAnchors(0.f, 0.5f));
		CSlot->SetAlignment(FVector2D(0.f, 0.5f));
		CSlot->SetPosition(FVector2D(24.f, 40.f));
		CSlot->SetAutoSize(true);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UWEHUDWidget::UpdateBuildPanel()
{
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	const UWEBuildComponent* Build = Character ? Character->GetBuildComponent() : nullptr;
	if (!Build || !Build->IsActive())
	{
		BuildPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	BuildPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	BuildMenuText->SetText(FText::FromString(FString::Join(Build->GetMenuLines(), TEXT("\n"))));
	BuildStatusText->SetText(Build->GetStatusText());
}

void UWEHUDWidget::UpdateWorkProgress()
{
	if (!WorkBox)
	{
		return;
	}
	// Show the bench this player works at, or a busy bench the player is looking at.
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

	if (!Shown)
	{
		// A machine under the crosshair: its state and progress (Phase 9).
		const AWEMachine* Machine = Interaction ? Cast<AWEMachine>(Interaction->GetFocusedActor()) : nullptr;
		if (Machine && Machine->GetMachineDefinition())
		{
			const float MachineProgress = Machine->GetProgress();
			const bool bCycle = Machine->IsRunning() || Machine->GetProcessing().Num() > 0;
			WorkBox->SetVisibility(ESlateVisibility::HitTestInvisible);
			WorkProgress->SetPercent(bCycle ? MachineProgress : 0.f);
			WorkText->SetText(FText::Format(NSLOCTEXT("WEHUD", "MachineFmt", "{0}: {1}{2} · Wartung {3} %"), Machine->GetMachineDefinition()->DisplayName,
				Machine->GetStatusText(), bCycle ? FText::Format(NSLOCTEXT("WEHUD", "MachinePct", " … {0} %"), FText::AsNumber(FMath::RoundToInt(MachineProgress * 100.f))) : FText::GetEmpty(),
				FText::AsNumber(FMath::RoundToInt(Machine->GetMaintenance()))));
			return;
		}
		WorkBox->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	WorkBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	const float Progress = Shown->GetProgress();
	WorkProgress->SetPercent(Progress);
	WorkText->SetText(FText::Format(NSLOCTEXT("WEHUD", "WorkFmt", "{0} … {1} %"), Shown->GetActiveJobText(),
		FText::AsNumber(FMath::RoundToInt(Progress * 100.f))));
}

void UWEHUDWidget::ShowMessage(const FText& Message, float Seconds)
{
	if (MessageText && MessagePanel)
	{
		MessageText->SetText(Message);
		MessagePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
		MessageTimeLeft = Seconds;
	}
}

UWEInteractionComponent* UWEHUDWidget::GetInteraction() const
{
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	return Character ? Character->GetInteractionComponent() : nullptr;
}

void UWEHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (MessageTimeLeft > 0.f)
	{
		MessageTimeLeft -= InDeltaTime;
		if (MessageTimeLeft <= 0.f && MessagePanel)
		{
			MessagePanel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	const UWEInteractionComponent* Interaction = GetInteraction();
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());

	TArray<FWEInteractionPrompt> Prompts;
	const AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer());
	const bool bMenuOpen = PC && PC->IsMenuOpen();
	const bool bBuildMode = Character && Character->GetBuildComponent() && Character->GetBuildComponent()->IsActive();
	if (Interaction && !bMenuOpen && !bBuildMode)
	{
		Interaction->BuildPrompts(Prompts);
	}
	UpdateBuildPanel();
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

	UpdateWorkProgress();

	if (const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this))
	{
		GuideText->SetText(Progression->GetGuideText());
		GuidePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		GuidePanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (const UWECompanyComponent* Company = UWECompanyComponent::Get(this))
	{
		FText Balance = FText::Format(NSLOCTEXT("WEHUD", "BalanceFmt", "Firmenkonto  {0}"), FText::FromString(Company->GetBalance().ToDisplayString()));
		// Power use, once the workshop has machines (Phase 9).
		if (const UWEPowerGridComponent* Grid = UWEPowerGridComponent::Get(this); Grid && Grid->HasMachines())
		{
			Balance = FText::Format(NSLOCTEXT("WEHUD", "BalancePowerFmt", "{0}\nStrom  {1} / {2} kW"), Balance,
				FText::AsNumber(FMath::RoundToInt(Grid->GetLoadKw() * 10.f) / 10.f), FText::AsNumber(FMath::RoundToInt(Grid->GetCapacityKw())));
		}
		BalanceText->SetText(Balance);
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
