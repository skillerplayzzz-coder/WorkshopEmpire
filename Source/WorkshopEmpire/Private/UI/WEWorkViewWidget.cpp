#include "UI/WEWorkViewWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Framework/WEPlayerController.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "Workshop/WEDisassemblyDefinition.h"
#include "Workshop/WEWorkbench.h"
#include "Workshop/WEWorkPlan.h"
#include "Workshop/WEWorkRig.h"

#define LOCTEXT_NAMESPACE "WEWorkView"

using namespace WEUI;

namespace
{
	constexpr int32 MaxMarkers = WEWorkPlan::MaxTasks;
	constexpr int32 MaxToolButtons = 8;
	constexpr float MarkerSize = 38.f;
	constexpr float PickRadius = 30.f;

	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position)
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Widget);
		S->SetAnchors(Anchors);
		S->SetAlignment(Alignment);
		S->SetPosition(Position);
		S->SetAutoSize(true);
		return S;
	}

	UBorder* Card(UWidgetTree* Tree, UWidget* Content)
	{
		UBorder* B = Box(Tree, Col::WithAlpha(Col::Panel, 0.94f), FMargin(18.f, 14.f), Col::Line, 2.f);
		B->SetContent(Content);
		return B;
	}
}

// --- Construction --------------------------------------------------------------------------------------

void UWEWorkViewWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;

	// Markers first, so the cards draw above them.
	for (int32 i = 0; i < MaxMarkers; ++i)
	{
		UVerticalBox* Marker = WidgetTree->ConstructWidget<UVerticalBox>();
		UBorder* Ring = Box(WidgetTree, Col::WithAlpha(Col::PanelDark, 0.75f), FMargin(7.f), Col::Accent, MarkerSize * 0.5f);
		UImage* MarkerIcon = Icon(WidgetTree, TEXT("Screw"), MarkerSize - 16.f, Col::Text);
		Ring->SetContent(MarkerIcon);
		AddV(Marker, Sized(WidgetTree, Ring, MarkerSize, MarkerSize), FMargin(), HAlign_Center);
		UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>();
		Bar->SetFillColorAndOpacity(Col::Good);
		Bar->SetPercent(0.f);
		AddV(Marker, Sized(WidgetTree, Bar, MarkerSize, 5.f), FMargin(0.f, 3.f, 0.f, 0.f), HAlign_Center);
		Marker->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Marker);
		S->SetAutoSize(true);
		S->SetAlignment(FVector2D(0.5f, 0.3f));
		Markers.Add(Marker);
		MarkerRings.Add(Ring);
		MarkerIcons.Add(MarkerIcon);
		MarkerBars.Add(Bar);
	}

	// Left: the device, mode and the list of steps.
	{
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
		AddV(V, SectionHeader(WidgetTree, LOCTEXT("Title", "Arbeitsansicht"), TEXT("Wrench")));
		DeviceTitle = Text(WidgetTree, FText::GetEmpty(), 20, Col::Text, EFont::Head, 380.f);
		AddV(V, DeviceTitle, FMargin(0.f, 10.f, 0.f, 0.f));
		DeviceInfo = Text(WidgetTree, FText::GetEmpty(), 12, Col::TextDim, EFont::Body, 380.f);
		AddV(V, DeviceInfo, FMargin(0.f, 2.f, 0.f, 10.f));
		UHorizontalBox* Modes = WidgetTree->ConstructWidget<UHorizontalBox>();
		ModeDisassemble = Button(WidgetTree, LOCTEXT("ModeDis", "Zerlegen"), EButton::TabActive, TEXT("Disassemble"), 13, &ModeDisassembleLabel);
		ModeRepair = Button(WidgetTree, LOCTEXT("ModeRep", "Reparieren"), EButton::Tab, TEXT("Repair"), 13, &ModeRepairLabel);
		ModeDisassemble->OnClicked.AddDynamic(this, &UWEWorkViewWidget::HandleModeDisassemble);
		ModeRepair->OnClicked.AddDynamic(this, &UWEWorkViewWidget::HandleModeRepair);
		AddH(Modes, ModeDisassemble, FMargin(0.f, 0.f, 6.f, 0.f));
		AddH(Modes, ModeRepair);
		AddV(V, Modes, FMargin(0.f, 0.f, 0.f, 10.f));
		AddV(V, Divider(WidgetTree));
		StepList = WidgetTree->ConstructWidget<UVerticalBox>();
		AddV(V, StepList, FMargin(0.f, 8.f, 0.f, 0.f));
		LeftCard = Card(WidgetTree, Sized(WidgetTree, V, 400.f));
		Place(Canvas, LeftCard, FAnchors(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(28.f, 28.f));
	}

	// Right: the current step, progress, hint, what it yields, buttons.
	{
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
		AddV(V, SectionHeader(WidgetTree, LOCTEXT("Current", "Aktueller Schritt"), TEXT("Play")));
		StepTitle = Text(WidgetTree, FText::GetEmpty(), 18, Col::Accent, EFont::Head, 360.f);
		AddV(V, StepTitle, FMargin(0.f, 10.f, 0.f, 6.f));
		UHorizontalBox* ProgressRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		StepBar = WidgetTree->ConstructWidget<UProgressBar>();
		StepBar->SetFillColorAndOpacity(Col::Accent);
		AddH(ProgressRow, Sized(WidgetTree, StepBar, 0.f, 8.f), FMargin(0.f, 0.f, 10.f, 0.f), true);
		StepCount = Text(WidgetTree, FText::GetEmpty(), 12, Col::TextDim, EFont::Strong);
		AddH(ProgressRow, StepCount);
		AddV(V, ProgressRow, FMargin(0.f, 0.f, 0.f, 10.f));
		HintText = Text(WidgetTree, FText::GetEmpty(), 14, Col::Text, EFont::Body, 360.f);
		AddV(V, HintText, FMargin(0.f, 0.f, 0.f, 8.f));
		YieldText = Text(WidgetTree, FText::GetEmpty(), 12, Col::TextDim, EFont::Body, 360.f);
		AddV(V, YieldText, FMargin(0.f, 0.f, 0.f, 12.f));
		UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
		UButton* Auto = Button(WidgetTree, LOCTEXT("Auto", "Automatisch [Q]"), EButton::Secondary, TEXT("Clock"), 12, &AutoLabel);
		Auto->OnClicked.AddDynamic(this, &UWEWorkViewWidget::HandleAuto);
		UButton* Close = Button(WidgetTree, LOCTEXT("Close", "Zurück [Esc]"), EButton::Ghost, TEXT("ChevronLeft"), 12);
		Close->OnClicked.AddDynamic(this, &UWEWorkViewWidget::HandleClose);
		AddH(Buttons, Auto, FMargin(0.f, 0.f, 8.f, 0.f));
		AddH(Buttons, Close);
		AddV(V, Buttons);
		RightCard = Card(WidgetTree, Sized(WidgetTree, V, 380.f));
		Place(Canvas, RightCard, FAnchors(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-28.f, 28.f));
	}

	// Bottom: tool bar.
	{
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
		ToolBar = WidgetTree->ConstructWidget<UHorizontalBox>();
		for (int32 i = 0; i < MaxToolButtons; ++i)
		{
			UTextBlock* Label = nullptr;
			UImage* ToolImage = nullptr;
			UButton* B = Button(WidgetTree, FText::FromString(TEXT("Werkzeug")), EButton::Secondary, TEXT("Hand"), 12, &Label, &ToolImage); // label set in RefreshTools
			switch (i)
			{
			case 0: B->OnClicked.AddDynamic(this, &UWEWorkViewWidget::HandleTool0); break;
			case 1: B->OnClicked.AddDynamic(this, &UWEWorkViewWidget::HandleTool1); break;
			case 2: B->OnClicked.AddDynamic(this, &UWEWorkViewWidget::HandleTool2); break;
			case 3: B->OnClicked.AddDynamic(this, &UWEWorkViewWidget::HandleTool3); break;
			case 4: B->OnClicked.AddDynamic(this, &UWEWorkViewWidget::HandleTool4); break;
			case 5: B->OnClicked.AddDynamic(this, &UWEWorkViewWidget::HandleTool5); break;
			case 6: B->OnClicked.AddDynamic(this, &UWEWorkViewWidget::HandleTool6); break;
			default: B->OnClicked.AddDynamic(this, &UWEWorkViewWidget::HandleTool7); break;
			}
			AddH(ToolBar, B, FMargin(0.f, 0.f, 6.f, 0.f));
			ToolButtons.Add(B);
			ToolLabels.Add(Label);
			ToolIcons.Add(ToolImage);
		}
		AddV(V, ToolBar, FMargin(), HAlign_Center);
		UHorizontalBox* Keys = WidgetTree->ConstructWidget<UHorizontalBox>();
		auto Key = [this, Keys](const TCHAR* Cap, const FText& What)
		{
			AddH(Keys, KeyCap(WidgetTree, FText::FromString(Cap), 10), FMargin(0.f, 0.f, 4.f, 0.f));
			AddH(Keys, Text(WidgetTree, What, 11, Col::TextDim), FMargin(0.f, 0.f, 14.f, 0.f));
		};
		Key(TEXT("1-8"), LOCTEXT("KeyTools", "Werkzeug / Mausrad"));
		Key(TEXT("Maus"), LOCTEXT("KeyHold", "auf Schraube, Stecker, Teil gedrückt halten"));
		Key(TEXT("Q"), LOCTEXT("KeyAuto", "automatisch"));
		Key(TEXT("Esc"), LOCTEXT("KeyBack", "zurück"));
		AddV(V, Keys, FMargin(0.f, 8.f, 0.f, 0.f), HAlign_Center);
		UBorder* Bar = Box(WidgetTree, Col::WithAlpha(Col::Panel, 0.92f), FMargin(14.f, 10.f), Col::Line, 2.f);
		Bar->SetContent(V);
		Place(Canvas, Bar, FAnchors(0.5f, 1.f), FVector2D(0.5f, 1.f), FVector2D(0.f, -24.f));
	}

	CenterText = Text(WidgetTree, FText::GetEmpty(), 26, Col::Text, EFont::Head, 900.f);
	CenterText->SetJustification(ETextJustify::Center);
	CenterText->SetShadowOffset(FVector2D(2.f, 2.f));
	CenterText->SetShadowColorAndOpacity(Col::WithAlpha(FLinearColor::Black, 0.85f));
	Place(Canvas, CenterText, FAnchors(0.5f, 0.42f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

	CursorIcon = Icon(WidgetTree, TEXT("Hand"), 30.f, Col::Accent);
	CursorIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* CursorSlot = Canvas->AddChildToCanvas(CursorIcon);
	CursorSlot->SetAutoSize(true);
	CursorSlot->SetAlignment(FVector2D(-0.25f, -0.25f));
	CursorSlot->SetZOrder(100);
}

void UWEWorkViewWidget::Setup(AWEWorkbench* InBench, bool bInRepair)
{
	Bench = InBench;
	bRepair = bInRepair;
	if (!CanDisassemble() && CanRepair())
	{
		bRepair = true;
	}
	else if (bRepair && !CanRepair() && CanDisassemble())
	{
		bRepair = false;
	}
	HoldTask = PendingTask = INDEX_NONE;
	LastStartRequest = -10.f;
	MarkerSignature = 0;
	LastInfoSignature.Reset();
	if (!Rig && InBench && GetWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Rig = GetWorld()->SpawnActor<AWEWorkRig>(InBench->GetActorLocation(), FRotator::ZeroRotator, Params);
	}
	if (Rig)
	{
		Rig->Init(InBench, bRepair);
	}
	RefreshTools();
	// Start with the tool the first task needs, if the player has it.
	if (Rig && Rig->GetPlan().IsValid())
	{
		const int32 Wanted = Tools.IndexOfByKey(Rig->GetPlan().Tasks[0].Tool);
		SelectToolIndex(Wanted != INDEX_NONE ? Wanted : 0);
	}
	Anim.Start();
	Play(this, ESound::Open, 0.4f);
}

void UWEWorkViewWidget::Shutdown()
{
	if (Rig)
	{
		Rig->Destroy();
		Rig = nullptr;
	}
	Bench.Reset();
}

AWECharacter* UWEWorkViewWidget::GetCharacter() const
{
	return GetOwningPlayer() ? Cast<AWECharacter>(GetOwningPlayer()->GetPawn()) : nullptr;
}

bool UWEWorkViewWidget::CanDisassemble() const
{
	const AWEWorkbench* B = Bench.Get();
	const AWEPickupItem* Item = B ? B->GetPlacedItem() : nullptr;
	return Item && Item->GetItem().Definition && Item->GetItem().Definition->CanBeDisassembled() && B->GetNextStep() && !Item->GetItem().IsCustomerItem();
}

bool UWEWorkViewWidget::CanRepair() const
{
	const AWEWorkbench* B = Bench.Get();
	FText Reason;
	return B && B->GetPlacedItem() && B->GetNextRepairStep(Reason).Kind != EWERepairStepKind::None;
}

void UWEWorkViewWidget::SetRepairMode(bool bInRepair)
{
	if (bRepair == bInRepair)
	{
		return;
	}
	AWEWorkbench* B = Bench.Get();
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer());
	// A half-done hands-on step of the other mode is dropped.
	if (B && PC && B->IsManualWork() && B->GetWorker() == GetCharacter())
	{
		PC->RequestWorkCancel(B);
	}
	bRepair = bInRepair;
	HoldTask = PendingTask = INDEX_NONE;
	LastStartRequest = -10.f;
	if (Rig)
	{
		Rig->SetRepairMode(bRepair);
	}
	LastInfoSignature.Reset();
	Play(this, ESound::Toggle, 0.4f);
}

void UWEWorkViewWidget::HandleModeDisassemble() { if (CanDisassemble()) { SetRepairMode(false); } else { Hint(LOCTEXT("NoDis", "Dieses Gerät lässt sich hier nicht weiter zerlegen."), true); } }
void UWEWorkViewWidget::HandleModeRepair() { if (CanRepair()) { SetRepairMode(true); } else { Hint(LOCTEXT("NoRep", "Hier gibt es nichts zu reparieren."), true); } }

void UWEWorkViewWidget::HandleClose()
{
	if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
	{
		PC->CloseWorkView();
	}
}

void UWEWorkViewWidget::HandleAuto()
{
	AWEWorkbench* B = Bench.Get();
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer());
	if (!B || !PC)
	{
		return;
	}
	if (B->IsBusy() && !B->IsManualWork())
	{
		return; // already running on the timer
	}
	PC->RequestWorkAuto(B, bRepair);
	HoldTask = PendingTask = INDEX_NONE;
	LastStartRequest = GetWorld()->GetTimeSeconds() + 1.f; // let the timed step start first
	Play(this, ESound::Click, 0.5f);
}

