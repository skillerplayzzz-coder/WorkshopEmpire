#include "Misc/AutomationTest.h"
#include "Framework/WEGameRulesComponent.h"

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
#include "Workshop/WEDisassemblyDefinition.h"
#include "Workshop/WERepairDefinition.h"
#include "Workshop/WEWorkbench.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace WETestItems;

namespace WERepairTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	UWEItemDefinition* Tool(const TCHAR* Id, EWEToolType Type)
	{
		UWEItemDefinition* Def = MakeDefinition(Id, 0.2f);
		Def->ToolType = Type;
		Def->ToolSpeedMultiplier = 1.f;
		return Def;
	}

	/** Synthetic device with two possible faults: A (screwdriver, returns the broken part) and B (hands). */
	struct FRepairFixture
	{
		FWETestWorld TW;
		AWEGameState* GameState = nullptr;
		UWEItemDefinition* Device = nullptr;
		UWEItemDefinition* PartA = nullptr;
		UWEItemDefinition* PartB = nullptr;
		UWEItemDefinition* Clean = nullptr;
		UWEItemDefinition* Multimeter = nullptr;
		UWEItemDefinition* Screwdriver = nullptr;
		UWERepairDefinition* Repair = nullptr;
		AWEWorkbench* Bench = nullptr;
		AWECharacter* Player = nullptr;

		FRepairFixture(const TCHAR* Suffix)
		{
			GameState = TW.SpawnGameState<AWEGameState>();
			GameState->GetContracts()->SetCatalog(nullptr);
			PartA = MakeDefinition(*FString::Printf(TEXT("R_PartA_%s"), Suffix), 0.5f);
			PartB = MakeDefinition(*FString::Printf(TEXT("R_PartB_%s"), Suffix), 0.1f);
			Clean = MakeDefinition(*FString::Printf(TEXT("R_Clean_%s"), Suffix), 0.2f, true, 10);
			Multimeter = Tool(TEXT("R_Multimeter"), EWEToolType::Multimeter);
			Screwdriver = Tool(TEXT("R_Screwdriver"), EWEToolType::Screwdriver);

			Repair = NewObject<UWERepairDefinition>(GetTransientPackage());
			Repair->AddToRoot();
			FWERepairFault A;
			A.DisplayName = FText::FromString(TEXT("Teil A defekt"));
			A.RemoveText = FText::FromString(TEXT("A ausbauen"));
			A.InstallText = FText::FromString(TEXT("A einbauen"));
			A.Part = PartA;
			A.Tool = EWEToolType::Screwdriver;
			FWERepairFault B = A;
			B.DisplayName = FText::FromString(TEXT("Teil B defekt"));
			B.Part = PartB;
			B.Tool = EWEToolType::None;
			B.bReturnsBrokenPart = false;
			Repair->Faults = { A, B };
			Repair->SecondFaultChance = 1.f;
			Repair->DamagedPartFailChance = 0.f;
			Repair->RefurbishMaterial = Clean;

			Device = MakeDefinition(*FString::Printf(TEXT("R_Device_%s"), Suffix), 8.f, false, 1, false, 5000, EWEItemCategory::Electronics);
			Device->bRepairable = true;
			Device->Repair = Repair;
			Device->Disassembly = nullptr;

			Bench = TW.Spawn<AWEWorkbench>(FVector(150.f, 0.f, 0.f));
			Bench->SetRandomSeed(42);
			Player = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
		}

		void Give(UWEItemDefinition* Def, int32 Quantity = 1, EWEItemCondition Condition = EWEItemCondition::Used, UWEInventoryComponent* To = nullptr)
		{
			int32 Added = 0;
			(To ? To : Player->GetPlayerInventory())->TryAddItem(FWEItemInstance::Make(Def, Quantity, Condition, 60), Added);
		}

		AWEPickupItem* PlaceDevice(EWEItemCondition Condition, uint8 Quality = 40)
		{
			AWEPickupItem* Item = AWEPickupItem::SpawnWorldItem(TW.World, FWEItemInstance::Make(Device, 1, Condition, Quality), FTransform(FVector(100.f, -100.f, 30.f)));
			Bench->PlaceItem(Item);
			return Item;
		}

		/** Runs the next repair step to completion. */
		bool Step()
		{
			if (!Bench->TryStartRepairStep(Player))
			{
				return false;
			}
			Bench->CompleteActiveStep();
			return true;
		}

		EWERepairStepKind NextKind() const
		{
			FText Reason;
			return Bench->GetNextRepairStep(Reason).Kind;
		}

		FText BlockReason() const
		{
			FText Reason;
			Bench->CanStartRepairStep(Player, Reason);
			return Reason;
		}

		int32 Count(const UWEInventoryComponent* Inventory, const UWEItemDefinition* Def, EWEItemCondition Condition) const
		{
			int32 N = 0;
			for (const FWEInventorySlotData& Slot : Inventory->GetAllSlots())
			{
				N += Slot.Item.Definition == Def && Slot.Item.Condition == Condition ? Slot.Item.Quantity : 0;
			}
			return N;
		}
	};
}

