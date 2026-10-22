#include "UI/WEMenuWidgets.h"
#include "PipelineStateCache.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Player/WEInputConfig.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Camera/CameraActor.h"
#include "Framework/WEMenuGameMode.h"
#include "Player/WEAvatarPreview.h"
#include "Framework/WEGameMode.h"
#include "Framework/WEPlayerController.h"
#include "Framework/WEPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Online/WESessionSubsystem.h"
#include "Progression/WETutorialComponent.h"
#include "Save/WESaveSystem.h"
#include "SocketSubsystem.h"
#include "UI/WEGraphicsSettings.h"
#include "WEMoney.h"

#define LOCTEXT_NAMESPACE "WEMenu"

using namespace WEUI;

namespace WEMenuStyle
{
	UScrollBox* List(UWidgetTree* Tree, UVerticalBox* Box, float Height)
	{
		UBorder* Inset = WEUI::Box(Tree, Col::PanelDark, FMargin(6.f), Col::Line, 2.f);
		UScrollBox* Scroll = Tree->ConstructWidget<UScrollBox>();
		Inset->SetContent(Scroll);
		AddV(Box, Sized(Tree, Inset, 0.f, Height), FMargin(0.f, 8.f, 0.f, 0.f));
		return Scroll;
	}

	UEditableTextBox* TextField(UWidgetTree* Tree, const FText& Value, const FText& Hint)
	{
		UEditableTextBox* Field = Tree->ConstructWidget<UEditableTextBox>();
		FEditableTextBoxStyle Style = FEditableTextBoxStyle::GetDefault();
		Style.SetBackgroundImageNormal(Brush(Col::PanelDark, Col::LineStrong, 1.f, 2.f));
		Style.SetBackgroundImageHovered(Brush(Col::PanelDark, Col::Accent, 1.f, 2.f));
		Style.SetBackgroundImageFocused(Brush(Col::PanelDark, Col::Accent, 1.f, 2.f));
		Style.SetBackgroundImageReadOnly(Brush(Col::WithAlpha(Col::PanelDark, 0.6f), Col::Line, 1.f, 2.f));
		Style.SetForegroundColor(FSlateColor(Col::Text));
		Style.SetFont(Font(EFont::Body, 14));
		Style.SetPadding(FMargin(10.f, 7.f));
		Field->WidgetStyle = Style;
		Field->SetText(Value);
		Field->SetHintText(Hint);
		return Field;
	}

	UCheckBox* Check(UWidgetTree* Tree, UVerticalBox* Box, const FText& Label, bool bChecked, float Top = 8.f)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>();
		UCheckBox* C = Tree->ConstructWidget<UCheckBox>();
		C->SetIsChecked(bChecked);
		AddH(Row, C, FMargin(0.f, 0.f, 8.f, 0.f));
		AddH(Row, Text(Tree, Label, 13, Col::Text, EFont::Body, 460.f), FMargin(), true);
		AddV(Box, Row, FMargin(0.f, Top, 0.f, 0.f));
		return C;
	}
}

// =================================================================================================================
// Rows
// =================================================================================================================

void UWEMenuRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	UBorder* Frame = Box(WidgetTree, Col::Raised, FMargin(10.f, 7.f), Col::Line, 2.f);
	WidgetTree->RootWidget = Frame;
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Frame->SetContent(Row);
	RowIcon = Icon(WidgetTree, TEXT("Folder"), 22.f, Col::TextDim);
	AddH(Row, RowIcon, FMargin(0.f, 0.f, 10.f, 0.f));
	UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>();
	TitleText = Text(WidgetTree, FText::GetEmpty(), 14, Col::Text, EFont::Strong, 360.f);
	DetailText = Text(WidgetTree, FText::GetEmpty(), 11, Col::TextDim, EFont::Body, 360.f);
	AddV(Texts, TitleText);
	AddV(Texts, DetailText);
	AddH(Row, Texts, FMargin(), true);
	Button = WEUI::Button(WidgetTree, FText::FromString(TEXT("·")), EButton::Primary, NAME_None, 12, &ButtonText);
	Button->OnClicked.AddDynamic(this, &UWEMenuRowWidget::HandleClicked);
	AddH(Row, Button, FMargin(8.f, 0.f, 0.f, 0.f));
}

