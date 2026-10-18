#include "Misc/AutomationTest.h"

#include "Economy/WEContractComponent.h"
#include "Framework/WEGameState.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "Save/WESaveGame.h"
#include "WETestItems.h"
#include "WETestWorld.h"
#include "Workshop/WEAssemblyDefinition.h"
#include "Workshop/WEDisassemblyDefinition.h"
#include "Workshop/WEWorkbench.h"
#include "Workshop/WEWorkPlan.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace WEAssemblyTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	struct FPcFixture
	{
		FWETestWorld TW;
		UWEItemDefinition* Pc = nullptr;
		const UWEAssemblyDefinition* Asm = nullptr;
		AWECharacter* Player = nullptr;
		AWEWorkbench* Bench = nullptr;

		bool Init(FAutomationTestBase& Test)
		{
			TW.SpawnGameState<AWEGameState>()->GetContracts()->SetCatalog(nullptr);
			Pc = UWEItemDefinition::FindById(TEXT("OldComputer"));
			if (!Test.TestNotNull(TEXT("PC data"), Pc) || !Test.TestTrue(TEXT("PC has a part layout"), Pc->HasAssembly()))
			{
				return false;
			}
			Asm = Pc->Assembly;
			Player = WETestItems::SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
			int32 Added = 0;
			Player->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(UWEItemDefinition::FindById(TEXT("Screwdriver"))), Added);
			Player->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(UWEItemDefinition::FindById(TEXT("SideCutter"))), Added);
			Bench = TW.Spawn<AWEWorkbench>(FVector(150.f, 0.f, 0.f));
			Bench->SetRandomSeed(11);
			return true;
		}

		AWEPickupItem* PlacePc()
		{
			AWEPickupItem* Item = AWEPickupItem::SpawnWorldItem(TW.World, FWEItemInstance::Make(Pc, 1, EWEItemCondition::Used, 60), FTransform(FVector(100.f, -100.f, 30.f)));
			Bench->PlaceItem(Item);
			return Item;
		}

		int32 Part(const TCHAR* Id) const { return Asm->FindPart(FName(Id)); }

		int32 Count(const TCHAR* ItemId) const { return Bench->GetStorage()->CountItems(UWEItemDefinition::FindById(FName(ItemId))); }
	};
}

