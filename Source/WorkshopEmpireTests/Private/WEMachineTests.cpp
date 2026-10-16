#include "Misc/AutomationTest.h"

#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WETraderDefinition.h"
#include "Economy/WETraderTerminal.h"
#include "EngineUtils.h"
#include "Framework/WEGameState.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Machines/WEMachine.h"
#include "Machines/WEMachineDefinition.h"
#include "Machines/WEPowerGridComponent.h"
#include "Player/WECharacter.h"
#include "Progression/WEProgressionComponent.h"
#include "Save/WESaveGame.h"
#include "Save/WESaveSystem.h"
#include "WETestItems.h"
#include "WETestWorld.h"
#include "Workshop/WEWorkshopLayoutComponent.h"
#include "World/WEBuildArea.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace WETestItems;

namespace WEMachineTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	/** Garage with the real furniture catalog and machine data, 5000 EUR, one player. */
	struct FMachineFixture
	{
		FWETestWorld TW;
		AWEGameState* GameState = nullptr;
		UWEWorkshopLayoutComponent* Layout = nullptr;
		UWECompanyComponent* Company = nullptr;
		UWEPowerGridComponent* Grid = nullptr;
		AWECharacter* Player = nullptr;

		FMachineFixture()
		{
			GameState = TW.SpawnGameState<AWEGameState>();
			GameState->GetContracts()->SetCatalog(nullptr);
			Layout = GameState->GetLayout();
			Company = GameState->GetCompany();
			Grid = GameState->GetPowerGrid();
			Company->SetBalance(FWEMoney::FromEuros(5000));
			TW.Spawn<AWEBuildArea>(FVector(0.f, 0.f, 150.f));
			Player = SpawnPlayer(TW, FVector(-300.f, -200.f, 100.f));
		}

		AWEMachine* Place(const TCHAR* Id, float X, float Y)
		{
			AWEPlaceholderProp* Prop = nullptr;
			Layout->PlaceFurniture(Player, FName(Id), FTransform(FVector(X, Y, 0.f)), FGuid::NewGuid(), &Prop);
			AWEMachine* Machine = Cast<AWEMachine>(Prop);
			if (Machine)
			{
				Machine->SetRandomSeed(4711);
			}
			return Machine;
		}

		AWEMachine* Shredder() { return Place(TEXT("Shredder"), -250.f, 150.f); }
		AWEMachine* Sorter() { return Place(TEXT("Sorter"), 0.f, 150.f); }
		AWEMachine* Press() { return Place(TEXT("Press"), 250.f, 150.f); }

		/** Steps the grid like the server timer does. */
		void Run(float Seconds)
		{
			for (float T = 0.f; T < Seconds - KINDA_SMALL_NUMBER; T += UWEPowerGridComponent::StepSeconds)
			{
				Grid->StepMachines(UWEPowerGridComponent::StepSeconds);
			}
		}

		static void Fill(AWEMachine* Machine, const TCHAR* ItemId, int32 Quantity)
		{
			int32 Added = 0;
			Machine->GetStorage()->TryAddItem(FWEItemInstance::Make(UWEItemDefinition::FindById(FName(ItemId)), Quantity), Added);
		}

		/** Bulky products the machines put down in front of them. */
		int32 CountWorldItems(const UWEItemDefinition* Def, bool bDestroy = false)
		{
			int32 Count = 0;
			for (TActorIterator<AWEPickupItem> It(TW.World); It; ++It)
			{
				if (IsValid(*It) && !It->IsActorBeingDestroyed() && It->GetItem().Definition == Def)
				{
					Count += It->GetItem().Quantity;
					if (bDestroy)
					{
						It->Destroy();
					}
				}
			}
			return Count;
		}
	};

	UWEItemDefinition* Item(const TCHAR* Id) { return UWEItemDefinition::FindById(FName(Id)); }
}