// --- Tools -------------------------------------------------------------------------------------------------

void UWEWorkViewWidget::RefreshTools()
{
	TArray<EWEToolType> Found;
	Found.Add(EWEToolType::None);
	if (const AWECharacter* Character = GetCharacter())
	{
		if (const UWEInventoryComponent* Inventory = Character->GetPlayerInventory())
		{
			for (const FWEInventorySlotData& Entry : Inventory->GetAllSlots())
			{
				const UWEItemDefinition* Def = Entry.Item.Definition;
				if (Def && Def->ToolType != EWEToolType::None)
				{
					Found.AddUnique(Def->ToolType);
				}
			}
		}
	}
	Found.SetNum(FMath::Min(Found.Num(), MaxToolButtons));
	if (Found == Tools)
	{
		return;
	}
	const EWEToolType Was = Tools.IsValidIndex(SelectedTool) ? Tools[SelectedTool] : EWEToolType::None;
	Tools = Found;
	for (int32 i = 0; i < ToolButtons.Num(); ++i)
	{
		const bool bUsed = Tools.IsValidIndex(i);
		ToolButtons[i]->SetVisibility(bUsed ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bUsed)
		{
			if (ToolLabels[i]) ToolLabels[i]->SetText(FText::Format(LOCTEXT("ToolKey", "{0}  {1}"), FText::AsNumber(i + 1), WEItems::GetToolText(Tools[i])));
			if (UTexture2D* Tex = ToolIcons[i] ? IconTexture(WEWorkPlan::GetToolIcon(Tools[i])) : nullptr)
			{
				ToolIcons[i]->SetBrushFromTexture(Tex);
				ToolIcons[i]->SetDesiredSizeOverride(FVector2D(20.f, 20.f));
			}
		}
	}
	const int32 Keep = Tools.IndexOfByKey(Was);
	SelectToolIndex(Keep != INDEX_NONE ? Keep : 0);
}