void UWEMenuRowWidget::Configure(const FText& Title, const FText& Detail, const FText& ButtonLabel, bool bButtonEnabled, TFunction<void()> InOnClicked,
	FName IconName, bool bPrimary)
{
	TitleText->SetText(Title);
	DetailText->SetText(Detail);
	DetailText->SetVisibility(Detail.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	ButtonText->SetText(Upper(ButtonLabel));
	Button->SetVisibility(ButtonLabel.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	Button->SetIsEnabled(bButtonEnabled);
	SetButtonKind(Button, bPrimary ? EButton::Primary : EButton::Danger, ButtonText);
	if (UTexture2D* Tex = IconTexture(IconName))
	{
		RowIcon->SetBrushFromTexture(Tex);
		RowIcon->SetDesiredSizeOverride(FVector2D(22.f, 22.f));
	}
	RowIcon->SetVisibility(IconName.IsNone() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	OnClicked = MoveTemp(InOnClicked);
}

void UWEMenuRowWidget::HandleClicked()
{
	if (OnClicked)
	{
		OnClicked();
	}
}

void UWESettingRow::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	UBorder* Frame = Box(WidgetTree, Col::Raised, FMargin(10.f, 5.f), Col::Line, 2.f);
	WidgetTree->RootWidget = Frame;
	UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>();
	Frame->SetContent(Lines);
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddV(Lines, Row);
	RowIcon = Icon(WidgetTree, TEXT("Gear"), 18.f, Col::TextDim);
	AddH(Row, RowIcon, FMargin(0.f, 0.f, 10.f, 0.f));
	LabelText = Text(WidgetTree, FText::GetEmpty(), 13, Col::Text, EFont::Strong);
	// Phase 19: a long label in a narrow card ends in "…" instead of running under the arrows.
	LabelText->SetClipping(EWidgetClipping::ClipToBounds);
	LabelText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	AddH(Row, LabelText, FMargin(0.f, 0.f, 4.f, 0.f), true);
	// Phase 20: optional slider on its own line under the label (hidden until EnableSlider).
	Slider = WidgetTree->ConstructWidget<USlider>();
	FSliderStyle SliderStyle = Slider->GetWidgetStyle();
	SliderStyle.SetBarThickness(4.f);
	Slider->SetWidgetStyle(SliderStyle);
	Slider->SetSliderBarColor(Col::LineStrong);
	Slider->SetSliderHandleColor(Col::Accent);
	Slider->OnValueChanged.AddDynamic(this, &UWESettingRow::HandleSlider);
	Slider->SetVisibility(ESlateVisibility::Collapsed);
	AddV(Lines, Slider, FMargin(28.f, 2.f, 4.f, 2.f));
	UButton* Left = Button(WidgetTree, FText::GetEmpty(), EButton::Icon, TEXT("ChevronLeft"), 12);
	Left->OnClicked.AddDynamic(this, &UWESettingRow::HandleLeft);
	AddH(Row, Left);
	ValueText = Text(WidgetTree, FText::GetEmpty(), 13, Col::Accent, EFont::Head);
	ValueText->SetJustification(ETextJustify::Center);
	ValueBox = Cast<USizeBox>(Sized(WidgetTree, ValueText, 170.f));
	AddH(Row, ValueBox);
	UButton* Right = Button(WidgetTree, FText::GetEmpty(), EButton::Icon, TEXT("ChevronRight"), 12);
	Right->OnClicked.AddDynamic(this, &UWESettingRow::HandleRight);
	AddH(Row, Right);
}

void UWESettingRow::Setup(UWESettingsPanel* InOwner, int32 InIndex, const FText& Label, FName IconName)
{
	Owner = InOwner;
	Index = InIndex;
	LabelText->SetText(Label);
	if (UTexture2D* Tex = IconTexture(IconName))
	{
		RowIcon->SetBrushFromTexture(Tex);
		RowIcon->SetDesiredSizeOverride(FVector2D(18.f, 18.f));
	}
}

void UWESettingRow::SetupCallback(TFunction<void(int32 Direction)> InOnStep, const FText& Label, FName IconName)
{
	OnStep = MoveTemp(InOnStep);
	LabelText->SetText(Label);
	if (UTexture2D* Tex = IconTexture(IconName))
	{
		RowIcon->SetBrushFromTexture(Tex);
		RowIcon->SetDesiredSizeOverride(FVector2D(18.f, 18.f));
	}
}

void UWESettingRow::SetValue(const FText& Value)
{
	ValueText->SetText(Upper(Value));
}

void UWESettingRow::EnableSlider(float Min, float Max, float StepSize)
{
	bSlider = true;
	Slider->SetMinValue(Min);
	Slider->SetMaxValue(Max);
	Slider->SetStepSize(StepSize);
	Slider->SetVisibility(ESlateVisibility::Visible);
}

void UWESettingRow::SetSliderValue(float Value)
{
	if (bSlider)
	{
		bSettingSlider = true;
		Slider->SetValue(Value);
		bSettingSlider = false;
	}
}

void UWESettingRow::HandleSlider(float Value)
{
	if (!bSettingSlider && Owner)
	{
		Owner->SetFromSlider(Index, Value);
	}
}

void UWESettingRow::HandleLeft()
{
	if (OnStep)
	{
		OnStep(-1);
	}
	else if (Owner)
	{
		Owner->Step(Index, -1);
	}
}

void UWESettingRow::HandleRight()
{
	if (OnStep)
	{
		OnStep(1);
	}
	else if (Owner)
	{
		Owner->Step(Index, 1);
	}
}

// =================================================================================================================
// Key row (Phase 20)
// =================================================================================================================

void UWEKeyBindRow::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	UBorder* Frame = Box(WidgetTree, Col::Raised, FMargin(10.f, 3.f), Col::Line, 1.f);
	WidgetTree->RootWidget = Frame;
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Frame->SetContent(Row);
	LabelText = Text(WidgetTree, FText::GetEmpty(), 13, Col::Text, EFont::Strong);
	AddH(Row, LabelText, FMargin(0.f, 0.f, 6.f, 0.f), true);
	KeyButton = Button(WidgetTree, FText::GetEmpty(), EButton::Secondary, NAME_None, 12);
	KeyButton->OnClicked.AddDynamic(this, &UWEKeyBindRow::HandleClicked);
	KeyText = Text(WidgetTree, FText::GetEmpty(), 13, Col::Accent, EFont::Head);
	KeyText->SetJustification(ETextJustify::Center);
	KeyButton->SetContent(KeyText);
	AddH(Row, Sized(WidgetTree, KeyButton, 170.f));
}

void UWEKeyBindRow::Setup(UWESettingsPanel* InOwner, int32 InEntry, const FText& Label)
{
	Owner = InOwner;
	Entry = InEntry;
	LabelText->SetText(Label);
}

void UWEKeyBindRow::SetKeyText(const FText& Key, bool bWaiting)
{
	KeyText->SetText(Upper(Key));
	KeyText->SetColorAndOpacity(FSlateColor(bWaiting ? Col::Good : Col::Accent));
}

void UWEKeyBindRow::HandleClicked()
{
	if (Owner)
	{
		Owner->BeginKeyCapture(Entry);
	}
}

// =================================================================================================================
// Settings panel
// =================================================================================================================

void UWESettingsPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	UVerticalBox* Box2 = WidgetTree->ConstructWidget<UVerticalBox>();
	WidgetTree->RootWidget = Box2;
	struct FRowDef { const TCHAR* Section; FText Label; FName Icon; };
	const FRowDef Defs[Count] = {
		{ TEXT("Grafik"), LOCTEXT("SQuality", "Grafikqualität"), TEXT("Desktop") },
		{ nullptr, LOCTEXT("SRenderScale", "Renderauflösung"), TEXT("Chart") },
		{ nullptr, LOCTEXT("SRes", "Auflösung"), TEXT("Window") },
		{ nullptr, LOCTEXT("SMode", "Anzeigemodus"), TEXT("Window") },
		{ nullptr, LOCTEXT("SVSync", "VSync"), TEXT("Clock") },
		{ nullptr, LOCTEXT("SFps", "Bildrate begrenzen"), TEXT("Chart") },
		{ nullptr, LOCTEXT("SBlur", "Bewegungsunschärfe"), TEXT("Play") },
		{ TEXT("Oberfläche & Steuerung"), LOCTEXT("SUi", "UI-Skalierung"), TEXT("Search") },
		{ nullptr, LOCTEXT("SMouse", "Mausempfindlichkeit"), TEXT("Hand") },
		{ nullptr, LOCTEXT("SInvert", "Maus Y invertieren"), TEXT("ChevronDown") },
		{ nullptr, LOCTEXT("SFov", "Sichtfeld"), TEXT("Search") },
		{ TEXT("Audio"), LOCTEXT("SVolume", "Gesamtlautstärke"), TEXT("Bell") },
	};
	for (int32 i = 0; i < Count; ++i)
	{
		if (Defs[i].Section)
		{
			AddV(Box2, SectionHeader(WidgetTree, FText::FromString(Defs[i].Section), NAME_None, nullptr, 12), FMargin(0.f, i == 0 ? 0.f : 10.f, 0.f, 6.f));
		}
		UWESettingRow* Row = CreateWidget<UWESettingRow>(GetOwningPlayer(), UWESettingRow::StaticClass());
		Row->Setup(this, i, Defs[i].Label, Defs[i].Icon);
		AddV(Box2, Row, FMargin(0.f, 0.f, 0.f, 3.f));
		Rows.Add(Row);
	}
	// Phase 20: sliders for the continuous values.
	Rows[MotionBlur]->EnableSlider(0.f, 1.f, 0.05f);
	Rows[RenderScale]->EnableSlider(50.f, 100.f, 1.f);
	Rows[UIScale]->EnableSlider(0.8f, 1.3f, 0.05f);
	Rows[Mouse]->EnableSlider(0.2f, 3.f, 0.05f);
	Rows[Fov]->EnableSlider(70.f, 110.f, 1.f);
	Rows[Volume]->EnableSlider(0.f, 1.f, 0.05f);
	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
	StatusText = Text(WidgetTree, FText::GetEmpty(), 12, Col::Good, EFont::Strong);
	AddH(Buttons, StatusText, FMargin(), true);
	UButton* Defaults = Button(WidgetTree, LOCTEXT("Defaults", "Standard"), EButton::Secondary, NAME_None, 12);
	Defaults->OnClicked.AddDynamic(this, &UWESettingsPanel::HandleDefaults);
	AddH(Buttons, Defaults, FMargin(0.f, 0.f, 6.f, 0.f));
	UButton* Apply = Button(WidgetTree, LOCTEXT("Apply", "Übernehmen"), EButton::Primary, TEXT("Check"), 12);
	Apply->OnClicked.AddDynamic(this, &UWESettingsPanel::HandleApply);
	AddH(Buttons, Apply);
	AddV(Box2, Buttons, FMargin(0.f, 12.f, 0.f, 0.f));

	// Phase 20: free key assignment (saved at once).
	AddV(Box2, SectionHeader(WidgetTree, LOCTEXT("KeysSection", "Tastenbelegung"), NAME_None, nullptr, 12), FMargin(0.f, 18.f, 0.f, 4.f));
	AddV(Box2, Text(WidgetTree, LOCTEXT("KeysHint", "Taste anklicken, dann die neue Taste drücken. [Esc] bricht ab. Ist die Taste schon belegt, werden beide getauscht."),
		11, Col::TextDim, EFont::Body, 480.f), FMargin(0.f, 0.f, 0.f, 6.f));
	const TArray<WEKeys::FEntry>& Entries = WEKeys::GetEntries();
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		UWEKeyBindRow* KeyRow = CreateWidget<UWEKeyBindRow>(GetOwningPlayer(), UWEKeyBindRow::StaticClass());
		KeyRow->Setup(this, i, Entries[i].Label);
		AddV(Box2, KeyRow, FMargin(0.f, 0.f, 0.f, 2.f));
		KeyRows.Add(KeyRow);
	}
	UHorizontalBox* KeyButtons = WidgetTree->ConstructWidget<UHorizontalBox>();
	KeyStatusText = Text(WidgetTree, FText::GetEmpty(), 12, Col::Good, EFont::Strong, 330.f);
	AddH(KeyButtons, KeyStatusText, FMargin(), true);
	UButton* ResetKeys = Button(WidgetTree, LOCTEXT("ResetKeys", "Tasten zurücksetzen"), EButton::Secondary, NAME_None, 12);
	ResetKeys->OnClicked.AddDynamic(this, &UWESettingsPanel::HandleResetKeys);
	AddH(KeyButtons, ResetKeys);
	AddV(Box2, KeyButtons, FMargin(0.f, 8.f, 0.f, 0.f));
	SetIsFocusable(true);
	Reload();
}

void UWESettingsPanel::RefreshKeys()
{
	const TArray<WEKeys::FEntry>& Entries = WEKeys::GetEntries();
	for (int32 i = 0; i < KeyRows.Num() && i < Entries.Num(); ++i)
	{
		const bool bWaiting = i == CaptureEntry;
		KeyRows[i]->SetKeyText(bWaiting ? LOCTEXT("PressKey", "Taste drücken …") : WEKeys::GetKey(Entries[i].Id).GetDisplayName(), bWaiting);
	}
}

void UWESettingsPanel::BeginKeyCapture(int32 Entry)
{
	CaptureEntry = WEKeys::GetEntries().IsValidIndex(Entry) ? Entry : INDEX_NONE;
	RefreshKeys();
	if (CaptureEntry != INDEX_NONE)
	{
		SetKeyboardFocus();
		KeyStatusText->SetText(FText::GetEmpty());
	}
}

bool UWESettingsPanel::CaptureKey(const FKey& Key)
{
	if (CaptureEntry == INDEX_NONE)
	{
		return false;
	}
	const WEKeys::FEntry& Entry = WEKeys::GetEntries()[CaptureEntry];
	CaptureEntry = INDEX_NONE;
	FName Swapped;
	const bool bOk = Key != EKeys::Escape && WEKeys::SetKey(Entry.Id, Key, Swapped);
	if (Key == EKeys::Escape)
	{
		KeyStatusText->SetText(LOCTEXT("KeyCancelled", "Abgebrochen"));
		KeyStatusText->SetColorAndOpacity(FSlateColor(Col::TextDim));
	}
	else if (!bOk)
	{
		KeyStatusText->SetText(LOCTEXT("KeyRefused", "Diese Taste geht nicht"));
		KeyStatusText->SetColorAndOpacity(FSlateColor(Col::Bad));
	}
	else
	{
		KeyStatusText->SetText(Swapped.IsNone()
			? FText::Format(LOCTEXT("KeySet", "{0}: {1} – gespeichert"), Entry.Label, Key.GetDisplayName())
			: FText::Format(LOCTEXT("KeySwapped", "{0}: {1} – getauscht mit {2}"), Entry.Label, Key.GetDisplayName(), WEKeys::GetLabel(Swapped)));
		KeyStatusText->SetColorAndOpacity(FSlateColor(Col::Good));
		Play(this, ESound::Confirm, 0.5f);
	}
	RefreshKeys();
	return bOk;
}

