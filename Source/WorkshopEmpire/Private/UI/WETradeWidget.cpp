#include "UI/WETradeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WEMarketComponent.h"
#include "Economy/WETraderDefinition.h"
#include "Economy/WETraderTerminal.h"
#include "Framework/WEPlayerController.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "WETrade"

namespace WETradeStyle
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
	const FLinearColor Green(0.45f, 0.9f, 0.5f);
	const FLinearColor Red(0.95f, 0.45f, 0.4f);

	FText Money(const FWEMoney& Value)
	{
		return FText::FromString(Value.ToDisplayString());
	}
}

// --- Row --------------------------------------------------------------------------------------------

void UWETradeRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
	Frame->SetBrushColor(FLinearColor(0.06f, 0.065f, 0.07f, 0.9f));
	Frame->SetPadding(FMargin(8.f, 5.f));
	WidgetTree->RootWidget = Frame;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Frame->SetContent(Row);

	UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>();
	TitleText = WETradeStyle::Text(WidgetTree, 12, FLinearColor::White);
	TitleText->SetAutoWrapText(true);
	DetailText = WETradeStyle::Text(WidgetTree, 9, WETradeStyle::Soft);
	DetailText->SetAutoWrapText(true);
	Texts->AddChildToVerticalBox(TitleText);
	Texts->AddChildToVerticalBox(DetailText);
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Texts))
	{
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetVerticalAlignment(VAlign_Center);
	}

	PriceText = WETradeStyle::Text(WidgetTree, 12, WETradeStyle::Gold, "Bold");
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(PriceText))
	{
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(8.f, 0.f));
	}

	auto MakeButton = [this, Row](TObjectPtr<UButton>& OutButton, TObjectPtr<UTextBlock>& OutText)
	{
		OutButton = WidgetTree->ConstructWidget<UButton>();
		OutText = WETradeStyle::Text(WidgetTree, 10, FLinearColor(0.05f, 0.05f, 0.05f));
		OutButton->SetContent(OutText);
		if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(OutButton))
		{
			S->SetVerticalAlignment(VAlign_Center);
			S->SetPadding(FMargin(2.f, 0.f));
		}
	};
	MakeButton(Button1, Button1Text);
	MakeButton(Button2, Button2Text);
	Button1->OnClicked.AddDynamic(this, &UWETradeRowWidget::HandleButton1);
	Button2->OnClicked.AddDynamic(this, &UWETradeRowWidget::HandleButton2);
}