void UWEWorkViewWidget::SelectToolIndex(int32 Index)
{
	if (!Tools.IsValidIndex(Index))
	{
		return;
	}
	if (Index != SelectedTool)
	{
		Play(this, ESound::Toggle, 0.35f);
	}
	SelectedTool = Index;
	HoldTask = INDEX_NONE;
	for (int32 i = 0; i < ToolButtons.Num(); ++i)
	{
		SetButtonKind(ToolButtons[i], i == SelectedTool ? EButton::Primary : EButton::Secondary, ToolLabels[i], ToolIcons[i]);
	}
	if (UTexture2D* Tex = IconTexture(WEWorkPlan::GetToolIcon(Tools[SelectedTool])))
	{
		CursorIcon->SetBrushFromTexture(Tex);
		CursorIcon->SetDesiredSizeOverride(FVector2D(30.f, 30.f));
	}
}

void UWEWorkViewWidget::HandleTool0() { SelectToolIndex(0); }
void UWEWorkViewWidget::HandleTool1() { SelectToolIndex(1); }
void UWEWorkViewWidget::HandleTool2() { SelectToolIndex(2); }
void UWEWorkViewWidget::HandleTool3() { SelectToolIndex(3); }
void UWEWorkViewWidget::HandleTool4() { SelectToolIndex(4); }
void UWEWorkViewWidget::HandleTool5() { SelectToolIndex(5); }
void UWEWorkViewWidget::HandleTool6() { SelectToolIndex(6); }
void UWEWorkViewWidget::HandleTool7() { SelectToolIndex(7); }

void UWEWorkViewWidget::DevSelectTool(EWEToolType Tool)
{
	RefreshTools();
	SelectToolIndex(Tools.IndexOfByKey(Tool));
}

// --- Input ------------------------------------------------------------------------------------------------

FVector2D UWEWorkViewWidget::MousePosition() const
{
	return UWidgetLayoutLibrary::GetMousePositionOnViewport(const_cast<UWEWorkViewWidget*>(this));
}