using namespace WERepairTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWERepairFlowTest, "WorkshopEmpire.Repair.FullFlowExchangeTestRefurbish", Flags)
bool FWERepairFlowTest::RunTest(const FString& Parameters)
{
	FRepairFixture F(TEXT("Flow"));
	F.Give(F.Screwdriver);
	AWEPickupItem* Device = F.PlaceDevice(EWEItemCondition::Defective, 40);
	if (!TestTrue(TEXT("Repairable device can be placed"), F.Bench->GetPlacedItem() == Device))
	{
		return false;
	}

	// 1) Diagnosis needs the multimeter.
	TestEqual(TEXT("First step: diagnose"), F.NextKind(), EWERepairStepKind::Diagnose);
	TestTrue(TEXT("Without multimeter: tool missing"), F.BlockReason().ToString().Contains(TEXT("Multimeter")));
	F.Give(F.Multimeter);
	TestTrue(TEXT("Diagnose"), F.Step());
	const FWERepairState& State = Device->GetItem().Repair;
	TestTrue(TEXT("Diagnosed"), State.HasFlag(FWERepairState::Diagnosed));
	TestEqual(TEXT("Defective + 100 % second-fault chance: both parts broken"), static_cast<int32>(Device->GetItem().Repair.Faults), 0b11);
	TestTrue(TEXT("Status names the faults"), AWEWorkbench::GetRepairStatusText(Device->GetItem()).ToString().Contains(TEXT("Teil A defekt")));

	// 2) Fault A: remove (broken part comes out), install from the bench storage.
	TestEqual(TEXT("Next: remove A"), F.NextKind(), EWERepairStepKind::Remove);
	TestTrue(TEXT("Remove A"), F.Step());
	TestEqual(TEXT("Broken part A in the bench storage"), F.Count(F.Bench->GetStorage(), F.PartA, EWEItemCondition::Defective), 1);
	TestTrue(TEXT("Name shows the repair is in progress"), Device->GetItem().GetDisplayName().ToString().Contains(TEXT("in Reparatur")));
	TestEqual(TEXT("Next: install A"), F.NextKind(), EWERepairStepKind::Install);
	TestTrue(TEXT("Broken part does not count as spare part"), F.BlockReason().ToString().Contains(TEXT("Ersatzteil fehlt")));
	F.Give(F.PartA, 1, EWEItemCondition::Used, F.Bench->GetStorage());
	TestTrue(TEXT("Install A from the bench storage"), F.Step());
	TestEqual(TEXT("Spare part consumed"), F.Count(F.Bench->GetStorage(), F.PartA, EWEItemCondition::Used), 0);

	// 3) Fault B: no broken part returned, spare part from the inventory.
	TestEqual(TEXT("Next: remove B"), F.NextKind(), EWERepairStepKind::Remove);
	TestTrue(TEXT("Remove B"), F.Step());
	F.Give(F.PartB, 1, EWEItemCondition::Used);
	TestTrue(TEXT("Install B"), F.Step());
	TestEqual(TEXT("Spare part B consumed"), F.Count(F.Player->GetPlayerInventory(), F.PartB, EWEItemCondition::Used), 0);

	// 4) Function test.
	TestEqual(TEXT("Next: function test"), F.NextKind(), EWERepairStepKind::Test);
	TestTrue(TEXT("Test"), F.Step());
	const FWEItemInstance& Repaired = Device->GetItem();
	TestEqual(TEXT("Works again: Gebraucht"), Repaired.Condition, EWEItemCondition::Used);
	TestEqual(TEXT("Quality +15"), static_cast<int32>(Repaired.Quality), 55);
	TestEqual(TEXT("No faults"), static_cast<int32>(Repaired.Repair.Faults), 0);
	TestFalse(TEXT("Not in progress anymore"), Repaired.Repair.IsInProgress());
	TestTrue(TEXT("Repaired value is much higher than the broken value"),
		Repaired.GetEstimatedValue().GetCents() > FWEItemInstance::Make(F.Device, 1, EWEItemCondition::Defective, 40).GetEstimatedValue().GetCents() * 2);

	// Maintenance (dust, thermal paste) is covered by Repair.MaintenanceCleanAndPaste; here the device counts as clean.
	{
		FWEItemInstance Clean = Device->GetItem();
		Clean.Repair.SetFlag(FWERepairState::Cleaned);
		Device->SetItem(Clean);
	}
	// 5) Refurbishing (quality improvement) needs the cleaning kit.
	TestEqual(TEXT("Next: refurbish"), F.NextKind(), EWERepairStepKind::Refurbish);
	TestTrue(TEXT("Material missing"), F.BlockReason().ToString().Contains(TEXT("Material fehlt")));
	F.Give(F.Clean, 2, EWEItemCondition::New);
	TestTrue(TEXT("Refurbish"), F.Step());
	TestEqual(TEXT("Quality +30"), static_cast<int32>(Device->GetItem().Quality), 85);
	TestEqual(TEXT("80 % or more: Sehr gut"), Device->GetItem().Condition, EWEItemCondition::VeryGood);
	TestEqual(TEXT("One kit used"), F.Count(F.Player->GetPlayerInventory(), F.Clean, EWEItemCondition::New), 1);
	TestEqual(TEXT("Only once"), F.NextKind(), EWERepairStepKind::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWERepairDudTest, "WorkshopEmpire.Repair.DamagedSparePartFailsTest", Flags)
bool FWERepairDudTest::RunTest(const FString& Parameters)
{
	FRepairFixture F(TEXT("Dud"));
	F.Repair->DamagedPartFailChance = 1.f;
	F.Give(F.Screwdriver);
	F.Give(F.Multimeter);
	AWEPickupItem* Device = F.PlaceDevice(EWEItemCondition::Damaged, 70);
	TestTrue(TEXT("Diagnose"), F.Step());
	const uint8 Faults = Device->GetItem().Repair.Faults;
	TestEqual(TEXT("Damaged devices have exactly one fault"), static_cast<int32>(FMath::CountBits(Faults)), 1);
	UWEItemDefinition* Part = Faults == 0b01 ? F.PartA : F.PartB;

	TestTrue(TEXT("Remove"), F.Step());
	F.Give(Part, 1, EWEItemCondition::Damaged);
	TestTrue(TEXT("Install a damaged spare part"), F.Step());
	TestTrue(TEXT("Looks replaced before the test"), AWEWorkbench::GetRepairStatusText(Device->GetItem()).ToString().Contains(TEXT("Test ausstehend")));
	TestTrue(TEXT("Run test"), F.Step());
	TestEqual(TEXT("Test failed: still broken"), Device->GetItem().Condition, EWEItemCondition::Damaged);
	TestEqual(TEXT("Back to: remove the bad part"), F.NextKind(), EWERepairStepKind::Remove);

	TestTrue(TEXT("Remove the dud"), F.Step());
	F.Give(Part, 1, EWEItemCondition::Used);
	TestTrue(TEXT("Install a good part"), F.Step());
	TestTrue(TEXT("Test again"), F.Step());
	TestEqual(TEXT("Now it works"), Device->GetItem().Condition, EWEItemCondition::Used);
	TestEqual(TEXT("Quality 70 + 15"), static_cast<int32>(Device->GetItem().Quality), 85);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWERepairRulesTest, "WorkshopEmpire.Repair.BlockedStatesAndCustomerDevices", Flags)
bool FWERepairRulesTest::RunTest(const FString& Parameters)
{
	FRepairFixture F(TEXT("Rules"));
	F.Give(F.Screwdriver);
	F.Give(F.Multimeter);

	AWEPickupItem* Scrap = F.PlaceDevice(EWEItemCondition::Scrap);
	TestEqual(TEXT("Scrap: no repair"), F.NextKind(), EWERepairStepKind::None);
	TestTrue(TEXT("Scrap: reason"), F.BlockReason().ToString().Contains(TEXT("Schrott")));
	F.Bench->ResetForLoad();
	Scrap->Destroy();

	AWEPickupItem* Working = F.PlaceDevice(EWEItemCondition::Used, 60);
	TestTrue(TEXT("Working device: only maintenance (dust) or refurbishing"), F.NextKind() == EWERepairStepKind::Refurbish || F.NextKind() == EWERepairStepKind::Clean);
	F.Bench->ResetForLoad();
	Working->Destroy();

	// Another player cannot start while someone works.
	AWEPickupItem* Device = F.PlaceDevice(EWEItemCondition::Defective);
	AWECharacter* Other = SpawnPlayer(F.TW, FVector(0.f, 80.f, 100.f));
	int32 Added = 0;
	Other->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(F.Multimeter), Added);
	TestTrue(TEXT("First player starts the diagnosis"), F.Bench->TryStartRepairStep(F.Player));
	TestFalse(TEXT("Second player is refused while busy"), F.Bench->TryStartRepairStep(Other));
	F.Bench->CompleteActiveStep();
	F.Bench->ResetForLoad();
	Device->Destroy();

	// Customer devices: repair yes, disassemble or sell no.
	UWEDisassemblyDefinition* Dis = NewObject<UWEDisassemblyDefinition>(GetTransientPackage());
	Dis->AddToRoot();
	Dis->Steps.AddDefaulted();
	F.Device->Disassembly = Dis;
	FWEItemInstance CustomerItem = FWEItemInstance::Make(F.Device, 1, EWEItemCondition::Defective, 40);
	CustomerItem.Repair.SetFlag(FWERepairState::CustomerItem);
	AWEPickupItem* Customer = AWEPickupItem::SpawnWorldItem(F.TW.World, CustomerItem, FTransform(FVector(100.f, -100.f, 30.f)));
	F.Bench->PlaceItem(Customer);
	FText Reason;
	TestFalse(TEXT("Customer device: no disassembly"), F.Bench->CanStartStep(F.Player, Reason));
	TestTrue(TEXT("...with reason"), Reason.ToString().Contains(TEXT("Kundengerät")));
	TestTrue(TEXT("Customer device: repair allowed"), F.Bench->CanStartRepairStep(F.Player, Reason));
	F.Bench->ResetForLoad();
	Customer->SetActorLocation(F.Player->GetActorLocation() + FVector(80.f, 0.f, -40.f));
	TestTrue(TEXT("Carry the customer device"), F.Player->TryPickUpItem(Customer));
	AWETraderTerminal* Terminal = F.TW.Spawn<AWETraderTerminal>(FVector(0.f, 150.f, 0.f));
	TestEqual(TEXT("Terminal refuses to sell it"), Terminal->SellHeldItem(F.Player, FGuid::NewGuid()), EWETradeResult::InvalidItem);
	TestTrue(TEXT("Still in the hands"), F.Player->GetHeldItem() == Customer);

	// Partially disassembled: no repair any more.
	F.Player->ReleaseHeldItem();
	Customer->Destroy();
	FWEItemInstance Opened = FWEItemInstance::Make(F.Device, 1, EWEItemCondition::Defective, 40);
	Opened.DisassemblyStep = 1;
	Dis->Steps.AddDefaulted();
	AWEPickupItem* OpenedItem = AWEPickupItem::SpawnWorldItem(F.TW.World, Opened, FTransform(FVector(100.f, -100.f, 30.f)));
	F.Bench->PlaceItem(OpenedItem);
	TestEqual(TEXT("Partially disassembled: no repair"), F.NextKind(), EWERepairStepKind::None);
	TestTrue(TEXT("...with reason"), F.BlockReason().ToString().Contains(TEXT("zerlegt")));
	F.Device->Disassembly = nullptr;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWERepairDisassemblyTest, "WorkshopEmpire.Repair.DisassemblyRespectsKnownFaults", Flags)
