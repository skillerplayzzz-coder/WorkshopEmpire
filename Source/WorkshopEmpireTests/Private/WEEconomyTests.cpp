#include "Misc/AutomationTest.h"

#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WEMarketComponent.h"
#include "Economy/WETraderDefinition.h"
#include "Economy/WETraderTerminal.h"
#include "EngineUtils.h"
#include "Framework/WEGameState.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "Save/WESaveGame.h"
#include "Save/WESaveSystem.h"
#include "WETestItems.h"
#include "WETestWorld.h"
#include "Workshop/WEWorkbench.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace WEEconomyTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	struct FEconomyFixture
	{
		FWETestWorld TW;
		AWEGameState* GameState = nullptr;
		UWECompanyComponent* Company = nullptr;
		UWEMarketComponent* Market = nullptr;
		UWEContractComponent* Contracts = nullptr;
		AWETraderTerminal* Terminal = nullptr;
		UWEItemDefinition* Screws = nullptr;
		UWEItemDefinition* Ram = nullptr;
		UWEItemDefinition* Pc = nullptr;
		UWETraderDefinition* Trader = nullptr;

		FEconomyFixture()
		{
			using namespace WETestItems;
			GameState = TW.SpawnGameState<AWEGameState>();
			Company = GameState->GetCompany();
			Market = GameState->GetMarket();
			Contracts = GameState->GetContracts();
			Contracts->SetCatalog(nullptr);
			Contracts->SetContracts({});

			Screws = MakeDefinition(TEXT("E_Screws"), 0.005f, true, 200, true, 100, EWEItemCategory::SparePart);   // 1,00 € new
			Ram = MakeDefinition(TEXT("E_Ram"), 0.03f, false, 1, true, 1000, EWEItemCategory::Electronics);       // 10,00 € new
			Pc = MakeDefinition(TEXT("E_Pc"), 9.f, false, 1, false, 4000, EWEItemCategory::Electronics);          // 40,00 € new, bulky

			Trader = NewObject<UWETraderDefinition>(GetTransientPackage());
			Trader->DisplayName = FText::FromString(TEXT("Testhändler"));
			Trader->Buys = { { EWEItemCategory::SparePart, 1.f }, { EWEItemCategory::Electronics, 1.f } };
			FWETraderStockEntry RamOffer;
			RamOffer.Item = Ram;
			RamOffer.Quantity = 2;
			RamOffer.Condition = EWEItemCondition::New;
			RamOffer.Quality = 50;
			FWETraderStockEntry PcOffer;
			PcOffer.Item = Pc;
			PcOffer.Quantity = 1;
			PcOffer.Condition = EWEItemCondition::Defective;
			PcOffer.Quality = 50;
			Trader->Stock = { RamOffer, PcOffer };
			Trader->RestockMinutes = 0.f;

			Terminal = TW.Spawn<AWETraderTerminal>(FVector(150.f, 0.f, 0.f));
			Terminal->SetTraders({ Trader });
			Company->SetBalance(FWEMoney::FromEuros(250));
		}

		void Give(AWECharacter* C, UWEItemDefinition* Def, int32 Qty, EWEItemCondition Condition = EWEItemCondition::Used)
		{
			int32 Added = 0;
			C->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(Def, Qty, Condition, 50), Added);
		}
	};
}