int32 UWEWorkViewWidget::FindTaskAt(const FVector2D& Position) const
{
	if (!Rig)
	{
		return INDEX_NONE;
	}
	int32 Best = INDEX_NONE;
	float BestDist = PickRadius;
	for (int32 i = 0; i < Markers.Num() && i < Rig->GetPlan().Tasks.Num(); ++i)
	{
		if (Markers[i]->GetVisibility() == ESlateVisibility::Collapsed)
		{
			continue;
		}
		const UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Markers[i]->Slot);
		const float Dist = S ? FVector2D::Distance(S->GetPosition(), Position) : BIG_NUMBER;
		if (Dist < BestDist)
		{
			BestDist = Dist;
			Best = i;
		}
	}
	return Best;
}

void UWEWorkViewWidget::Hint(const FText& Text, bool bError)
{
	HintText->SetText(Text);
	HintText->SetColorAndOpacity(bError ? Col::Bad : Col::Text);
	HintTime = 2.5f;
	if (bError)
	{
		Play(this, ESound::Error, 0.45f);
	}
}

FReply UWEWorkViewWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !Rig)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	const int32 Task = FindTaskAt(MousePosition());
	if (Task == INDEX_NONE)
	{
		return FReply::Handled();
	}
	DevHoldTask(Task);
	return FReply::Handled();
}

void UWEWorkViewWidget::DevHoldTask(int32 Task)
{
	const AWEWorkbench* B = Bench.Get();
	if (!Rig || !B || !Rig->GetPlan().Tasks.IsValidIndex(Task))
	{
		return;
	}
	const FWEWorkPlan& Plan = Rig->GetPlan();
	const FWEWorkTask& T = Plan.Tasks[Task];
	const uint32 Mask = B->IsManualWork() ? B->GetTaskMask() : 0;
	if (!Plan.IsTaskAvailable(Task, Mask))
	{
		Hint(LOCTEXT("NotYet", "Erst das andere: Schrauben und Klammern vor Abdeckung und Teilen."), true);
		return;
	}
	if (!Tools.IsValidIndex(SelectedTool) || Tools[SelectedTool] != T.Tool)
	{
		const int32 Key = Tools.IndexOfByKey(T.Tool);
		Hint(Key != INDEX_NONE
			? FText::Format(LOCTEXT("WrongTool", "Falsches Werkzeug – dafür brauchst du: {0} (Taste {1})"), WEItems::GetToolText(T.Tool), FText::AsNumber(Key + 1))
			: FText::Format(LOCTEXT("MissingTool", "Dafür fehlt dir: {0}"), WEItems::GetToolText(T.Tool)), true);
		return;
	}
	if (!B->IsManualWork() || B->GetWorker() != GetCharacter() || B->IsRepairJob() != bRepair)
	{
		Hint(LOCTEXT("Wait", "Einen Moment …"));
		TryStartWork();
		return;
	}
	HoldTask = Task;
	HoldTime = 0.f;
}

FReply UWEWorkViewWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		HoldTask = INDEX_NONE;
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UWEWorkViewWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (Tools.Num() > 0)
	{
		const int32 Step = InMouseEvent.GetWheelDelta() > 0.f ? -1 : 1;
		SelectToolIndex((SelectedTool + Step + Tools.Num()) % Tools.Num());
	}
	return FReply::Handled();
}

FReply UWEWorkViewWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Tab || Key == EKeys::E)
	{
		HandleClose();
		return FReply::Handled();
	}
	if (Key == EKeys::Q)
	{
		HandleAuto();
		return FReply::Handled();
	}
	const FKey Numbers[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight };
	for (int32 i = 0; i < UE_ARRAY_COUNT(Numbers); ++i)
	{
		if (Key == Numbers[i])
		{
			SelectToolIndex(i);
			return FReply::Handled();
		}
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// --- Per frame ---------------------------------------------------------------------------------------------

void UWEWorkViewWidget::TryStartWork()
{
	AWEWorkbench* B = Bench.Get();
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer());
	AWECharacter* Character = GetCharacter();
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (!B || !PC || !Character || B->IsBusy() || !B->GetPlacedItem() || Now - LastStartRequest < 1.f)
	{
		return;
	}
	FText Reason;
	const bool bCan = bRepair ? B->CanStartRepairStep(Character, Reason) : B->CanStartStep(Character, Reason);
	if (!bCan)
	{
		if (!Reason.IsEmpty() && HintTime <= 0.f)
		{
			HintText->SetText(Reason);
			HintText->SetColorAndOpacity(Col::Bad);
		}
		return;
	}
	LastStartRequest = Now;
	PC->RequestWorkStart(B, bRepair);
}

void UWEWorkViewWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Anim.Tick(LeftCard, InDeltaTime);
	Anim.Tick(RightCard, InDeltaTime);
	AWEWorkbench* B = Bench.Get();
	if (!B || !Rig)
	{
		return;
	}
	HintTime -= InDeltaTime;
	ToolRefreshTimer -= InDeltaTime;
	if (ToolRefreshTimer <= 0.f)
	{
		ToolRefreshTimer = 0.5f;
		RefreshTools();
	}
	// Mode no longer possible (e.g. repaired, or fully disassembled into a repairable state): switch.
	if (!B->IsBusy())
	{
		if (bRepair && !CanRepair() && CanDisassemble())
		{
			SetRepairMode(false);
		}
		else if (!bRepair && !CanDisassemble() && CanRepair())
		{
			SetRepairMode(true);
		}
	}
	TryStartWork();

	// Holding on a task: when the time is reached, the server gets the request.
	const bool bMine = B->IsManualWork() && B->GetWorker() == GetCharacter() && B->IsRepairJob() == bRepair;
	const uint32 Mask = bMine ? B->GetTaskMask() : 0;
	if (PendingTask != INDEX_NONE)
	{
		PendingTime += InDeltaTime;
		if ((Mask & (1u << PendingTask)) != 0 || PendingTime > 1.5f || !bMine)
		{
			PendingTask = INDEX_NONE;
		}
	}
	if (HoldTask != INDEX_NONE)
	{
		const FWEWorkPlan& Plan = Rig->GetPlan();
		if (!bMine || !Plan.IsTaskAvailable(HoldTask, Mask) || PendingTask != INDEX_NONE)
		{
			HoldTask = INDEX_NONE;
		}
		else
		{
			HoldTime += InDeltaTime;
			const float Needed = FMath::Max(0.15f, Plan.GetTaskSeconds(HoldTask, B->GetActiveStepDuration()));
			Rig->SetActive(HoldTask, FMath::Clamp(HoldTime / Needed, 0.f, 1.f));
			if (HoldTime >= Needed)
			{
				if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
				{
					PC->RequestWorkTask(B, HoldTask, Plan.Tasks[HoldTask].Tool);
				}
				PendingTask = HoldTask;
				PendingTime = 0.f;
				HoldTask = INDEX_NONE;
			}
		}
	}
	if (HoldTask == INDEX_NONE)
	{
		Rig->SetActive(PendingTask, PendingTask != INDEX_NONE ? 1.f : 0.f);
	}
	UpdateMarkers();
	RefreshInfo();

	// The tool follows the mouse.
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(CursorIcon->Slot))
	{
		S->SetPosition(MousePosition());
	}
}