FReply UWESettingsPanel::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (CaptureEntry != INDEX_NONE)
	{
		CaptureKey(InKeyEvent.GetKey());
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UWESettingsPanel::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Right, middle and side mouse buttons can be keys too (the left one clicks the buttons).
	if (CaptureEntry != INDEX_NONE && InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		CaptureKey(InMouseEvent.GetEffectingButton());
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UWESettingsPanel::HandleResetKeys()
{
	CaptureEntry = INDEX_NONE;
	WEKeys::ResetAll();
	KeyStatusText->SetText(LOCTEXT("KeysReset", "Standardtasten wiederhergestellt"));
	KeyStatusText->SetColorAndOpacity(FSlateColor(Col::Good));
	RefreshKeys();
}

void UWESettingsPanel::SetFromSlider(int32 Index, float Value)
{
	switch (Index)
	{
	case MotionBlur: Values.MotionBlur = FMath::Clamp(Value, 0.f, 1.f); break;
	case RenderScale: Values.RenderScale = FMath::Clamp(FMath::RoundToFloat(Value), 50.f, 100.f); break;
	case UIScale:    Values.UIScale = FMath::Clamp(Value, 0.8f, 1.3f); break;
	case Mouse:      Values.MouseSensitivity = FMath::Clamp(Value, 0.2f, 3.f); break;
	case Fov:        Values.FieldOfView = FMath::Clamp(FMath::RoundToFloat(Value), 70.f, 110.f); break;
	case Volume:     Values.MasterVolume = FMath::Clamp(Value, 0.f, 1.f); break;
	default: return;
	}
	StatusText->SetText(LOCTEXT("Unsaved", "Noch nicht übernommen"));
	StatusText->SetColorAndOpacity(FSlateColor(Col::Accent));
	Refresh();
}

void UWESettingsPanel::Reload()
{
	Values = WESettings::Load();
	QualityLevel = WEGraphics::GetQuality();
	Resolutions = WESettings::GetResolutions();
	Refresh();
	if (StatusText)
	{
		StatusText->SetText(FText::GetEmpty());
	}
}

void UWESettingsPanel::Refresh()
{
	if (Rows.Num() < Count)
	{
		return;
	}
	auto Pct = [](float V) { return FText::Format(LOCTEXT("PctFmt", "{0} %"), FText::AsNumber(FMath::RoundToInt(V * 100.f))); };
	auto OnOff = [](bool b) { return b ? LOCTEXT("On", "Ein") : LOCTEXT("Off", "Aus"); };
	Rows[ERow::Quality]->SetValue(WEGraphics::GetQualityName(QualityLevel));
	Rows[Resolution]->SetValue(FText::FromString(FString::Printf(TEXT("%d × %d"), Values.Resolution.X, Values.Resolution.Y)));
	Rows[WindowMode]->SetValue(WESettings::GetWindowModeName(Values.WindowMode));
	Rows[VSync]->SetValue(OnOff(Values.bVSync));
	Rows[FrameLimit]->SetValue(WESettings::GetFrameLimitName(Values.FrameLimit));
	Rows[UIScale]->SetValue(Pct(Values.UIScale));
	{ FNumberFormattingOptions One; One.MinimumFractionalDigits = 1; One.MaximumFractionalDigits = 1; Rows[Mouse]->SetValue(FText::Format(LOCTEXT("MouseValue", "{0}×"), FText::AsNumber(Values.MouseSensitivity, &One))); }
	Rows[InvertY]->SetValue(OnOff(Values.bInvertY));
	Rows[Fov]->SetValue(FText::Format(LOCTEXT("Deg", "{0}°"), FText::AsNumber(FMath::RoundToInt(Values.FieldOfView))));
	Rows[Volume]->SetValue(Pct(Values.MasterVolume));
	Rows[MotionBlur]->SetValue(Values.MotionBlur <= 0.f ? OnOff(false) : Pct(Values.MotionBlur));
	Rows[MotionBlur]->SetSliderValue(Values.MotionBlur);
	Rows[RenderScale]->SetValue(FText::Format(LOCTEXT("RenderScaleValue", "{0} %"), FText::AsNumber(FMath::RoundToInt(Values.RenderScale))));
	Rows[RenderScale]->SetSliderValue(Values.RenderScale);
	Rows[UIScale]->SetSliderValue(Values.UIScale);
	Rows[Mouse]->SetSliderValue(Values.MouseSensitivity);
	Rows[Fov]->SetSliderValue(Values.FieldOfView);
	Rows[Volume]->SetSliderValue(Values.MasterVolume);
	RefreshKeys();
}

void UWESettingsPanel::Step(int32 Index, int32 Direction)
{
	static const float Limits[] = { 0.f, 30.f, 60.f, 120.f, 144.f, 240.f };
	switch (Index)
	{
	case ERow::Quality:
		QualityLevel = (QualityLevel + Direction + WEGraphics::NumLevels) % WEGraphics::NumLevels;
		break;
	case Resolution:
	{
		int32 Current = Resolutions.IndexOfByKey(Values.Resolution);
		Current = Current == INDEX_NONE ? Resolutions.Num() - 1 : (Current + Direction + Resolutions.Num()) % Resolutions.Num();
		Values.Resolution = Resolutions[Current];
		break;
	}
	case WindowMode:   Values.WindowMode = (Values.WindowMode + Direction + 3) % 3; break;
	case VSync:        Values.bVSync = !Values.bVSync; break;
	case FrameLimit:
	{
		int32 Current = 0;
		for (int32 i = 0; i < UE_ARRAY_COUNT(Limits); ++i)
		{
			Current = FMath::IsNearlyEqual(Limits[i], Values.FrameLimit) ? i : Current;
		}
		Values.FrameLimit = Limits[(Current + Direction + UE_ARRAY_COUNT(Limits)) % UE_ARRAY_COUNT(Limits)];
		break;
	}
	case UIScale:      Values.UIScale = FMath::Clamp(Values.UIScale + Direction * 0.1f, 0.8f, 1.3f); break;
	case Mouse:        Values.MouseSensitivity = FMath::Clamp(Values.MouseSensitivity + Direction * 0.1f, 0.2f, 3.f); break;
	case InvertY:      Values.bInvertY = !Values.bInvertY; break;
	case Fov:          Values.FieldOfView = FMath::Clamp(Values.FieldOfView + Direction * 5.f, 70.f, 110.f); break;
	case Volume:       Values.MasterVolume = FMath::Clamp(Values.MasterVolume + Direction * 0.1f, 0.f, 1.f); break;
	case MotionBlur:   Values.MotionBlur = FMath::Clamp(Values.MotionBlur + Direction * 0.1f, 0.f, 1.f); break;
	case RenderScale:  Values.RenderScale = FMath::Clamp(Values.RenderScale + Direction * 5.f, 50.f, 100.f); break;
	default: break;
	}
	Play(this, ESound::Toggle, 0.4f);
	StatusText->SetText(LOCTEXT("Unsaved", "Noch nicht übernommen"));
	StatusText->SetColorAndOpacity(FSlateColor(Col::Accent));
	Refresh();
}

void UWESettingsPanel::HandleApply()
{
	WEGraphics::SetQuality(QualityLevel);
	WESettings::Save(Values);
	StatusText->SetText(LOCTEXT("Applied", "Übernommen und gespeichert"));
	StatusText->SetColorAndOpacity(FSlateColor(Col::Good));
	Play(this, ESound::Confirm, 0.5f);
}

void UWESettingsPanel::HandleDefaults()
{
	const FIntPoint KeepResolution = Values.Resolution;
	Values = WESettings::FValues();
	Values.Resolution = KeepResolution;
	QualityLevel = 2;
	Refresh();
	StatusText->SetText(LOCTEXT("DefaultsSet", "Standardwerte – noch nicht übernommen"));
	StatusText->SetColorAndOpacity(FSlateColor(Col::Accent));
}

// =================================================================================================================
// Main menu
// =================================================================================================================

UButton* UWEMainMenuWidget::MakeMenuButton(UVerticalBox* Parent, const FText& Label, FName IconName, TObjectPtr<UTextBlock>* OutDetail)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = ButtonStyle(EButton::Ghost);
	Style.SetHovered(Brush(Col::WithAlpha(Col::Accent, 0.14f), FLinearColor::Transparent, 0.f, 0.f));
	Style.SetPressed(Brush(Col::WithAlpha(Col::Accent, 0.24f), FLinearColor::Transparent, 0.f, 0.f));
	Style.SetNormalPadding(FMargin(14.f, 10.f));
	Style.SetPressedPadding(FMargin(16.f, 11.f, 12.f, 9.f));
	Style.SetHoveredSound(FSlateSound());
	B->SetStyle(Style);
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddH(Row, Icon(WidgetTree, IconName, 26.f, Col::Accent), FMargin(0.f, 0.f, 16.f, 0.f));
	UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>();
	UTextBlock* T = Text(WidgetTree, Upper(Label), 24, Col::Text, EFont::Head);
	T->SetFont(Font(EFont::Head, 24, 80));
	AddV(Texts, T);
	if (OutDetail)
	{
		*OutDetail = Text(WidgetTree, FText::GetEmpty(), 11, Col::TextDim, EFont::Body, 360.f);
		AddV(Texts, *OutDetail);
	}
	AddH(Row, Texts, FMargin(), true);
	B->SetContent(Row);
	if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Row->Slot))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Left);
	}
	AddV(Parent, Sized(WidgetTree, B, 420.f), FMargin(0.f, 0.f, 0.f, 4.f), HAlign_Left);
	return B;
}

void UWEMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	const UWESessionSubsystem* Session = UWESessionSubsystem::Get(this);
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;

	// Left: dark band with the logo and the main entries; the scene stays visible on the right.
	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
	if (UOverlaySlot* S = Root->AddChildToOverlay(Columns))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Fill);
	}
	UBorder* Band = Box(WidgetTree, Col::Hex(0x0B0D10, 0.86f), FMargin(64.f, 52.f, 40.f, 40.f), FLinearColor::Transparent, 0.f);
	AddH(Columns, Sized(WidgetTree, Band, 560.f), FMargin(), false, VAlign_Fill);
	AddH(Columns, Sized(WidgetTree, Box(WidgetTree, Col::Accent, FMargin(0.f), FLinearColor::Transparent, 0.f), 3.f), FMargin(), false, VAlign_Fill);
	UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>();
	Band->SetContent(Left);
	LeftColumn = Left;
	AddV(Left, Hazard(WidgetTree, 7.f, 150.f));
	UTextBlock* Title1 = Text(WidgetTree, FText::FromString(TEXT("WORKSHOP")), 58, Col::Text, EFont::Black);
	Title1->SetFont(Font(EFont::Black, 58, 40));
	AddV(Left, Title1, FMargin(0.f, 16.f, 0.f, 0.f));
	UTextBlock* Title2 = Text(WidgetTree, FText::FromString(TEXT("EMPIRE")), 58, Col::Accent, EFont::Black);
	Title2->SetFont(Font(EFont::Black, 58, 40));
	AddV(Left, Title2, FMargin(0.f, -14.f, 0.f, 0.f));
	AddV(Left, Heading(WidgetTree, LOCTEXT("Tagline", "Werkstatt · Recycling · Industrie"), 13, Col::TextDim), FMargin(0.f, 2.f, 0.f, 40.f));

	ContinueButton = MakeMenuButton(Left, LOCTEXT("Continue", "Spiel fortsetzen"), TEXT("Play"), &ContinueDetail);
	ContinueButton->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleContinue);
	MakeMenuButton(Left, LOCTEXT("NewGame", "Neues Spiel"), TEXT("Plus"))->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleShowNew);
	MakeMenuButton(Left, LOCTEXT("Multiplayer", "Multiplayer"), TEXT("People"))->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleShowJoin);
	MakeMenuButton(Left, LOCTEXT("SettingsBtn", "Einstellungen"), TEXT("Gear"))->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleShowSettings);
	MakeMenuButton(Left, LOCTEXT("Quit", "Beenden"), TEXT("Exit"))->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleQuit);

	AddV(Left, WidgetTree->ConstructWidget<USpacer>(), FMargin(), HAlign_Fill, true);
	MessageText = Text(WidgetTree, FText::GetEmpty(), 13, Col::Accent, EFont::Strong, 440.f);
	AddV(Left, MessageText, FMargin(0.f, 0.f, 0.f, 6.f));
	StatusText = Text(WidgetTree, FText::GetEmpty(), 12, Col::TextDim, EFont::Body, 440.f);
	AddV(Left, StatusText, FMargin(0.f, 0.f, 0.f, 10.f));
	ServiceText = Text(WidgetTree, Session ? Session->GetServiceText() : FText::GetEmpty(), 11, Col::Good, EFont::Strong);
	AddV(Left, ServiceText);
	PrepareText = Text(WidgetTree, FText::GetEmpty(), 11, Col::Accent, EFont::Strong);
	PrepareText->SetVisibility(ESlateVisibility::Collapsed);
	AddV(Left, PrepareText);
	AddV(Left, Text(WidgetTree, FText::Format(LOCTEXT("Version", "Version {0} · allein oder im Koop mit bis zu 4 Spielern"), FText::FromString(UWESessionSubsystem::GetGameVersion())),
		10, Col::TextMuted), FMargin(0.f, 2.f, 0.f, 0.f));

	// Right: a card with the open page.
	UVerticalBox* CardBox = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBox* CardHead = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddH(CardHead, Sized(WidgetTree, Box(WidgetTree, Col::Accent, FMargin(0.f), FLinearColor::Transparent, 0.f), 4.f, 26.f), FMargin(0.f, 0.f, 10.f, 0.f));
	PageTitle = Heading(WidgetTree, FText::GetEmpty(), 20, Col::Text);
	AddH(CardHead, PageTitle, FMargin(), true);
	UButton* Back = Button(WidgetTree, LOCTEXT("Back", "Zurück"), EButton::Ghost, TEXT("ChevronLeft"), 12);
	Back->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleBack);
	AddH(CardHead, Back);
	AddV(CardBox, CardHead, FMargin(0.f, 0.f, 0.f, 14.f));
	Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>();
	AddV(CardBox, Pages, FMargin(), HAlign_Fill, true);

	// Page 0: empty (home).
	Pages->AddChild(WidgetTree->ConstructWidget<USpacer>());

	// Page 1: load a save.
	UVerticalBox* Load = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(Load, Text(WidgetTree, LOCTEXT("LoadHint", "Du bist der Host: Mitspieler können jederzeit beitreten und gehen – ihr Inventar wartet auf sie."), 12, Col::TextDim, EFont::Body, 600.f));
	SaveList = WEMenuStyle::List(WidgetTree, Load, 380.f);
	Pages->AddChild(Load);

	// Page 2: multiplayer (host and join).
	UVerticalBox* Join = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBox* NameRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddH(NameRow, Text(WidgetTree, LOCTEXT("Name", "Dein Name"), 13, Col::TextDim, EFont::Strong), FMargin(0.f, 0.f, 10.f, 0.f));
	NameBox = WEMenuStyle::TextField(WidgetTree, FText::FromString(Session ? Session->GetLocalPlayerName() : FString()), FText::GetEmpty());
	NameBox->SetIsReadOnly(!Session || !Session->CanChangePlayerName());
	NameBox->OnTextCommitted.AddDynamic(this, &UWEMainMenuWidget::HandleNameCommitted);
	AddH(NameRow, NameBox, FMargin(), true);
	AddV(Join, NameRow);
	AddV(Join, SectionHeader(WidgetTree, LOCTEXT("HostHead", "Spiel eröffnen"), TEXT("Home"), nullptr, 12), FMargin(0.f, 14.f, 0.f, 6.f));
	UHorizontalBox* HostRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	UButton* HostNew = Button(WidgetTree, LOCTEXT("HostNew", "Neues Spiel hosten"), EButton::Primary, TEXT("Plus"), 12);
	HostNew->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleShowNew);
	AddH(HostRow, HostNew, FMargin(0.f, 0.f, 6.f, 0.f));
	UButton* HostLoad = Button(WidgetTree, LOCTEXT("HostLoad", "Spielstand hosten"), EButton::Secondary, TEXT("Folder"), 12);
	HostLoad->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleShowLoad);
	AddH(HostRow, HostLoad, FMargin(0.f, 0.f, 6.f, 0.f));
	UButton* EditLookButton = Button(WidgetTree, LOCTEXT("EditLook", "Aussehen ändern"), EButton::Secondary, TEXT("Person"), 12);
	EditLookButton->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleEditLook);
	AddH(HostRow, EditLookButton);
	AddV(Join, HostRow);
	LANCheck = WEMenuStyle::Check(WidgetTree, Join, LOCTEXT("LANOnly", "Im LAN / per IP hosten statt über Steam (Freunde einladen geht nur über Steam)"),
		!Session || Session->GetService() != EWEOnlineService::Steam);
	LANCheck->SetIsEnabled(Session && Session->GetService() == EWEOnlineService::Steam);
	AddV(Join, SectionHeader(WidgetTree, LOCTEXT("JoinHead", "Spiel beitreten"), TEXT("People"), nullptr, 12), FMargin(0.f, 16.f, 0.f, 6.f));
	UHorizontalBox* SearchRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	SteamSearchButton = Button(WidgetTree, LOCTEXT("SearchSteam", "Steam-Spiele suchen"), EButton::Secondary, TEXT("Search"), 12);
	SteamSearchButton->SetIsEnabled(Session && Session->GetService() == EWEOnlineService::Steam);
	SteamSearchButton->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleSearchSteam);
	AddH(SearchRow, SteamSearchButton, FMargin(0.f, 0.f, 6.f, 0.f));
	UButton* LANButton = Button(WidgetTree, LOCTEXT("SearchLAN", "LAN-Spiele suchen"), EButton::Secondary, TEXT("Search"), 12);
	LANButton->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleSearchLAN);
	AddH(SearchRow, LANButton);
	AddV(Join, SearchRow);
	GameList = WEMenuStyle::List(WidgetTree, Join, 170.f);
	UHorizontalBox* AddressRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddressBox = WEMenuStyle::TextField(WidgetTree, FText::GetEmpty(), LOCTEXT("AddressHint", "IP-Adresse, z. B. 192.168.0.5"));
	AddH(AddressRow, AddressBox, FMargin(), true);
	UButton* Connect = Button(WidgetTree, LOCTEXT("Connect", "Verbinden"), EButton::Primary, TEXT("ChevronRight"), 12);
	Connect->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleJoinAddress);
	AddH(AddressRow, Connect, FMargin(6.f, 0.f, 0.f, 0.f));
	AddV(Join, AddressRow, FMargin(0.f, 10.f, 0.f, 0.f));
	Pages->AddChild(Join);

	// Page 3: new game.
	UVerticalBox* New = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(New, Text(WidgetTree, LOCTEXT("NewHint", "Du startest mit einer kleinen Garage, 250 € und ein paar alten Geräten. Freunde können später über Multiplayer beitreten."),
		12, Col::TextDim, EFont::Body, 600.f));
	AddV(New, Text(WidgetTree, LOCTEXT("CompanyName", "Name deines Unternehmens"), 13, Col::Text, EFont::Strong), FMargin(0.f, 16.f, 0.f, 6.f));
	CompanyBox = WEMenuStyle::TextField(WidgetTree, LOCTEXT("DefaultCompany", "Werkstatt"), LOCTEXT("CompanyHint", "z. B. Schraubwerk_Mueller"));
	AddV(New, CompanyBox);
	AddV(New, Text(WidgetTree, LOCTEXT("CompanyRule", "Wird auch der Name des Spielstands (Buchstaben, Ziffern, - und _)."), 10, Col::TextMuted), FMargin(0.f, 4.f, 0.f, 0.f));
	TutorialCheck = WEMenuStyle::Check(WidgetTree, New, LOCTEXT("TutorialOpt", "Tutorial spielen (empfohlen) – kurze Hinweise direkt in der Werkstatt, jederzeit überspringbar"), true, 16.f);
	// Phase 21: difficulty - a preset, or every option on its own.
	AddV(New, Text(WidgetTree, LOCTEXT("RulesTitle", "Schwierigkeit"), 13, Col::Text, EFont::Strong), FMargin(0.f, 14.f, 0.f, 4.f));
	{
		static const FName RuleIcons[] = { TEXT("Star"), TEXT("Search"), TEXT("Wrench"), TEXT("Star"), TEXT("Euro"), TEXT("Gear") };
		const FText RuleLabels[] = { LOCTEXT("RulePreset", "Voreinstellung"), LOCTEXT("RuleDiag", "Diagnose erforderlich"), LOCTEXT("RuleMistakes", "Reparaturfehler"),
			LOCTEXT("RuleReviews", "Kundenbewertungen"), LOCTEXT("RuleEconomy", "Wirtschaft"), LOCTEXT("RuleComplexity", "Reparaturkomplexität") };
		for (int32 r = 0; r < UE_ARRAY_COUNT(RuleLabels); ++r)
		{
			UWESettingRow* Row = CreateWidget<UWESettingRow>(GetOwningPlayer(), UWESettingRow::StaticClass());
			Row->SetupCallback([this, r](int32 Direction) { StepRule(r, Direction); }, RuleLabels[r], RuleIcons[r]);
			AddV(New, Row, FMargin(0.f, 0.f, 0.f, 2.f));
			RuleRows.Add(Row);
		}
		RulesDescription = Text(WidgetTree, FText::GetEmpty(), 11, Col::TextDim, EFont::Body, 600.f);
		AddV(New, RulesDescription, FMargin(0.f, 4.f, 0.f, 0.f));
		RefreshRuleRows();
	}
	UButton* Start = Button(WidgetTree, LOCTEXT("ToCreator", "Weiter: Charakter erstellen"), EButton::Primary, TEXT("Person"), 15);
	Start->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleShowCreator);
	// (Phase 21: with the difficulty rows the page is taller than the card - it scrolls, the button stays in view.)
	UScrollBox* NewScroll = WidgetTree->ConstructWidget<UScrollBox>();
	NewScroll->AddChild(New);
	UVerticalBox* NewPage = WidgetTree->ConstructWidget<UVerticalBox>();
	if (UVerticalBoxSlot* ScrollSlot = NewPage->AddChildToVerticalBox(NewScroll))
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	AddV(NewPage, Start, FMargin(0.f, 12.f, 0.f, 0.f), HAlign_Left);
	Pages->AddChild(NewPage);

	// Page 4: settings.
	Settings = CreateWidget<UWESettingsPanel>(GetOwningPlayer(), UWESettingsPanel::StaticClass());
	UScrollBox* SettingsScroll = WidgetTree->ConstructWidget<UScrollBox>();
	SettingsScroll->AddChild(Settings);
	Pages->AddChild(SettingsScroll);

	// Page 5 (Phase 18c): character creator - one row per part of the look, the figure in 3D beside the card.
	UVerticalBox* Creator = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(Creator, Text(WidgetTree, LOCTEXT("CreatorHint", "So siehst du für deine Mitspieler aus. Die Figur steht links daneben und lässt sich drehen."), 12, Col::TextDim, EFont::Body, 600.f),
		FMargin(0.f, 0.f, 0.f, 8.f));
	// The rows scroll; the buttons below stay in view.
	UScrollBox* LookScroll = WidgetTree->ConstructWidget<UScrollBox>();
	UVerticalBox* LookList = WidgetTree->ConstructWidget<UVerticalBox>();
	LookScroll->AddChild(LookList);
	AddV(Creator, Sized(WidgetTree, LookScroll, 0.f, 380.f));
	static const FName LookIcons[] = { TEXT("Person"), TEXT("Person"), TEXT("Paint"), TEXT("Star"), TEXT("Paint"), TEXT("Tag"), TEXT("Paint"), TEXT("Box"), TEXT("Star"), TEXT("Search") };
	for (int32 f = 0; f < static_cast<int32>(EWELookField::Num); ++f)
	{
		UWESettingRow* Row = CreateWidget<UWESettingRow>(GetOwningPlayer(), UWESettingRow::StaticClass());
		Row->SetupCallback([this, f](int32 Direction) { StepLook(f, Direction); }, WELook::GetFieldName(static_cast<EWELookField>(f)), LookIcons[f]);
		AddV(LookList, Row, FMargin(0.f, 0.f, 0.f, 2.f));
		LookRows.Add(Row);
	}
	UWESettingRow* TurnRow = CreateWidget<UWESettingRow>(GetOwningPlayer(), UWESettingRow::StaticClass());
	TurnRow->SetupCallback([this](int32 Direction) { TurnPreview(Direction * 45.f); }, LOCTEXT("Turn", "Figur drehen"), TEXT("Roller"));
	TurnRow->SetValue(LOCTEXT("TurnValue", "45°"));
	AddV(LookList, TurnRow, FMargin(0.f, 6.f, 0.f, 2.f));
	UHorizontalBox* CreatorButtons = WidgetTree->ConstructWidget<UHorizontalBox>();
	UButton* Random = Button(WidgetTree, LOCTEXT("RandomLook", "Zufällig"), EButton::Secondary, TEXT("Star"), 12);
	Random->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleRandomLook);
	AddH(CreatorButtons, Random, FMargin(0.f, 0.f, 6.f, 0.f));
	UButton* Close = Button(WidgetTree, LOCTEXT("CloseUp", "Gesicht / Ganzkörper"), EButton::Secondary, TEXT("Search"), 12);
	Close->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleCloseUp);
	AddH(CreatorButtons, Close, FMargin(0.f, 0.f, 6.f, 0.f));
	CreatorStatus = Text(WidgetTree, FText::GetEmpty(), 11, Col::Good, EFont::Strong);
	AddH(CreatorButtons, CreatorStatus, FMargin(), true);
	AddV(Creator, CreatorButtons, FMargin(0.f, 10.f, 0.f, 0.f));
	UHorizontalBox* Confirm = WidgetTree->ConstructWidget<UHorizontalBox>();
	UButton* SaveLook = Button(WidgetTree, LOCTEXT("SaveLook", "Aussehen speichern"), EButton::Primary, TEXT("Check"), 13);
	SaveLook->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleSaveLook);
	AddH(Confirm, SaveLook, FMargin(0.f, 0.f, 6.f, 0.f));
	CreatorSaveButton = SaveLook;
	UButton* Found = Button(WidgetTree, LOCTEXT("StartNew", "Unternehmen gründen"), EButton::Primary, TEXT("Play"), 15);
	Found->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleStartNew);
	AddH(Confirm, Found);
	CreatorStartButton = Found;
	AddV(Creator, Confirm, FMargin(0.f, 12.f, 0.f, 0.f), HAlign_Left);
	Pages->AddChild(Creator);

	UBorder* Card = WindowPanel(WidgetTree, FMargin(26.f, 22.f), Col::Hex(0x121519, 0.95f));
	Card->SetContent(Sized(WidgetTree, CardBox, 660.f, 600.f));
	PageCard = Shadowed(WidgetTree, Card);
	if (UOverlaySlot* S = Root->AddChildToOverlay(PageCard))
	{
		S->SetHorizontalAlignment(HAlign_Right);
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 52.f, 0.f));
	}
	ShowPage(0);
	IntroAnim.Duration = 0.5f;
	IntroAnim.Offset = 30.f;
	IntroAnim.Start();
}

