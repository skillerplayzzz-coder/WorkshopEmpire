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
#include "Components/WidgetSwitcher.h"
#include "Economy/WEAuctionComponent.h"
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
#include "Progression/WEProgressionComponent.h"
#include "Progression/WEUpgradeDefinition.h"
#include "Styling/CoreStyle.h"
#include "Staff/WEEmployee.h"
#include "Staff/WEStaffComponent.h"
#include "Logistics/WEVehicle.h"
#include "Logistics/WEForklift.h"
#include "EngineUtils.h"

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
	TitleText->SetAutoWrapText(false);
	DetailText = WETradeStyle::Text(WidgetTree, 9, WETradeStyle::Soft);
	DetailText->SetAutoWrapText(false);
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
	// Fixed wrap widths (columns are 300 px): auto-wrap needs last frame's geometry, so new rows
	// were laid out with zero width for one frame and flashed on top of each other.
	const float Wrap = WrapOverride > 0.f ? WrapOverride : InKind == EWETradeRowKind::Auction ? 400.f : InKind == EWETradeRowKind::Info ? 270.f : (InButton2.IsEmpty() ? 125.f : 85.f);
	TitleText->SetWrapTextAt(Wrap);
	DetailText->SetWrapTextAt(Wrap);
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

UButton* UWETradeWidget::MakeTabButton(const FText& Label, TObjectPtr<UTextBlock>& OutText)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	OutText = WETradeStyle::Text(WidgetTree, 12, FLinearColor(0.05f, 0.05f, 0.05f), "Bold");
	OutText->SetText(Label);
	Button->SetContent(OutText);
	return Button;
}

void UWETradeWidget::ShowPage(int32 Page)
{
	CurrentPage = Page;
	if (Pages)
	{
		Pages->SetActiveWidgetIndex(Page);
	}
	if (TabMarketText && TabAuctionsText && TabStaffText)
	{
		TabMarketText->SetText(Page == 0 ? LOCTEXT("TabMarketOn", "▶ Marktplatz") : LOCTEXT("TabMarket", "Marktplatz"));
		TabAuctionsText->SetText(Page == 1 ? LOCTEXT("TabAuctionsOn", "▶ Auktionen & Beschaffung") : LOCTEXT("TabAuctions", "Auktionen & Beschaffung"));
		TabStaffText->SetText(Page == 2 ? LOCTEXT("TabStaffOn", "▶ Personal & Logistik") : LOCTEXT("TabStaff", "Personal & Logistik"));
	}
	LastSignature.Reset();
}

void UWETradeWidget::HandleTabMarket()
{
	ShowPage(0);
}

void UWETradeWidget::HandleTabAuctions()
{
	ShowPage(1);
}

void UWETradeWidget::HandleTabStaff()
{
	ShowPage(2);
}

