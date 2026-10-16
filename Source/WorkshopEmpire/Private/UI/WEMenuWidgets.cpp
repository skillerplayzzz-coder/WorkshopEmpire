#include "UI/WEMenuWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Framework/WEGameMode.h"
#include "Framework/WEPlayerController.h"
#include "Framework/WEPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Online/WESessionSubsystem.h"
#include "Save/WESaveSystem.h"
#include "SocketSubsystem.h"
#include "Styling/CoreStyle.h"
#include "WEMoney.h"

#define LOCTEXT_NAMESPACE "WEMenu"

namespace WEMenuStyle
{
	UTextBlock* Text(UWidgetTree* Tree, int32 Size, const FLinearColor& Color, const char* Face = "Regular")
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>();
		T->SetFont(FCoreStyle::GetDefaultFontStyle(Face, Size));
		T->SetColorAndOpacity(FSlateColor(Color));
		return T;
	}

	const FLinearColor Gold(1.f, 0.78f, 0.35f);
	const FLinearColor Soft(0.75f, 0.8f, 0.85f);
	const FLinearColor Dark(0.05f, 0.05f, 0.05f);
	const FLinearColor PanelColor(0.035f, 0.04f, 0.045f, 0.92f);

	UButton* Button(UWidgetTree* Tree, const FText& Label, int32 Size = 13)
	{
		UButton* B = Tree->ConstructWidget<UButton>();
		UTextBlock* T = Text(Tree, Size, Dark, "Bold");
		T->SetText(Label);
		T->SetJustification(ETextJustify::Center);
		B->SetContent(T);
		return B;
	}

	UVerticalBox* Panel(UWidgetTree* Tree, UPanelWidget* Parent, float Width)
	{
		USizeBox* Size = Tree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(Width);
		UBorder* Frame = Tree->ConstructWidget<UBorder>();
		Frame->SetBrushColor(PanelColor);
		Frame->SetPadding(FMargin(28.f, 22.f));
		Size->SetContent(Frame);
		UVerticalBox* Box = Tree->ConstructWidget<UVerticalBox>();
		Frame->SetContent(Box);
		if (UBorder* ParentBorder = Cast<UBorder>(Parent))
		{
			ParentBorder->SetContent(Size);
		}
		else if (Parent)
		{
			Parent->AddChild(Size);
		}
		return Box;
	}

	void Add(UVerticalBox* Box, UWidget* Child, float Top = 0.f, EHorizontalAlignment Align = HAlign_Fill)
	{
		if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Child))
		{
			S->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
			S->SetHorizontalAlignment(Align);
		}
	}

	UScrollBox* List(UWidgetTree* Tree, UVerticalBox* Box, float Height)
	{
		USizeBox* Size = Tree->ConstructWidget<USizeBox>();
		Size->SetHeightOverride(Height);
		UScrollBox* Scroll = Tree->ConstructWidget<UScrollBox>();
		Size->SetContent(Scroll);
		Add(Box, Size, 8.f);
		return Scroll;
	}
}

// --- Row --------------------------------------------------------------------------------------------

void UWEMenuRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
	Frame->SetBrushColor(FLinearColor(0.08f, 0.085f, 0.09f, 0.95f));
	Frame->SetPadding(FMargin(10.f, 6.f));
	WidgetTree->RootWidget = Frame;
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Frame->SetContent(Row);
	UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>();
	TitleText = WEMenuStyle::Text(WidgetTree, 13, FLinearColor::White);
	DetailText = WEMenuStyle::Text(WidgetTree, 10, WEMenuStyle::Soft);
	Texts->AddChildToVerticalBox(TitleText);
	Texts->AddChildToVerticalBox(DetailText);
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Texts))
	{
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetVerticalAlignment(VAlign_Center);
	}
	Button = WidgetTree->ConstructWidget<UButton>();
	ButtonText = WEMenuStyle::Text(WidgetTree, 11, WEMenuStyle::Dark, "Bold");
	Button->SetContent(ButtonText);
	Button->OnClicked.AddDynamic(this, &UWEMenuRowWidget::HandleClicked);
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Button))
	{
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	}
}