bool FWERepairDisassemblyTest::RunTest(const FString& Parameters)
{
	// Real PC data. Fault 0 of REP_OldComputer is the power supply.
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	GameState->GetContracts()->SetCatalog(nullptr);
	UWEItemDefinition* Pc = UWEItemDefinition::FindById(TEXT("OldComputer"));
	UWEItemDefinition* Psu = UWEItemDefinition::FindById(TEXT("PowerSupply"));
	if (!TestNotNull(TEXT("PC data"), Pc) || !TestNotNull(TEXT("PSU data"), Psu) || !TestTrue(TEXT("PC is repairable"), Pc->CanBeRepaired())
		|| !TestTrue(TEXT("Fault 0 is the power supply"), Pc->Repair->Faults[0].Part.Get() == Psu))
	{
		return false;
	}
	AWECharacter* Player = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	int32 Added = 0;
	Player->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(UWEItemDefinition::FindById(TEXT("Screwdriver"))), Added);
	Player->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(UWEItemDefinition::FindById(TEXT("SideCutter"))), Added);
	AWEWorkbench* Bench = TW.Spawn<AWEWorkbench>(FVector(150.f, 0.f, 0.f));

	auto Disassemble = [&](uint8 Removed) -> TArray<FWEItemInstance>
	{
		Bench->GetStorage()->ClearAll();
		Bench->SetRandomSeed(7);
		FWEItemInstance Item = FWEItemInstance::Make(Pc, 1, EWEItemCondition::Defective, 35);
		Item.Repair.Faults = 0b1;
		Item.Repair.Removed = Removed;
		Item.Repair.Flags = FWERepairState::FaultsRolled | FWERepairState::Diagnosed;
		Item.DisassemblyStep = 0;
		Bench->PlaceItem(AWEPickupItem::SpawnWorldItem(TW.World, Item, FTransform(FVector(100.f, -100.f, 30.f))));
		while (Bench->GetPlacedItem() && Bench->TryStartStep(Player))
		{
			Bench->CompleteActiveStep();
		}
		TArray<FWEItemInstance> Psus;
		for (const FWEInventorySlotData& Slot : Bench->GetStorage()->GetAllSlots())
		{
			if (Slot.Item.Definition == Psu)
			{
				Psus.Add(Slot.Item);
			}
		}
		return Psus;
	};

	const TArray<FWEItemInstance> Broken = Disassemble(0);
	TestEqual(TEXT("Known broken PSU comes out once"), Broken.Num(), 1);
	TestTrue(TEXT("...as Defekt"), Broken.Num() == 1 && Broken[0].Condition == EWEItemCondition::Defective);
	TestEqual(TEXT("PSU already taken out for repair: none comes out (no duplicate)"), Disassemble(0b1).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWERepairBalanceTest, "WorkshopEmpire.Economy.BalanceRepairBeatsPartingOut", Flags)
