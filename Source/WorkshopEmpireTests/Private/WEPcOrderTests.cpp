#include "Misc/AutomationTest.h"

#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
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
#include "Workshop/WEAssemblyDefinition.h"
#include "Workshop/WEBenchmark.h"
#include "Workshop/WEWorkbench.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace WEPcOrderTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	struct FFixture
	{
		FWETestWorld TW;
		AWEGameState* GameState = nullptr;
		UWEItemDefinition* Pc = nullptr;
		const UWEAssemblyDefinition* Asm = nullptr;
		AWECharacter* Player = nullptr;
		AWEWorkbench* Bench = nullptr;
		AWETraderTerminal* Terminal = nullptr;

		bool Init(FAutomationTestBase& Test)
		{
			GameState = TW.SpawnGameState<AWEGameState>();
			GameState->GetContracts()->SetCatalog(nullptr);
			Pc = UWEItemDefinition::FindById(TEXT("OldComputer"));
			if (!Test.TestTrue(TEXT("PC data with parts"), Pc && Pc->HasAssembly()))
			{
				return false;
			}
			Asm = Pc->Assembly;
			Player = WETestItems::SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
			Give(TEXT("Screwdriver"), 1, EWEItemCondition::Used);
			Bench = TW.Spawn<AWEWorkbench>(FVector(150.f, 0.f, 0.f));
			Bench->SetRandomSeed(5);
			Terminal = TW.Spawn<AWETraderTerminal>(FVector(0.f, 150.f, 0.f));
			return true;
		}

		void Give(const TCHAR* Id, int32 Quantity, EWEItemCondition Condition)
		{
			int32 Added = 0;
			Player->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(UWEItemDefinition::FindById(FName(Id)), Quantity, Condition, 60), Added);
		}

		int32 Part(const TCHAR* Id) const { return Asm->FindPart(FName(Id)); }

		bool Remove(const TCHAR* Id)
		{
			if (!Bench->TryStartStep(Player, Part(Id)))
			{
				return false;
			}
			Bench->CompleteActiveStep();
			return true;
		}

		bool Install(const TCHAR* Id)
		{
			if (!Bench->TryStartInstall(Player, Part(Id)))
			{
				return false;
			}
			Bench->CompleteActiveStep();
			return true;
		}

		bool Test()
		{
			if (!Bench->TryStartRepairStep(Player))
			{
				return false;
			}
			Bench->CompleteActiveStep();
			return true;
		}

		/** Takes every stack of ItemId out of the bench storage. */
		void ClearStorage(const TCHAR* ItemId)
		{
			for (const FWEInventorySlotData& Slot : Bench->GetStorage()->GetAllSlots())
			{
				if (Slot.Item.Definition && Slot.Item.Definition->ItemId == FName(ItemId))
				{
					FWEItemInstance Out;
					Bench->GetStorage()->RemoveFromSlot(Slot.SlotIndex, Slot.Item.Quantity, Out);
				}
			}
		}
	};
}