using namespace WEMachineTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEMachineChainTest, "WorkshopEmpire.Machines.ShredSortPressChain", Flags)
bool FWEMachineChainTest::RunTest(const FString& Parameters)
{
	FMachineFixture F;
	AWEMachine* Shredder = F.Shredder();
	AWEMachine* Sorter = F.Sorter();
	AWEMachine* Press = F.Press();
	if (!TestNotNull(TEXT("Shredder placed"), Shredder) || !TestNotNull(TEXT("Sorter placed"), Sorter) || !TestNotNull(TEXT("Press placed"), Press)
		|| !TestNotNull(TEXT("Shredder data"), Shredder->GetMachineDefinition()) || !TestNotNull(TEXT("PC data"), Item(TEXT("OldComputer"))))
	{
		return false;
	}
	TestEqual(TEXT("3 machines paid (450 + 380 + 320)"), F.Company->GetBalance().GetCents(), 500000ll - 115000ll);
	TestEqual(TEXT("Starts off"), Shredder->GetStatus().State, EWEMachineState::Off);
	TestEqual(TEXT("Shredder input: 6 slots"), Shredder->GetStorage()->GetNumSlots(), 6);
	TestEqual(TEXT("Shredder output: 10 slots"), Shredder->GetOutputStorage()->GetNumSlots(), 10);
	TestTrue(TEXT("Shredder takes bulky devices"), Shredder->GetStorage()->AcceptsBulky());
	TestFalse(TEXT("Sorter does not"), Sorter->GetStorage()->AcceptsBulky());

	// Shredder: a whole defective PC (9 kg, bulky) -> 8 units of shredded electronics.
	AWEPickupItem* Pc = SpawnItem(F.TW, Item(TEXT("OldComputer")), 1, FVector(-100.f, 0.f, 30.f), EWEItemCondition::Defective);
	TestTrue(TEXT("PC goes into the shredder"), Shredder->InsertItem(Pc));
	TestEqual(TEXT("PC in the input"), Shredder->GetStorage()->CountItems(Item(TEXT("OldComputer"))), 1);
	TestTrue(TEXT("Switch on"), Shredder->SetPower(true));
	TestEqual(TEXT("Runs at once"), Shredder->GetStatus().State, EWEMachineState::Running);
	TestEqual(TEXT("Input taken at the start"), Shredder->GetStorage()->GetAllSlots().Num(), 0);
	TestEqual(TEXT("PC is being processed"), Shredder->GetProcessing().Num(), 1);
	TestEqual(TEXT("Cycle 3 s + 1.2 s/kg x 9 kg = 13.8 s"), Shredder->GetStatus().CycleSeconds, 13.8f, 0.01f);
	TestEqual(TEXT("Grid load 7.5 kW"), F.Grid->GetLoadKw(), 7.5f, 0.01f);
	TestTrue(TEXT("Status names the recipe"), Shredder->GetStatusText().ToString().Contains(TEXT("Elektronik schreddern")));
	F.Run(7.f);
	TestEqual(TEXT("Half way"), Shredder->GetProgress(), 7.f / 13.8f, 0.05f);
	F.Run(7.5f);
	TestEqual(TEXT("Done: idle"), Shredder->GetStatus().State, EWEMachineState::Idle);
	TestEqual(TEXT("9 kg x 0.9 = 8 shredded"), Shredder->GetOutputStorage()->CountItems(Item(TEXT("ShredElectronics"))), 8);
	TestTrue(TEXT("Wear"), Shredder->GetMaintenance() < 100.f);
	TestEqual(TEXT("Idle draw 0.4 kW"), F.Grid->GetLoadKw(), 0.4f, 0.01f);
	Shredder->SetPower(false);

	// Sorter: 10 shredded -> two cycles of pure fractions.
	FMachineFixture::Fill(Sorter, TEXT("ShredElectronics"), 10);
	Sorter->SetPower(true);
	F.Run(13.f);
	TestEqual(TEXT("All sorted"), Sorter->GetStorage()->GetAllSlots().Num(), 0);
	TestEqual(TEXT("Sorter idle again"), Sorter->GetStatus().State, EWEMachineState::Idle);
	const UWEInventoryComponent* Sorted = Sorter->GetOutputStorage();
	TestTrue(TEXT("Electronic scrap 6-10"), Sorted->CountItems(Item(TEXT("ElectronicScrap"))) >= 6 && Sorted->CountItems(Item(TEXT("ElectronicScrap"))) <= 10);
	TestTrue(TEXT("Steel scrap 4-6"), Sorted->CountItems(Item(TEXT("SteelScrap"))) >= 4 && Sorted->CountItems(Item(TEXT("SteelScrap"))) <= 6);
	TestTrue(TEXT("Plastic 2-4"), Sorted->CountItems(Item(TEXT("PlasticParts"))) >= 2);

	// Press: 20 steel scrap -> one bale. Bales are bulky: they land in front of the press.
	FMachineFixture::Fill(Press, TEXT("SteelScrap"), 25);
	Press->SetPower(true);
	F.Run(9.f);
	TestEqual(TEXT("One steel bale in front of the press"), F.CountWorldItems(Item(TEXT("SteelBale"))), 1);
	TestEqual(TEXT("5 steel scrap left (not enough for a second)"), Press->GetStorage()->CountItems(Item(TEXT("SteelScrap"))), 5);
	TestEqual(TEXT("Press waits for material"), Press->GetStatus().State, EWEMachineState::Idle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEMachineInsertTest, "WorkshopEmpire.Machines.InsertRules", Flags)
bool FWEMachineInsertTest::RunTest(const FString& Parameters)
{
	FMachineFixture F;
	AWEMachine* Shredder = F.Shredder();
	AWEMachine* Sorter = F.Sorter();
	if (!TestNotNull(TEXT("Shredder"), Shredder) || !TestNotNull(TEXT("Sorter"), Sorter))
	{
		return false;
	}
	FText Reason;
	AWEPickupItem* Granulate = SpawnItem(F.TW, Item(TEXT("CopperGranulate")), 3, FVector(-100.f, 0.f, 30.f));
	TestFalse(TEXT("Shredder refuses raw material"), Shredder->CanInsertItem(Granulate, Reason));
	TestTrue(TEXT("...and says so"), Reason.ToString().Contains(TEXT("verarbeitet das nicht")));
	TestFalse(TEXT("InsertItem refuses too"), Shredder->InsertItem(Granulate));
	TestTrue(TEXT("Item still there"), IsValid(Granulate) && !Granulate->IsActorBeingDestroyed());

	AWEPickupItem* Collectible = SpawnItem(F.TW, Item(TEXT("VintageRadio")), 1, FVector(-100.f, 50.f, 30.f));
	TestFalse(TEXT("Collectibles are never shredded"), Shredder->CanInsertItem(Collectible, Reason));

	FWEItemInstance CustomerPc = FWEItemInstance::Make(Item(TEXT("OldComputer")), 1, EWEItemCondition::Defective);
	CustomerPc.Repair.SetFlag(FWERepairState::CustomerItem);
	AWEPickupItem* Customer = AWEPickupItem::SpawnWorldItem(F.TW.World, CustomerPc, FTransform(FVector(-100.f, 100.f, 30.f)));
	TestFalse(TEXT("Customer device refused"), Shredder->CanInsertItem(Customer, Reason));
	TestTrue(TEXT("...as a customer device"), Reason.ToString().Contains(TEXT("Kundengerät")));

	AWEPickupItem* Pc = SpawnItem(F.TW, Item(TEXT("OldComputer")), 1, F.Player->GetActorLocation() + FVector(80.f, 0.f, -40.f), EWEItemCondition::Defective);
	TestFalse(TEXT("Sorter refuses a whole PC"), Sorter->CanInsertItem(Pc, Reason));
	TestTrue(TEXT("Carry the PC"), F.Player->TryPickUpItem(Pc));
	TestTrue(TEXT("Carried PC into the shredder"), Shredder->InsertItem(Pc));
	TestNull(TEXT("Hands free again"), F.Player->GetHeldItem());

	AWEPickupItem* Shred = SpawnItem(F.TW, Item(TEXT("ShredMetal")), 7, FVector(0.f, 0.f, 30.f));
	TestTrue(TEXT("Sorter takes shredded metal"), Sorter->InsertItem(Shred));
	TestEqual(TEXT("7 in the sorter"), Sorter->GetStorage()->CountItems(Item(TEXT("ShredMetal"))), 7);

	// Maintenance kits are bought at the tool shop.
	UWETraderDefinition* ToolShop = LoadObject<UWETraderDefinition>(nullptr, TEXT("/Game/WorkshopEmpire/Data/Economy/DA_Trader_ToolShop.DA_Trader_ToolShop"));
	bool bSellsKit = false;
	for (const FWETraderStockEntry& Entry : ToolShop ? ToolShop->Stock : TArray<FWETraderStockEntry>())
	{
		bSellsKit |= Entry.Item == Item(TEXT("MaintenanceKit"));
	}
	TestTrue(TEXT("Tool shop sells maintenance kits"), bSellsKit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEMachinePowerTest, "WorkshopEmpire.Machines.PowerLimitAndUpgrade", Flags)
bool FWEMachinePowerTest::RunTest(const FString& Parameters)
{
	FMachineFixture F;
	AWEMachine* Shredder = F.Shredder();
	AWEMachine* Sorter = F.Sorter();
	if (!TestNotNull(TEXT("Shredder"), Shredder) || !TestNotNull(TEXT("Sorter"), Sorter))
	{
		return false;
	}
	TestEqual(TEXT("House connection 11 kW"), F.Grid->GetCapacityKw(), 11.f);
	FMachineFixture::Fill(Shredder, TEXT("CableBox"), 1);
	FMachineFixture::Fill(Sorter, TEXT("ShredMetal"), 20);
	Shredder->SetPower(true);
	Sorter->SetPower(true);
	// 7.5 + 4 = 11.5 kW > 11 kW: only one runs, the other waits for power.
	const int32 Running = (Shredder->IsRunning() ? 1 : 0) + (Sorter->IsRunning() ? 1 : 0);
	TestEqual(TEXT("Only one machine runs"), Running, 1);
	const AWEMachine* Waiting = Shredder->IsRunning() ? Sorter : Shredder;
	TestEqual(TEXT("The other waits: no power"), Waiting->GetStatus().State, EWEMachineState::NoPower);
	TestTrue(TEXT("Load stays within 11 kW"), F.Grid->GetLoadKw() <= 11.f + 0.5f);
	TestTrue(TEXT("Status says why"), Waiting->GetStatusText().ToString().Contains(TEXT("Strom")));

	// Starkstromanschluss: +11 kW, both run.
	TestTrue(TEXT("Buy the upgrade"), F.GameState->GetProgression()->GrantUpgrade(TEXT("HeavyCurrent")));
	TestEqual(TEXT("22 kW"), F.Grid->GetCapacityKw(), 22.f);
	F.Run(0.5f);
	TestTrue(TEXT("Shredder runs"), Shredder->IsRunning());
	TestTrue(TEXT("Sorter runs"), Sorter->IsRunning());
	TestEqual(TEXT("Load 11.5 kW"), F.Grid->GetLoadKw(), 11.5f, 0.01f);
	TestTrue(TEXT("Grid knows it has machines"), F.Grid->HasMachines());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEMachineBillTest, "WorkshopEmpire.Machines.PowerBill", Flags)
bool FWEMachineBillTest::RunTest(const FString& Parameters)
{
	FMachineFixture F;
	AWEMachine* Shredder = F.Shredder();
	if (!TestNotNull(TEXT("Shredder"), Shredder))
	{
		return false;
	}
	// Switched on but idle for 10 minutes: 0.4 kW x 1/6 h = 0.0667 kWh.
	Shredder->SetPower(true);
	F.Run(600.f);
	TestEqual(TEXT("Idle energy counted"), F.Grid->GetUnbilledKWh(), 0.4f / 6.f, 0.001f);
	// Running: one cable box (3 kg, 6 s at 7.5 kW).
	const float Before = F.Grid->GetUnbilledKWh();
	FMachineFixture::Fill(Shredder, TEXT("CableBox"), 1);
	F.Run(6.f);
	TestEqual(TEXT("Shredding energy 7.5 kW x 6 s"), F.Grid->GetUnbilledKWh() - Before, 7.5f * 6.f / 3600.f, 0.002f);

	const int64 Balance = F.Company->GetBalance().GetCents();
	F.Grid->SetUnbilledKWh(2.f);
	const FWEMoney Bill = F.Grid->Bill();
	TestEqual(TEXT("2 kWh x 2.50 EUR = 5 EUR"), Bill.GetCents(), 500ll);
	TestEqual(TEXT("Withdrawn from the company"), F.Company->GetBalance().GetCents(), Balance - 500);
	TestEqual(TEXT("Meter reset"), F.Grid->GetUnbilledKWh(), 0.f);
	TestEqual(TEXT("Last bill shown"), F.Grid->GetLastBill().GetCents(), 500ll);

	F.Company->SetBalance(FWEMoney::FromCents(120));
	F.Grid->SetUnbilledKWh(4.f);
	F.Grid->Bill();
	TestEqual(TEXT("Never below zero"), F.Company->GetBalance().GetCents(), 0ll);
	Shredder->SetPower(false);
	TestEqual(TEXT("Off: no draw"), F.Grid->GetLoadKw(), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEMachineWearTest, "WorkshopEmpire.Machines.WearAndMaintenance", Flags)
bool FWEMachineWearTest::RunTest(const FString& Parameters)
{
	FMachineFixture F;
	AWEMachine* Press = F.Press();
	if (!TestNotNull(TEXT("Press"), Press))
	{
		return false;
	}
	// Worn below 30 %: works at 70 % speed.
	Press->RestoreState(false, 25, INDEX_NONE, 0.f, 0.f, {});
	TestEqual(TEXT("Worn: efficiency 0.7"), Press->GetEfficiency(), 0.7f);
	FMachineFixture::Fill(Press, TEXT("SteelScrap"), 40);
	Press->SetPower(true);
	TestTrue(TEXT("Runs"), Press->IsRunning());
	TestEqual(TEXT("8 s / 0.7"), Press->GetStatus().CycleSeconds, 8.f / 0.7f, 0.01f);
	F.Run(12.f);
	TestEqual(TEXT("Wear 3 per cycle"), static_cast<int32>(Press->GetMaintenance()), 22);

	// At 0 % it stops.
	Press->RestoreState(true, 2, INDEX_NONE, 0.f, 0.f, {});
	Press->GetStorage()->ClearAll();
	FMachineFixture::Fill(Press, TEXT("SteelScrap"), 40);
	F.Run(12.f);
	TestEqual(TEXT("Worn out after the cycle"), static_cast<int32>(Press->GetMaintenance()), 0);
	TestEqual(TEXT("Needs maintenance"), Press->GetStatus().State, EWEMachineState::NeedsMaintenance);
	TestEqual(TEXT("No new cycle: input kept"), Press->GetStorage()->CountItems(Item(TEXT("SteelScrap"))), 20);
	F.Run(2.f);
	TestFalse(TEXT("Does not start without maintenance"), Press->IsRunning());

	FText Text, Reason;
	TestFalse(TEXT("No kit: no [X]"), Press->GetExtraInteraction(F.Player, Text, Reason));
	TestTrue(TEXT("...but the reason names the kit"), Reason.ToString().Contains(TEXT("Wartungsset")));
	int32 Added = 0;
	F.Player->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(Item(TEXT("MaintenanceKit")), 2), Added);
	TestTrue(TEXT("[X] Warten"), Press->GetExtraInteraction(F.Player, Text, Reason));
	TestTrue(TEXT("Maintain"), Press->Maintain(F.Player));
	TestEqual(TEXT("Back to 100 %"), static_cast<int32>(Press->GetMaintenance()), 100);
	TestEqual(TEXT("One kit used"), F.Player->GetPlayerInventory()->CountItems(Item(TEXT("MaintenanceKit"))), 1);
	TestFalse(TEXT("At 100 % nothing to maintain"), Press->Maintain(F.Player));
	F.Run(0.5f);
	TestTrue(TEXT("Runs again"), Press->IsRunning());
	TestEqual(TEXT("Full speed"), Press->GetStatus().CycleSeconds, 8.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEMachineOutputPauseTest, "WorkshopEmpire.Machines.OutputFullAndPause", Flags)
bool FWEMachineOutputPauseTest::RunTest(const FString& Parameters)
{
	FMachineFixture F;
	AWEMachine* Sorter = F.Sorter();
	if (!TestNotNull(TEXT("Sorter"), Sorter))
	{
		return false;
	}
	// Output nearly full (12 slots, 9 used): a recipe with 4 outputs cannot start.
	for (int32 i = 0; i < 9; ++i)
	{
		int32 Added = 0;
		Sorter->GetOutputStorage()->TryAddItem(FWEItemInstance::Make(MakeDefinition(*FString::Printf(TEXT("MT_Filler%d"), i), 0.1f)), Added);
	}
	FMachineFixture::Fill(Sorter, TEXT("ShredMetal"), 5);
	Sorter->SetPower(true);
	TestEqual(TEXT("Output full"), Sorter->GetStatus().State, EWEMachineState::OutputFull);
	TestEqual(TEXT("Input untouched"), Sorter->GetStorage()->CountItems(Item(TEXT("ShredMetal"))), 5);
	FText Text;
	TestTrue(TEXT("[R] take products"), Sorter->GetSecondaryInteraction(F.Player, Text));
	TestEqual(TEXT("Took 9"), Sorter->TakeOutputs(F.Player), 9);
	TestEqual(TEXT("Output empty"), Sorter->GetOutputStorage()->GetAllSlots().Num(), 0);
	F.Run(0.5f);
	TestTrue(TEXT("Runs now"), Sorter->IsRunning());

	// Switching off pauses the cycle; switching on continues it.
	F.Run(2.f);
	const float Progress = Sorter->GetProgress();
	TestTrue(TEXT("Some progress"), Progress > 0.3f && Progress < 0.8f);
	FText MoveReason;
	TestFalse(TEXT("Running machine cannot be moved"), F.Layout->CanPickUpFurniture(Sorter, MoveReason));
	Sorter->SetPower(false);
	TestEqual(TEXT("Off"), Sorter->GetStatus().State, EWEMachineState::Off);
	F.Run(10.f);
	TestEqual(TEXT("Progress kept while off"), Sorter->GetProgress(), Progress, 0.001f);
	TestEqual(TEXT("Still holds its input"), Sorter->GetProcessing().Num(), 1);
	TestFalse(TEXT("Paused with material inside: still not movable"), F.Layout->CanPickUpFurniture(Sorter, MoveReason));
	Sorter->SetPower(true);
	TestTrue(TEXT("Continues"), Sorter->IsRunning());
	F.Run(3.5f);
	TestEqual(TEXT("Finished"), Sorter->GetStatus().State, EWEMachineState::Idle);
	TestTrue(TEXT("Steel came out"), Sorter->GetOutputStorage()->CountItems(Item(TEXT("SteelScrap"))) >= 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEMachineSaveTest, "WorkshopEmpire.Save.MachineRoundTrip", Flags)
bool FWEMachineSaveTest::RunTest(const FString& Parameters)
{
	FMachineFixture F;
	AWEMachine* Shredder = F.Shredder();
	if (!TestNotNull(TEXT("Shredder"), Shredder))
	{
		return false;
	}
	FMachineFixture::Fill(Shredder, TEXT("CableBox"), 2);
	Shredder->SetPower(true);
	F.Run(3.f);
	int32 Added = 0;
	Shredder->GetOutputStorage()->TryAddItem(FWEItemInstance::Make(Item(TEXT("ShredMetal")), 4), Added);
	const float Progress = Shredder->GetProgress();
	const float KWh = F.Grid->GetUnbilledKWh();
	TestTrue(TEXT("Mid cycle"), Shredder->IsRunning() && Progress > 0.2f);

	UWESaveGame* Save = FWESaveSystem::Capture(F.TW.World);
	TestEqual(TEXT("Current version (8 added machines)"), Save->SaveVersion, UWESaveGame::CurrentVersion);
	TestEqual(TEXT("Machine saved"), Save->Machines.Num(), 1);

	// Change everything, then load.
	F.Run(20.f);
	Shredder->SetPower(false);
	Shredder->GetOutputStorage()->ClearAll();
	Shredder->GetStorage()->ClearAll();
	F.Grid->Bill();
	FWESaveSystem::Apply(F.TW.World, Save);

	AWEMachine* Loaded = nullptr;
	for (TActorIterator<AWEMachine> It(F.TW.World); It; ++It)
	{
		Loaded = IsValid(*It) && !It->IsActorBeingDestroyed() ? *It : Loaded;
	}
	if (!TestNotNull(TEXT("Machine after loading"), Loaded))
	{
		return false;
	}
	TestTrue(TEXT("On again"), Loaded->IsOn());
	TestEqual(TEXT("Cycle material restored"), Loaded->GetProcessing().Num(), 1);
	TestEqual(TEXT("Progress restored"), Loaded->GetProgress(), Progress, 0.01f);
	TestEqual(TEXT("Input (second cable box) restored"), Loaded->GetStorage()->CountItems(Item(TEXT("CableBox"))), 1);
	TestEqual(TEXT("Output restored separately"), Loaded->GetOutputStorage()->CountItems(Item(TEXT("ShredMetal"))), 4);
	TestEqual(TEXT("Energy meter restored"), F.Grid->GetUnbilledKWh(), KWh, 0.0001f);
	F.Run(0.5f);
	TestTrue(TEXT("Continues running"), Loaded->IsRunning());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEMachineBalanceTest, "WorkshopEmpire.Economy.BalanceRecyclingChain", Flags)
bool FWEMachineBalanceTest::RunTest(const FString& Parameters)
{
	// Real data: junk through shredder -> sorter -> press, sold at the best trader, minus the power bill,
	// compared with selling the junk as it is. Machines should pay off for junk, without a runaway.
	FMachineFixture F;
	F.GameState->GetProgression()->GrantUpgrade(TEXT("HeavyCurrent"));
	AWEMachine* Shredder = F.Shredder();
	AWEMachine* Sorter = F.Sorter();
	AWEMachine* Press = F.Press();
	TArray<UWETraderDefinition*> Traders;
	for (const TCHAR* Name : { TEXT("Scrapyard"), TEXT("UsedGoods"), TEXT("ToolShop"), TEXT("SpareParts"), TEXT("Returns") })
	{
		Traders.Add(LoadObject<UWETraderDefinition>(nullptr, *FString::Printf(TEXT("/Game/WorkshopEmpire/Data/Economy/DA_Trader_%s.DA_Trader_%s"), Name, Name)));
	}
	if (!Shredder || !Sorter || !Press || !TestTrue(TEXT("Traders"), !Traders.Contains(nullptr)))
	{
		return false;
	}
	AWETraderTerminal* Terminal = F.TW.Spawn<AWETraderTerminal>(FVector(0.f, -250.f, 0.f));
	Terminal->SetTraders(Traders);

	auto SellAll = [&](UWEInventoryComponent* Inventory)
	{
		int64 Cents = 0;
		for (const FWEInventorySlotData& Slot : Inventory->GetAllSlots())
		{
			Cents += Terminal->GetBestSellPrice(Slot.Item).GetCents();
		}
		Inventory->ClearAll();
		return Cents;
	};
	auto MoveAll = [](UWEInventoryComponent* From, UWEInventoryComponent* To)
	{
		From->TransferAllTo(To);
	};
	// Only what the press can use goes to the press.
	const TArray<UWEItemDefinition*> Metals = { Item(TEXT("SteelScrap")), Item(TEXT("AluminiumScrap")), Item(TEXT("CopperGranulate")) };
	auto MoveMetals = [&Metals](UWEInventoryComponent* From, UWEInventoryComponent* To)
	{
		const TArray<FWEInventorySlotData> Slots = From->GetAllSlots();
		for (const FWEInventorySlotData& Slot : Slots)
		{
			if (Metals.Contains(Slot.Item.Definition) && To->GetAddableQuantity(Slot.Item) >= Slot.Item.Quantity)
			{
				FWEItemInstance Taken;
				int32 Added = 0;
				From->RemoveFromSlot(Slot.SlotIndex, Slot.Item.Quantity, Taken);
				To->TryAddItem(Taken, Added);
			}
		}
	};

	struct FJunk { const TCHAR* Id; EWEItemCondition Condition; };
	const FJunk Junk[] = { { TEXT("ElectricMotor"), EWEItemCondition::Defective }, { TEXT("CrtMonitor"), EWEItemCondition::Defective },
		{ TEXT("CableBox"), EWEItemCondition::Used }, { TEXT("OldComputer"), EWEItemCondition::Defective } };
	constexpr int32 Runs = 8;
	UWEItemDefinition* Kit = Item(TEXT("MaintenanceKit"));
	Shredder->SetPower(true);
	Sorter->SetPower(true);
	Press->SetPower(true);
	double TotalDirect = 0.0;
	double TotalChain = 0.0;
	for (const FJunk& J : Junk)
	{
		UWEItemDefinition* Def = Item(J.Id);
		if (!TestNotNull(J.Id, Def))
		{
			return false;
		}
		const FWEItemInstance One = FWEItemInstance::Make(Def, 1, J.Condition);
		const int64 Direct = Terminal->GetBestSellPrice(One).GetCents() * Runs;
		const FWEMoney BillBefore = F.Company->GetBalance();
		int64 Revenue = 0;
		int64 KitCents = 0;
		for (int32 Run = 0; Run < Runs; ++Run)
		{
			int32 Added = 0;
			Shredder->GetStorage()->TryAddItem(One, Added);
			F.Run(70.f);
			// Shredded -> sorter, sorted metal -> press; whatever does not make a full batch is sold loose.
			MoveAll(Shredder->GetOutputStorage(), Sorter->GetStorage());
			F.Run(80.f);
			MoveMetals(Sorter->GetOutputStorage(), Press->GetStorage());
			F.Run(80.f);
			// Maintenance is part of the running costs: a kit whenever a machine gets low.
			for (AWEMachine* Machine : { Shredder, Sorter, Press })
			{
				if (Machine->GetMaintenance() < 40.f)
				{
					F.Player->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(Kit), Added);
					Machine->Maintain(F.Player);
					KitCents += Kit->BaseValue.GetCents() * 13 / 10; // bought at the tool shop
				}
			}
		}
		Revenue += SellAll(Sorter->GetStorage()) + SellAll(Shredder->GetOutputStorage()) + SellAll(Sorter->GetOutputStorage()) + SellAll(Press->GetStorage());
		for (const TCHAR* Bale : { TEXT("SteelBale"), TEXT("AluBale"), TEXT("CopperBale") })
		{
			const int32 Count = F.CountWorldItems(Item(Bale), true);
			Revenue += Terminal->GetBestSellPrice(FWEItemInstance::Make(Item(Bale), 1, EWEItemCondition::Used, 50)).GetCents() * Count;
		}
		F.Grid->Bill();
		const int64 Power = BillBefore.GetCents() - F.Company->GetBalance().GetCents();
		const double Net = (Revenue - Power - KitCents) / 100.0 / Runs;
		const double Sale = Direct / 100.0 / Runs;
		AddInfo(FString::Printf(TEXT("%s: sell as is %.2f EUR, recycled %.2f EUR (revenue %.2f, power %.2f, maintenance %.2f EUR) per unit"),
			J.Id, Sale, Net, Revenue / 100.0 / Runs, Power / 100.0 / Runs, KitCents / 100.0 / Runs));
		TotalDirect += Sale;
		TotalChain += Net;
		if (FCString::Strcmp(J.Id, TEXT("OldComputer")) != 0)
		{
			TestTrue(*FString::Printf(TEXT("Recycling %s pays more than selling it as is"), J.Id), Net > Sale);
		}
		TestTrue(*FString::Printf(TEXT("%s: no runaway (< 3x)"), J.Id), Net < FMath::Max(Sale, 1.0) * 3.0);
	}
	AddInfo(FString::Printf(TEXT("Total: sell as is %.2f EUR, recycled %.2f EUR"), TotalDirect, TotalChain));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