void UWEMainMenuWidget::ShowPage(int32 Page)
{
	if (Pages)
	{
		Pages->SetActiveWidgetIndex(Page);
	}
	if (PageCard)
	{
		PageCard->SetVisibility(Page == 0 ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		CardAnim.Start();
	}
	static const FText Titles[] = { FText::GetEmpty(), LOCTEXT("PLoad", "Spiel fortsetzen"), LOCTEXT("PMulti", "Multiplayer"), LOCTEXT("PNew", "Neues Spiel"), LOCTEXT("PSettings", "Einstellungen"),
		LOCTEXT("PCreator", "Charakter erstellen") };
	if (Page == 5)
	{
		OpenPreview();
	}
	else
	{
		ClosePreview();
	}
	if (PageTitle && Page >= 0 && Page < UE_ARRAY_COUNT(Titles))
	{
		PageTitle->SetText(Upper(Titles[Page]));
	}
	if (Page == 4 && Settings)
	{
		Settings->Reload();
	}
	LastSignature.Reset();
}

void UWEMainMenuWidget::ShowMessage(const FText& Message)
{
	if (MessageText)
	{
		MessageText->SetText(Message);
	}
}

bool UWEMainMenuWidget::WantsLAN() const
{
	return !LANCheck || LANCheck->IsChecked();
}

void UWEMainMenuWidget::HandleContinue()
{
	if (LatestSlot.IsEmpty())
	{
		ShowPage(1);
		return;
	}
	if (UWESessionSubsystem* Session = UWESessionSubsystem::Get(this))
	{
		Session->HostGame(LatestSlot, WantsLAN());
	}
}

void UWEMainMenuWidget::HandleShowNew()
{
	ShowPage(3);
}

// --- Character creator (Phase 18c) ---------------------------------------------------------------------------

void UWEMainMenuWidget::HandleShowCreator()
{
	bCreatorForNewGame = true;
	ShowPage(5);
}

void UWEMainMenuWidget::HandleEditLook()
{
	bCreatorForNewGame = false;
	ShowPage(5);
}

void UWEMainMenuWidget::OpenPreview()
{
	bool bFound = false;
	EditLook = WELook::LoadProfileLook(&bFound);
	if (!bFound)
	{
		EditLook = WELook::Random(FMath::Rand());
	}
	CreatorStartButton->SetVisibility(bCreatorForNewGame ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	CreatorSaveButton->SetVisibility(bCreatorForNewGame ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	CreatorStatus->SetText(FText::GetEmpty());
	APlayerController* PC = GetOwningPlayer();
	if (!Preview.IsValid() && PC && GetWorld())
	{
		// On the yard in front of the garage, daylight; it brings its own camera and lights.
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Preview = GetWorld()->SpawnActor<AWEAvatarPreview>(AWEAvatarPreview::StaticClass(), FTransform(FRotator(0.f, 0.f, 0.f), FVector(850.f, -700.f, 0.f)), Params);
	}
	if (AWEAvatarPreview* P = Preview.Get())
	{
		P->SetYaw(-20.f);
		P->SetCloseUp(bCloseUp = false);
		if (PC)
		{
			PC->SetViewTargetWithBlend(P, 0.5f, VTBlend_EaseInOut, 2.f);
		}
	}
	RefreshLook();
}

void UWEMainMenuWidget::ClosePreview()
{
	if (AWEAvatarPreview* P = Preview.Get())
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (AWEMenuPlayerController* Menu = Cast<AWEMenuPlayerController>(PC); Menu && Menu->GetMenuCamera())
			{
				PC->SetViewTargetWithBlend(Menu->GetMenuCamera(), 0.4f);
			}
		}
		P->Destroy();
	}
	Preview.Reset();
}

void UWEMainMenuWidget::StepLook(int32 Field, int32 Direction)
{
	WELook::Step(EditLook, static_cast<EWELookField>(Field), Direction);
	Play(this, ESound::Toggle, 0.4f);
	// Face and hair are easier to judge close up.
	const EWELookField F = static_cast<EWELookField>(Field);
	if (AWEAvatarPreview* P = Preview.Get())
	{
		P->SetCloseUp(bCloseUp = F == EWELookField::Face || F == EWELookField::Hair || F == EWELookField::HairColor || F == EWELookField::Glasses || F == EWELookField::Headwear);
	}
	CreatorStatus->SetText(FText::GetEmpty());
	RefreshLook();
}

void UWEMainMenuWidget::TurnPreview(float Degrees)
{
	if (AWEAvatarPreview* P = Preview.Get())
	{
		P->Turn(Degrees);
	}
}

void UWEMainMenuWidget::RefreshLook()
{
	for (int32 f = 0; f < LookRows.Num(); ++f)
	{
		const EWELookField Field = static_cast<EWELookField>(f);
		LookRows[f]->SetValue(WELook::GetOptionName(Field, EditLook.Get(Field), EditLook.Body));
	}
	if (AWEAvatarPreview* P = Preview.Get())
	{
		P->SetLook(EditLook);
	}
}

void UWEMainMenuWidget::HandleRandomLook()
{
	EditLook = WELook::Random(FMath::Rand());
	RefreshLook();
	Play(this, ESound::Toggle, 0.5f);
}

void UWEMainMenuWidget::HandleCloseUp()
{
	if (AWEAvatarPreview* P = Preview.Get())
	{
		P->SetCloseUp(bCloseUp = !bCloseUp);
	}
}

void UWEMainMenuWidget::HandleSaveLook()
{
	WELook::SaveProfileLook(EditLook);
	CreatorStatus->SetText(LOCTEXT("LookSaved", "Gespeichert – gilt ab dem nächsten Spiel"));
	Play(this, ESound::Confirm, 0.5f);
}

void UWEMainMenuWidget::StepRule(int32 Row, int32 Direction)
{
	auto Cycle = [Direction](int32 Value, int32 Count) { return (Value + Direction + Count) % Count; };
	switch (Row)
	{
	case 0:
	{
		// Presets only (Individuell comes by itself when an option is changed).
		const int32 Current = NewRules.Preset == EWEDifficultyPreset::Custom ? 1 : static_cast<int32>(NewRules.Preset);
		NewRules = FWEGameRules::FromPreset(static_cast<EWEDifficultyPreset>(Cycle(Current, 3)));
		break;
	}
	case 1: NewRules.bDiagnosisRequired = !NewRules.bDiagnosisRequired; break;
	case 2: NewRules.bRepairMistakes = !NewRules.bRepairMistakes; break;
	case 3: NewRules.bCustomerReviews = !NewRules.bCustomerReviews; break;
	case 4: NewRules.Economy = static_cast<EWEEconomyLevel>(Cycle(static_cast<int32>(NewRules.Economy), 3)); break;
	case 5: NewRules.Complexity = static_cast<EWERepairComplexity>(Cycle(static_cast<int32>(NewRules.Complexity), 3)); break;
	default: break;
	}
	if (Row != 0)
	{
		NewRules.Preset = EWEDifficultyPreset::Custom;
		NewRules.Preset = NewRules.DetectPreset();
	}
	NewRules.bCourier = true;
	RefreshRuleRows();
}

void UWEMainMenuWidget::RefreshRuleRows()
{
	if (RuleRows.Num() < 6)
	{
		return;
	}
	auto OnOff = [](bool b) { return b ? LOCTEXT("RuleOn", "An") : LOCTEXT("RuleOff", "Aus"); };
	RuleRows[0]->SetValue(FWEGameRules::GetPresetName(NewRules.Preset));
	RuleRows[1]->SetValue(OnOff(NewRules.bDiagnosisRequired));
	RuleRows[2]->SetValue(OnOff(NewRules.bRepairMistakes));
	RuleRows[3]->SetValue(OnOff(NewRules.bCustomerReviews));
	RuleRows[4]->SetValue(FWEGameRules::GetEconomyName(NewRules.Economy));
	RuleRows[5]->SetValue(FWEGameRules::GetComplexityName(NewRules.Complexity));
	if (RulesDescription)
	{
		RulesDescription->SetText(FText::Format(LOCTEXT("RulesDescLine", "{0} Bestellungen bringt der Paketbote. Gilt für diesen Spielstand."),
			FWEGameRules::GetPresetDescription(NewRules.Preset)));
	}
}

void UWEMainMenuWidget::HandleStartNew()
{
	// The character from the creator goes into the profile; the game sends it when the player spawns.
	if (bCreatorForNewGame)
	{
		WELook::SaveProfileLook(EditLook);
	}
	if (UWESessionSubsystem* Session = UWESessionSubsystem::Get(this))
	{
		Session->HostNewGame(CompanyBox ? CompanyBox->GetText().ToString() : FString(), !TutorialCheck || TutorialCheck->IsChecked(), WantsLAN(), NewRules.ToOption());
	}
}

void UWEMainMenuWidget::HandleHostNew()
{
	HandleStartNew();
}

void UWEMainMenuWidget::HandleShowLoad()
{
	ShowPage(1);
}

void UWEMainMenuWidget::HandleShowJoin()
{
	ShowPage(2);
	if (UWESessionSubsystem* Session = UWESessionSubsystem::Get(this))
	{
		Session->FindGames(Session->GetService() != EWEOnlineService::Steam);
	}
}

void UWEMainMenuWidget::HandleShowSettings()
{
	ShowPage(4);
}

void UWEMainMenuWidget::HandleQuit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UWEMainMenuWidget::HandleBack()
{
	ShowPage(0);
}

void UWEMainMenuWidget::HandleSearchSteam()
{
	if (UWESessionSubsystem* Session = UWESessionSubsystem::Get(this))
	{
		Session->FindGames(false);
	}
}

void UWEMainMenuWidget::HandleSearchLAN()
{
	if (UWESessionSubsystem* Session = UWESessionSubsystem::Get(this))
	{
		Session->FindGames(true);
	}
}

void UWEMainMenuWidget::HandleJoinAddress()
{
	if (UWESessionSubsystem* Session = UWESessionSubsystem::Get(this); Session && AddressBox)
	{
		Session->JoinAddress(AddressBox->GetText().ToString());
	}
}

void UWEMainMenuWidget::HandleNameCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (UWESessionSubsystem* Session = UWESessionSubsystem::Get(this); Session && Session->CanChangePlayerName())
	{
		Session->SetLocalPlayerName(Text.ToString());
		NameBox->SetText(FText::FromString(Session->GetLocalPlayerName()));
	}
}