void UWEWorkViewWidget::UpdateMarkers()
{
	AWEWorkbench* B = Bench.Get();
	APlayerController* PC = GetOwningPlayer();
	const FWEWorkPlan& Plan = Rig->GetPlan();
	const bool bMine = B && B->IsManualWork() && B->GetWorker() == GetCharacter() && B->IsRepairJob() == bRepair;
	const uint32 Mask = bMine ? B->GetTaskMask() : 0;
	const float Scale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), 0.01f);
	const int32 Hover = FindTaskAt(MousePosition());
	Rig->SetHover(Hover);
	const float Now = GetWorld()->GetTimeSeconds();
	for (int32 i = 0; i < Markers.Num(); ++i)
	{
		const bool bTask = Plan.Tasks.IsValidIndex(i);
		FVector2D Screen;
		if (!bTask || (Mask & (1u << i)) != 0 || !PC || !PC->ProjectWorldLocationToScreen(Rig->GetTaskLocation(i), Screen, true))
		{
			Markers[i]->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		const FWEWorkTask& T = Plan.Tasks[i];
		const bool bAvailable = Plan.IsTaskAvailable(i, Mask);
		const bool bToolOk = Tools.IsValidIndex(SelectedTool) && Tools[SelectedTool] == T.Tool;
		Markers[i]->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Markers[i]->Slot))
		{
			S->SetPosition(Screen / Scale);
		}
		if (UTexture2D* Tex = IconTexture(WEWorkPlan::GetTaskIcon(T.Kind)))
		{
			MarkerIcons[i]->SetBrushFromTexture(Tex);
			MarkerIcons[i]->SetDesiredSizeOverride(FVector2D(MarkerSize - 16.f));
		}
		// Amber: do this now with this tool. White: available, other tool. Grey: later.
		FLinearColor Ring = !bAvailable ? Col::WithAlpha(Col::TextMuted, 0.5f) : bToolOk ? Col::Accent : Col::Text;
		if (i == Hover && bAvailable)
		{
			Ring = FMath::Lerp(Ring, FLinearColor::White, 0.35f + 0.25f * FMath::Sin(Now * 8.f));
		}
		FSlateBrush Brush = WEUI::Brush(Col::WithAlpha(Col::PanelDark, bAvailable ? 0.8f : 0.45f), Ring, i == Hover ? 3.f : 2.f, MarkerSize * 0.5f);
		MarkerRings[i]->SetBrush(Brush);
		MarkerIcons[i]->SetColorAndOpacity(bAvailable ? Col::Text : Col::TextMuted);
		const float Progress = i == HoldTask ? HoldTime / FMath::Max(0.15f, Plan.GetTaskSeconds(i, B ? B->GetActiveStepDuration() : 1.f)) : (i == PendingTask ? 1.f : 0.f);
		MarkerBars[i]->SetPercent(FMath::Clamp(Progress, 0.f, 1.f));
		MarkerBars[i]->SetVisibility(Progress > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UWEWorkViewWidget::RefreshInfo()
{
	AWEWorkbench* B = Bench.Get();
	const AWEPickupItem* Item = B ? B->GetPlacedItem() : nullptr;
	const FWEWorkPlan& Plan = Rig->GetPlan();
	const bool bMine = B && B->IsManualWork() && B->GetWorker() == GetCharacter() && B->IsRepairJob() == bRepair;
	const uint32 Mask = bMine ? B->GetTaskMask() : 0;

	if (!Item || !Item->GetItem().Definition)
	{
		CenterText->SetText(LOCTEXT("Empty", "Die Werkbank ist leer – die Teile liegen im Werkbanklager."));
		StepTitle->SetText(LOCTEXT("Done", "Fertig"));
		StepBar->SetPercent(1.f);
		StepCount->SetText(FText::GetEmpty());
		DeviceTitle->SetText(LOCTEXT("NoDevice", "Kein Gerät"));
		DeviceInfo->SetText(FText::GetEmpty());
		StepList->ClearChildren();
		LastInfoSignature.Reset();
		return;
	}
	CenterText->SetText(FText::GetEmpty());
	const FWEItemInstance& Instance = Item->GetItem();
	const UWEItemDefinition* Def = Instance.Definition;
	DeviceTitle->SetText(Instance.GetDisplayName());
	const FText Status = AWEWorkbench::GetRepairStatusText(Instance);
	DeviceInfo->SetText(Status.IsEmpty()
		? FText::Format(LOCTEXT("Info", "{0} · Qualität {1} %"), WEItems::GetConditionText(Instance.Condition), FText::AsNumber(Instance.Quality))
		: FText::Format(LOCTEXT("InfoRepair", "{0} · Qualität {1} % · {2}"), WEItems::GetConditionText(Instance.Condition), FText::AsNumber(Instance.Quality), Status));
	SetButtonKind(ModeDisassemble, !bRepair ? EButton::TabActive : EButton::Tab, ModeDisassembleLabel);
	SetButtonKind(ModeRepair, bRepair ? EButton::TabActive : EButton::Tab, ModeRepairLabel);
	ModeDisassemble->SetIsEnabled(CanDisassemble());
	ModeRepair->SetIsEnabled(CanRepair());

	// Current step.
	FText Title;
	FText Yield;
	if (bRepair)
	{
		FText Reason;
		const FWERepairStepInfo Info = B->GetNextRepairStep(Reason);
		Title = Info.Kind != EWERepairStepKind::None ? B->GetNextRepairPrompt() : Reason;
		if (Info.Material)
		{
			Yield = FText::Format(LOCTEXT("Uses", "Verbraucht: {0}"), Info.Material->DisplayName);
		}
	}
	else
	{
		Title = B->GetNextStepPrompt();
		if (const FWEDisassemblyStep* Step = B->GetNextStep())
		{
			TArray<FString> Names;
			for (const FWEDisassemblyOutput& Out : Step->Outputs)
			{
				if (Out.Item)
				{
					Names.Add(Out.Item->DisplayName.ToString());
				}
			}
			if (Names.Num() > 0)
			{
				Yield = FText::Format(LOCTEXT("Yields", "Ergibt: {0}"), FText::FromString(FString::Join(Names, TEXT(", "))));
			}
		}
	}
	StepTitle->SetText(Title);
	YieldText->SetText(Yield);
	const int32 Done = FMath::CountBits(Mask);
	const bool bTimed = B->IsBusy() && !B->IsManualWork();
	StepBar->SetPercent(bTimed ? B->GetProgress() : (Plan.Tasks.Num() > 0 ? Done / float(Plan.Tasks.Num()) : 0.f));
	StepCount->SetText(bTimed ? LOCTEXT("Auto", "automatisch …") : FText::Format(LOCTEXT("Count", "{0} / {1}"), FText::AsNumber(Done), FText::AsNumber(Plan.Tasks.Num())));
	if (HintTime <= 0.f)
	{
		FText Line;
		if (B->IsBusy() && B->GetWorker() != GetCharacter())
		{
			Line = LOCTEXT("Other", "Jemand anderes arbeitet gerade an diesem Gerät.");
		}
		else if (bTimed)
		{
			Line = LOCTEXT("TimedLine", "Der Schritt läuft automatisch – du kannst zuschauen oder gehen.");
		}
		else
		{
			// What to do next: the first available task.
			FText Ignored;
			for (int32 i = 0; i < Plan.Tasks.Num(); ++i)
			{
				if (Plan.IsTaskAvailable(i, Mask))
				{
					const FWEWorkTask& T = Plan.Tasks[i];
					Line = FText::Format(LOCTEXT("Next", "{0}: Werkzeug „{1}“, dann auf die Markierung gedrückt halten."),
						WEWorkPlan::GetTaskText(T.Kind, Plan.bRepair && B->GetNextRepairStep(Ignored).Kind == EWERepairStepKind::Install), WEItems::GetToolText(T.Tool));
					break;
				}
			}
		}
		HintText->SetText(Line);
		HintText->SetColorAndOpacity(Col::Text);
	}

	// Step list (rebuilt only when it changes).
	FString Signature = FString::Printf(TEXT("%d|%d|%d|%s"), bRepair ? 1 : 0, B->GetNextStepIndex(), Instance.Repair.Faults, *Status.ToString());
	if (Signature == LastInfoSignature)
	{
		return;
	}
	LastInfoSignature = Signature;
	StepList->ClearChildren();
	auto Line = [this](const FText& Label, int32 State) // 0 done, 1 current, 2 later
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		const FName IconName = State == 0 ? FName(TEXT("Check")) : State == 1 ? FName(TEXT("Play")) : FName(TEXT("Minus"));
		AddH(Row, Icon(WidgetTree, IconName, 14.f, State == 0 ? Col::Good : State == 1 ? Col::Accent : Col::TextMuted), FMargin(0.f, 0.f, 8.f, 0.f));
		AddH(Row, Text(WidgetTree, Label, 13, State == 0 ? Col::TextMuted : State == 1 ? Col::Text : Col::TextDim, State == 1 ? EFont::Strong : EFont::Body, 340.f), FMargin(), true);
		AddV(StepList, Row, FMargin(0.f, 0.f, 0.f, 4.f));
	};
	if (!bRepair && Def->Disassembly)
	{
		for (int32 i = 0; i < Def->Disassembly->Steps.Num(); ++i)
		{
			const int32 Next = B->GetNextStepIndex();
			Line(Def->Disassembly->Steps[i].ActionText, i < Next ? 0 : i == Next ? 1 : 2);
		}
	}
	else if (bRepair)
	{
		const bool bDiagnosed = Instance.Repair.HasFlag(FWERepairState::Diagnosed);
		Line(LOCTEXT("RDiag", "Fehler suchen"), bDiagnosed ? 0 : 1);
		Line(LOCTEXT("RParts", "Defekte Teile tauschen"), !bDiagnosed ? 2 : (Instance.IsBroken() ? 1 : 0));
		Line(LOCTEXT("RTest", "Funktionstest"), Instance.IsBroken() ? 2 : 0);
		Line(LOCTEXT("RClean", "Reinigen und aufbereiten (optional)"), Instance.Repair.HasFlag(FWERepairState::Refurbished) ? 0 : Instance.IsBroken() ? 2 : 1);
	}
}