using namespace WEAssemblyTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEAssemblyDataTest, "WorkshopEmpire.Workshop.Assembly.PcLayout", Flags)
bool FWEAssemblyDataTest::RunTest(const FString& Parameters)
{
	FPcFixture F;
	if (!F.Init(*this))
	{
		return false;
	}
	// One part per disassembly step, every part with its own model; the case comes out last.
	TestEqual(TEXT("Parts match steps"), F.Asm->Parts.Num(), F.Pc->Disassembly->NumSteps());
	TestTrue(TEXT("At least 12 parts"), F.Asm->Parts.Num() >= 12);
	for (const FWEAssemblyPart& P : F.Asm->Parts)
	{
		TestNotNull(*FString::Printf(TEXT("%s has a model"), *P.Id.ToString()), P.Mesh.Get());
		for (const FName& R : P.Requires)
		{
			TestTrue(*FString::Printf(TEXT("%s requires a known part (%s)"), *P.Id.ToString(), *R.ToString()), F.Asm->FindPart(R) != INDEX_NONE);
		}
	}
	for (const TCHAR* Id : { TEXT("SidePanel"), TEXT("GraphicsCard"), TEXT("Ram"), TEXT("CpuCooler"), TEXT("Cpu"), TEXT("HardDrive"), TEXT("Ssd"),
		TEXT("CaseFan"), TEXT("PowerSupply"), TEXT("Mainboard"), TEXT("Case") })
	{
		TestTrue(*FString::Printf(TEXT("Part %s exists"), Id), F.Part(Id) != INDEX_NONE);
	}
	// Closed case: only the side panel is reachable.
	TestTrue(TEXT("Side panel first"), F.Asm->IsAvailable(F.Part(TEXT("SidePanel")), 0));
	TestFalse(TEXT("GPU behind the side panel"), F.Asm->IsAvailable(F.Part(TEXT("GraphicsCard")), 0));
	const int32 Open = 1 << F.Part(TEXT("SidePanel"));
	TestTrue(TEXT("GPU after the panel"), F.Asm->IsAvailable(F.Part(TEXT("GraphicsCard")), Open));
	TestTrue(TEXT("Hard drive after the panel"), F.Asm->IsAvailable(F.Part(TEXT("HardDrive")), Open));
	TestFalse(TEXT("CPU under the cooler"), F.Asm->IsAvailable(F.Part(TEXT("Cpu")), Open));
	TestTrue(TEXT("CPU after the cooler"), F.Asm->IsAvailable(F.Part(TEXT("Cpu")), Open | (1 << F.Part(TEXT("CpuCooler")))));
	TestFalse(TEXT("Mainboard needs everything on it out"), F.Asm->IsAvailable(F.Part(TEXT("Mainboard")), Open | (1 << F.Part(TEXT("GraphicsCard")))));
	TestTrue(TEXT("Blockers are named"), F.Asm->GetBlockers(F.Part(TEXT("Mainboard")), Open).Num() >= 4);

	// The GPU plan follows its real fasteners: power plug, bracket screw, slot latch, then the card itself.
	const FWEWorkPlan Plan = WEWorkPlan::ForAssemblyPart(F.Pc, F.Part(TEXT("GraphicsCard")));
	const FWEAssemblyPart& Gpu = F.Asm->Parts[F.Part(TEXT("GraphicsCard"))];
	TestEqual(TEXT("One task per fastener plus the part"), Plan.Tasks.Num(), Gpu.Fasteners.Num() + 1);
	if (Plan.Tasks.Num() == 4)
	{
		TestTrue(TEXT("Plug first"), Plan.Tasks[0].Kind == EWEWorkTaskKind::Unplug);
		TestTrue(TEXT("Then the bracket screw"), Plan.Tasks[1].Kind == EWEWorkTaskKind::Screw && Plan.Tasks[1].Phase > Plan.Tasks[0].Phase);
		TestTrue(TEXT("Then the latch"), Plan.Tasks[2].Kind == EWEWorkTaskKind::Clip && Plan.Tasks[2].Phase > Plan.Tasks[1].Phase);
		TestTrue(TEXT("Card last"), Plan.Tasks[3].Kind == EWEWorkTaskKind::PullPart && Plan.Tasks[3].Phase > Plan.Tasks[2].Phase);
		TestTrue(TEXT("Tasks sit on the part"), Plan.Tasks[0].AssemblyPart == F.Part(TEXT("GraphicsCard")) && Plan.Tasks[0].Fastener == 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEAssemblyOrderTest, "WorkshopEmpire.Workshop.Assembly.FreeOrderAndRealParts", Flags)
bool FWEAssemblyOrderTest::RunTest(const FString& Parameters)
{
	FPcFixture F;
	if (!F.Init(*this))
	{
		return false;
	}
	AWEPickupItem* Pc = F.PlacePc();
	FText Reason;
	TestFalse(TEXT("Hard drive not reachable with the panel on"), F.Bench->CanStartStep(F.Player, Reason, F.Part(TEXT("HardDrive"))));
	TestTrue(TEXT("Reason names the panel"), Reason.ToString().Contains(TEXT("Seitenteil")));

	auto Remove = [&](const TCHAR* Id)
	{
		const bool bStarted = F.Bench->TryStartStep(F.Player, F.Part(Id));
		if (bStarted)
		{
			F.Bench->CompleteActiveStep();
		}
		return bStarted;
	};
	TestTrue(TEXT("Side panel off"), Remove(TEXT("SidePanel")));
	TestEqual(TEXT("Its two screws are real screws"), F.Count(TEXT("SheetMetalScrews")), 2);
	TestTrue(TEXT("Panel marked out"), Pc->GetItem().IsPartRemoved(F.Part(TEXT("SidePanel"))));

	// Not the list order: hard drive before the graphics card.
	TestTrue(TEXT("Hard drive out first"), Remove(TEXT("HardDrive")));
	TestEqual(TEXT("Hard drive is an item"), F.Count(TEXT("HardDrive")), 1);
	TestFalse(TEXT("GPU still inside"), Pc->GetItem().IsPartRemoved(F.Part(TEXT("GraphicsCard"))));
	TestFalse(TEXT("Same part twice: refused"), F.Bench->CanStartStep(F.Player, Reason, F.Part(TEXT("HardDrive"))));
	TestFalse(TEXT("CPU before cooler: refused"), F.Bench->CanStartStep(F.Player, Reason, F.Part(TEXT("Cpu"))));
	TestTrue(TEXT("...and the cooler is named"), Reason.ToString().Contains(TEXT("Kühler")));
	TestEqual(TEXT("Progress counts removed parts"), static_cast<int32>(Pc->GetItem().DisassemblyStep), 2);

	// The rest in default order (what employees do): every part comes out, the case is last and uses the PC up.
	int32 Guard = 0;
	while (IsValid(Pc) && F.Bench->GetPlacedItem() && F.Bench->TryStartStep(F.Player) && ++Guard < 32)
	{
		F.Bench->CompleteActiveStep();
	}
	TestFalse(TEXT("PC used up"), IsValid(Pc));
	TestEqual(TEXT("Graphics card"), F.Count(TEXT("GraphicsCard")), 1);
	TestEqual(TEXT("Two RAM modules (both are mounted)"), F.Count(TEXT("RamModule")), 2);
	TestEqual(TEXT("Cooler"), F.Count(TEXT("CpuCooler")), 1);
	TestEqual(TEXT("Processor"), F.Count(TEXT("Processor")), 1);
	TestEqual(TEXT("SSD"), F.Count(TEXT("SsdDrive")), 1);
	TestEqual(TEXT("Case fan"), F.Count(TEXT("CaseFan")), 1);
	TestEqual(TEXT("Power supply"), F.Count(TEXT("PowerSupply")), 1);
	TestEqual(TEXT("Mainboard"), F.Count(TEXT("Mainboard")), 1);
	int32 Screws = 0;
	for (const FWEAssemblyPart& P : F.Asm->Parts)
	{
		for (const FWEFastener& Fa : P.Fasteners)
		{
			Screws += Fa.Kind == EWEFastenerKind::Screw ? 1 : 0;
		}
	}
	TestEqual(TEXT("Every visible screw came out as an item"), F.Count(TEXT("SheetMetalScrews")), Screws);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEAssemblySaveTest, "WorkshopEmpire.Workshop.Assembly.SaveKeepsRemovedParts", Flags)
bool FWEAssemblySaveTest::RunTest(const FString& Parameters)
{
	FPcFixture F;
	if (!F.Init(*this))
	{
		return false;
	}
	FWEItemInstance Item = FWEItemInstance::Make(F.Pc, 1, EWEItemCondition::Used, 60);
	Item.RemovedParts = (1 << F.Part(TEXT("SidePanel"))) | (1 << F.Part(TEXT("HardDrive")));
	Item.DisassemblyStep = 2;
	FWEItemInstance Copy = Item;
	TestTrue(TEXT("Same state compares equal"), Copy == Item);
	Copy.RemovedParts = 1;
	TestFalse(TEXT("Other parts out: not the same item"), Copy == Item);
	TestFalse(TEXT("...and they do not stack"), Copy.CanStackWith(Item));
	TestTrue(TEXT("Saved version knows removed parts"), UWESaveGame::CurrentVersion >= 15);
	return true;
}

#endif
