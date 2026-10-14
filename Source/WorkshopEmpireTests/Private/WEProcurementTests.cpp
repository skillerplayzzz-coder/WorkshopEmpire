#include "Misc/AutomationTest.h"

#include "Economy/WEAuctionComponent.h"
#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WEMarketComponent.h"
#include "Economy/WETraderDefinition.h"
#include "Economy/WETraderTerminal.h"
#include "EngineUtils.h"
#include "Framework/WEGameState.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WELootTable.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "Save/WESaveGame.h"
#include "Save/WESaveSystem.h"
#include "WETestItems.h"
#include "WETestWorld.h"
#include "World/WEScrapPile.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace WETestItems;

namespace WEProcurementTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	UWETraderDefinition* LoadTrader(const TCHAR* Name)
	{
		return LoadObject<UWETraderDefinition>(nullptr, *FString::Printf(TEXT("/Game/WorkshopEmpire/Data/Economy/%s.%s"), Name, Name));
	}

	UWELootTable* LoadLoot(const TCHAR* Name)
	{
		return LoadObject<UWELootTable>(nullptr, *FString::Printf(TEXT("/Game/WorkshopEmpire/Data/Loot/%s.%s"), Name, Name));
	}

	TArray<AWEPickupItem*> ItemsIn(UWorld* World)
	{
		TArray<AWEPickupItem*> Result;
		for (TActorIterator<AWEPickupItem> It(World); It; ++It)
		{
			if (IsValid(*It) && !It->IsActorBeingDestroyed())
			{
				Result.Add(*It);
			}
		}
		return Result;
	}

	/** All five real traders, like the terminal in the garage. */
	AWETraderTerminal* SpawnRealTerminal(FWETestWorld& TW, const FVector& Location)
	{
		AWETraderTerminal* Terminal = TW.Spawn<AWETraderTerminal>(Location);
		Terminal->SetTraders({ LoadTrader(TEXT("DA_Trader_Scrapyard")), LoadTrader(TEXT("DA_Trader_UsedGoods")), LoadTrader(TEXT("DA_Trader_ToolShop")),
			LoadTrader(TEXT("DA_Trader_SpareParts")), LoadTrader(TEXT("DA_Trader_Returns")) });
		return Terminal;
	}
}

using namespace WEProcurementTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEReturnPalletTest, "WorkshopEmpire.Procurement.ReturnPalletBuyAndOpen", Flags)
bool FWEReturnPalletTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	GameState->GetContracts()->SetCatalog(nullptr);
	GameState->GetCompany()->SetBalance(FWEMoney::FromEuros(250));
	AWEDeliveryZone* Pallet = TW.Spawn<AWEDeliveryZone>(FVector(150.f, -150.f, 6.f));
	AWETraderTerminal* Terminal = SpawnRealTerminal(TW, FVector(0.f, 250.f, 0.f));
	AWECharacter* Player = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	UWEItemDefinition* PalletDef = UWEItemDefinition::FindById(TEXT("ReturnPalletElectronics"));
	if (!TestNotNull(TEXT("Pallet data"), PalletDef) || !TestTrue(TEXT("Pallet is openable"), PalletDef->IsOpenable()))
	{
		return false;
	}
	int32 Offer = INDEX_NONE;
	for (const FWEMarketOffer& O : Terminal->GetOffers())
	{
		Offer = O.Item.Definition == PalletDef ? O.OfferIndex : Offer;
	}
	TestTrue(TEXT("Pallet on offer"), Offer != INDEX_NONE);
	TestEqual(TEXT("Buy pallet"), Terminal->Buy(Player, Offer, 1, FGuid::NewGuid()), EWETradeResult::Success);
	AWEPickupItem* Sealed = nullptr;
	for (AWEPickupItem* Item : ItemsIn(TW.World))
	{
		Sealed = Item->GetItem().Definition == PalletDef ? Item : Sealed;
	}
	if (!TestNotNull(TEXT("Pallet delivered to the delivery zone"), Sealed))
	{
		return false;
	}
	TestTrue(TEXT("...near the pallet"), FVector::Dist2D(Sealed->GetActorLocation(), Pallet->GetActorLocation()) < 150.f);

	FText Text, Reason;
	TestTrue(TEXT("[Q] Öffnen offered"), Sealed->GetAlternativeInteraction(Player, Text, Reason));
	TestTrue(TEXT("...with that text"), Text.ToString().Contains(TEXT("Öffnen")));
	TestEqual(TEXT("Nobody buys a sealed pallet"), Terminal->GetBestSellPrice(Sealed->GetItem()).GetCents(), 0ll);

	// Same pallet (same id) -> same contents: no re-rolling by loading an older save.
	const FWEItemInstance Copy = Sealed->GetItem();
	const TArray<FWEItemInstance> Found = Sealed->OpenContainer(Player);
	TestTrue(TEXT("3-5 finds"), Found.Num() >= 3 && Found.Num() <= 5);
	TestFalse(TEXT("Pallet is gone"), IsValid(Sealed) && !Sealed->IsActorBeingDestroyed());
	TestEqual(TEXT("Finds lie in the world"), ItemsIn(TW.World).Num(), Found.Num());
	AWEPickupItem* Again = AWEPickupItem::SpawnWorldItem(TW.World, Copy, FTransform(FVector(150.f, 150.f, 50.f)));
	const TArray<FWEItemInstance> FoundAgain = Again->OpenContainer(Player);
	bool bSame = FoundAgain.Num() == Found.Num();
	for (int32 i = 0; bSame && i < Found.Num(); ++i)
	{
		bSame = Found[i].Definition == FoundAgain[i].Definition && Found[i].Condition == FoundAgain[i].Condition && Found[i].Quantity == FoundAgain[i].Quantity;
	}
	TestTrue(TEXT("Contents fixed per pallet"), bSame);

	// A carried pallet cannot be opened.
	AWEPickupItem* Carried = AWEPickupItem::SpawnWorldItem(TW.World, FWEItemInstance::Make(PalletDef, 1, EWEItemCondition::New, 50),
		FTransform(Player->GetActorLocation() + FVector(80.f, 0.f, -40.f)));
	TestTrue(TEXT("Carry pallet"), Player->TryPickUpItem(Carried));
	TestFalse(TEXT("Not while carried"), Carried->GetAlternativeInteraction(Player, Text, Reason));
	TestEqual(TEXT("OpenContainer refuses while carried"), Carried->OpenContainer(Player).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEProcurementBalanceTest, "WorkshopEmpire.Economy.BalanceReturnPalletsAndScrap", Flags)