void UWETradeRowWidget::Configure(UWETradeWidget* InOwner, EWETradeRowKind InKind, const FText& Title, const FText& Detail, const FText& Price,
	const FText& InButton1, bool bButton1Enabled, const FText& InButton2, bool bButton2Enabled)
{
	Owner = InOwner;
	Kind = InKind;
	TitleText->SetText(Title);
	DetailText->SetText(Detail);
	PriceText->SetText(Price);
	Button1Text->SetText(InButton1);
	Button1->SetVisibility(InButton1.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	Button1->SetIsEnabled(bButton1Enabled);
	Button2Text->SetText(InButton2);
	Button2->SetVisibility(InButton2.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	Button2->SetIsEnabled(bButton2Enabled);
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

// --- Screen -----------------------------------------------------------------------------------------

UVerticalBox* UWETradeWidget::MakeColumn(const FText& Title, TObjectPtr<UScrollBox>& OutList)
{
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	UTextBlock* Header = WETradeStyle::Text(WidgetTree, 15, WETradeStyle::Gold, "Bold");
	Header->SetText(Title);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Header))
	{
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}
	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
	Box->SetWidthOverride(400.f);
	Box->SetHeightOverride(430.f);
	OutList = WidgetTree->ConstructWidget<UScrollBox>();
	Box->SetContent(OutList);
	Column->AddChildToVerticalBox(Box);
	return Column;
}

void UWETradeWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.5f));
	Dim->SetHorizontalAlignment(HAlign_Center);
	Dim->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Dim;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(FLinearColor(0.025f, 0.027f, 0.03f, 0.95f));
	Panel->SetPadding(FMargin(18.f));
	Dim->SetContent(Panel);

	UVerticalBox* Outer = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Outer);

	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
	UTextBlock* Title = WETradeStyle::Text(WidgetTree, 20, FLinearColor::White, "Bold");
	Title->SetText(LOCTEXT("Title", "Online-Marktplatz"));
	if (UHorizontalBoxSlot* S = Header->AddChildToHorizontalBox(Title))
	{
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	BalanceText = WETradeStyle::Text(WidgetTree, 18, WETradeStyle::Green, "Bold");
	Header->AddChildToHorizontalBox(BalanceText);
	if (UVerticalBoxSlot* S = Outer->AddChildToVerticalBox(Header))
	{
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	}

	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
	UVerticalBox* SellColumn = MakeColumn(LOCTEXT("Sell", "Verkaufen"), SellList);
	UVerticalBox* BuyColumn = MakeColumn(LOCTEXT("Buy", "Kaufen"), BuyList);
	UVerticalBox* ContractColumn = MakeColumn(LOCTEXT("Contracts", "Kundenaufträge"), ContractList);
	for (UVerticalBox* Column : { SellColumn, BuyColumn, ContractColumn })
	{
		if (UHorizontalBoxSlot* S = Columns->AddChildToHorizontalBox(Column))
		{
			S->SetPadding(FMargin(6.f, 0.f));
		}
	}
	Outer->AddChildToVerticalBox(Columns);

	HistoryText = WETradeStyle::Text(WidgetTree, 10, WETradeStyle::Soft);
	if (UVerticalBoxSlot* S = Outer->AddChildToVerticalBox(HistoryText))
	{
		S->SetPadding(FMargin(6.f, 12.f, 6.f, 0.f));
	}
	UTextBlock* Help = WETradeStyle::Text(WidgetTree, 10, FLinearColor(0.6f, 0.6f, 0.6f));
	Help->SetText(LOCTEXT("Help", "Verkauft wird an den Händler, der am meisten zahlt. Große Einkäufe kommen auf die Lieferpalette am Tor.  ·  Esc/Tab: schließen"));
	if (UVerticalBoxSlot* S = Outer->AddChildToVerticalBox(Help))
	{
		S->SetPadding(FMargin(6.f, 6.f, 6.f, 0.f));
	}
}

void UWETradeWidget::Setup(AWETraderTerminal* InTerminal)
{
	Terminal = InTerminal;
	LastSignature.Reset();
	Rebuild();
}

UWETradeRowWidget* UWETradeWidget::AddRow(UScrollBox* List)
{
	UWETradeRowWidget* Row = CreateWidget<UWETradeRowWidget>(GetOwningPlayer(), UWETradeRowWidget::StaticClass());
	List->AddChild(Row);
	Row->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
	return Row;
}

FString UWETradeWidget::BuildSignature() const
{
	TStringBuilder<1024> Sig;
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	Sig << (Company ? Company->GetBalance().GetCents() : -1) << TEXT("|");
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	if (const UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr)
	{
		for (const FWEInventorySlotData& SlotData : Inventory->GetAllSlots())
		{
			Sig << SlotData.SlotIndex << TEXT(":") << GetNameSafe(SlotData.Item.Definition) << TEXT("x") << SlotData.Item.Quantity
				<< TEXT("c") << static_cast<int32>(SlotData.Item.Condition) << TEXT(",");
		}
	}
	Sig << TEXT("|held:") << (Character && Character->GetHeldItem() ? *Character->GetHeldItem()->GetName() : TEXT("-"));
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
	if (const UWEContractComponent* Contracts = UWEContractComponent::Get(this))
	{
		const float Now = Contracts->GetServerTime();
		for (const FWEContract& C : Contracts->GetContracts())
		{
			Sig << TEXT("|") << C.Id.ToString() << static_cast<int32>(C.State) << TEXT("m") << FMath::CeilToInt((C.ExpiresAt - Now) / 60.f);
		}
	}
	return FString(Sig.ToView());
}