using namespace WEEconomyTests;
using namespace WETestItems;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWECompanyTest, "WorkshopEmpire.Economy.CompanyAccount", Flags)
bool FWECompanyTest::RunTest(const FString& Parameters)
{
	FEconomyFixture F;
	TestEqual(TEXT("Start capital 250 EUR"), F.Company->GetBalance(), FWEMoney::FromEuros(250));
	TestFalse(TEXT("Cannot spend more than available"), F.Company->TryWithdraw(FWEMoney::FromCents(25001), TEXT("zu viel")));
	TestEqual(TEXT("Unchanged after refused withdraw"), F.Company->GetBalance(), FWEMoney::FromEuros(250));
	TestFalse(TEXT("Negative deposit rejected"), F.Company->Deposit(FWEMoney::FromCents(-5), TEXT("negativ")));
	TestFalse(TEXT("Negative withdraw rejected"), F.Company->TryWithdraw(FWEMoney::FromCents(-5), TEXT("negativ")));
	TestTrue(TEXT("Exact withdraw"), F.Company->TryWithdraw(FWEMoney::FromEuros(250), TEXT("alles")));
	TestTrue(TEXT("Balance zero, never negative"), F.Company->GetBalance().IsZero());
	TestTrue(TEXT("Deposit"), F.Company->Deposit(FWEMoney::FromCents(1234), TEXT("Verkauf")));
	TestEqual(TEXT("Records newest first"), F.Company->GetRecentTransactions()[0].Amount.GetCents(), int64(1234));
	TestEqual(TEXT("Withdrawals recorded negative"), F.Company->GetRecentTransactions()[1].Amount.GetCents(), int64(-25000));

	const FGuid Id = FGuid::NewGuid();
	TestTrue(TEXT("New request accepted"), F.Company->RegisterRequest(Id));
	AddExpectedMessage(TEXT("duplicate or invalid request"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2, false);
	TestFalse(TEXT("Same request id rejected"), F.Company->RegisterRequest(Id));
	TestFalse(TEXT("Invalid request id rejected"), F.Company->RegisterRequest(FGuid()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEPricingTest, "WorkshopEmpire.Economy.PricingRules", Flags)
bool FWEPricingTest::RunTest(const FString& Parameters)
{
	FEconomyFixture F;
	const FWEItemInstance NewRam = FWEItemInstance::Make(F.Ram, 1, EWEItemCondition::New, 50);
	const FWEItemInstance UsedRam = FWEItemInstance::Make(F.Ram, 1, EWEItemCondition::Used, 50);
	const FWEItemInstance ScrapRam = FWEItemInstance::Make(F.Ram, 1, EWEItemCondition::Scrap, 50);

	TestEqual(TEXT("Market value of new RAM = 10,00 EUR at demand 1"), UWEMarketComponent::GetMarketValue(NewRam, F.Market).GetCents(), int64(1000));
	TestEqual(TEXT("Trader pays 60 % = 6,00 EUR"), UWEMarketComponent::GetSellPrice(NewRam, F.Trader, F.Market).GetCents(), int64(600));
	TestTrue(TEXT("Better condition sells for more"), UWEMarketComponent::GetSellPrice(NewRam, F.Trader, F.Market) > UWEMarketComponent::GetSellPrice(UsedRam, F.Trader, F.Market)
		&& UWEMarketComponent::GetSellPrice(UsedRam, F.Trader, F.Market) > UWEMarketComponent::GetSellPrice(ScrapRam, F.Trader, F.Market));
	TestTrue(TEXT("Buying costs more than selling (no arbitrage)"), UWEMarketComponent::GetBuyPrice(NewRam, 1.f, F.Market) > UWEMarketComponent::GetSellPrice(NewRam, F.Trader, F.Market));

	F.Market->SetDemand(EWEItemCategory::Electronics, 1.2f);
	TestEqual(TEXT("Demand 1.2 raises the price"), UWEMarketComponent::GetSellPrice(NewRam, F.Trader, F.Market).GetCents(), int64(720));
	F.Market->SetDemand(EWEItemCategory::Electronics, 5.f);
	TestEqual(TEXT("Demand is clamped to 1.2"), F.Market->GetDemand(EWEItemCategory::Electronics), 1.2f);
	F.Market->SetDemand(EWEItemCategory::Electronics, 1.f);
	for (int32 i = 0; i < 200; ++i)
	{
		F.Market->DriftDemand();
	}
	const float Drifted = F.Market->GetDemand(EWEItemCategory::Electronics);
	TestTrue(TEXT("Drift stays within 0.8-1.2"), Drifted >= 0.8f && Drifted <= 1.2f);

	const FWEItemInstance Toolish = FWEItemInstance::Make(MakeDefinition(TEXT("E_Hammer"), 1.f, false, 1, true, 500, EWEItemCategory::Tool));
	TestTrue(TEXT("Trader does not buy categories it does not list"), UWEMarketComponent::GetSellPrice(Toolish, F.Trader, F.Market).IsZero());
	FWEItemInstance Half = NewRam;
	Half.DisassemblyStep = 1;
	TestTrue(TEXT("Half-disassembled items are not bought"), UWEMarketComponent::GetSellPrice(Half, F.Trader, F.Market).IsZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWESellTest, "WorkshopEmpire.Economy.SellFromInventory", Flags)
bool FWESellTest::RunTest(const FString& Parameters)
{
	FEconomyFixture F;
	AWECharacter* A = SpawnPlayer(F.TW, FVector(0.f, 0.f, 100.f));
	AWECharacter* B = SpawnPlayer(F.TW, FVector(0.f, 150.f, 100.f));
	F.Give(A, F.Screws, 10, EWEItemCondition::New);

	// 4 new screws at 1,00 EUR, 60 % -> 2,40 EUR.
	const FWEMoney Before = F.Company->GetBalance();
	TestEqual(TEXT("Sell 4"), F.Terminal->SellFromInventory(A, A->GetPlayerInventory(), 0, 4, FGuid::NewGuid()), EWETradeResult::Success);
	TestEqual(TEXT("6 left"), A->GetPlayerInventory()->CountItems(F.Screws), 6);
	TestEqual(TEXT("+2,40 EUR"), F.Company->GetBalance().GetCents() - Before.GetCents(), int64(240));

	// The same request twice: booked once.
	const FGuid Same = FGuid::NewGuid();
	AddExpectedMessage(TEXT("duplicate or invalid request"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
	TestEqual(TEXT("First"), F.Terminal->SellFromInventory(A, A->GetPlayerInventory(), 0, 1, Same), EWETradeResult::Success);
	TestEqual(TEXT("Repeat ignored"), F.Terminal->SellFromInventory(A, A->GetPlayerInventory(), 0, 1, Same), EWETradeResult::Duplicate);
	TestEqual(TEXT("Only one screw gone"), A->GetPlayerInventory()->CountItems(F.Screws), 5);

	TestEqual(TEXT("B cannot sell A's items"), F.Terminal->SellFromInventory(B, A->GetPlayerInventory(), 0, 1, FGuid::NewGuid()), EWETradeResult::InvalidItem);
	A->SetActorLocation(FVector(3000.f, 0.f, 100.f));
	TestEqual(TEXT("Too far from the terminal"), F.Terminal->SellFromInventory(A, A->GetPlayerInventory(), 0, 1, FGuid::NewGuid()), EWETradeResult::OutOfReach);
	TestEqual(TEXT("Nothing sold from afar"), A->GetPlayerInventory()->CountItems(F.Screws), 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEBuyTest, "WorkshopEmpire.Economy.BuyStockAndDelivery", Flags)
bool FWEBuyTest::RunTest(const FString& Parameters)
{
	FEconomyFixture F;
	AWECharacter* A = SpawnPlayer(F.TW, FVector(0.f, 0.f, 100.f));
	AWEDeliveryZone* Zone = F.TW.Spawn<AWEDeliveryZone>(FVector(0.f, -300.f, 6.f));

	const TArray<FWEMarketOffer> Offers = F.Terminal->GetOffers();
	TestEqual(TEXT("Two offers"), Offers.Num(), 2);
	TestEqual(TEXT("RAM: 10,00 x 1.3 = 13,00 EUR"), Offers[0].UnitPrice.GetCents(), int64(1300));

	const FWEMoney Before = F.Company->GetBalance();
	TestEqual(TEXT("Buy RAM"), F.Terminal->Buy(A, 0, 1, FGuid::NewGuid()), EWETradeResult::Success);
	TestEqual(TEXT("Paid 13,00 EUR"), Before.GetCents() - F.Company->GetBalance().GetCents(), int64(1300));
	TestEqual(TEXT("RAM in the inventory"), A->GetPlayerInventory()->CountItems(F.Ram), 1);
	TestEqual(TEXT("Stock 2 -> 1"), F.Terminal->GetStockRemaining()[0], 1);

	TestEqual(TEXT("Buy bulky PC"), F.Terminal->Buy(A, 1, 1, FGuid::NewGuid()), EWETradeResult::Success);
	int32 PcsInWorld = 0;
	for (TActorIterator<AWEPickupItem> It(F.TW.World); It; ++It)
	{
		PcsInWorld += It->GetItem().Definition == F.Pc ? 1 : 0;
		TestTrue(TEXT("Delivered near the pallet"), FVector::Dist2D(It->GetActorLocation(), Zone->GetActorLocation()) < 100.f);
	}
	TestEqual(TEXT("PC delivered to the pallet"), PcsInWorld, 1);
	TestEqual(TEXT("Second PC: out of stock"), F.Terminal->Buy(A, 1, 1, FGuid::NewGuid()), EWETradeResult::OutOfStock);

	F.Company->SetBalance(FWEMoney::FromCents(500));
	TestEqual(TEXT("Not enough money"), F.Terminal->Buy(A, 0, 1, FGuid::NewGuid()), EWETradeResult::NotEnoughMoney);
	TestEqual(TEXT("Money untouched"), F.Company->GetBalance().GetCents(), int64(500));
	TestEqual(TEXT("Stock untouched"), F.Terminal->GetStockRemaining()[0], 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWESellHeldTest, "WorkshopEmpire.Economy.SellCarriedItem", Flags)
bool FWESellHeldTest::RunTest(const FString& Parameters)
{
	FEconomyFixture F;
	AWECharacter* A = SpawnPlayer(F.TW, FVector(0.f, 0.f, 100.f));
	AWEPickupItem* Pc = SpawnItem(F.TW, F.Pc, 1, FVector(60.f, -80.f, 30.f), EWEItemCondition::Defective);
	TestTrue(TEXT("Carry"), A->TryPickUpItem(Pc));
	const FWEMoney Expected = F.Terminal->GetBestSellPrice(Pc->GetItem());
	const FWEMoney Before = F.Company->GetBalance();
	TestEqual(TEXT("Sell carried PC"), F.Terminal->SellHeldItem(A, FGuid::NewGuid()), EWETradeResult::Success);
	TestNull(TEXT("Hands empty"), A->GetHeldItem());
	TestFalse(TEXT("Item gone"), IsValid(Pc));
	TestEqual(TEXT("Paid best price"), F.Company->GetBalance().GetCents() - Before.GetCents(), Expected.GetCents());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEContractTest, "WorkshopEmpire.Economy.CustomerOrders", Flags)
bool FWEContractTest::RunTest(const FString& Parameters)
{
	FEconomyFixture F;
	AWECharacter* A = SpawnPlayer(F.TW, FVector(0.f, 0.f, 100.f));

	UWEContractCatalog* Catalog = NewObject<UWEContractCatalog>(GetTransientPackage());
	FWEContractTemplate Template;
	Template.Title = FText::FromString(TEXT("Testkunde sucht RAM"));
	Template.Item = F.Ram;
	Template.MinQuantity = 2;
	Template.MaxQuantity = 2;
	Template.MinCondition = EWEItemCondition::Damaged;
	Template.RewardFactor = 1.5f;
	Template.DurationMinutes = 10.f;
	Catalog->Templates = { Template };
	F.Contracts->SetCatalog(Catalog);
	F.Contracts->UpdateContracts();

	TestEqual(TEXT("Board filled to 3 offers"), F.Contracts->GetContracts().Num(), 3);
	const FWEContract Offer = F.Contracts->GetContracts()[0];
	// Reward: 2 RAM at "Damaged", quality 50 = 2 x 10,00 x 0.5 = 10,00; x1.5 = 15,00 EUR.
	TestEqual(TEXT("Reward from market value"), Offer.Reward.GetCents(), int64(1500));

	TestEqual(TEXT("Deliver before accepting fails"), F.Terminal->DeliverContract(A, Offer.Id, FGuid::NewGuid()), EWETradeResult::ContractFailed);
	TestEqual(TEXT("Accept"), F.Terminal->AcceptContract(A, Offer.Id, FGuid::NewGuid()), EWETradeResult::Success);
	TestEqual(TEXT("State accepted"), F.Contracts->FindContract(Offer.Id)->State, EWEContractState::Accepted);

	F.Give(A, F.Ram, 1, EWEItemCondition::Used);
	F.Give(A, F.Ram, 1, EWEItemCondition::Scrap); // too bad to count
	TestEqual(TEXT("Only 1 deliverable"), UWEContractComponent::CountDeliverable(*F.Contracts->FindContract(Offer.Id), A), 1);
	TestEqual(TEXT("Missing items"), F.Terminal->DeliverContract(A, Offer.Id, FGuid::NewGuid()), EWETradeResult::ContractFailed);

	F.Give(A, F.Ram, 1, EWEItemCondition::Damaged);
	const FWEMoney Before = F.Company->GetBalance();
	TestEqual(TEXT("Deliver"), F.Terminal->DeliverContract(A, Offer.Id, FGuid::NewGuid()), EWETradeResult::Success);
	TestEqual(TEXT("Reward paid"), F.Company->GetBalance().GetCents() - Before.GetCents(), int64(1500));
	TestEqual(TEXT("Two good RAM taken, scrap one stays"), A->GetPlayerInventory()->CountItems(F.Ram), 1);
	TestNull(TEXT("Contract finished"), F.Contracts->FindContract(Offer.Id));

	// Limit of accepted orders and expiry.
	F.Contracts->UpdateContracts();
	for (const FWEContract& C : F.Contracts->GetContracts())
	{
		F.Contracts->Accept(C.Id, TEXT("Test"));
	}
	F.Contracts->UpdateContracts();
	const FGuid Extra = F.Contracts->GetContracts().Last().Id;
	TestEqual(TEXT("At most 3 accepted orders"), F.Contracts->Accept(Extra, TEXT("Test")), EWEContractResult::TooManyAccepted);

	TArray<FWEContract> Expiring = F.Contracts->GetContracts();
	for (FWEContract& C : Expiring)
	{
		C.ExpiresAt = F.Contracts->GetServerTime() - 1.f;
	}
	F.Contracts->SetContracts(Expiring);
	F.Contracts->UpdateContracts();
	for (const FWEContract& C : F.Contracts->GetContracts())
	{
		TestEqual(TEXT("Expired orders removed, only fresh offers remain"), C.State, EWEContractState::Offered);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEEconomyBalanceTest, "WorkshopEmpire.Economy.BalanceDefectivePcIsProfitable", Flags)
bool FWEEconomyBalanceTest::RunTest(const FString& Parameters)
{
	// Uses the real game data: buy a defective PC, disassemble it at the workbench, sell all parts.
	// Spec §10.5: small profits early on, no runaway. Averaged over many seeds.
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	GameState->GetContracts()->SetCatalog(nullptr);
	UWEItemDefinition* Pc = UWEItemDefinition::FindById(TEXT("OldComputer"));
	UWEItemDefinition* Screwdriver = UWEItemDefinition::FindById(TEXT("Screwdriver"));
	UWEItemDefinition* Cutter = UWEItemDefinition::FindById(TEXT("SideCutter"));
	UWETraderDefinition* UsedGoods = LoadObject<UWETraderDefinition>(nullptr, TEXT("/Game/WorkshopEmpire/Data/Economy/DA_Trader_UsedGoods.DA_Trader_UsedGoods"));
	UWETraderDefinition* Scrapyard = LoadObject<UWETraderDefinition>(nullptr, TEXT("/Game/WorkshopEmpire/Data/Economy/DA_Trader_Scrapyard.DA_Trader_Scrapyard"));
	if (!TestNotNull(TEXT("PC data"), Pc) || !TestNotNull(TEXT("Screwdriver data"), Screwdriver) || !TestNotNull(TEXT("Cutter data"), Cutter)
		|| !TestNotNull(TEXT("Trader data"), UsedGoods) || !TestNotNull(TEXT("Scrapyard data"), Scrapyard))
	{
		return false;
	}

	AWETraderTerminal* Terminal = TW.Spawn<AWETraderTerminal>(FVector(0.f, 400.f, 0.f));
	Terminal->SetTraders({ Scrapyard, UsedGoods });
	int32 PcOffer = INDEX_NONE;
	for (const FWEMarketOffer& Offer : Terminal->GetOffers())
	{
		PcOffer = Offer.Item.Definition == Pc ? Offer.OfferIndex : PcOffer;
	}
	if (!TestTrue(TEXT("PC is on offer"), PcOffer != INDEX_NONE))
	{
		return false;
	}
	const FWEMoney Cost = Terminal->GetOffers()[PcOffer].UnitPrice;

	AWECharacter* Player = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	int32 Added = 0;
	Player->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(Screwdriver), Added);
	Player->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(Cutter), Added);
	AWEWorkbench* Bench = TW.Spawn<AWEWorkbench>(FVector(150.f, 0.f, 0.f));

	constexpr int32 Runs = 30;
	int64 TotalRevenue = 0;
	for (int32 Run = 0; Run < Runs; ++Run)
	{
		Bench->GetStorage()->ClearAll();
		Bench->SetRandomSeed(1000 + Run);
		AWEPickupItem* Item = SpawnItem(TW, Pc, 1, FVector(100.f, -100.f, 30.f), EWEItemCondition::Defective);
		Bench->PlaceItem(Item);
		while (Bench->GetPlacedItem())
		{
			if (!Bench->TryStartStep(Player))
			{
				break;
			}
			Bench->CompleteActiveStep();
		}
		for (const FWEInventorySlotData& Slot : Bench->GetStorage()->GetAllSlots())
		{
			TotalRevenue += Terminal->GetBestSellPrice(Slot.Item).GetCents();
		}
	}
	const double AverageRevenue = TotalRevenue / static_cast<double>(Runs) / 100.0;
	const double Profit = AverageRevenue - Cost.GetCents() / 100.0;
	AddInfo(FString::Printf(TEXT("Defective PC: cost %.2f EUR, average parts revenue %.2f EUR, profit %.2f EUR"), Cost.GetCents() / 100.0, AverageRevenue, Profit));
	TestTrue(TEXT("Parting out a defective PC is profitable on average"), Profit > 0.5);
	TestTrue(TEXT("...but only a small profit early on (< 30 EUR)"), Profit < 30.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEEconomySaveTest, "WorkshopEmpire.Save.EconomyRoundTrip", Flags)
bool FWEEconomySaveTest::RunTest(const FString& Parameters)
{
	FEconomyFixture F;
	AWECharacter* A = SpawnPlayer(F.TW, FVector(0.f, 0.f, 100.f));
	F.Company->SetBalance(FWEMoney::FromCents(31415));
	F.Market->SetDemand(EWEItemCategory::Electronics, 1.15f);
	F.Terminal->Buy(A, 0, 1, FGuid::NewGuid());
	const int64 Balance = F.Company->GetBalance().GetCents();

	UWEContractCatalog* Catalog = NewObject<UWEContractCatalog>(GetTransientPackage());
	FWEContractTemplate Template;
	Template.Title = FText::FromString(TEXT("Speichertest"));
	Template.Item = F.Ram;
	Catalog->Templates = { Template };
	F.Contracts->SetCatalog(Catalog);
	F.Contracts->AddOffer(0);
	const FGuid ContractId = F.Contracts->GetContracts()[0].Id;
	F.Contracts->Accept(ContractId, TEXT("Mike"));

	UWESaveGame* Save = FWESaveSystem::Capture(F.TW.World);
	TestEqual(TEXT("Current save version"), Save->SaveVersion, UWESaveGame::CurrentVersion);

	F.Company->SetBalance(FWEMoney::FromEuros(1));
	F.Market->SetDemand(EWEItemCategory::Electronics, 1.f);
	F.Terminal->Restock();
	F.Contracts->SetContracts({});
	FWESaveSystem::Apply(F.TW.World, Save);

	TestEqual(TEXT("Balance restored"), F.Company->GetBalance().GetCents(), Balance);
	TestEqual(TEXT("Demand restored"), F.Market->GetDemand(EWEItemCategory::Electronics), 1.15f);
	TestEqual(TEXT("Stock restored (one RAM sold)"), F.Terminal->GetStockRemaining()[0], 1);
	const FWEContract* Restored = F.Contracts->FindContract(ContractId);
	TestTrue(TEXT("Accepted order restored"), Restored && Restored->State == EWEContractState::Accepted && Restored->AcceptedBy == TEXT("Mike"));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