int32 UWEWorkViewWidget::DevFindAvailableTask() const
{
	const AWEWorkbench* B = Bench.Get();
	if (!Rig || !B || !B->IsManualWork())
	{
		return INDEX_NONE;
	}
	for (int32 i = 0; i < Rig->GetPlan().Tasks.Num(); ++i)
	{
		if (Rig->GetPlan().IsTaskAvailable(i, B->GetTaskMask()))
		{
			return i;
		}
	}
	return INDEX_NONE;
}

bool UWEWorkViewWidget::DevWorkNext()
{
	const int32 Task = DevFindAvailableTask();
	if (Task == INDEX_NONE || !DevIsIdle())
	{
		TryStartWork();
		return false;
	}
	DevSelectTool(Rig->GetPlan().Tasks[Task].Tool);
	DevHoldTask(Task);
	return HoldTask != INDEX_NONE;
}

FString UWEWorkViewWidget::DescribeState() const
{
	const AWEWorkbench* B = Bench.Get();
	return FString::Printf(TEXT("mode=%s tasks=%d mask=%u manual=%d busy=%d tool=%d hold=%d"), bRepair ? TEXT("repair") : TEXT("disassemble"),
		Rig ? Rig->GetPlan().Tasks.Num() : -1, B ? B->GetTaskMask() : 0, B && B->IsManualWork() ? 1 : 0, B && B->IsBusy() ? 1 : 0,
		Tools.IsValidIndex(SelectedTool) ? static_cast<int32>(Tools[SelectedTool]) : -1, HoldTask);
}

#undef LOCTEXT_NAMESPACE