void UWETradeWidget::Rebuild()
{
	if (!Terminal || !SellList)
	{
		return;
	}
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
	const FWEMoney Balance = Company ? Company->GetBalance() : FWEMoney();

	BalanceText->SetText(FText::Format(LOCTEXT("Balance", "Firmenkonto: {0}"), WETradeStyle::Money(Balance)));

	// Sell: carried item + inventory.
	SellList->ClearChildren();
	if (const AWEPickupItem* Held = Character ? Character->GetHeldItem() : nullptr)
	{
		const UWETraderDefinition* Trader = nullptr;
		const FWEMoney Price = Terminal->GetBestSellPrice(Held->GetItem(), &Trader);
		UWETradeRowWidget* Row = AddRow(SellList);
		Row->Configure(this, EWETradeRowKind::SellHeld,
			FText::Format(LOCTEXT("Held", "In der Hand: {0}"), Held->GetDisplayName()),
			Trader ? FText::Format(LOCTEXT("SellTo", "{0} · an {1}"), WEItems::GetConditionText(Held->GetItem().Condition), Trader->DisplayName)
				: LOCTEXT("NoBuyerDetail", "Kein Händler interessiert"),
			Trader ? WETradeStyle::Money(Price) : FText::FromString(TEXT("—")),
			LOCTEXT("SellBtn", "Verkaufen"), Trader != nullptr);
	}
	if (const UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr)
	{
		for (const FWEInventorySlotData& SlotData : Inventory->GetAllSlots())
		{
			FWEItemInstance One = SlotData.Item;
			One.Quantity = 1;
			const UWETraderDefinition* Trader = nullptr;
			const FWEMoney UnitPrice = Terminal->GetBestSellPrice(One, &Trader);
			const FWEMoney AllPrice = Terminal->GetBestSellPrice(SlotData.Item);
			UWETradeRowWidget* Row = AddRow(SellList);
			Row->InventorySlot = SlotData.SlotIndex;
			const bool bStack = SlotData.Item.Quantity > 1;
			Row->Configure(this, EWETradeRowKind::Sell,
				bStack ? FText::Format(LOCTEXT("Stack", "{0} ×{1}"), SlotData.Item.GetDisplayName(), FText::AsNumber(SlotData.Item.Quantity)) : SlotData.Item.GetDisplayName(),
				Trader ? FText::Format(LOCTEXT("SellTo2", "{0} · an {1}"), WEItems::GetConditionText(SlotData.Item.Condition), Trader->DisplayName)
					: LOCTEXT("NoBuyerDetail2", "Kein Händler interessiert"),
				Trader ? (bStack ? FText::Format(LOCTEXT("PerUnit", "{0} /Stk."), WETradeStyle::Money(UnitPrice)) : WETradeStyle::Money(UnitPrice)) : FText::FromString(TEXT("—")),
				bStack ? LOCTEXT("SellOne", "1 verkaufen") : LOCTEXT("SellBtn2", "Verkaufen"), Trader != nullptr,
				bStack ? FText::Format(LOCTEXT("SellAll", "Alle ({0})"), WETradeStyle::Money(AllPrice)) : FText::GetEmpty(), Trader != nullptr);
		}
	}
	if (SellList->GetChildrenCount() == 0)
	{
		AddRow(SellList)->Configure(this, EWETradeRowKind::Info, LOCTEXT("NothingToSell", "Nichts zu verkaufen"),
			LOCTEXT("NothingToSellDetail", "Zerlege Gegenstände an der Werkbank und nimm die Teile ins Inventar."), FText::GetEmpty(), FText::GetEmpty(), false);
	}

	// Buy.
	BuyList->ClearChildren();
	for (const FWEMarketOffer& Offer : Terminal->GetOffers())
	{
		UWETradeRowWidget* Row = AddRow(BuyList);
		Row->OfferIndex = Offer.OfferIndex;
		Row->Configure(this, EWETradeRowKind::Buy, Offer.Item.GetDisplayName(),
			FText::Format(LOCTEXT("OfferDetail", "{0} · {1} · noch {2}"), WEItems::GetConditionText(Offer.Item.Condition), Offer.TraderName, FText::AsNumber(Offer.Remaining)),
			WETradeStyle::Money(Offer.UnitPrice), LOCTEXT("BuyBtn", "Kaufen"), Offer.Remaining > 0 && Balance >= Offer.UnitPrice);
	}

	// Contracts.
	ContractList->ClearChildren();
	if (Contracts)
	{
		const float Now = Contracts->GetServerTime();
		for (const FWEContract& Contract : Contracts->GetContracts())
		{
			const bool bAccepted = Contract.State == EWEContractState::Accepted;
			const int32 Have = UWEContractComponent::CountDeliverable(Contract, Character);
			const int32 Minutes = FMath::Max(0, FMath::CeilToInt((Contract.ExpiresAt - Now) / 60.f));
			UWETradeRowWidget* Row = AddRow(ContractList);
			Row->ContractId = Contract.Id;
			Row->bContractAccepted = bAccepted;
			const FText ItemName = Contract.Item ? Contract.Item->DisplayName : FText::GetEmpty();
			Row->Configure(this, EWETradeRowKind::Contract, Contract.Title,
				FText::Format(LOCTEXT("ContractDetail", "{0}× {1}, mind. {2} · {3} · noch {4} min"), FText::AsNumber(Contract.Quantity), ItemName,
					WEItems::GetConditionText(Contract.MinCondition),
					bAccepted ? FText::Format(LOCTEXT("AcceptedBy", "angenommen von {0}"), FText::FromString(Contract.AcceptedBy)) : LOCTEXT("Open", "offen"),
					FText::AsNumber(Minutes)),
				WETradeStyle::Money(Contract.Reward),
				bAccepted ? FText::Format(LOCTEXT("Deliver", "Abliefern ({0}/{1})"), FText::AsNumber(FMath::Min(Have, Contract.Quantity)), FText::AsNumber(Contract.Quantity))
					: LOCTEXT("Accept", "Annehmen"),
				bAccepted ? Have >= Contract.Quantity : true);
		}
	}
	if (ContractList->GetChildrenCount() == 0)
	{
		AddRow(ContractList)->Configure(this, EWETradeRowKind::Info, LOCTEXT("NoContracts", "Gerade keine Aufträge"),
			LOCTEXT("NoContractsDetail", "Neue Anfragen kommen laufend herein."), FText::GetEmpty(), FText::GetEmpty(), false);
	}

	// Recent bookings.
	FString History = LOCTEXT("History", "Letzte Buchungen:").ToString();
	if (Company)
	{
		int32 Shown = 0;
		for (const FWETransactionRecord& Record : Company->GetRecentTransactions())
		{
			if (Shown++ >= 4)
			{
				break;
			}
			History += FString::Printf(TEXT("   %s%s %s"), Record.Amount.IsNegative() ? TEXT("") : TEXT("+"), *Record.Amount.ToDisplayString(), *Record.Description);
		}
		if (Shown == 0)
		{
			History += TEXT("   –");
		}
	}
	HistoryText->SetText(FText::FromString(History));
}

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
	// Rebuild only when something visible changed (keeps buttons stable under the mouse).
	const FString Signature = BuildSignature();
	if (Signature != LastSignature)
	{
		LastSignature = Signature;
		Rebuild();
	}
}

FReply UWETradeWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Tab || Key == EKeys::E)
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer()))
		{
			PC->CloseTrade();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UWETradeWidget::HandleRowAction(const UWETradeRowWidget* Row, int32 ButtonIndex)
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwningPlayer());
	const AWECharacter* Character = Cast<AWECharacter>(GetOwningPlayerPawn());
	if (!PC || !Row || !Terminal)
	{
		return;
	}
	switch (Row->Kind)
	{
	case EWETradeRowKind::Sell:
		PC->RequestSell(Terminal, Character ? Character->GetPlayerInventory() : nullptr, Row->InventorySlot, ButtonIndex == 1 ? 1 : 0);
		break;
	case EWETradeRowKind::SellHeld:
		PC->RequestSellHeld(Terminal);
		break;
	case EWETradeRowKind::Buy:
		PC->RequestBuy(Terminal, Row->OfferIndex, 1);
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
	default:
		break;
	}
}

#undef LOCTEXT_NAMESPACE