FString UWEMainMenuWidget::BuildSignature() const
{
	const UWESessionSubsystem* Session = UWESessionSubsystem::Get(this);
	FString Sig = FString::Printf(TEXT("page=%d state=%d status=%s"), Pages ? Pages->GetActiveWidgetIndex() : -1,
		Session ? static_cast<int32>(Session->GetState()) : -1, Session ? *Session->GetStatusText().ToString() : TEXT(""));
	if (Session)
	{
		for (const FWESessionEntry& E : Session->GetFoundGames())
		{
			Sig += FString::Printf(TEXT(" | %s %d/%d %dms %s"), *E.HostName, E.Players, E.MaxPlayers, E.PingMs, *E.Version);
		}
	}
	return Sig;
}

void UWEMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	IntroAnim.Tick(LeftColumn, InDeltaTime);
	if (PrepareText)
	{
		// The first start compiles the graphics pipelines (PSOs) - the frame rate is low until that is through.
		const int32 Pending = static_cast<int32>(PipelineStateCache::NumActivePrecacheRequests()) + PipelineStateCache::GetNumActivePipelinePrecompileTasks();
		PrepareText->SetVisibility(Pending > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (Pending > 0)
		{
			PrepareText->SetText(FText::Format(LOCTEXT("PreparingGraphics", "Grafik wird vorbereitet … ({0}) – beim ersten Start kurz ruckelig"), FText::AsNumber(Pending)));
		}
	}
	if (PageCard && PageCard->GetVisibility() == ESlateVisibility::Visible)
	{
		CardAnim.Tick(PageCard, InDeltaTime);
	}
	const FString Sig = BuildSignature();
	if (Sig != LastSignature)
	{
		LastSignature = Sig;
		Rebuild();
	}
}

