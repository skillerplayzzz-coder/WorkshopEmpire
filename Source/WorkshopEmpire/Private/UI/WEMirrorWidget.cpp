#include "UI/WEMirrorWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Framework/WEPlayerController.h"
#include "Player/WECharacter.h"
#include "UI/WEMenuWidgets.h"

#define LOCTEXT_NAMESPACE "WEMirrorUI"

using namespace WEUI;

void UWEMirrorWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	// The card stands on the right, so the mirror stays in view.
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
	AddV(Content, Hazard(WidgetTree, 6.f, 120.f));
	AddV(Content, SectionHeader(WidgetTree, LOCTEXT("Title", "Spiegel"), TEXT("Person"), nullptr, 18), FMargin(0.f, 10.f, 0.f, 2.f));
	AddV(Content, Text(WidgetTree, LOCTEXT("Hint", "Jede Änderung siehst du sofort im Spiegel – deine Mitspieler auch."), 11, Col::TextDim, EFont::Body, 450.f),
		FMargin(0.f, 0.f, 0.f, 10.f));
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	UVerticalBox* List = WidgetTree->ConstructWidget<UVerticalBox>();
	Scroll->AddChild(List);
	AddV(Content, Sized(WidgetTree, Scroll, 470.f, 400.f));
	static const FName Icons[] = { TEXT("Person"), TEXT("Person"), TEXT("Paint"), TEXT("Star"), TEXT("Paint"), TEXT("Tag"), TEXT("Paint"), TEXT("Box"), TEXT("Star"), TEXT("Search") };
	for (int32 f = 0; f < static_cast<int32>(EWELookField::Num); ++f)
	{
		UWESettingRow* Row = CreateWidget<UWESettingRow>(GetOwningPlayer(), UWESettingRow::StaticClass());
		Row->SetupCallback([this, f](int32 Direction) { StepLook(f, Direction); }, WELook::GetFieldName(static_cast<EWELookField>(f)), Icons[f]);
		AddV(List, Row, FMargin(0.f, 0.f, 0.f, 2.f));
		Rows.Add(Row);
	}
	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
	UButton* Random = Button(WidgetTree, LOCTEXT("Random", "Zufällig"), EButton::Secondary, TEXT("Star"), 12);
	Random->OnClicked.AddDynamic(this, &UWEMirrorWidget::HandleRandom);
	AddH(Buttons, Random, FMargin(0.f, 0.f, 6.f, 0.f));
	UButton* View = Button(WidgetTree, LOCTEXT("View", "Ansicht [V]"), EButton::Secondary, TEXT("Search"), 12);
	View->OnClicked.AddDynamic(this, &UWEMirrorWidget::HandleView);
	AddH(Buttons, View, FMargin(0.f, 0.f, 6.f, 0.f));
	UButton* Done = Button(WidgetTree, LOCTEXT("Done", "Fertig"), EButton::Primary, TEXT("Check"), 13);
	Done->OnClicked.AddDynamic(this, &UWEMirrorWidget::HandleDone);
	AddH(Buttons, Done);
	AddV(Content, Buttons, FMargin(0.f, 12.f, 0.f, 0.f));
	Status = Text(WidgetTree, FText::GetEmpty(), 11, Col::Good, EFont::Strong, 450.f);
	AddV(Content, Status, FMargin(0.f, 6.f, 0.f, 0.f));

	UBorder* Frame = WindowPanel(WidgetTree, FMargin(24.f, 20.f), Col::Panel);
	Frame->SetContent(Content);
	Card = Shadowed(WidgetTree, Frame);
	if (UOverlaySlot* S = Root->AddChildToOverlay(Card))
	{
		S->SetHorizontalAlignment(HAlign_Right);
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 60.f, 0.f));
	}
}

void UWEMirrorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	const AWECharacter* Me = GetOwningPlayer() ? Cast<AWECharacter>(GetOwningPlayer()->GetPawn()) : nullptr;
	EditLook = Me && Me->HasLook() ? Me->GetLook() : WELook::LoadProfileLook();
	bWasThirdPerson = Me && Me->IsThirdPerson();
	Status->SetText(FText::GetEmpty());
	Refresh();
	Anim.Start();
	Play(this, ESound::Open, 0.4f);
}

void UWEMirrorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Anim.Tick(Card, InDeltaTime);
}

void UWEMirrorWidget::Refresh()
{
	for (int32 f = 0; f < Rows.Num(); ++f)
	{
		const EWELookField Field = static_cast<EWELookField>(f);
		Rows[f]->SetValue(WELook::GetOptionName(Field, EditLook.Get(Field), EditLook.Body));
	}
}

void UWEMirrorWidget::Send()
{
	if (AWECharacter* Me = GetOwningPlayer() ? Cast<AWECharacter>(GetOwningPlayer()->GetPawn()) : nullptr)
	{
		Me->RequestChangeLook(EditLook);
	}
}

void UWEMirrorWidget::StepLook(int32 Field, int32 Direction)
{
	WELook::Step(EditLook, static_cast<EWELookField>(Field), Direction);
	Play(this, ESound::Toggle, 0.4f);
	Status->SetText(FText::GetEmpty());
	Refresh();
	Send();
}

void UWEMirrorWidget::HandleRandom()
{
	EditLook = WELook::Random(FMath::Rand());
	Play(this, ESound::Toggle, 0.5f);
	Refresh();
	Send();
}

void UWEMirrorWidget::HandleView()
{
	if (AWECharacter* Me = GetOwningPlayer() ? Cast<AWECharacter>(GetOwningPlayer()->GetPawn()) : nullptr)
	{
		Me->SetThirdPerson(!Me->IsThirdPerson());
	}
}

void UWEMirrorWidget::HandleDone()
{
	WELook::SaveProfileLook(EditLook);
	Play(this, ESound::Confirm, 0.5f);
	if (AWECharacter* Me = GetOwningPlayer() ? Cast<AWECharacter>(GetOwningPlayer()->GetPawn()) : nullptr; Me && Me->IsThirdPerson() != bWasThirdPerson)
	{
		Me->SetThirdPerson(bWasThirdPerson);
	}
	if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
	{
		PC->CloseMirror();
	}
}

FReply UWEMirrorWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::E)
	{
		HandleDone();
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::V)
	{
		HandleView();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