using namespace WEPcOrderTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEPcBenchmarkTest, "WorkshopEmpire.PcOrders.BenchmarkAndBetterParts", Flags)
bool FWEPcBenchmarkTest::RunTest(const FString& Parameters)
{
	FFixture F;
	if (!F.Init(*this))
	{
		return false;
	}
	// Data: better parts fit the places of the old ones.
	TestTrue(TEXT("Graphics card place takes better cards"), F.Asm->Parts[F.Part(TEXT("GraphicsCard"))].Alternatives.Num() >= 1);
	TestTrue(TEXT("Parts have benchmark points"), UWEItemDefinition::FindById(TEXT("GraphicsCardPro"))->BenchmarkPoints > UWEItemDefinition::FindById(TEXT("GraphicsCard"))->BenchmarkPoints);

	AWEPickupItem* Pc = AWEPickupItem::SpawnWorldItem(F.TW.World, FWEItemInstance::Make(F.Pc, 1, EWEItemCondition::Used, 50), FTransform(FVector(100.f, -100.f, 30.f)));
	F.Bench->PlaceItem(Pc);
	const int32 Stock = WEBenchmark::Score(Pc->GetItem());
	AddInfo(FString::Printf(TEXT("Stock PC: %d points (raw %d)"), Stock, WEBenchmark::StockScore(F.Pc)));
	TestTrue(TEXT("A working stock PC has a score"), Stock > 600 && Stock < 1500);
	const FWEMoney StockValue = Pc->GetItem().GetEstimatedValue();

	// Swap the graphics card for a better one.
	TestTrue(TEXT("Panel off"), F.Remove(TEXT("SidePanel")));
	TestTrue(TEXT("Old card out"), F.Remove(TEXT("GraphicsCard")));
	TestEqual(TEXT("No score with the card out"), WEBenchmark::Score(Pc->GetItem()), 0);
	F.Give(TEXT("GraphicsCardPro"), 1, EWEItemCondition::New);
	TestTrue(TEXT("Better card in (picked over the old one in the storage)"), F.Install(TEXT("GraphicsCard")));
	TestEqual(TEXT("Variant remembered"), Pc->GetItem().GetPartVariant(F.Part(TEXT("GraphicsCard"))), 1);
	TestEqual(TEXT("Old card still in the storage"), F.Bench->GetStorage()->CountItems(UWEItemDefinition::FindById(TEXT("GraphicsCard"))), 1);
	TestEqual(TEXT("Untested: no score"), WEBenchmark::Score(Pc->GetItem()), 0);
	TestTrue(TEXT("Test"), F.Test());
	const int32 Upgraded = WEBenchmark::Score(Pc->GetItem());
	AddInfo(FString::Printf(TEXT("With the mid-range card: %d points"), Upgraded));
	TestTrue(TEXT("Faster"), Upgraded > Stock + 250);
	TestTrue(TEXT("Worth more"), Pc->GetItem().GetEstimatedValue() > StockValue);

	// Out again, it is the better card again; the save keeps the variant.
	FWEItemInstance Loaded;
	TestTrue(TEXT("Save roundtrip"), FWESaveSystem::FromSaved(FWESaveSystem::ToSaved(Pc->GetItem()), Loaded) && Loaded == Pc->GetItem());
	TestTrue(TEXT("Card out again"), F.Remove(TEXT("GraphicsCard")));
	TestEqual(TEXT("...as the mid-range card"), F.Bench->GetStorage()->CountItems(UWEItemDefinition::FindById(TEXT("GraphicsCardPro"))), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEPcOrdersTest, "WorkshopEmpire.PcOrders.BuildAndUpgradeOrders", Flags)
bool FWEPcOrdersTest::RunTest(const FString& Parameters)
{
	FFixture F;
	if (!F.Init(*this))
	{
		return false;
	}
	UWEContractComponent* Contracts = F.GameState->GetContracts();
	UWECompanyComponent* Company = F.GameState->GetCompany();
	UWEContractCatalog* Catalog = NewObject<UWEContractCatalog>(GetTransientPackage());
	FWEContractTemplate Build;
	Build.Title = FText::FromString(TEXT("Spiele-PC"));
	Build.Kind = EWEContractKind::BuildPc;
	Build.Item = F.Pc;
	Build.MinCondition = EWEItemCondition::Used;
	Build.RewardFactor = 1.0f;
	Build.MinBenchmark = 1200;
	Build.MaxBenchmark = 1200;
	FWEContractTemplate Upgrade = Build;
	Upgrade.Title = FText::FromString(TEXT("Aufrüsten"));
	Upgrade.Kind = EWEContractKind::UpgradePc;
	Catalog->Templates = { Build, Upgrade };
	Contracts->SetCatalog(Catalog);
	Contracts->SetContracts({});

	// Build order: a slow PC is refused, an upgraded one is taken and rated.
	TestTrue(TEXT("Build offer"), Contracts->AddOffer(0));
	const FWEContract Order = Contracts->GetContracts()[0];
	TestEqual(TEXT("Minimum score"), Order.MinBenchmark, 1200);
	TestTrue(TEXT("Pays more than the plain PC"), Order.Reward.GetCents() > 0);
	TestEqual(TEXT("Accept"), F.Terminal->AcceptContract(F.Player, Order.Id, FGuid::NewGuid()), EWETradeResult::Success);

	AWEPickupItem* Pc = AWEPickupItem::SpawnWorldItem(F.TW.World, FWEItemInstance::Make(F.Pc, 1, EWEItemCondition::Used, 50), FTransform(FVector(60.f, 0.f, 60.f)));
	TestTrue(TEXT("Carry a stock PC"), F.Player->TryPickUpItem(Pc));
	TestEqual(TEXT("Too slow: refused"), F.Terminal->DeliverContract(F.Player, Order.Id, FGuid::NewGuid()), EWETradeResult::ContractFailed);
	F.Player->ReleaseHeldItem();
	F.Bench->PlaceItem(Pc);
	F.Give(TEXT("GraphicsCardPro"), 1, EWEItemCondition::New);
	F.Give(TEXT("ProcessorFast"), 1, EWEItemCondition::New);
	TestTrue(TEXT("Panel"), F.Remove(TEXT("SidePanel")));
	TestTrue(TEXT("GPU out"), F.Remove(TEXT("GraphicsCard")));
	TestTrue(TEXT("Better GPU in"), F.Install(TEXT("GraphicsCard")));
	TestTrue(TEXT("Cooler out"), F.Remove(TEXT("CpuCooler")));
	TestTrue(TEXT("CPU out"), F.Remove(TEXT("Cpu")));
	TestTrue(TEXT("Faster CPU in"), F.Install(TEXT("Cpu")));
	TestTrue(TEXT("Cooler back"), F.Install(TEXT("CpuCooler")));
	TestTrue(TEXT("Panel back"), F.Install(TEXT("SidePanel")));
	TestTrue(TEXT("Test"), F.Test());
	const int32 Score = WEBenchmark::Score(Pc->GetItem());
	AddInfo(FString::Printf(TEXT("Built PC: %d points"), Score));
	TestTrue(TEXT("Fast enough"), Score >= 1200);
	TestTrue(TEXT("Off the bench"), F.Bench->TakeOffItem(F.Player));
	const int64 Before = Company->GetBalance().GetCents();
	TestEqual(TEXT("Delivered"), F.Terminal->DeliverContract(F.Player, Order.Id, FGuid::NewGuid()), EWETradeResult::Success);
	TestTrue(TEXT("Rated 3-5 stars"), Contracts->GetLastStars() >= 3);
	TestEqual(TEXT("Paid with the rating bonus"), Company->GetBalance().GetCents() - Before, UWEContractComponent::RewardWithBonus(Order.Reward, Contracts->GetLastStars()).GetCents());
	TestFalse(TEXT("PC handed over"), IsValid(Pc));

	// Upgrade order: the customer's PC comes in, parts may be swapped (not the case), it goes back faster.
	TestTrue(TEXT("Upgrade offer"), Contracts->AddOffer(1));
	const FWEContract Up = Contracts->GetContracts().Last();
	TestEqual(TEXT("Accept upgrade"), F.Terminal->AcceptContract(F.Player, Up.Id, FGuid::NewGuid()), EWETradeResult::Success);
	AWEPickupItem* Customer = nullptr;
	for (TActorIterator<AWEPickupItem> It(F.TW.World); It; ++It)
	{
		Customer = It->GetItem().InstanceId == Contracts->FindContract(Up.Id)->CustomerItemId ? *It : Customer;
	}
	if (!TestNotNull(TEXT("Customer PC dropped off"), Customer))
	{
		return false;
	}
	TestTrue(TEXT("It works but is the customer's"), !Customer->GetItem().IsBroken() && Customer->GetItem().IsCustomerItem());
	F.Bench->PlaceItem(Customer);
	F.ClearStorage(TEXT("GraphicsCard"));
	F.ClearStorage(TEXT("GraphicsCardPro"));
	F.Give(TEXT("GraphicsCardPro"), 1, EWEItemCondition::New);
	F.Give(TEXT("RamModule8"), 2, EWEItemCondition::New);
	FText Reason;
	TestFalse(TEXT("The case stays"), F.Bench->CanStartStep(F.Player, Reason, F.Part(TEXT("Case"))));
	TestTrue(TEXT("Panel"), F.Remove(TEXT("SidePanel")));
	TestTrue(TEXT("Customer GPU out"), F.Remove(TEXT("GraphicsCard")));
	TestTrue(TEXT("Better GPU in"), F.Install(TEXT("GraphicsCard")));
	TestTrue(TEXT("RAM out"), F.Remove(TEXT("Ram")));
	TestTrue(TEXT("8 GB in"), F.Install(TEXT("Ram")));
	TestTrue(TEXT("Panel back"), F.Install(TEXT("SidePanel")));
	TestTrue(TEXT("Test"), F.Test());
	AddInfo(FString::Printf(TEXT("Upgraded customer PC: %d points (needs %d)"), WEBenchmark::Score(Customer->GetItem()), Up.MinBenchmark));
	TestTrue(TEXT("Off the bench"), F.Bench->TakeOffItem(F.Player));
	TestEqual(TEXT("Upgrade delivered"), F.Terminal->DeliverContract(F.Player, Up.Id, FGuid::NewGuid()), EWETradeResult::Success);
	TestEqual(TEXT("Two ratings"), Contracts->GetRatingCount(), 2);
	TestEqual(TEXT("The old card stays in the workshop"), F.Bench->GetStorage()->CountItems(UWEItemDefinition::FindById(TEXT("GraphicsCard"))), 1);
	return true;
}

#endif