void UWEMainMenuWidget::Rebuild()
{
	UWESessionSubsystem* Session = UWESessionSubsystem::Get(this);
	if (StatusText)
	{
		StatusText->SetText(Session ? Session->GetStatusText() : FText::GetEmpty());
	}
	// "Spiel fortsetzen": the newest readable save.
	const TArray<FWESaveSlotInfo> Slots = FWESaveSystem::ListSlots();
	LatestSlot.Reset();
	for (const FWESaveSlotInfo& Info : Slots)
	{
		if (Info.bReadable)
		{
			LatestSlot = Info.Slot;
			if (ContinueDetail)
			{
				ContinueDetail->SetText(FText::Format(LOCTEXT("ContinueDetail", "{0} · {1} · {2}"), FText::FromString(Info.Slot), FText::AsDateTime(Info.SavedAtUtc),
					FText::FromString(FWEMoney::FromCents(FMath::Max<int64>(0, Info.BalanceCents)).ToDisplayString())));
			}
			break;
		}
	}
	if (ContinueDetail && LatestSlot.IsEmpty())
	{
		ContinueDetail->SetText(LOCTEXT("NoSaveYet", "Noch kein Spielstand – starte ein neues Spiel"));
	}
	if (ContinueButton)
	{
		ContinueButton->SetIsEnabled(!LatestSlot.IsEmpty());
	}
	if (SaveList && Pages && Pages->GetActiveWidgetIndex() == 1)
	{
		SaveList->ClearChildren();
		for (const FWESaveSlotInfo& Info : Slots)
		{
			UWEMenuRowWidget* Row = CreateWidget<UWEMenuRowWidget>(this, UWEMenuRowWidget::StaticClass());
			const FString SlotName = Info.Slot;
			Row->Configure(FText::FromString(Info.Slot),
				Info.bReadable ? FText::Format(LOCTEXT("SaveDetail", "{0} · Firmenkonto {1}"), FText::AsDateTime(Info.SavedAtUtc), FText::FromString(FWEMoney::FromCents(FMath::Max<int64>(0, Info.BalanceCents)).ToDisplayString()))
					: LOCTEXT("SaveBroken", "Datei beschädigt oder neuere Version"),
				LOCTEXT("LoadHost", "Laden"), Info.bReadable, [this, SlotName]()
				{
					if (UWESessionSubsystem* S = UWESessionSubsystem::Get(this))
					{
						S->HostGame(SlotName, WantsLAN());
					}
				}, SlotName == TEXT("Autosave") ? FName(TEXT("Clock")) : FName(TEXT("Save")));
			SaveList->AddChild(Row);
			Row->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}
		if (Slots.IsEmpty())
		{
			UWEMenuRowWidget* Row = CreateWidget<UWEMenuRowWidget>(this, UWEMenuRowWidget::StaticClass());
			Row->Configure(LOCTEXT("NoSaves", "Noch keine Spielstände"), LOCTEXT("NoSavesHint", "Im Spiel speichert der Host mit [F5] oder im Menü [Esc]."), FText::GetEmpty(), false, nullptr, TEXT("Info"));
			SaveList->AddChild(Row);
		}
	}
	if (GameList && Session)
	{
		GameList->ClearChildren();
		for (const FWESessionEntry& Entry : Session->GetFoundGames())
		{
			UWEMenuRowWidget* Row = CreateWidget<UWEMenuRowWidget>(this, UWEMenuRowWidget::StaticClass());
			const int32 Index = Entry.ResultIndex;
			const bool bFull = Entry.Players >= Entry.MaxPlayers;
			Row->Configure(FText::Format(LOCTEXT("GameTitle", "Spiel von {0}"), FText::FromString(Entry.HostName)),
				FText::Format(LOCTEXT("GameDetail", "{0}/{1} Spieler · {2} ms · Version {3}{4}"), Entry.Players, Entry.MaxPlayers, Entry.PingMs,
					FText::FromString(Entry.Version), Entry.bCompatible ? FText::GetEmpty() : LOCTEXT("Incompatible", " (andere Version)")),
				bFull ? LOCTEXT("Full", "Voll") : LOCTEXT("JoinBtn", "Beitreten"), Entry.bCompatible && !bFull, [this, Index]()
				{
					if (UWESessionSubsystem* S = UWESessionSubsystem::Get(this))
					{
						S->JoinGame(Index);
					}
				}, TEXT("People"));
			GameList->AddChild(Row);
			Row->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}
	}
}

FReply UWEMainMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape && Pages && Pages->GetActiveWidgetIndex() != 0)
	{
		ShowPage(0);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// =================================================================================================================
// Pause menu
// =================================================================================================================

void UWEPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	UBorder* Root = Box(WidgetTree, Col::Backdrop, FMargin(0.f), FLinearColor::Transparent, 0.f);
	Root->SetHorizontalAlignment(HAlign_Center);
	Root->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Root;

	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
	// Left: buttons.
	UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(Left, Hazard(WidgetTree, 6.f, 120.f));
	AddV(Left, Heading(WidgetTree, LOCTEXT("PauseTitle", "Menü"), 30, Col::Text), FMargin(0.f, 10.f, 0.f, 2.f));
	AddV(Left, Text(WidgetTree, LOCTEXT("Running", "Das Spiel läuft im Koop weiter."), 11, Col::TextDim), FMargin(0.f, 0.f, 0.f, 18.f));
	auto AddButton = [this, Left](const FText& Label, FName IconName, EButton Kind, TObjectPtr<UTextBlock>* OutText = nullptr)
	{
		UTextBlock* LabelText = nullptr;
		UButton* B = Button(WidgetTree, Label, Kind, IconName, 14, &LabelText);
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
		AddV(Left, Sized(WidgetTree, B, 300.f, 42.f), FMargin(0.f, 0.f, 0.f, 6.f));
		return B;
	};
	AddButton(LOCTEXT("Continue", "Weiterspielen"), TEXT("Play"), EButton::Primary)->OnClicked.AddDynamic(this, &UWEPauseMenuWidget::HandleContinue);
	SaveButton = AddButton(LOCTEXT("Save", "Speichern"), TEXT("Save"), EButton::Secondary);
	SaveButton->OnClicked.AddDynamic(this, &UWEPauseMenuWidget::HandleSave);
	AddButton(LOCTEXT("SettingsBtn2", "Einstellungen"), TEXT("Gear"), EButton::Secondary)->OnClicked.AddDynamic(this, &UWEPauseMenuWidget::HandleSettings);
	AddButton(LOCTEXT("Tutorial", "Tutorial"), TEXT("Bulb"), EButton::Secondary, &TutorialText)->OnClicked.AddDynamic(this, &UWEPauseMenuWidget::HandleTutorial);
	AddButton(LOCTEXT("Invite", "Steam-Freunde einladen"), TEXT("People"), EButton::Secondary)->OnClicked.AddDynamic(this, &UWEPauseMenuWidget::HandleInvite);
	AddButton(LOCTEXT("Leave", "Zum Hauptmenü"), TEXT("Home"), EButton::Secondary, &LeaveText)->OnClicked.AddDynamic(this, &UWEPauseMenuWidget::HandleLeave);
	AddButton(LOCTEXT("QuitGame", "Spiel beenden"), TEXT("Exit"), EButton::Danger)->OnClicked.AddDynamic(this, &UWEPauseMenuWidget::HandleQuit);
	InfoText = Text(WidgetTree, FText::GetEmpty(), 12, Col::Accent, EFont::Strong, 300.f);
	AddV(Left, InfoText, FMargin(0.f, 8.f, 0.f, 0.f));
	AddH(Columns, Left, FMargin(0.f, 0.f, 28.f, 0.f), false, VAlign_Top);

	// Right: session and players, or the settings.
	Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>();
	UVerticalBox* Session = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(Session, SectionHeader(WidgetTree, LOCTEXT("SessionHead", "Sitzung"), TEXT("Desktop"), nullptr, 13), FMargin(0.f, 0.f, 0.f, 6.f));
	SessionText = Text(WidgetTree, FText::GetEmpty(), 12, Col::Good, EFont::Strong, 470.f);
	AddV(Session, SessionText, FMargin(0.f, 0.f, 0.f, 14.f));
	PlayersTitle = Heading(WidgetTree, FText::GetEmpty(), 13, Col::Text);
	AddV(Session, PlayersTitle);
	PlayerList = WEMenuStyle::List(WidgetTree, Session, 260.f);
	Pages->AddChild(Session);
	Settings = CreateWidget<UWESettingsPanel>(GetOwningPlayer(), UWESettingsPanel::StaticClass());
	UScrollBox* SettingsScroll = WidgetTree->ConstructWidget<UScrollBox>();
	SettingsScroll->AddChild(Settings);
	Pages->AddChild(SettingsScroll);

	AddH(Columns, Sized(WidgetTree, Pages, 520.f, 560.f), FMargin(), false, VAlign_Top);

	UBorder* Frame = WindowPanel(WidgetTree, FMargin(30.f, 26.f), Col::Panel);
	Frame->SetContent(Columns);
	Card = Shadowed(WidgetTree, Frame);
	Root->SetContent(Card);
}

void UWEPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Pages)
	{
		Pages->SetActiveWidgetIndex(0);
	}
	Anim.Start();
	LastSignature.Reset();
	Play(this, ESound::Open, 0.4f);
}

bool UWEPauseMenuWidget::IsHost() const
{
	const APlayerController* PC = GetOwningPlayer();
	const AWEPlayerState* PS = PC ? PC->GetPlayerState<AWEPlayerState>() : nullptr;
	return PC && (PC->GetNetMode() == NM_Standalone || (PS && PS->IsHost()));
}

FString UWEPauseMenuWidget::BuildSignature() const
{
	FString Sig;
	if (const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr)
	{
		for (const APlayerState* PS : GS->PlayerArray)
		{
			if (PS)
			{
				Sig += FString::Printf(TEXT("%s:%d:%d|"), *PS->GetPlayerName(), FMath::RoundToInt(PS->GetPingInMilliseconds() / 10.f) * 10,
					Cast<AWEPlayerState>(PS) && Cast<AWEPlayerState>(PS)->IsHost() ? 1 : 0);
			}
		}
	}
	if (const UWETutorialComponent* Tutorial = UWETutorialComponent::Get(this))
	{
		Sig += FString::Printf(TEXT("tut%d"), static_cast<int32>(Tutorial->GetState()));
	}
	return Sig + (IsHost() ? TEXT("host") : TEXT("client"));
}

void UWEPauseMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Anim.Tick(Card, InDeltaTime);
	const FString Sig = BuildSignature();
	if (Sig != LastSignature)
	{
		LastSignature = Sig;
		Rebuild();
	}
}

void UWEPauseMenuWidget::Rebuild()
{
	const bool bHost = IsHost();
	const UWESessionSubsystem* Session = UWESessionSubsystem::Get(this);
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const int32 Count = GS ? GS->PlayerArray.Num() : 1;

	FText SessionLine;
	if (GetOwningPlayer() && GetOwningPlayer()->GetNetMode() == NM_Standalone)
	{
		SessionLine = LOCTEXT("SessionSolo", "Allein (ohne Netzwerk) – für Koop im Hauptmenü unter Multiplayer hosten.");
	}
	else if (Session && Session->GetService() == EWEOnlineService::Steam && !Session->IsLANSession())
	{
		SessionLine = bHost ? LOCTEXT("SessionSteamHost", "Steam-Lobby: Freunde laden über den Knopf links ein oder treten über die Steam-Freundesliste bei.")
			: LOCTEXT("SessionSteamClient", "Steam-Spiel");
	}
	else if (bHost)
	{
		bool bCanBind = false;
		const TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBind);
		SessionLine = FText::Format(LOCTEXT("SessionLANHost", "LAN-Spiel · Mitspieler finden es unter \"LAN-Spiele suchen\" oder verbinden sich mit {0}:{1}"),
			FText::FromString(Addr->ToString(false)), FText::FromString(FString::FromInt(UWESessionSubsystem::GamePort)));
	}
	else
	{
		SessionLine = LOCTEXT("SessionLANClient", "LAN-Spiel");
	}
	SessionText->SetText(SessionLine);
	PlayersTitle->SetText(Upper(FText::Format(LOCTEXT("Players", "Spieler ({0}/{1})"), Count, UWESessionSubsystem::MaxPlayers)));
	SaveButton->SetIsEnabled(bHost);
	if (!LeaveText->GetText().ToString().StartsWith(TEXT("WIRKLICH")))
	{
		LeaveText->SetText(bHost && Count > 1 ? LOCTEXT("LeaveHost", "Spiel beenden (alle zum Hauptmenü)") : LOCTEXT("LeaveClient", "Zum Hauptmenü"));
	}
	const UWETutorialComponent* Tutorial = UWETutorialComponent::Get(this);
	TutorialText->SetText(Tutorial && Tutorial->IsActive() ? LOCTEXT("TutSkip", "Tutorial überspringen") : LOCTEXT("TutStart", "Tutorial starten"));

	PlayerList->ClearChildren();
	ListedPlayers = 0;
	const APlayerState* Me = GetOwningPlayer() ? GetOwningPlayer()->PlayerState : nullptr;
	for (APlayerState* PS : GS ? GS->PlayerArray : TArray<TObjectPtr<APlayerState>>())
	{
		if (!PS)
		{
			continue;
		}
		++ListedPlayers;
		const AWEPlayerState* WEPS = Cast<AWEPlayerState>(PS);
		const bool bIsHostRow = WEPS && WEPS->IsHost();
		const FText Detail = bIsHostRow ? LOCTEXT("RowHost", "Host · besitzt den Spielstand")
			: FText::Format(LOCTEXT("RowPing", "Ping {0} ms"), FMath::RoundToInt(PS->GetPingInMilliseconds()));
		UWEMenuRowWidget* Row = CreateWidget<UWEMenuRowWidget>(this, UWEMenuRowWidget::StaticClass());
		TWeakObjectPtr<APlayerState> Weak(PS);
		Row->Configure(FText::FromString(PS == Me ? FString::Printf(TEXT("%s (du)"), *PS->GetPlayerName()) : PS->GetPlayerName()), Detail,
			bHost && PS != Me ? LOCTEXT("Kick", "Entfernen") : FText::GetEmpty(), true, [this, Weak]()
			{
				if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()); PC && Weak.IsValid())
				{
					PC->RequestKick(Weak.Get());
				}
			}, bIsHostRow ? FName(TEXT("Home")) : FName(TEXT("Person")), false);
		PlayerList->AddChild(Row);
		Row->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	}
}

void UWEPauseMenuWidget::HandleContinue()
{
	if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
	{
		PC->ClosePauseMenu();
	}
}

void UWEPauseMenuWidget::HandleInvite()
{
	FText Error;
	if (UWESessionSubsystem* Session = UWESessionSubsystem::Get(this); Session && !Session->ShowInviteUI(Error))
	{
		InfoText->SetText(Error);
	}
}

void UWEPauseMenuWidget::HandleSave()
{
	if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
	{
		PC->RequestSaveCurrent();
		InfoText->SetText(LOCTEXT("SavedInfo", "Gespeichert."));
	}
}

void UWEPauseMenuWidget::HandleSettings()
{
	ShowSettings(Pages && Pages->GetActiveWidgetIndex() != 1);
}

void UWEPauseMenuWidget::ShowSettings(bool bShow)
{
	if (Pages)
	{
		Pages->SetActiveWidgetIndex(bShow ? 1 : 0);
		if (bShow && Settings)
		{
			Settings->Reload();
		}
	}
}

void UWEPauseMenuWidget::HandleTutorial()
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer());
	const UWETutorialComponent* Tutorial = UWETutorialComponent::Get(this);
	if (!PC)
	{
		return;
	}
	if (Tutorial && Tutorial->IsActive())
	{
		PC->RequestTutorial(1);
		InfoText->SetText(LOCTEXT("TutSkipped", "Tutorial übersprungen – hier oder am PC unter \"Hilfe\" neu starten."));
	}
	else
	{
		PC->RequestTutorial(0);
		PC->ClosePauseMenu();
	}
}

void UWEPauseMenuWidget::HandleLeave()
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer());
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!PC)
	{
		return;
	}
	// The host ends the game for everybody: ask once more.
	if (IsHost() && GS && GS->PlayerArray.Num() > 1 && !LeaveText->GetText().ToString().StartsWith(TEXT("WIRKLICH")))
	{
		LeaveText->SetText(LOCTEXT("LeaveConfirm", "Wirklich beenden? Nochmal klicken"));
		InfoText->SetText(LOCTEXT("LeaveInfo", "Alle Mitspieler kehren ins Hauptmenü zurück. Vorher wird automatisch gespeichert."));
		return;
	}
	PC->RequestLeaveGame();
}

void UWEPauseMenuWidget::HandleQuit()
{
	if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
	{
		PC->RequestLeaveGame(/*bQuitAfterwards*/ true);
	}
}

FReply UWEPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (Pages && Pages->GetActiveWidgetIndex() == 1)
		{
			Pages->SetActiveWidgetIndex(0);
			return FReply::Handled();
		}
		HandleContinue();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
