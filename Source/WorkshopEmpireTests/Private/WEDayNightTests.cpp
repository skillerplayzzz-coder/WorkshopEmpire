#include "Misc/AutomationTest.h"

#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "EngineUtils.h"
#include "Framework/WEGameState.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "WETestItems.h"
#include "WETestWorld.h"
#include "World/WECustomer.h"
#include "World/WEDayNightComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace WEDayNightTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;
}

using namespace WEDayNightTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEDayNightClockTest, "WorkshopEmpire.DayNight.ClockAndOpeningHours", Flags)
bool FWEDayNightClockTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	UWEDayNightComponent* Clock = GameState->GetDayNight();
	if (!TestNotNull(TEXT("Clock on the game state"), Clock))
	{
		return false;
	}
	Clock->SetTime(1, 9.f * 60.f);
	TestEqual(TEXT("Clock text"), Clock->GetClockText().ToString(), FString(TEXT("09:00")));
	TestTrue(TEXT("Open in the morning"), Clock->IsOpen());
	TestTrue(TEXT("Daylight in the morning"), Clock->GetDaylight() > 0.2f);
	Clock->SetTime(1, 18.f * 60.f + 59.f);
	Clock->Advance(2.f);
	TestFalse(TEXT("Closed after 19:00"), Clock->IsOpen());
	Clock->SetTime(1, 23.f * 60.f);
	TestEqual(TEXT("Dark at night"), Clock->GetDaylight(), 0.f);
	Clock->Advance(120.f);
	TestEqual(TEXT("Next day after midnight"), Clock->GetDay(), 2);
	TestEqual(TEXT("01:00"), Clock->GetClockText().ToString(), FString(TEXT("01:00")));
	TestFalse(TEXT("No customers at night"), WEWalkIn::TrySpawn(TW.World) != nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEWalkInTest, "WorkshopEmpire.DayNight.WalkInCustomers", Flags)
bool FWEWalkInTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	GameState->GetContracts()->SetCatalog(nullptr);
	GameState->GetContracts()->SetContracts({});
	UWECompanyComponent* Company = GameState->GetCompany();
	Company->SetBalance(FWEMoney::FromCents(100000));
	AWECharacter* Player = WETestItems::SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	auto Customer = [&TW](EWECustomerWish Wish, const TCHAR* ItemId, int64 Cents)
	{
		AWECustomer* C = TW.Spawn<AWECustomer>(FVector(300.f, 0.f, 100.f));
		C->SetInstantTravel(true);
		C->Setup(TEXT("Test"), Wish, FWEItemInstance::Make(UWEItemDefinition::FindById(FName(ItemId)), 1, EWEItemCondition::Damaged, 40), Cents,
			FVector(350.f, 0.f, 0.f), FVector(2000.f, 0.f, 0.f), 7);
		C->Tick(0.1f); // arrives at once
		return C;
	};

	// Sells us a device.
	AWECustomer* Seller = Customer(EWECustomerWish::SellToUs, TEXT("CrtMonitor"), 1200);
	FText Reason;
	TestTrue(TEXT("Waiting at the door"), Seller->GetCustomerState() == EWECustomerState::Waiting && Seller->CanInteract(Player, Reason));
	int64 Before = Company->GetBalance().GetCents();
	Seller->Interact(Player);
	TestEqual(TEXT("Paid the customer"), Before - Company->GetBalance().GetCents(), int64(1200));
	TestTrue(TEXT("Leaves"), Seller->GetCustomerState() == EWECustomerState::Leaving);
	bool bDevice = false;
	for (TActorIterator<AWEPickupItem> It(TW.World); It; ++It)
	{
		bDevice |= It->GetItem().Definition && It->GetItem().Definition->ItemId == TEXT("CrtMonitor");
	}
	TestTrue(TEXT("The device is ours, at the door"), bDevice);

	// Wants a part we have.
	AWECustomer* Buyer = Customer(EWECustomerWish::BuyFromUs, TEXT("RamModule"), 900);
	TestFalse(TEXT("Nothing to sell yet"), Buyer->CanInteract(Player, Reason));
	int32 Added = 0;
	Player->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(UWEItemDefinition::FindById(TEXT("RamModule")), 1, EWEItemCondition::Used, 60), Added);
	TestTrue(TEXT("Now we have it"), Buyer->CanInteract(Player, Reason));
	Before = Company->GetBalance().GetCents();
	Buyer->Interact(Player);
	TestEqual(TEXT("Customer paid"), Company->GetBalance().GetCents() - Before, int64(900));
	TestEqual(TEXT("Part handed over"), Player->GetPlayerInventory()->GetAllSlots().Num(), 0);

	// Brings a repair: becomes an order with the device at the door.
	AWECustomer* Repair = Customer(EWECustomerWish::Repair, TEXT("OldComputer"), 3000);
	Repair->Interact(Player);
	const TArray<FWEContract>& Contracts = GameState->GetContracts()->GetContracts();
	TestTrue(TEXT("Repair order running"), Contracts.Num() == 1 && Contracts[0].Kind == EWEContractKind::Repair && Contracts[0].State == EWEContractState::Accepted);
	bool bCustomerDevice = false;
	for (TActorIterator<AWEPickupItem> It(TW.World); It; ++It)
	{
		bCustomerDevice |= Contracts.Num() == 1 && It->GetItem().InstanceId == Contracts[0].CustomerItemId && It->GetItem().IsCustomerItem();
	}
	TestTrue(TEXT("Customer device handed over"), bCustomerDevice);

	// Sent away.
	AWECustomer* Other = Customer(EWECustomerWish::SellToUs, TEXT("CableBox"), 300);
	Other->InteractAlternative(Player);
	TestTrue(TEXT("Sent away"), Other->GetCustomerState() == EWECustomerState::Leaving);
	return true;
}

#endif