bool FWERepairBalanceTest::RunTest(const FString& Parameters)
{
	// Real data, 30 seeds: buy a defective PC, diagnose, buy the needed spare parts (Used), repair, sell.
	// Compared with parting the same PC out (Economy.BalanceDefectivePcIsProfitable). Spec Phase 6:
	// "Gegenstände können sinnvoll repariert und gewinnbringend verkauft werden."
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	GameState->GetContracts()->SetCatalog(nullptr);
	const auto Trader = [](const TCHAR* Name)
	{
		return LoadObject<UWETraderDefinition>(nullptr, *FString::Printf(TEXT("/Game/WorkshopEmpire/Data/Economy/%s.%s"), Name, Name));
	};
	UWEItemDefinition* Pc = UWEItemDefinition::FindById(TEXT("OldComputer"));
	UWEItemDefinition* Kit = UWEItemDefinition::FindById(TEXT("CleaningKit"));
	UWETraderDefinition* SpareParts = Trader(TEXT("DA_Trader_SpareParts"));
	if (!TestNotNull(TEXT("PC"), Pc) || !TestNotNull(TEXT("Cleaning kit"), Kit) || !TestNotNull(TEXT("Spare parts trader"), SpareParts))
	{
		return false;
	}
	AWETraderTerminal* Terminal = TW.Spawn<AWETraderTerminal>(FVector(0.f, 400.f, 0.f));
	Terminal->SetTraders({ Trader(TEXT("DA_Trader_Scrapyard")), Trader(TEXT("DA_Trader_UsedGoods")), Trader(TEXT("DA_Trader_ToolShop")), SpareParts });
	const UWEMarketComponent* Market = UWEMarketComponent::Get(TW.World);
	auto BuyPrice = [Terminal, Market](const UWEItemDefinition* Def, EWEItemCondition Condition, uint8 Quality)
	{
		for (const FWEMarketOffer& Offer : Terminal->GetOffers())
		{
			if (Offer.Item.Definition == Def && Offer.Item.Condition == Condition)
			{
				return Offer.UnitPrice.GetCents();
			}
		}
		// Not in stock anywhere (e.g. mainboard): what a trader would ask for it.
		return UWEMarketComponent::GetBuyPrice(FWEItemInstance::Make(const_cast<UWEItemDefinition*>(Def), 1, Condition, Quality), 1.f, Market).GetCents();
	};

	AWECharacter* Player = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	UWEInventoryComponent* Inventory = Player->GetPlayerInventory();
	int32 Added = 0;
	for (const TCHAR* ToolId : { TEXT("Screwdriver"), TEXT("SideCutter"), TEXT("Multimeter") })
	{
		Inventory->TryAddItem(FWEItemInstance::Make(UWEItemDefinition::FindById(ToolId)), Added);
	}
	AWEWorkbench* Bench = TW.Spawn<AWEWorkbench>(FVector(150.f, 0.f, 0.f));
	const int64 PcCost = BuyPrice(Pc, EWEItemCondition::Defective, 35);
	const int64 KitCost = BuyPrice(Kit, EWEItemCondition::New, 80);

	constexpr int32 Runs = 30;
	int64 PartsCost = 0, RepairedRevenue = 0, RefurbishedRevenue = 0, DefectiveReturned = 0;
	for (int32 Run = 0; Run < Runs; ++Run)
	{
		Bench->GetStorage()->ClearAll();
		Bench->SetRandomSeed(2000 + Run);
		AWEPickupItem* Item = AWEPickupItem::SpawnWorldItem(TW.World, FWEItemInstance::Make(Pc, 1, EWEItemCondition::Defective, 35), FTransform(FVector(100.f, -100.f, 30.f)));
		Bench->PlaceItem(Item);
		int32 Guard = 0;
		FText Reason;
		while (Guard++ < 20 && Item->GetItem().IsBroken())
		{
			const FWERepairStepInfo Next = Bench->GetNextRepairStep(Reason);
			if (Next.Kind == EWERepairStepKind::Install)
			{
				// Buy the spare part used (like from Ersatzteil-Express).
				PartsCost += BuyPrice(Next.Material, EWEItemCondition::Used, 55);
				Inventory->TryAddItem(FWEItemInstance::Make(const_cast<UWEItemDefinition*>(Next.Material), 1, EWEItemCondition::Used, 55), Added);
			}
			if (!Bench->TryStartRepairStep(Player))
			{
				break;
			}
			Bench->CompleteActiveStep();
		}
		if (!TestFalse(FString::Printf(TEXT("Run %d repaired"), Run), Item->GetItem().IsBroken()))
		{
			return false;
		}
		RepairedRevenue += Terminal->GetBestSellPrice(Item->GetItem()).GetCents();
		for (const FWEInventorySlotData& Slot : Bench->GetStorage()->GetAllSlots())
		{
			DefectiveReturned += Terminal->GetBestSellPrice(Slot.Item).GetCents(); // the broken parts can still be sold
		}
		Inventory->TryAddItem(FWEItemInstance::Make(Kit, 1, EWEItemCondition::New, 80), Added);
		{
			// The balance compares repair and refurbishing; dust and thermal paste are optional extras (skipped here).
			FWEItemInstance Done = Item->GetItem();
			Done.Repair.SetFlag(FWERepairState::Cleaned);
			Done.Repair.SetFlag(FWERepairState::PasteRenewed);
			Item->SetItem(Done);
		}
		Bench->TryStartRepairStep(Player);
		Bench->CompleteActiveStep();
		RefurbishedRevenue += Terminal->GetBestSellPrice(Item->GetItem()).GetCents();
		Bench->ResetForLoad();
		Item->Destroy();
	}
	const double Cost = PcCost / 100.0;
	const double Parts = PartsCost / 100.0 / Runs;
	const double Repaired = RepairedRevenue / 100.0 / Runs;
	const double Returned = DefectiveReturned / 100.0 / Runs;
	const double Refurbished = RefurbishedRevenue / 100.0 / Runs;
	const double RepairProfit = Repaired + Returned - Cost - Parts;
	const double RefurbishProfit = Refurbished + Returned - Cost - Parts - KitCost / 100.0;
	AddInfo(FString::Printf(TEXT("Repair: PC %.2f + bought parts %.2f -> sells %.2f (+ broken parts %.2f) = profit %.2f EUR; refurbished sells %.2f (kit %.2f) = profit %.2f EUR"),
		Cost, Parts, Repaired, Returned, RepairProfit, Refurbished, KitCost / 100.0, RefurbishProfit));
	TestTrue(TEXT("Repairing with bought spare parts is profitable"), RepairProfit > 4.0);
	TestTrue(TEXT("Refurbishing is worth the kit"), RefurbishProfit > RepairProfit);
	TestTrue(TEXT("No runaway early on (< 40 EUR per PC)"), RefurbishProfit < 40.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWERepairOrderTest, "WorkshopEmpire.Repair.CustomerRepairOrder", Flags)
bool FWERepairOrderTest::RunTest(const FString& Parameters)
{
	FRepairFixture F(TEXT("Order"));
	UWECompanyComponent* Company = F.GameState->GetCompany();
	UWEContractComponent* Contracts = F.GameState->GetContracts();
	AWEDeliveryZone* Pallet = F.TW.Spawn<AWEDeliveryZone>(FVector(60.f, -120.f, 6.f));
	AWETraderTerminal* Terminal = F.TW.Spawn<AWETraderTerminal>(FVector(0.f, 150.f, 0.f));
	UWEContractCatalog* Catalog = NewObject<UWEContractCatalog>(GetTransientPackage());
	FWEContractTemplate Template;
	Template.Title = FText::FromString(TEXT("Kunde bringt ein Gerät"));
	Template.Kind = EWEContractKind::Repair;
	Template.Item = F.Device;
	Template.CustomerCondition = EWEItemCondition::Damaged;
	Template.MinCondition = EWEItemCondition::Used;
	Template.RewardFactor = 0.5f;
	Catalog->Templates = { Template };
	Contracts->SetCatalog(Catalog);
	Contracts->SetContracts({});
	TestTrue(TEXT("Offer"), Contracts->AddOffer(0));
	const FWEContract Offer = Contracts->GetContracts()[0];
	TestEqual(TEXT("Kind"), Offer.Kind, EWEContractKind::Repair);
	TestTrue(TEXT("Reward = labour fee > 0"), Offer.Reward.GetCents() > 0);

	TestEqual(TEXT("Accept"), Terminal->AcceptContract(F.Player, Offer.Id, FGuid::NewGuid()), EWETradeResult::Success);
	const FWEContract* Accepted = Contracts->FindContract(Offer.Id);
	AWEPickupItem* Device = nullptr;
	for (TActorIterator<AWEPickupItem> It(F.TW.World); It; ++It)
	{
		Device = It->GetItem().InstanceId == Accepted->CustomerItemId ? *It : Device;
	}
	if (!TestNotNull(TEXT("Customer device dropped off"), Device))
	{
		return false;
	}
	TestTrue(TEXT("...near the pallet"), FVector::Dist2D(Device->GetActorLocation(), Pallet->GetActorLocation()) < 150.f);
	TestTrue(TEXT("...marked as customer device"), Device->GetItem().IsCustomerItem());
	TestEqual(TEXT("...in the ordered condition"), Device->GetItem().Condition, EWEItemCondition::Damaged);

	// Broken device in the hands: cannot be delivered yet.
	Device->SetActorLocation(F.Player->GetActorLocation() + FVector(80.f, 0.f, -40.f));
	TestTrue(TEXT("Carry"), F.Player->TryPickUpItem(Device));
	TestEqual(TEXT("Not repaired: refused"), Terminal->DeliverContract(F.Player, Offer.Id, FGuid::NewGuid()), EWETradeResult::ContractFailed);

	// Repair it.
	F.Player->ReleaseHeldItem();
	TestTrue(TEXT("Place"), F.Bench->PlaceItem(Device));
	F.Give(F.Multimeter);
	F.Give(F.Screwdriver);
	F.Give(F.PartA);
	F.Give(F.PartB);
	int32 Guard = 0;
	while (Guard++ < 10 && Device->GetItem().IsBroken() && F.Step())
	{
	}
	TestFalse(TEXT("Repaired"), Device->GetItem().IsBroken());
	TestTrue(TEXT("Still the customer's"), Device->GetItem().IsCustomerItem());
	TestTrue(TEXT("Take it off the bench"), F.Bench->TakeOffItem(F.Player));

	const int64 Before = Company->GetBalance().GetCents();
	TestEqual(TEXT("Deliver"), Terminal->DeliverContract(F.Player, Offer.Id, FGuid::NewGuid()), EWETradeResult::Success);
	// Phase 19: the customer rates the repair; a good rating pays a bonus.
	TestTrue(TEXT("Rated"), Contracts->GetLastStars() >= 1 && Contracts->GetRatingCount() == 1);
	TestEqual(TEXT("Reward paid (with the rating bonus)"), Company->GetBalance().GetCents(), Before + UWEContractComponent::RewardWithBonus(Offer.Reward, Contracts->GetLastStars()).GetCents());
	TestNull(TEXT("Hands empty"), F.Player->GetHeldItem());
	TestFalse(TEXT("Device handed back (destroyed)"), IsValid(Device));
	TestNull(TEXT("Order done"), Contracts->FindContract(Offer.Id));

	// Expired order: the customer takes the device back.
	TestTrue(TEXT("Second offer"), Contracts->AddOffer(0));
	const FGuid SecondId = Contracts->GetContracts().Last().Id;
	TestEqual(TEXT("Accept second"), Contracts->Accept(SecondId, TEXT("Test")), EWEContractResult::Success);
	TArray<FWEContract> Expired = Contracts->GetContracts();
	const FGuid SecondDevice = Expired.Last().CustomerItemId;
	Expired.Last().ExpiresAt = -1.f;
	Contracts->SetContracts(Expired);
	Contracts->UpdateContracts();
	bool bStillThere = false;
	for (TActorIterator<AWEPickupItem> It(F.TW.World); It; ++It)
	{
		bStillThere |= IsValid(*It) && !It->IsActorBeingDestroyed() && It->GetItem().InstanceId == SecondDevice;
	}
	TestFalse(TEXT("Expired: device taken back"), bStillThere);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWERepairSaveTest, "WorkshopEmpire.Save.RepairStateRoundTrip", Flags)
bool FWERepairSaveTest::RunTest(const FString& Parameters)
{
	FRepairFixture F(TEXT("Save"));
	F.Give(F.Screwdriver);
	F.Give(F.Multimeter);
	AWEPickupItem* Device = F.PlaceDevice(EWEItemCondition::Defective, 40);
	F.Step(); // diagnose
	F.Step(); // remove first broken part
	const FWEItemInstance Before = Device->GetItem();
	TestTrue(TEXT("Mid-repair"), Before.Repair.IsInProgress());

	UWEContractComponent* Contracts = F.GameState->GetContracts();
	FWEContract Order;
	Order.Id = FGuid::NewGuid();
	Order.Title = FText::FromString(TEXT("Reparatur"));
	Order.Item = F.Device;
	Order.Kind = EWEContractKind::Repair;
	Order.State = EWEContractState::Accepted;
	Order.CustomerItemId = FGuid::NewGuid();
	Order.ExpiresAt = 999999.f;
	Contracts->SetContracts({ Order });

	UWESaveGame* Save = FWESaveSystem::Capture(F.TW.World);
	TestEqual(TEXT("Current version"), Save->SaveVersion, UWESaveGame::CurrentVersion);
	Contracts->SetContracts({});
	F.Bench->ResetForLoad();
	Device->Destroy();
	FWESaveSystem::Apply(F.TW.World, Save);

	AWEPickupItem* Restored = F.Bench->GetPlacedItem();
	if (!TestNotNull(TEXT("Device back on the bench"), Restored))
	{
		return false;
	}
	TestTrue(TEXT("Repair state restored"), Restored->GetItem().Repair == Before.Repair);
	TestEqual(TEXT("Same next step"), F.NextKind(), EWERepairStepKind::Install);
	const FWEContract* Loaded = Contracts->FindContract(Order.Id);
	TestTrue(TEXT("Repair order restored"), Loaded && Loaded->Kind == EWEContractKind::Repair && Loaded->CustomerItemId == Order.CustomerItemId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWERepairPhase20DevicesTest, "WorkshopEmpire.Repair.Phase20DevicesRealData", Flags)
bool FWERepairPhase20DevicesTest::RunTest(const FString& Parameters)
{
	// Phase 20: television, radio, game console and cordless drill are repaired like the PC, with their own faults and
	// spare parts that the spare parts dealer sells.
	const UWETraderDefinition* Dealer = LoadObject<UWETraderDefinition>(nullptr, TEXT("/Game/WorkshopEmpire/Data/Economy/DA_Trader_SpareParts.DA_Trader_SpareParts"));
	TestNotNull(TEXT("Spare parts dealer"), Dealer);
	for (const TCHAR* Id : { TEXT("OldTelevision"), TEXT("PortableRadio"), TEXT("GameConsole"), TEXT("CordlessDrill") })
	{
		UWEItemDefinition* Def = LoadObject<UWEItemDefinition>(nullptr, *FString::Printf(TEXT("/Game/WorkshopEmpire/Data/Items/DA_Item_%s.DA_Item_%s"), Id, Id));
		if (!TestNotNull(FString::Printf(TEXT("%s exists"), Id), Def) || !TestTrue(FString::Printf(TEXT("%s repairable"), Id), Def->bRepairable && Def->Repair != nullptr))
		{
			continue;
		}
		const UWERepairDefinition* Repair = Def->Repair;
		TestTrue(FString::Printf(TEXT("%s has own faults"), Id), Repair->Faults.Num() >= 2);
		for (const FWERepairFault& Fault : Repair->Faults)
		{
			if (!TestNotNull(FString::Printf(TEXT("%s: part of '%s'"), Id, *Fault.DisplayName.ToString()), Fault.Part.Get()))
			{
				continue;
			}
			const bool bSold = Dealer && Dealer->Stock.ContainsByPredicate([&Fault](const FWETraderStockEntry& E) { return E.Item == Fault.Part; });
			TestTrue(FString::Printf(TEXT("%s: spare part %s is sold by the dealer"), Id, *Fault.Part->ItemId.ToString()), bSold);
		}

		// Full repair at a bench with the real data: diagnose, exchange every broken part, test.
		FWETestWorld TW;
		AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
		GameState->GetContracts()->SetCatalog(nullptr);
		AWEWorkbench* Bench = TW.Spawn<AWEWorkbench>(FVector(150.f, 0.f, 0.f));
		Bench->SetRandomSeed(7);
		AWECharacter* Player = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
		auto Give = [Player](UWEItemDefinition* Item, int32 Quantity)
		{
			int32 Added = 0;
			Player->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(Item, Quantity, EWEItemCondition::Used, 70), Added);
		};
		TSet<EWEToolType> Tools = { Repair->DiagnoseTool, Repair->TestTool };
		for (const FWERepairFault& Fault : Repair->Faults)
		{
			Tools.Add(Fault.Tool);
			if (Fault.Part)
			{
				Give(Fault.Part, 2);
			}
		}
		for (const EWEToolType Type : Tools)
		{
			if (Type != EWEToolType::None)
			{
				Give(WERepairTests::Tool(*FString::Printf(TEXT("P20Tool_%d"), static_cast<int32>(Type)), Type), 1);
			}
		}
		AWEPickupItem* Device = AWEPickupItem::SpawnWorldItem(TW.World, FWEItemInstance::Make(Def, 1, EWEItemCondition::Defective, 40), FTransform(FVector(100.f, -100.f, 30.f)));
		Bench->PlaceItem(Device);
		if (!TestTrue(FString::Printf(TEXT("%s placed on the bench"), Id), Bench->GetPlacedItem() == Device))
		{
			continue;
		}
		int32 Steps = 0;
		FText Reason;
		while (Steps < 12)
		{
			const EWERepairStepKind Kind = Bench->GetNextRepairStep(Reason).Kind;
			if (Kind == EWERepairStepKind::None || Kind == EWERepairStepKind::Refurbish || Kind == EWERepairStepKind::Clean || Kind == EWERepairStepKind::Paste)
			{
				break;
			}
			if (!TestTrue(FString::Printf(TEXT("%s step %d (%d) starts: %s"), Id, Steps, static_cast<int32>(Kind), *Reason.ToString()), Bench->TryStartRepairStep(Player)))
			{
				break;
			}
			Bench->CompleteActiveStep();
			++Steps;
		}
		TestTrue(FString::Printf(TEXT("%s: at least diagnose, remove, install, test"), Id), Steps >= 4);
		TestEqual(FString::Printf(TEXT("%s works again"), Id), Device->GetItem().Condition, EWEItemCondition::Used);
		TestEqual(FString::Printf(TEXT("%s has no faults left"), Id), static_cast<int32>(Device->GetItem().Repair.Faults), 0);
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWERepairMaintenanceTest, "WorkshopEmpire.Repair.MaintenanceCleanAndPaste", Flags)
bool FWERepairMaintenanceTest::RunTest(const FString& Parameters)
{
	// Phase 21: a working but dusty device - blow the dust out (air duster), renew the thermal paste (consumed), then refurbish.
	FRepairFixture F(TEXT("Maint"));
	UWEItemDefinition* Paste = MakeDefinition(TEXT("R_Paste_Maint"), 0.03f, true, 10);
	F.Repair->PasteMaterial = Paste;
	AWEPickupItem* Device = F.PlaceDevice(EWEItemCondition::Used, 30);
	TestTrue(TEXT("Dusty device looks dirty"), Device->GetItem().GetWear().Dirt > 0.15f);
	TestEqual(TEXT("First: blow out the dust"), F.NextKind(), EWERepairStepKind::Clean);
	TestTrue(TEXT("Without air duster: tool missing"), F.BlockReason().ToString().Contains(TEXT("Druckluftspray")));
	F.Give(Tool(TEXT("R_AirDuster"), EWEToolType::AirDuster));
	TestTrue(TEXT("Clean"), F.Step());
	TestTrue(TEXT("Cleaned flag"), Device->GetItem().Repair.HasFlag(FWERepairState::Cleaned));
	TestTrue(TEXT("Dirt gone"), Device->GetItem().GetWear().Dirt < 0.1f);
	TestEqual(TEXT("Quality +5"), static_cast<int32>(Device->GetItem().Quality), 35);
	TestEqual(TEXT("Then: thermal paste"), F.NextKind(), EWERepairStepKind::Paste);
	TestTrue(TEXT("No paste: material missing"), F.BlockReason().ToString().Contains(TEXT("Material fehlt")));
	F.Give(Paste, 2, EWEItemCondition::New);
	TestTrue(TEXT("Renew paste"), F.Step());
	TestTrue(TEXT("Paste flag"), Device->GetItem().Repair.HasFlag(FWERepairState::PasteRenewed));
	TestEqual(TEXT("One tube used"), F.Count(F.Player->GetPlayerInventory(), Paste, EWEItemCondition::New), 1);
	TestEqual(TEXT("Quality +5 again"), static_cast<int32>(Device->GetItem().Quality), 40);
	TestEqual(TEXT("Then: refurbish"), F.NextKind(), EWERepairStepKind::Refurbish);
	TestTrue(TEXT("Wear text says what is seen"), !Device->GetItem().GetWearText().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWERepairRulesDifficultyTest, "WorkshopEmpire.Repair.DifficultyRules", Flags)
bool FWERepairRulesDifficultyTest::RunTest(const FString& Parameters)
{
	// Phase 21: "Entspannt" - faults known at once, one fault only, shorter steps; "Experte" with mistakes forced: a slip repeats the step.
	{
		FRepairFixture F(TEXT("Easy"));
		F.GameState->GetRules()->SetRules(FWEGameRules::FromPreset(EWEDifficultyPreset::Relaxed));
		F.Repair->SecondFaultChance = 1.f;
		F.Give(F.Screwdriver);
		F.Give(F.Multimeter);
		AWEPickupItem* Device = F.PlaceDevice(EWEItemCondition::Defective, 40);
		TestTrue(TEXT("Relaxed: diagnosed on the bench"), Device->GetItem().Repair.HasFlag(FWERepairState::Diagnosed));
		TestEqual(TEXT("Relaxed: never two faults"), static_cast<int32>(FMath::CountBits(Device->GetItem().Repair.Faults)), 1);
		TestEqual(TEXT("Relaxed: no search step"), F.NextKind(), EWERepairStepKind::Remove);
		const int32 Fault = FMath::CountTrailingZeros(static_cast<uint32>(Device->GetItem().Repair.Faults));
		TestTrue(TEXT("Relaxed: remove"), F.Bench->TryStartRepairStep(F.Player));
		TestTrue(TEXT("Relaxed steps take 70 %"), FMath::IsNearlyEqual(F.Bench->GetActiveStepDuration(), F.Repair->Faults[Fault].RemoveSeconds * 0.7f, 0.05f));
	}
	{
		FRepairFixture F(TEXT("Hard"));
		FWEGameRules Rules = FWEGameRules::FromPreset(EWEDifficultyPreset::Expert);
		F.GameState->GetRules()->SetRules(Rules);
		F.Give(F.Screwdriver);
		F.Give(F.Multimeter);
		AWEPickupItem* Device = F.PlaceDevice(EWEItemCondition::Defective, 40);
		TestFalse(TEXT("Expert: not known yet"), Device->GetItem().Repair.HasFlag(FWERepairState::Diagnosed));
		TestEqual(TEXT("Expert: search first"), F.NextKind(), EWERepairStepKind::Diagnose);
		TestTrue(TEXT("Diagnose"), F.Step());
		// Every remove step slips until the rule is off again (a hand screwdriver: speed 1).
		int32 Slips = 0;
		for (int32 i = 0; i < 40 && F.NextKind() == EWERepairStepKind::Remove; ++i)
		{
			const uint8 Before = Device->GetItem().Repair.Removed;
			F.Step();
			Slips += Device->GetItem().Repair.Removed == Before ? 1 : 0;
		}
		TestTrue(TEXT("Expert: mistakes happen (12 % per step)"), Slips > 0 || F.NextKind() != EWERepairStepKind::Remove);
		TestTrue(TEXT("Expert: still finishes"), F.NextKind() != EWERepairStepKind::Remove);
		TestTrue(TEXT("Mistake chance Expert 12 %, Realistic 7 %, none without the rule"), FMath::IsNearlyEqual(Rules.MistakeChance(), 0.12f)
			&& FMath::IsNearlyEqual(FWEGameRules::FromPreset(EWEDifficultyPreset::Realistic).MistakeChance(), 0.07f) && FWEGameRules().MistakeChance() == 0.f);
	}
	return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS
