#include "Misc/AutomationTest.h"

#include "Economy/WEClassifiedsComponent.h"
#include "Economy/WECompanyComponent.h"
#include "Economy/WETraderTerminal.h"
#include "EngineUtils.h"
#include "Framework/WEGameState.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "WETestItems.h"
#include "WETestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace WEClassifiedsTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;
}

using namespace WEClassifiedsTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEClassifiedsBuyTest, "WorkshopEmpire.Classifieds.NegotiateAndDeliver", Flags)
bool FWEClassifiedsBuyTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	UWEClassifiedsComponent* Classifieds = GameState->GetClassifieds();
	UWECompanyComponent* Company = GameState->GetCompany();
	if (!TestNotNull(TEXT("Classifieds on the game state"), Classifieds))
	{
		return false;
	}
	TW.Spawn<AWEDeliveryZone>(FVector(200.f, 0.f, 6.f));
	Company->SetBalance(FWEMoney::FromCents(100000));
	Classifieds->SetState({}, {});
	UWEItemDefinition* Gpu = UWEItemDefinition::FindById(TEXT("GraphicsCardPro"));
	if (!TestNotNull(TEXT("Item data"), Gpu))
	{
		return false;
	}
	const FWEItemInstance Card = FWEItemInstance::Make(Gpu, 1, EWEItemCondition::Used, 60);
	TestTrue(TEXT("Ad"), Classifieds->AddAd(&Card, 1000, 700));
	const FGuid Id = Classifieds->GetAds().Last().Id;

	TestEqual(TEXT("Far too low: declined"), Classifieds->MakeOffer(Id, 300, TEXT("Test")), EWEClassifiedResult::Declined);
	TestEqual(TEXT("A bit low: counter offer"), Classifieds->MakeOffer(Id, 600, TEXT("Test")), EWEClassifiedResult::Countered);
	const FWEClassified* Ad = Classifieds->GetAds().FindByPredicate([&Id](const FWEClassified& A) { return A.Id == Id; });
	TestTrue(TEXT("Counter between the offer and the asking price, not below the lowest"), Ad && Ad->Counter.GetCents() >= 700 && Ad->Counter.GetCents() < 1000);
	const int64 Counter = Ad ? Ad->Counter.GetCents() : 0;

	const int64 Before = Company->GetBalance().GetCents();
	TestEqual(TEXT("Counter accepted"), Classifieds->AcceptCounter(Id, TEXT("Test")), EWEClassifiedResult::Accepted);
	TestEqual(TEXT("Paid the counter offer"), Before - Company->GetBalance().GetCents(), Counter);
	TestEqual(TEXT("Ad gone"), Classifieds->GetAds().ContainsByPredicate([&Id](const FWEClassified& A) { return A.Id == Id; }), false);
	TestEqual(TEXT("On its way"), Classifieds->GetDeliveries().Num(), 1);
	Classifieds->DeliverAllNow();
	bool bArrived = false;
	for (TActorIterator<AWEPickupItem> It(TW.World); It; ++It)
	{
		bArrived |= It->GetItem().Definition == Gpu;
	}
	TestTrue(TEXT("Delivered to the pallet"), bArrived);

	// A seller stops answering after too many offers.
	TestTrue(TEXT("Second ad"), Classifieds->AddAd(&Card, 1000, 900));
	const FGuid Second = Classifieds->GetAds().Last().Id;
	for (int32 i = 0; i < UWEClassifiedsComponent::MaxOffersPerAd; ++i)
	{
		Classifieds->MakeOffer(Second, 650, TEXT("Test"));
	}
	TestEqual(TEXT("No more answers"), Classifieds->MakeOffer(Second, 650, TEXT("Test")), EWEClassifiedResult::Ignored);
	TestEqual(TEXT("Asking price still buys? no - the seller is gone"), Classifieds->MakeOffer(Second, 1000, TEXT("Test")), EWEClassifiedResult::Ignored);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEClassifiedsSellTest, "WorkshopEmpire.Classifieds.OwnAds", Flags)
bool FWEClassifiedsSellTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	UWEClassifiedsComponent* Classifieds = GameState->GetClassifieds();
	UWECompanyComponent* Company = GameState->GetCompany();
	AWECharacter* Player = WETestItems::SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	UWEInventoryComponent* Inventory = Player->GetPlayerInventory();
	Classifieds->SetState({}, {});
	int32 Added = 0;
	Inventory->TryAddItem(FWEItemInstance::Make(UWEItemDefinition::FindById(TEXT("RamModule8")), 1, EWEItemCondition::Used, 60), Added);
	TestEqual(TEXT("Listed"), Classifieds->ListItem(Inventory, 0, 500, TEXT("Test")), EWEClassifiedResult::Accepted);
	TestEqual(TEXT("Out of the inventory while listed"), Inventory->GetAllSlots().Num(), 0);

	// A buyer comes along (the price is low, so the full price is offered).
	TArray<FWEOwnListing> Listings = Classifieds->GetListings();
	Listings[0].NextBuyerAt = -1.f;
	Classifieds->SetState({}, Listings);
	Classifieds->Update();
	const FWEOwnListing& L = Classifieds->GetListings()[0];
	TestTrue(TEXT("Buyer made an offer"), !L.Offer.IsZero() && !L.Buyer.IsEmpty());
	const int64 Offer = L.Offer.GetCents();
	const int64 Before = Company->GetBalance().GetCents();
	TestEqual(TEXT("Sold"), Classifieds->AcceptBuyer(L.Id, TEXT("Test")), EWEClassifiedResult::Accepted);
	TestEqual(TEXT("Paid"), Company->GetBalance().GetCents() - Before, Offer);

	// Withdrawn: back into the inventory.
	Inventory->TryAddItem(FWEItemInstance::Make(UWEItemDefinition::FindById(TEXT("SsdFast")), 1, EWEItemCondition::Used, 60), Added);
	TestEqual(TEXT("Listed again"), Classifieds->ListItem(Inventory, 0, 2000, TEXT("Test")), EWEClassifiedResult::Accepted);
	TestEqual(TEXT("Withdrawn"), Classifieds->Withdraw(Classifieds->GetListings()[0].Id, Inventory), EWEClassifiedResult::Accepted);
	TestEqual(TEXT("Back in the inventory"), Inventory->CountItems(UWEItemDefinition::FindById(TEXT("SsdFast"))), 1);
	return true;
}

#endif