UVerticalBox* UWETradeWidget::MakeColumn(const FText& Title, TObjectPtr<UScrollBox>& OutList, float Width)
{
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	UTextBlock* Header = WETradeStyle::Text(WidgetTree, 15, WETradeStyle::Gold, "Bold");
	Header->SetText(Title);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Header))
	{
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}
	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
	Box->SetWidthOverride(Width);
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
	UButton* TabMarket = MakeTabButton(LOCTEXT("TabMarket", "Marktplatz"), TabMarketText);
	UButton* TabAuctions = MakeTabButton(LOCTEXT("TabAuctions", "Auktionen & Beschaffung"), TabAuctionsText);
	UButton* TabStaff = MakeTabButton(LOCTEXT("TabStaff", "Personal & Logistik"), TabStaffText);
	TabMarket->OnClicked.AddDynamic(this, &UWETradeWidget::HandleTabMarket);
	TabAuctions->OnClicked.AddDynamic(this, &UWETradeWidget::HandleTabAuctions);
	TabStaff->OnClicked.AddDynamic(this, &UWETradeWidget::HandleTabStaff);
	for (UButton* Tab : { TabMarket, TabAuctions, TabStaff })
	{
		if (UHorizontalBoxSlot* S = Header->AddChildToHorizontalBox(Tab))
		{
			S->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
			S->SetVerticalAlignment(VAlign_Center);
		}
	}
	BalanceText = WETradeStyle::Text(WidgetTree, 18, WETradeStyle::Green, "Bold");
	if (UHorizontalBoxSlot* S = Header->AddChildToHorizontalBox(BalanceText))
	{
		S->SetPadding(FMargin(20.f, 0.f, 0.f, 0.f));
	}
	if (UVerticalBoxSlot* S = Outer->AddChildToVerticalBox(Header))
	{
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	}

	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
	UVerticalBox* SellColumn = MakeColumn(LOCTEXT("Sell", "Verkaufen"), SellList);
	UVerticalBox* BuyColumn = MakeColumn(LOCTEXT("Buy", "Kaufen"), BuyList);
	UVerticalBox* ContractColumn = MakeColumn(LOCTEXT("Contracts", "Kundenaufträge"), ContractList);
	UVerticalBox* UpgradeColumn = MakeColumn(LOCTEXT("Upgrades", "Werkstatt verbessern"), UpgradeList);
	for (UVerticalBox* Column : { SellColumn, BuyColumn, ContractColumn, UpgradeColumn })
	{
		if (UHorizontalBoxSlot* S = Columns->AddChildToHorizontalBox(Column))
		{
			S->SetPadding(FMargin(6.f, 0.f));
		}
	}
	// Page 2: auctions and the other sources (Phase 7).
	UHorizontalBox* AuctionColumns = WidgetTree->ConstructWidget<UHorizontalBox>();
	UVerticalBox* AuctionColumn = MakeColumn(LOCTEXT("Auctions", "Auktionen (Geschäftsauflösungen, Industrie, Nachlässe)"), AuctionList, 640.f);
	UVerticalBox* SourcesColumn = MakeColumn(LOCTEXT("Sources", "Weitere Quellen"), SourcesList, 560.f);
	for (UVerticalBox* Column : { AuctionColumn, SourcesColumn })
	{
		if (UHorizontalBoxSlot* S = AuctionColumns->AddChildToHorizontalBox(Column))
		{
			S->SetPadding(FMargin(6.f, 0.f));
		}
	}
	// Page 3: staff and logistics (Phase 11).
	UHorizontalBox* StaffColumns = WidgetTree->ConstructWidget<UHorizontalBox>();
	UVerticalBox* ApplicantColumn = MakeColumn(LOCTEXT("Applicants", "Bewerber"), ApplicantList, 350.f);
	UVerticalBox* StaffColumn = MakeColumn(LOCTEXT("Staff", "Mitarbeiter"), StaffList, 420.f);
	UVerticalBox* LogisticsColumn = MakeColumn(LOCTEXT("Logistics", "Fahrzeuge & Löhne"), LogisticsList, 340.f);
	for (UVerticalBox* Column : { ApplicantColumn, StaffColumn, LogisticsColumn })
	{
		if (UHorizontalBoxSlot* S = StaffColumns->AddChildToHorizontalBox(Column))
		{
			S->SetPadding(FMargin(6.f, 0.f));
		}
	}
	Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>();
	Pages->AddChild(Columns);
	Pages->AddChild(AuctionColumns);
	Pages->AddChild(StaffColumns);
	Outer->AddChildToVerticalBox(Pages);
	ShowPage(0);

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
	Sig << TEXT("|page:") << CurrentPage;
	if (const UWEAuctionComponent* Auctions = UWEAuctionComponent::Get(this))
	{
		const float AuctionNow = Auctions->GetServerTime();
		for (const FWEAuctionLot& Lot : Auctions->GetLots())
		{
			Sig << TEXT("|a:") << Lot.Id.ToString() << Lot.CurrentBid.GetCents() << (Lot.bCompanyLeads ? TEXT("L") : TEXT("-"));
			if (CurrentPage == 1)
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
		if (CurrentPage == 2)
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
		const bool bCustomer = Held->GetItem().IsCustomerItem();
		UWETradeRowWidget* Row = AddRow(SellList);
		Row->Configure(this, EWETradeRowKind::SellHeld,
			FText::Format(LOCTEXT("Held", "In der Hand: {0}"), Held->GetDisplayName()),
			bCustomer ? LOCTEXT("HeldCustomer", "Kundengerät – über den Reparaturauftrag abliefern")
			: Trader ? FText::Format(LOCTEXT("SellTo", "{0} · an {1}"), WEItems::GetConditionText(Held->GetItem().Condition), Trader->DisplayName)
				: LOCTEXT("NoBuyerDetail", "Kein Händler interessiert"),
			Trader && !bCustomer ? WETradeStyle::Money(Price) : FText::FromString(TEXT("—")),
			LOCTEXT("SellBtn", "Verkaufen"), Trader != nullptr && !bCustomer);
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
			FText::Format(LOCTEXT("OfferDetail", "{0}{1} · {2} · noch {3}"),
				Offer.Item.Definition->Rarity >= EWEItemRarity::Rare ? LOCTEXT("RareTag", "★ Selten · ") : FText::GetEmpty(),
				WEItems::GetConditionText(Offer.Item.Condition), Offer.TraderName, FText::AsNumber(Offer.Remaining)),
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
			const FText State = bAccepted ? FText::Format(LOCTEXT("AcceptedBy", "angenommen von {0}"), FText::FromString(Contract.AcceptedBy)) : LOCTEXT("Open", "offen");
			const FText Detail = Contract.Kind == EWEContractKind::Repair
				? FText::Format(bAccepted
						? LOCTEXT("RepairContractAccepted", "Reparieren: {0} (liegt auf der Palette), in der Hand abliefern · {1} · noch {2} min")
						: LOCTEXT("RepairContractOpen", "Reparieren: {0} · {1} · noch {2} min"),
					FText::Format(LOCTEXT("RepairItem", "{0}, {1}"), ItemName, WEItems::GetConditionText(Contract.CustomerCondition)), State, FText::AsNumber(Minutes))
				: FText::Format(LOCTEXT("ContractDetail", "{0}× {1}, mind. {2} · {3} · noch {4} min"), FText::AsNumber(Contract.Quantity), ItemName,
					WEItems::GetConditionText(Contract.MinCondition), State, FText::AsNumber(Minutes));
			Row->Configure(this, EWETradeRowKind::Contract, Contract.Title, Detail,
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

	// Workshop upgrades.
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
			Row->Configure(this, EWETradeRowKind::Upgrade, Upgrade.DisplayName,
				bOwned ? LOCTEXT("Owned", "✓ gekauft") : (Blocked.IsEmpty() ? Upgrade.Description : Blocked),
				bOwned ? FText::GetEmpty() : WETradeStyle::Money(Upgrade.Cost),
				bOwned ? FText::GetEmpty() : LOCTEXT("BuyUpgrade", "Kaufen"), !bOwned && Blocked.IsEmpty() && Balance >= Upgrade.Cost);
		}
	}

	RebuildAuctions(Balance);
	RebuildStaff(Balance);

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

void UWETradeWidget::RebuildAuctions(const FWEMoney& Balance)
{
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
			const FText Leader = Lot.NumBids == 0 ? LOCTEXT("NoBids", "noch kein Gebot")
				: Lot.bCompanyLeads ? FText::Format(LOCTEXT("YouLead", "ihr führt ({0})"), FText::FromString(Lot.LeaderName))
				: FText::Format(LOCTEXT("OtherLeads", "Höchstbietender: {0}"), FText::FromString(Lot.LeaderName));
			const FWEMoney Next = UWEAuctionComponent::GetNextBid(Lot);
			UWETradeRowWidget* Row = AddRow(AuctionList);
			Row->LotId = Lot.Id;
			Row->Configure(this, EWETradeRowKind::Auction, Lot.Title,
				FText::Format(LOCTEXT("LotDetail", "{0}\nSchätzwert {1} · {2} · noch {3}:{4}"), FText::FromString(FString::Join(Parts, TEXT(", "))),
					WETradeStyle::Money(Lot.EstimatedValue), Leader, FText::AsNumber(Seconds / 60), FText::FromString(FString::Printf(TEXT("%02d"), Seconds % 60))),
				Lot.NumBids == 0 ? FText::Format(LOCTEXT("StartPrice", "ab {0}"), WETradeStyle::Money(Lot.CurrentBid)) : WETradeStyle::Money(Lot.CurrentBid),
				Lot.bCompanyLeads ? LOCTEXT("Leading", "Führend") : FText::Format(LOCTEXT("BidBtn", "Bieten ({0})"), WETradeStyle::Money(Next)),
				!Lot.bCompanyLeads && Balance >= Next);
		}
	}
	if (AuctionList->GetChildrenCount() == 0)
	{
		AddRow(AuctionList)->Configure(this, EWETradeRowKind::Info, LOCTEXT("NoAuctions", "Gerade keine Auktionen"),
			LOCTEXT("NoAuctionsDetail", "Neue Posten kommen laufend herein."), FText::GetEmpty(), FText::GetEmpty(), false);
	}

	// Other sources and recent auction results.
	auto Info = [this](const FText& Title, const FText& Detail)
	{
		UWETradeRowWidget* Row = AddRow(SourcesList);
		Row->WrapOverride = 520.f;
		Row->Configure(this, EWETradeRowKind::Info, Title, Detail, FText::GetEmpty(), FText::GetEmpty(), false);
	};
	Info(LOCTEXT("SrcRules", "So funktionieren Auktionen"),
		LOCTEXT("SrcRulesDetail", "Ein Mitbieter hat ein geheimes Limit. Bezahlt wird erst beim Zuschlag; reicht das Geld dann nicht, verfällt er. Gebote kurz vor Schluss verlängern um 20 s. Gewonnenes steht auf der Lieferpalette."));
	Info(LOCTEXT("SrcScrap", "Schrotthaufen hinter dem Tor"),
		LOCTEXT("SrcScrapDetail", "Was dort liegt, ist gratis. Alle paar Minuten kommt neuer Schrott – manchmal ist etwas Wertvolles dabei."));
	Info(LOCTEXT("SrcPallets", "Retourenpaletten (Reiter Marktplatz → Kaufen)"),
		LOCTEXT("SrcPalletsDetail", "Unbekannter Inhalt. Auf der Lieferpalette mit [Q] öffnen."));
	Info(LOCTEXT("SrcRotating", "Gebrauchtwaren"),
		LOCTEXT("SrcRotatingDetail", "Das Angebot der Händler wechselt bei jedem Nachschub: Zustand, Qualität und seltene Stücke."));
	if (Auctions)
	{
		for (const FString& Result : Auctions->GetRecentResults())
		{
			Info(LOCTEXT("SrcResult", "Auktionsergebnis"), FText::FromString(Result));
		}
	}
}

void UWETradeWidget::RebuildStaff(const FWEMoney& Balance)
{
	ApplicantList->ClearChildren();
	StaffList->ClearChildren();
	LogisticsList->ClearChildren();
	const UWEStaffComponent* Staff = UWEStaffComponent::Get(this);
	if (!Staff)
	{
		return;
	}
	const TArray<AWEEmployee*> Employees = Staff->GetEmployees();
	const int32 MaxEmployees = Staff->GetCatalog() ? Staff->GetCatalog()->MaxEmployees : 6;
	const TArray<FWEEmployeeProfile>& Applicants = Staff->GetApplicants();
	for (int32 i = 0; i < Applicants.Num(); ++i)
	{
		const FWEEmployeeProfile& A = Applicants[i];
		UWETradeRowWidget* Row = AddRow(ApplicantList);
		Row->OfferIndex = i;
		Row->WrapOverride = 165.f;
		Row->Configure(this, EWETradeRowKind::Applicant,
			FText::Format(LOCTEXT("ApplicantTitle", "{0} – {1}"), FText::FromString(A.Name), AWEEmployee::GetRoleText(A.Role)),
			FText::Format(LOCTEXT("ApplicantDetail", "Erfahrung {0}/3 · {1}"), FText::AsNumber(A.Skill), Staff->GetRoleDescription(A.Role)),
			FText::Format(LOCTEXT("PerMinute", "{0}/min"), WETradeStyle::Money(FWEMoney::FromCents(A.WageCentsPerMinute))),
			LOCTEXT("Hire", "Einstellen"), Employees.Num() < MaxEmployees);
	}
	for (AWEEmployee* Employee : Employees)
	{
		UWETradeRowWidget* Row = AddRow(StaffList);
		Row->Employee = Employee;
		Row->bEmployeePaused = Employee->IsPaused();
		Row->WrapOverride = 150.f;
		Row->Configure(this, EWETradeRowKind::Employee,
			FText::Format(LOCTEXT("EmployeeTitle", "{0} – {1}"), FText::FromString(Employee->GetProfile().Name), Employee->GetRoleText()),
			FText::Format(LOCTEXT("EmployeeDetail", "{0}\n{1}"), Employee->GetTaskText(), Employee->GetStateText()),
			Employee->GetWageText(),
			Employee->IsPaused() ? LOCTEXT("ResumeBtn", "Arbeiten") : LOCTEXT("PauseBtn", "Pause"), Employee->GetState() != EWEEmployeeState::Away,
			LOCTEXT("FireBtn", "Entlassen"), Employee->GetState() != EWEEmployeeState::Away);
	}
	if (Employees.Num() == 0)
	{
		UWETradeRowWidget* Row = AddRow(StaffList);
		Row->WrapOverride = 400.f;
		Row->Configure(this, EWETradeRowKind::Info, LOCTEXT("NoStaff", "Noch keine Mitarbeiter"),
			LOCTEXT("NoStaffDetail", "Neue Mitarbeiter warten am Tor. Aufgaben verteilt ihr in der Werkstatt: [E] auf den Mitarbeiter, dann Quelle und Ziel anschauen und [E]."),
			FText::GetEmpty(), FText::GetEmpty(), false);
	}

	auto Info = [this](const FText& Title, const FText& Detail)
	{
		UWETradeRowWidget* Row = AddRow(LogisticsList);
		Row->WrapOverride = 350.f;
		Row->Configure(this, EWETradeRowKind::Info, Title, Detail, FText::GetEmpty(), FText::GetEmpty(), false);
	};
	const int32 PayrollSeconds = FMath::CeilToInt(Staff->GetSecondsToPayroll());
	Info(LOCTEXT("Wages", "Löhne"), FText::Format(LOCTEXT("WagesDetail", "{0} pro Minute · nächste Abrechnung in {1}:{2}{3}"),
		WETradeStyle::Money(Staff->GetWagesPerMinute()), FText::AsNumber(PayrollSeconds / 60), FText::FromString(FString::Printf(TEXT("%02d"), PayrollSeconds % 60)),
		Staff->GetOwedWages().IsZero() ? FText::GetEmpty() : FText::Format(LOCTEXT("Owed", "\nAusstehend: {0} – ohne Lohn wird nicht gearbeitet"), WETradeStyle::Money(Staff->GetOwedWages()))));
	for (TActorIterator<AWEVehicle> It(GetWorld()); It; ++It)
	{
		if (!It->IsUnlocked())
		{
			Info(LOCTEXT("VanLocked", "Firmenwagen"), LOCTEXT("VanLockedDetail", "Noch keiner. Unter \"Werkstatt verbessern\": Alter Transporter."));
			continue;
		}
		const FWETourDefinition* Tour = It->FindTour(It->IsAway() ? It->GetActiveTour() : It->GetSelectedTour());
		const int32 Seconds = FMath::CeilToInt(It->GetSecondsToReturn());
		const AWEEmployee* Driver = It->FindDriver();
		Info(It->GetStats().DisplayName, It->IsAway()
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
	Info(LOCTEXT("Forklift", "Gabelstapler"), bForklift ? LOCTEXT("ForkliftOwned", "In der Werkstatt: [E] aufsteigen, hebt bis 1.500 kg, [X] absteigen.")
		: LOCTEXT("ForkliftLocked", "Noch keiner. Unter \"Werkstatt verbessern\": Gabelstapler."));
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
		// New rows have no size until the next layout pass; without this they flash stacked for one frame.
		ForceLayoutPrepass();
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
			PC->RequestStaffAction(Terminal, Employee, ButtonIndex == 2 ? AWEPlayerController::StaffFire
				: Row->bEmployeePaused ? AWEPlayerController::StaffResume : AWEPlayerController::StaffPause);
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
	default:
		break;
	}
}

#undef LOCTEXT_NAMESPACE