void UWEMenuRowWidget::Configure(const FText& Title, const FText& Detail, const FText& ButtonLabel, bool bButtonEnabled, TFunction<void()> InOnClicked)
{
	TitleText->SetText(Title);
	DetailText->SetText(Detail);
	DetailText->SetVisibility(Detail.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	ButtonText->SetText(ButtonLabel);
	Button->SetVisibility(ButtonLabel.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	Button->SetIsEnabled(bButtonEnabled);
	OnClicked = MoveTemp(InOnClicked);
}

void UWEMenuRowWidget::HandleClicked()
{
	if (OnClicked)
	{
		OnClicked();
	}
}

// --- Main menu --------------------------------------------------------------------------------------

UButton* UWEMainMenuWidget::MakeButton(UVerticalBox* Parent, const FText& Label, float Width)
{
	UButton* B = WEMenuStyle::Button(WidgetTree, Label, 15);
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(Width);
	Size->SetHeightOverride(44.f);
	Size->SetContent(B);
	WEMenuStyle::Add(Parent, Size, 10.f, HAlign_Center);
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

	// Dim the 3D scene a little, panel on the left.
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>();
	Root->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.25f));
	Root->SetPadding(FMargin(70.f, 50.f));
	Root->SetHorizontalAlignment(HAlign_Left);
	Root->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Root;
	UVerticalBox* Box = WEMenuStyle::Panel(WidgetTree, Root, 560.f);

	UTextBlock* Title = WEMenuStyle::Text(WidgetTree, 34, WEMenuStyle::Gold, "Bold");
	Title->SetText(LOCTEXT("Title", "WORKSHOP EMPIRE"));
	WEMenuStyle::Add(Box, Title);
	UTextBlock* Sub = WEMenuStyle::Text(WidgetTree, 11, WEMenuStyle::Soft);
	Sub->SetText(FText::Format(LOCTEXT("Subtitle", "Werkstatt, Recycling und Industrie – allein oder im Koop mit bis zu 4 Spielern · Version {0}"),
		FText::FromString(UWESessionSubsystem::GetGameVersion())));
	Sub->SetAutoWrapText(true);
	WEMenuStyle::Add(Box, Sub, 2.f);
	ServiceText = WEMenuStyle::Text(WidgetTree, 11, FLinearColor(0.55f, 0.85f, 0.6f));
	ServiceText->SetText(Session ? Session->GetServiceText() : FText::GetEmpty());
	WEMenuStyle::Add(Box, ServiceText, 6.f);

	// Name (LAN only; Steam shows the Steam name).
	UHorizontalBox* NameRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	UTextBlock* NameLabel = WEMenuStyle::Text(WidgetTree, 12, FLinearColor::White);
	NameLabel->SetText(LOCTEXT("Name", "Dein Name: "));
	NameRow->AddChildToHorizontalBox(NameLabel)->SetVerticalAlignment(VAlign_Center);
	NameBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	NameBox->SetText(FText::FromString(Session ? Session->GetLocalPlayerName() : FString()));
	NameBox->SetIsReadOnly(!Session || !Session->CanChangePlayerName());
	NameBox->OnTextCommitted.AddDynamic(this, &UWEMainMenuWidget::HandleNameCommitted);
	if (UHorizontalBoxSlot* S = NameRow->AddChildToHorizontalBox(NameBox))
	{
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	WEMenuStyle::Add(Box, NameRow, 10.f);

	Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>();
	WEMenuStyle::Add(Box, Pages, 8.f);

	// Page 0: main.
	UVerticalBox* Main = WidgetTree->ConstructWidget<UVerticalBox>();
	MakeButton(Main, LOCTEXT("HostNew", "Neues Spiel hosten"))->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleHostNew);
	MakeButton(Main, LOCTEXT("Load", "Spielstand laden und hosten"))->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleShowLoad);
	MakeButton(Main, LOCTEXT("Join", "Spiel beitreten"))->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleShowJoin);
	MakeButton(Main, LOCTEXT("Quit", "Beenden"))->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleQuit);
	UHorizontalBox* LANRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	LANCheck = WidgetTree->ConstructWidget<UCheckBox>();
	LANCheck->SetIsChecked(!Session || Session->GetService() != EWEOnlineService::Steam);
	LANCheck->SetIsEnabled(Session && Session->GetService() == EWEOnlineService::Steam);
	LANRow->AddChildToHorizontalBox(LANCheck)->SetVerticalAlignment(VAlign_Center);
	UTextBlock* LANLabel = WEMenuStyle::Text(WidgetTree, 11, WEMenuStyle::Soft);
	LANLabel->SetText(LOCTEXT("LANOnly", " Im LAN / per IP hosten statt über Steam (Freunde laden geht nur über Steam)"));
	LANRow->AddChildToHorizontalBox(LANLabel)->SetVerticalAlignment(VAlign_Center);
	WEMenuStyle::Add(Main, LANRow, 12.f);
	Pages->AddChild(Main);

	// Page 1: load a save.
	UVerticalBox* Load = WidgetTree->ConstructWidget<UVerticalBox>();
	UTextBlock* LoadTitle = WEMenuStyle::Text(WidgetTree, 14, WEMenuStyle::Gold, "Bold");
	LoadTitle->SetText(LOCTEXT("LoadTitle", "Spielstand laden – du bist der Host, Mitspieler treten bei"));
	WEMenuStyle::Add(Load, LoadTitle);
	SaveList = WEMenuStyle::List(WidgetTree, Load, 260.f);
	MakeButton(Load, LOCTEXT("Back", "Zurück"), 200.f)->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleBack);
	Pages->AddChild(Load);

	// Page 2: join.
	UVerticalBox* Join = WidgetTree->ConstructWidget<UVerticalBox>();
	UTextBlock* JoinTitle = WEMenuStyle::Text(WidgetTree, 14, WEMenuStyle::Gold, "Bold");
	JoinTitle->SetText(LOCTEXT("JoinTitle", "Spiel beitreten"));
	WEMenuStyle::Add(Join, JoinTitle);
	UHorizontalBox* SearchRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	SteamSearchButton = WEMenuStyle::Button(WidgetTree, LOCTEXT("SearchSteam", "Steam-Spiele suchen"), 12);
	SteamSearchButton->SetIsEnabled(Session && Session->GetService() == EWEOnlineService::Steam);
	SteamSearchButton->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleSearchSteam);
	SearchRow->AddChildToHorizontalBox(SteamSearchButton)->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	UButton* LANButton = WEMenuStyle::Button(WidgetTree, LOCTEXT("SearchLAN", "LAN-Spiele suchen"), 12);
	LANButton->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleSearchLAN);
	SearchRow->AddChildToHorizontalBox(LANButton);
	WEMenuStyle::Add(Join, SearchRow, 8.f);
	GameList = WEMenuStyle::List(WidgetTree, Join, 200.f);
	UHorizontalBox* AddressRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	UTextBlock* AddressLabel = WEMenuStyle::Text(WidgetTree, 12, FLinearColor::White);
	AddressLabel->SetText(LOCTEXT("Address", "IP-Adresse: "));
	AddressRow->AddChildToHorizontalBox(AddressLabel)->SetVerticalAlignment(VAlign_Center);
	AddressBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	AddressBox->SetHintText(LOCTEXT("AddressHint", "z. B. 192.168.0.5"));
	if (UHorizontalBoxSlot* S = AddressRow->AddChildToHorizontalBox(AddressBox))
	{
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	UButton* Connect = WEMenuStyle::Button(WidgetTree, LOCTEXT("Connect", "Verbinden"), 12);
	Connect->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleJoinAddress);
	AddressRow->AddChildToHorizontalBox(Connect)->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	WEMenuStyle::Add(Join, AddressRow, 10.f);
	MakeButton(Join, LOCTEXT("Back2", "Zurück"), 200.f)->OnClicked.AddDynamic(this, &UWEMainMenuWidget::HandleBack);
	Pages->AddChild(Join);

	StatusText = WEMenuStyle::Text(WidgetTree, 12, WEMenuStyle::Soft);
	StatusText->SetAutoWrapText(true);
	WEMenuStyle::Add(Box, StatusText, 12.f);
	MessageText = WEMenuStyle::Text(WidgetTree, 13, WEMenuStyle::Gold, "Bold");
	MessageText->SetAutoWrapText(true);
	WEMenuStyle::Add(Box, MessageText, 6.f);
	UTextBlock* Hint = WEMenuStyle::Text(WidgetTree, 10, WEMenuStyle::Soft);
	Hint->SetAutoWrapText(true);
	Hint->SetText(LOCTEXT("Hint", "Der Host besitzt den Spielstand und speichert. Mitspieler können jederzeit beitreten und gehen – ihr Inventar wartet auf sie."));
	WEMenuStyle::Add(Box, Hint, 12.f);
}