bool FWEProcurementBalanceTest::RunTest(const FString& Parameters)
{
	// Real data, many pallets: what the contents are worth at the traders right away (no repair, no
	// disassembly) compared with the pallet price; how often a rare find turns up. Spec §11: exciting,
	// but progress must not depend on luck.
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	GameState->GetContracts()->SetCatalog(nullptr);
	AWETraderTerminal* Terminal = SpawnRealTerminal(TW, FVector(0.f, 250.f, 0.f));
	const UWEMarketComponent* Market = UWEMarketComponent::Get(TW.World);
	constexpr int32 Runs = 400;

	for (const TCHAR* PalletId : { TEXT("ReturnPalletElectronics"), TEXT("ReturnPalletTools") })
	{
		UWEItemDefinition* Def = UWEItemDefinition::FindById(PalletId);
		if (!TestNotNull(PalletId, Def) || !TestNotNull(TEXT("Contents"), Def->Contents.Get()))
		{
			return false;
		}
		int64 Price = 0;
		for (const FWEMarketOffer& O : Terminal->GetOffers())
		{
			Price = O.Item.Definition == Def ? O.UnitPrice.GetCents() : Price;
		}
		FRandomStream Random(99);
		int64 Revenue = 0, RevenueWithoutRare = 0, MarketValue = 0;
		int32 RarePallets = 0;
		for (int32 Run = 0; Run < Runs; ++Run)
		{
			bool bRare = false;
			for (const FWEItemInstance& Item : Def->Contents->Roll(Random))
			{
				const int64 Sell = Terminal->GetBestSellPrice(Item).GetCents();
				Revenue += Sell;
				MarketValue += UWEMarketComponent::GetMarketValue(Item, Market).GetCents();
				const bool bItemRare = Item.Definition->Rarity >= EWEItemRarity::Rare;
				RevenueWithoutRare += bItemRare ? 0 : Sell;
				bRare |= bItemRare;
			}
			RarePallets += bRare ? 1 : 0;
		}
		const double Avg = Revenue / 100.0 / Runs;
		const double AvgNoRare = RevenueWithoutRare / 100.0 / Runs;
		const double AvgValue = MarketValue / 100.0 / Runs;
		const double RareShare = RarePallets * 100.0 / Runs;
		AddInfo(FString::Printf(TEXT("%s: price %.2f EUR, contents sell for %.2f EUR (without rare finds %.2f), market value %.2f EUR, rare find in %.1f %% of pallets"),
			PalletId, Price / 100.0, Avg, AvgNoRare, AvgValue, RareShare));
		TestTrue(FString::Printf(TEXT("%s: straight resale does not beat the price (needs work to profit)"), PalletId), Avg < Price / 100.0 * 1.1);
		TestTrue(FString::Printf(TEXT("%s: contents are worth more than the price (market value)"), PalletId), AvgValue > Price / 100.0);
		TestTrue(FString::Printf(TEXT("%s: rare finds stay rare (< 10 %% of pallets)"), PalletId), RareShare < 10.0);
		TestTrue(FString::Printf(TEXT("%s: profit does not depend on rare finds"), PalletId), AvgNoRare > Avg * 0.75);
	}

	// Scrap heap: value of one free item, straight to the trader.
	UWELootTable* Scrap = LoadLoot(TEXT("LT_ScrapPile"));
	if (!TestNotNull(TEXT("Scrap loot"), Scrap))
	{
		return false;
	}
	FRandomStream Random(5);
	int64 ScrapRevenue = 0;
	int32 RareScrap = 0;
	for (int32 Run = 0; Run < Runs; ++Run)
	{
		const FWEItemInstance Item = Scrap->RollOne(Random);
		ScrapRevenue += Terminal->GetBestSellPrice(Item).GetCents();
		RareScrap += Item.Definition->Rarity >= EWEItemRarity::Rare ? 1 : 0;
	}
	AddInfo(FString::Printf(TEXT("Scrap heap: one free item sells for %.2f EUR on average, rare in %.1f %%"), ScrapRevenue / 100.0 / Runs, RareScrap * 100.0 / Runs));
	TestTrue(TEXT("Free scrap is worth little (< 5 EUR per item): no money printer"), ScrapRevenue / 100.0 / Runs < 5.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEScrapPileTest, "WorkshopEmpire.Procurement.ScrapPileRefills", Flags)
bool FWEScrapPileTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	TW.SpawnGameState<AWEGameState>();
	AWEScrapPile* Pile = TW.Spawn<AWEScrapPile>(FVector(1000.f, 0.f, 0.f));
	Pile->SetLootTable(LoadLoot(TEXT("LT_ScrapPile")));
	Pile->SetRandomSeed(3);
	Pile->Refill();
	TestEqual(TEXT("Filled up to the maximum"), Pile->CountItemsAround(), Pile->GetMaxItems());
	TestEqual(TEXT("Full: nothing added"), Pile->Refill(), 0);

	TArray<AWEPickupItem*> Items = ItemsIn(TW.World);
	Items[0]->Destroy();
	Items[1]->SetActorLocation(FVector(0.f, 0.f, 50.f)); // carried away
	TestEqual(TEXT("Two missing"), Pile->CountItemsAround(), Pile->GetMaxItems() - 2);
	TestEqual(TEXT("Refill adds two"), Pile->Refill(), 2);
	for (AWEPickupItem* Item : ItemsIn(TW.World))
	{
		TestTrue(TEXT("Only scrap-heap items"), Item->GetItem().IsValid());
	}
	FText Reason;
	TestFalse(TEXT("Nothing to press"), Pile->CanInteract(nullptr, Reason));
	TestTrue(TEXT("Hint says it is free"), Reason.ToString().Contains(TEXT("gratis")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWERotatingStockTest, "WorkshopEmpire.Procurement.RotatingTraderStock", Flags)
bool FWERotatingStockTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	TW.SpawnGameState<AWEGameState>()->GetCompany()->SetBalance(FWEMoney::FromEuros(100));
	UWEItemDefinition* Fixed = MakeDefinition(TEXT("RS_Fixed"), 1.f);
	UWEItemDefinition* Rotating = MakeDefinition(TEXT("RS_Rotating"), 1.f);
	UWETraderDefinition* Trader = NewObject<UWETraderDefinition>(GetTransientPackage());
	FWETraderStockEntry A;
	A.Item = Fixed;
	A.Quantity = 3;
	A.Condition = EWEItemCondition::Used;
	A.Quality = 50;
	FWETraderStockEntry B = A;
	B.Item = Rotating;
	B.Chance = 0.5f;
	B.ConditionSpread = 2;
	B.QualitySpread = 20;
	Trader->Stock = { A, B };
	AWETraderTerminal* Terminal = TW.Spawn<AWETraderTerminal>(FVector(0.f, 250.f, 0.f));
	Terminal->SetRandomSeed(11);
	Terminal->SetTraders({ Trader });

	int32 FixedSeen = 0, RotatingSeen = 0;
	TSet<int32> Conditions, Qualities;
	constexpr int32 Restocks = 60;
	for (int32 i = 0; i < Restocks; ++i)
	{
		Terminal->Restock();
		for (const FWEMarketOffer& Offer : Terminal->GetOffers())
		{
			if (Offer.Item.Definition == Fixed)
			{
				++FixedSeen;
				TestTrue(TEXT("Fixed offer keeps its state"), Offer.Item.Condition == EWEItemCondition::Used && Offer.Item.Quality == 50);
			}
			else
			{
				++RotatingSeen;
				Conditions.Add(static_cast<int32>(Offer.Item.Condition));
				Qualities.Add(Offer.Item.Quality);
				TestTrue(TEXT("Condition within Used..2 worse"), Offer.Item.Condition >= EWEItemCondition::Used && Offer.Item.Condition <= EWEItemCondition::Defective);
				TestTrue(TEXT("Quality within 30..70"), Offer.Item.Quality >= 30 && Offer.Item.Quality <= 70);
			}
		}
	}
	TestEqual(TEXT("Chance 1: always there"), FixedSeen, Restocks);
	TestTrue(TEXT("Chance 0.5: sometimes there (15-45 of 60)"), RotatingSeen >= 15 && RotatingSeen <= 45);
	TestTrue(TEXT("Varying condition"), Conditions.Num() >= 2);
	TestTrue(TEXT("Varying quality"), Qualities.Num() >= 4);

	// Buying takes the rolled state and reduces the remaining units.
	AWECharacter* Player = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	Terminal->Restock();
	const FWEMarketOffer First = Terminal->GetOffers()[0];
	TestEqual(TEXT("Buy"), Terminal->Buy(Player, First.OfferIndex, 1, FGuid::NewGuid()), EWETradeResult::Success);
	TestEqual(TEXT("Remaining -1"), Terminal->GetOffers()[0].Remaining, First.Remaining - 1);
	return true;
}

namespace WEProcurementTests
{
	struct FAuctionFixture
	{
		FWETestWorld TW;
		AWEGameState* GameState = nullptr;
		UWEAuctionComponent* Auctions = nullptr;
		UWECompanyComponent* Company = nullptr;
		UWEItemDefinition* Def = nullptr;
		AWEDeliveryZone* Pallet = nullptr;

		FAuctionFixture()
		{
			GameState = TW.SpawnGameState<AWEGameState>();
			GameState->GetContracts()->SetCatalog(nullptr);
			Auctions = GameState->GetAuctions();
			Company = GameState->GetCompany();
			Company->SetBalance(FWEMoney::FromEuros(500));
			Def = MakeDefinition(TEXT("AU_Lot"), 5.f, false, 1, false, 10000, EWEItemCategory::Electronics);
			UWEAuctionCatalog* Catalog = NewObject<UWEAuctionCatalog>(GetTransientPackage());
			FWEAuctionTemplate Template;
			Template.Title = FText::FromString(TEXT("Testposten"));
			FWEAuctionLotItem LotItem;
			LotItem.Item = Def;
			LotItem.Quantity = 2;
			LotItem.Condition = EWEItemCondition::Used;
			LotItem.Quality = 50;
			Template.Items = { LotItem };
			Template.StartFactor = 0.3f;
			Catalog->Templates = { Template };
			Catalog->BidderNames = { TEXT("Rivale") };
			Auctions->SetCatalog(Catalog);
			Auctions->SetLots({}, {});
			Pallet = TW.Spawn<AWEDeliveryZone>(FVector(150.f, -150.f, 6.f));
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEAuctionTest, "WorkshopEmpire.Procurement.AuctionBiddingAndDelivery", Flags)
bool FWEAuctionTest::RunTest(const FString& Parameters)
{
	FAuctionFixture F;
	TestTrue(TEXT("Lot added"), F.Auctions->AddLot(0));
	const FGuid Id = F.Auctions->GetLots()[0].Id;
	const FWEAuctionLot Lot0 = F.Auctions->GetLots()[0];
	TestEqual(TEXT("Estimate = 2 x market value 70 EUR (Used, q50)"), Lot0.EstimatedValue.GetCents(), 14000ll);
	TestEqual(TEXT("Start = 30 % of estimate"), Lot0.CurrentBid.GetCents(), 4200ll);
	TestEqual(TEXT("First bid = start price"), UWEAuctionComponent::GetNextBid(Lot0).GetCents(), 4200ll);

	// Rival limit 50 EUR: he answers until then.
	F.Auctions->SetBidderMaxCents(Id, 5000);
	FWEMoney Bid;
	TestEqual(TEXT("42 EUR: rival answers"), F.Auctions->PlaceBid(Id, TEXT("Mike"), Bid), EWEAuctionResult::Outbid);
	TestEqual(TEXT("Bid was 42 EUR"), Bid.GetCents(), 4200ll);
	TestEqual(TEXT("Rival at 42 + 2,50 (5 %, rounded up to 50 ct) = 44,50 EUR"), F.Auctions->FindLot(Id)->CurrentBid.GetCents(), 4450ll);
	TestEqual(TEXT("Rival leads"), F.Auctions->FindLot(Id)->LeaderName, FString(TEXT("Rivale")));
	int32 Guard = 0;
	EWEAuctionResult Result = EWEAuctionResult::Outbid;
	while (Result == EWEAuctionResult::Outbid && Guard++ < 20)
	{
		Result = F.Auctions->PlaceBid(Id, TEXT("Mike"), Bid);
	}
	TestEqual(TEXT("Above his limit we lead"), Result, EWEAuctionResult::Leading);
	TestTrue(TEXT("Winning bid just above 50 EUR"), Bid.GetCents() > 5000 && Bid.GetCents() <= 5600);
	TestEqual(TEXT("No money taken before the end"), F.Company->GetBalance().GetCents(), 50000ll);
	TestEqual(TEXT("Leading: cannot bid against ourselves"), F.Auctions->PlaceBid(Id, TEXT("Anna"), Bid), EWEAuctionResult::AlreadyLeading);

	// Anti-sniping: a bid in the last seconds extends the deadline.
	TestTrue(TEXT("Deadline at least 20 s away after a bid"), F.Auctions->FindLot(Id)->EndsAt >= F.Auctions->GetServerTime() + 19.f);

	// Win: money is charged now, items on the pallet.
	const int64 Price = F.Auctions->FindLot(Id)->CurrentBid.GetCents();
	TestTrue(TEXT("Won"), F.Auctions->FinishLot(Id));
	TestEqual(TEXT("Charged once"), F.Company->GetBalance().GetCents(), 50000ll - Price);
	int32 Delivered = 0;
	for (AWEPickupItem* Item : ItemsIn(F.TW.World))
	{
		Delivered += Item->GetItem().Definition == F.Def ? 1 : 0;
	}
	TestEqual(TEXT("Both items delivered"), Delivered, 2);
	TestNull(TEXT("Lot gone"), F.Auctions->FindLot(Id));
	TestTrue(TEXT("Result shown"), F.Auctions->GetRecentResults().Num() > 0 && F.Auctions->GetRecentResults()[0].Contains(TEXT("Gewonnen")));

	// Not enough money at the deadline: the lot is lost, nothing is charged or delivered.
	TestTrue(TEXT("Second lot"), F.Auctions->AddLot(0));
	const FGuid Id2 = F.Auctions->GetLots().Last().Id;
	F.Auctions->SetBidderMaxCents(Id2, 0);
	TestEqual(TEXT("Lead second lot"), F.Auctions->PlaceBid(Id2, TEXT("Mike"), Bid), EWEAuctionResult::Leading);
	F.Company->SetBalance(FWEMoney::FromEuros(5));
	TestFalse(TEXT("Lost for lack of money"), F.Auctions->FinishLot(Id2));
	TestEqual(TEXT("Balance untouched"), F.Company->GetBalance().GetCents(), 500ll);
	TestEqual(TEXT("Nothing more delivered"), ItemsIn(F.TW.World).Num(), 2);

	// Too little money to even bid.
	TestTrue(TEXT("Third lot"), F.Auctions->AddLot(0));
	TestEqual(TEXT("Bid refused"), F.Auctions->PlaceBid(F.Auctions->GetLots().Last().Id, TEXT("Mike"), Bid), EWEAuctionResult::NotEnoughMoney);

	// UpdateAuctions keeps three lots running.
	F.Auctions->UpdateAuctions();
	TestEqual(TEXT("Topped up, but the same kind of lot never twice at once (catalog has one)"), F.Auctions->GetLots().Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEAuctionTerminalTest, "WorkshopEmpire.Procurement.AuctionThroughTerminal", Flags)
bool FWEAuctionTerminalTest::RunTest(const FString& Parameters)
{
	FAuctionFixture F;
	F.Auctions->AddLot(0);
	const FGuid Id = F.Auctions->GetLots()[0].Id;
	F.Auctions->SetBidderMaxCents(Id, 0);
	AWETraderTerminal* Terminal = F.TW.Spawn<AWETraderTerminal>(FVector(0.f, 250.f, 0.f));
	AWECharacter* Near = SpawnPlayer(F.TW, FVector(0.f, 0.f, 100.f));
	AWECharacter* Far = SpawnPlayer(F.TW, FVector(3000.f, 0.f, 100.f));
	TestEqual(TEXT("Out of reach"), Terminal->BidOnAuction(Far, Id, FGuid::NewGuid()), EWETradeResult::OutOfReach);
	const FGuid Request = FGuid::NewGuid();
	TestEqual(TEXT("Bid"), Terminal->BidOnAuction(Near, Id, Request), EWETradeResult::Success);
	AddExpectedMessage(TEXT("duplicate or invalid request"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	TestEqual(TEXT("Same request twice: once"), Terminal->BidOnAuction(Near, Id, Request), EWETradeResult::Duplicate);
	TestEqual(TEXT("One bid"), F.Auctions->FindLot(Id)->NumBids, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEExtendedOrdersTest, "WorkshopEmpire.Procurement.ExtendedOrders", Flags)
bool FWEExtendedOrdersTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	UWEContractComponent* Contracts = GameState->GetContracts();
	UWEItemDefinition* Device = MakeDefinition(TEXT("EO_Device"), 8.f, false, 1, false, 5000, EWEItemCategory::Electronics);
	UWEContractCatalog* Catalog = NewObject<UWEContractCatalog>(GetTransientPackage());
	FWEContractTemplate Template;
	Template.Title = FText::FromString(TEXT("Schule sucht ein Gerät"));
	Template.Item = Device;
	Template.MinCondition = EWEItemCondition::Used;
	Template.MinQuality = 50;
	Catalog->Templates = { Template };
	Contracts->SetCatalog(Catalog);
	Contracts->SetContracts({});
	TestTrue(TEXT("Offer"), Contracts->AddOffer(0));
	const FGuid Id = Contracts->GetContracts()[0].Id;
	TestEqual(TEXT("Accept"), Contracts->Accept(Id, TEXT("Test")), EWEContractResult::Success);
	AWECharacter* Player = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));

	auto Carry = [&](uint8 Quality, EWEItemCondition Condition)
	{
		if (AWEPickupItem* Old = Player->ReleaseHeldItem())
		{
			Old->Destroy();
		}
		AWEPickupItem* Item = AWEPickupItem::SpawnWorldItem(TW.World, FWEItemInstance::Make(Device, 1, Condition, Quality),
			FTransform(Player->GetActorLocation() + FVector(80.f, 0.f, -40.f)));
		Player->TryPickUpItem(Item);
		return Item;
	};
	Carry(40, EWEItemCondition::Used);
	TestEqual(TEXT("Quality 40 < 50: does not count"), UWEContractComponent::CountDeliverable(*Contracts->FindContract(Id), Player), 0);
	Carry(70, EWEItemCondition::Damaged);
	TestEqual(TEXT("Broken: does not count"), UWEContractComponent::CountDeliverable(*Contracts->FindContract(Id), Player), 0);
	AWEPickupItem* Good = Carry(70, EWEItemCondition::Used);
	TestEqual(TEXT("Working, quality 70, in the hands: counts"), UWEContractComponent::CountDeliverable(*Contracts->FindContract(Id), Player), 1);
	TestEqual(TEXT("Deliver from the hands"), Contracts->Deliver(Id, Player), EWEContractResult::Success);
	TestNull(TEXT("Hands empty"), Player->GetHeldItem());
	TestFalse(TEXT("Item handed over"), IsValid(Good) && !Good->IsActorBeingDestroyed());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEProcurementSaveTest, "WorkshopEmpire.Save.ProcurementRoundTrip", Flags)
bool FWEProcurementSaveTest::RunTest(const FString& Parameters)
{
	FAuctionFixture F;
	AWETraderTerminal* Terminal = SpawnRealTerminal(F.TW, FVector(0.f, 250.f, 0.f));
	F.Auctions->AddLot(0);
	const FGuid Id = F.Auctions->GetLots()[0].Id;
	F.Auctions->SetBidderMaxCents(Id, 12345);
	FWEMoney Bid;
	F.Auctions->PlaceBid(Id, TEXT("Mike"), Bid);
	const FWEAuctionLot Before = *F.Auctions->FindLot(Id);
	const TArray<int32> StockBefore = Terminal->GetStockRemaining();
	const TArray<FWEStockLine> LinesBefore = Terminal->GetStockLines();

	UWESaveGame* Save = FWESaveSystem::Capture(F.TW.World);
	TestEqual(TEXT("Version 6"), Save->SaveVersion, UWESaveGame::CurrentVersion);
	F.Auctions->SetLots({}, {});
	Terminal->SetRandomSeed(424242);
	Terminal->Restock();
	FWESaveSystem::Apply(F.TW.World, Save);

	const FWEAuctionLot* After = F.Auctions->FindLot(Id);
	if (!TestNotNull(TEXT("Lot restored"), After))
	{
		return false;
	}
	TestEqual(TEXT("Bid restored"), After->CurrentBid.GetCents(), Before.CurrentBid.GetCents());
	TestEqual(TEXT("Leader restored"), After->LeaderName, Before.LeaderName);
	TestEqual(TEXT("Items restored"), After->Items.Num(), Before.Items.Num());
	TestEqual(TEXT("Hidden limit restored"), F.Auctions->GetBidderMaxCents(Id), 12345ll);
	TestEqual(TEXT("Stock lines restored"), Terminal->GetStockLines().Num(), LinesBefore.Num());
	TestTrue(TEXT("Remaining restored"), Terminal->GetStockRemaining() == StockBefore);
	bool bSameStates = Terminal->GetStockLines().Num() == LinesBefore.Num();
	for (int32 i = 0; bSameStates && i < LinesBefore.Num(); ++i)
	{
		bSameStates = Terminal->GetStockLines()[i].Item.Condition == LinesBefore[i].Item.Condition && Terminal->GetStockLines()[i].Item.Quality == LinesBefore[i].Item.Quality;
	}
	TestTrue(TEXT("Rolled conditions restored"), bSameStates);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