void UWEMainMenuWidget::ShowPage(int32 Page)
{
	if (Pages)
	{
		Pages->SetActiveWidgetIndex(Page);
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

void UWEMainMenuWidget::HandleHostNew()
{
	if (UWESessionSubsystem* Session = UWESessionSubsystem::Get(this))
	{
		Session->HostGame(FString(), WantsLAN());
	}
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
	if (SaveList && Pages && Pages->GetActiveWidgetIndex() == 1)
	{
		SaveList->ClearChildren();
		const TArray<FWESaveSlotInfo> Slots = FWESaveSystem::ListSlots();
		for (const FWESaveSlotInfo& Info : Slots)
		{
			UWEMenuRowWidget* Row = CreateWidget<UWEMenuRowWidget>(this, UWEMenuRowWidget::StaticClass());
			const FString SlotName = Info.Slot;
			Row->Configure(FText::FromString(Info.Slot),
				Info.bReadable ? FText::Format(LOCTEXT("SaveDetail", "{0} · Firmenkonto {1}"), FText::AsDateTime(Info.SavedAtUtc), FText::FromString(FWEMoney::FromCents(FMath::Max<int64>(0, Info.BalanceCents)).ToDisplayString()))
					: LOCTEXT("SaveBroken", "Datei beschädigt oder neuere Version"),
				LOCTEXT("LoadHost", "Laden & hosten"), Info.bReadable, [this, SlotName]()
				{
					if (UWESessionSubsystem* S = UWESessionSubsystem::Get(this))
					{
						S->HostGame(SlotName, WantsLAN());
					}
				});
			SaveList->AddChild(Row);
		}
		if (Slots.IsEmpty())
		{
			UWEMenuRowWidget* Row = CreateWidget<UWEMenuRowWidget>(this, UWEMenuRowWidget::StaticClass());
			Row->Configure(LOCTEXT("NoSaves", "Noch keine Spielstände"), LOCTEXT("NoSavesHint", "Im Spiel speichert der Host mit [F5] oder im Menü [Esc]."), FText::GetEmpty(), false, nullptr);
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
				});
			GameList->AddChild(Row);
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

// --- Pause menu -------------------------------------------------------------------------------------

void UWEPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>();
	Root->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.45f));
	Root->SetHorizontalAlignment(HAlign_Center);
	Root->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Root;
	UVerticalBox* Box = WEMenuStyle::Panel(WidgetTree, Root, 620.f);

	UTextBlock* Title = WEMenuStyle::Text(WidgetTree, 22, WEMenuStyle::Gold, "Bold");
	Title->SetText(LOCTEXT("PauseTitle", "Menü"));
	WEMenuStyle::Add(Box, Title);
	UTextBlock* Running = WEMenuStyle::Text(WidgetTree, 10, WEMenuStyle::Soft);
	Running->SetText(LOCTEXT("Running", "Das Spiel läuft im Koop weiter."));
	WEMenuStyle::Add(Box, Running, 2.f);
	SessionText = WEMenuStyle::Text(WidgetTree, 11, FLinearColor(0.55f, 0.85f, 0.6f));
	SessionText->SetAutoWrapText(true);
	WEMenuStyle::Add(Box, SessionText, 8.f);

	PlayersTitle = WEMenuStyle::Text(WidgetTree, 14, FLinearColor::White, "Bold");
	WEMenuStyle::Add(Box, PlayersTitle, 12.f);
	PlayerList = WEMenuStyle::List(WidgetTree, Box, 190.f);

	auto AddButton = [this, Box](const FText& Label, TObjectPtr<UTextBlock>* OutText = nullptr)
	{
		UButton* B = WEMenuStyle::Button(WidgetTree, Label, 14);
		if (OutText)
		{
			*OutText = Cast<UTextBlock>(B->GetContent());
		}
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(380.f);
		Size->SetHeightOverride(40.f);
		Size->SetContent(B);
		WEMenuStyle::Add(Box, Size, 8.f, HAlign_Center);
		return B;
	};
	AddButton(LOCTEXT("Continue", "Weiterspielen"))->OnClicked.AddDynamic(this, &UWEPauseMenuWidget::HandleContinue);
	AddButton(LOCTEXT("Invite", "Steam-Freunde einladen"))->OnClicked.AddDynamic(this, &UWEPauseMenuWidget::HandleInvite);
	SaveButton = AddButton(LOCTEXT("Save", "Speichern"));
	SaveButton->OnClicked.AddDynamic(this, &UWEPauseMenuWidget::HandleSave);
	AddButton(LOCTEXT("Leave", "Zum Hauptmenü"), &LeaveText)->OnClicked.AddDynamic(this, &UWEPauseMenuWidget::HandleLeave);
	AddButton(LOCTEXT("QuitGame", "Spiel beenden"))->OnClicked.AddDynamic(this, &UWEPauseMenuWidget::HandleQuit);

	InfoText = WEMenuStyle::Text(WidgetTree, 11, WEMenuStyle::Gold);
	InfoText->SetAutoWrapText(true);
	WEMenuStyle::Add(Box, InfoText, 10.f);
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
	return Sig + (IsHost() ? TEXT("host") : TEXT("client"));
}

void UWEPauseMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
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
		SessionLine = LOCTEXT("SessionSolo", "Allein (ohne Netzwerk) – für Koop im Hauptmenü \"Neues Spiel hosten\" wählen.");
	}
	else if (Session && Session->GetService() == EWEOnlineService::Steam && !Session->IsLANSession())
	{
		SessionLine = bHost ? LOCTEXT("SessionSteamHost", "Steam-Lobby: Freunde laden über den Knopf unten ein oder treten über die Steam-Freundesliste bei.")
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
	PlayersTitle->SetText(FText::Format(LOCTEXT("Players", "Spieler ({0}/{1})"), Count, UWESessionSubsystem::MaxPlayers));
	SaveButton->SetIsEnabled(bHost);
	if (!LeaveText->GetText().ToString().StartsWith(TEXT("Wirklich")))
	{
		LeaveText->SetText(bHost && Count > 1 ? LOCTEXT("LeaveHost", "Spiel beenden (alle zum Hauptmenü)") : LOCTEXT("LeaveClient", "Zum Hauptmenü"));
	}

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
		FText Detail = bIsHostRow ? LOCTEXT("RowHost", "Host · besitzt den Spielstand")
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
			});
		PlayerList->AddChild(Row);
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

void UWEPauseMenuWidget::HandleLeave()
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer());
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!PC)
	{
		return;
	}
	// The host ends the game for everybody: ask once more.
	if (IsHost() && GS && GS->PlayerArray.Num() > 1 && !LeaveText->GetText().ToString().StartsWith(TEXT("Wirklich")))
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
		HandleContinue();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
