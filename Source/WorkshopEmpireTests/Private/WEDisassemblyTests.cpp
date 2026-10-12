#include "Misc/AutomationTest.h"

#include "EngineUtils.h"
#include "Interaction/WEInteractionComponent.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "Save/WESaveGame.h"
#include "Save/WESaveSystem.h"
#include "WETestItems.h"
#include "WETestWorld.h"
#include "Workshop/WEDisassemblyDefinition.h"
#include "Workshop/WEWorkbench.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace WEDisassemblyTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	struct FTestDefs
	{
		UWEItemDefinition* Screwdriver = nullptr;
		UWEItemDefinition* Drill = nullptr;
		UWEItemDefinition* Cutter = nullptr;
		UWEItemDefinition* Screws = nullptr;
		UWEItemDefinition* Part = nullptr;
		UWEItemDefinition* Steel = nullptr;
		UWEItemDefinition* Pc = nullptr;
		UWEItemDefinition* Box = nullptr;
	};

	FWEDisassemblyOutput Out(UWEItemDefinition* Item, int32 Min, int32 Max, bool bInherit)
	{
		FWEDisassemblyOutput O;
		O.Item = Item;
		O.MinQuantity = Min;
		O.MaxQuantity = Max;
		O.bInheritCondition = bInherit;
		return O;
	}

	FWEDisassemblyStep Step(const TCHAR* Text, EWEToolType Tool, float Seconds, TArray<FWEDisassemblyOutput> Outputs)
	{
		FWEDisassemblyStep S;
		S.ActionText = FText::FromString(Text);
		S.RequiredTool = Tool;
		S.BaseSeconds = Seconds;
		S.Outputs = MoveTemp(Outputs);
		return S;
	}

	/** A 3-step "computer": screwdriver (2 s) -> 4 screws; hands (1 s) -> 1 part; side cutter (1 s) -> 3 steel. */
	FTestDefs MakeDefs()
	{
		using namespace WETestItems;
		FTestDefs D;
		D.Screwdriver = MakeDefinition(TEXT("D_Screwdriver"), 0.2f);
		D.Screwdriver->ToolType = EWEToolType::Screwdriver;
		D.Screwdriver->ToolSpeedMultiplier = 1.f;
		D.Drill = MakeDefinition(TEXT("D_Drill"), 1.3f);
		D.Drill->ToolType = EWEToolType::Screwdriver;
		D.Drill->ToolSpeedMultiplier = 2.5f;
		D.Cutter = MakeDefinition(TEXT("D_Cutter"), 0.15f);
		D.Cutter->ToolType = EWEToolType::SideCutter;
		D.Screws = MakeDefinition(TEXT("D_Screws"), 0.005f, true, 200);
		D.Part = MakeDefinition(TEXT("D_Part"), 0.6f);
		D.Steel = MakeDefinition(TEXT("D_Steel"), 1.f, true, 50);
		D.Box = MakeDefinition(TEXT("D_Box"), 6.f, false, 1, false);

		D.Pc = MakeDefinition(TEXT("D_Pc"), 9.f, false, 1, false);
		if (!D.Pc->Disassembly)
		{
			UWEDisassemblyDefinition* Dis = NewObject<UWEDisassemblyDefinition>(GetTransientPackage(), TEXT("D_PcDisassembly"));
			Dis->AddToRoot();
			D.Pc->Disassembly = Dis;
		}
		D.Pc->Disassembly->Steps = {
			Step(TEXT("Schrauben lösen"), EWEToolType::Screwdriver, 2.f, { Out(D.Screws, 4, 4, false) }),
			Step(TEXT("Teil entnehmen"), EWEToolType::None, 1.f, { Out(D.Part, 1, 1, true) }),
			Step(TEXT("Kabel abtrennen"), EWEToolType::SideCutter, 1.f, { Out(D.Steel, 3, 3, false) }),
		};
		return D;
	}

	void Give(AWECharacter* C, UWEItemDefinition* Def)
	{
		int32 Added = 0;
		C->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(Def), Added);
	}

	AWEWorkbench* SpawnBench(FWETestWorld& TW, const FVector& Location)
	{
		AWEWorkbench* Bench = TW.Spawn<AWEWorkbench>(Location);
		Bench->SetRandomSeed(42);
		return Bench;
	}
}

using namespace WEDisassemblyTests;
using namespace WETestItems;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEDisFullTest, "WorkshopEmpire.Workshop.Disassembly.FullSequence", Flags)
bool FWEDisFullTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	const FTestDefs D = MakeDefs();
	AWECharacter* A = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	Give(A, D.Screwdriver);
	Give(A, D.Cutter);
	AWEWorkbench* Bench = SpawnBench(TW, FVector(150.f, 0.f, 0.f));
	AWEPickupItem* Pc = SpawnItem(TW, D.Pc, 1, FVector(100.f, -100.f, 30.f), EWEItemCondition::Damaged);

	TestTrue(TEXT("Placed"), Bench->PlaceItem(Pc));
	TestTrue(TEXT("Item knows its bench"), Pc->GetPlacedOn() == Bench);
	TestEqual(TEXT("Prompt names step and tool"), Bench->GetNextStepPrompt().ToString(), FString(TEXT("Schrauben lösen (Schraubendreher)")));
	FText Reason;
	TestFalse(TEXT("Item on the bench cannot be carried away with E"), Pc->CanBePickedUpBy(A, Reason));

	// Step 1 via the normal [E] path on the item.
	TestTrue(TEXT("Step 1 started via interaction"), A->GetInteractionComponent()->ExecuteInteract(Pc));
	TestTrue(TEXT("Busy"), Bench->IsBusy());
	TestEqual(TEXT("Duration with basic screwdriver"), Bench->GetActiveStepDuration(), 2.f);
	Bench->CompleteActiveStep();
	TestFalse(TEXT("Idle after step"), Bench->IsBusy());
	TestEqual(TEXT("Item progress 1"), static_cast<int32>(Pc->GetItem().DisassemblyStep), 1);
	TestEqual(TEXT("4 screws in bench storage"), Bench->GetStorage()->CountItems(D.Screws), 4);

	TestTrue(TEXT("Step 2 (hands)"), Bench->TryStartStep(A));
	Bench->CompleteActiveStep();
	TestEqual(TEXT("Part in storage"), Bench->GetStorage()->CountItems(D.Part), 1);
	const FWEItemInstance* PartItem = nullptr;
	for (int32 i = 0; i < Bench->GetStorage()->GetNumSlots(); ++i)
	{
		if (const FWEItemInstance* It = Bench->GetStorage()->GetItemInSlot(i); It && It->Definition == D.Part)
		{
			PartItem = It;
		}
	}
	TestTrue(TEXT("Component inherits a condition near the source (Damaged +-1)"), PartItem &&
		FMath::Abs(static_cast<int32>(PartItem->Condition) - static_cast<int32>(EWEItemCondition::Damaged)) <= 1);

	TestTrue(TEXT("Step 3 (side cutter)"), Bench->TryStartStep(A));
	Bench->CompleteActiveStep();
	TestFalse(TEXT("Fully disassembled item is used up"), IsValid(Pc));
	TestNull(TEXT("Bench is free again"), Bench->GetPlacedItem());
	TestEqual(TEXT("3 steel in storage"), Bench->GetStorage()->CountItems(D.Steel), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEDisToolTest, "WorkshopEmpire.Workshop.Disassembly.ToolsRequiredAndFaster", Flags)
bool FWEDisToolTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	const FTestDefs D = MakeDefs();
	AWECharacter* A = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	AWEWorkbench* Bench = SpawnBench(TW, FVector(150.f, 0.f, 0.f));
	AWEPickupItem* Pc = SpawnItem(TW, D.Pc, 1, FVector(100.f, -100.f, 30.f));
	Bench->PlaceItem(Pc);

	FText Reason;
	TestFalse(TEXT("No screwdriver: refused"), Bench->CanStartStep(A, Reason));
	TestTrue(TEXT("Reason names the tool"), Reason.ToString().Contains(TEXT("Schraubendreher")));
	TestFalse(TEXT("Server refuses too"), Bench->TryStartStep(A));
	TestEqual(TEXT("FindToolSpeed: missing"), AWEWorkbench::FindToolSpeed(A, EWEToolType::Screwdriver), 0.f);
	TestEqual(TEXT("Hands always available"), AWEWorkbench::FindToolSpeed(A, EWEToolType::None), 1.f);

	Give(A, D.Screwdriver);
	Give(A, D.Drill);
	TestEqual(TEXT("Best tool is used"), AWEWorkbench::FindToolSpeed(A, EWEToolType::Screwdriver), 2.5f);
	TestTrue(TEXT("Start with drill"), Bench->TryStartStep(A));
	TestEqual(TEXT("Drill: 2 s / 2.5 = 0.8 s"), Bench->GetActiveStepDuration(), 0.8f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEDisBusyTest, "WorkshopEmpire.Workshop.Disassembly.BusyAndReach", Flags)
bool FWEDisBusyTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	const FTestDefs D = MakeDefs();
	AWECharacter* A = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	AWECharacter* B = SpawnPlayer(TW, FVector(0.f, 150.f, 100.f));
	Give(A, D.Screwdriver);
	Give(B, D.Screwdriver);
	AWEWorkbench* Bench = SpawnBench(TW, FVector(150.f, 0.f, 0.f));
	AWEPickupItem* Pc = SpawnItem(TW, D.Pc, 1, FVector(100.f, -100.f, 30.f));
	Bench->PlaceItem(Pc);

	TestTrue(TEXT("A starts"), Bench->TryStartStep(A));
	FText Reason;
	TestFalse(TEXT("B cannot start a second job"), Bench->CanStartStep(B, Reason));
	TestFalse(TEXT("B cannot take the item off while A works"), Bench->TakeOffItem(B));
	FText Secondary;
	TestFalse(TEXT("No [R] prompt while busy"), Pc->GetSecondaryInteraction(B, Secondary));

	// A walks away before the step finishes: no result, step not advanced.
	A->SetActorLocation(FVector(3000.f, 0.f, 100.f));
	Bench->CompleteActiveStep();
	TestFalse(TEXT("Cancelled"), Bench->IsBusy());
	TestEqual(TEXT("No progress"), static_cast<int32>(Pc->GetItem().DisassemblyStep), 0);
	TestEqual(TEXT("No outputs"), Bench->GetStorage()->CountItems(D.Screws), 0);

	TestTrue(TEXT("B can work now"), Bench->TryStartStep(B));
	Bench->CompleteActiveStep();
	TestEqual(TEXT("B's step counted"), static_cast<int32>(Pc->GetItem().DisassemblyStep), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEDisTakeOffTest, "WorkshopEmpire.Workshop.Disassembly.TakeOffKeepsProgress", Flags)
bool FWEDisTakeOffTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	const FTestDefs D = MakeDefs();
	AWECharacter* A = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	Give(A, D.Screwdriver);
	AWEWorkbench* Bench = SpawnBench(TW, FVector(150.f, 0.f, 0.f));
	AWEPickupItem* Pc = SpawnItem(TW, D.Pc, 1, FVector(100.f, -100.f, 30.f));
	Bench->PlaceItem(Pc);
	Bench->TryStartStep(A);
	Bench->CompleteActiveStep();

	FText Secondary;
	TestTrue(TEXT("[R] offered"), Pc->GetSecondaryInteraction(A, Secondary));
	TestTrue(TEXT("Take off via [R]"), A->GetInteractionComponent()->ExecuteInteractSecondary(Pc));
	TestTrue(TEXT("Now carried"), A->GetHeldItem() == Pc);
	TestNull(TEXT("Bench free"), Bench->GetPlacedItem());
	TestEqual(TEXT("Progress kept on the item"), static_cast<int32>(Pc->GetItem().DisassemblyStep), 1);
	TestTrue(TEXT("Name shows partial state"), Pc->GetDisplayName().ToString().Contains(TEXT("teilweise")));

	A->ReleaseHeldItem();
	TestTrue(TEXT("Placed again"), Bench->PlaceItem(Pc));
	TestEqual(TEXT("Continues at step 2"), Bench->GetNextStepIndex(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEDisPlaceTest, "WorkshopEmpire.Workshop.PlaceByAimingAndRules", Flags)
bool FWEDisPlaceTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	const FTestDefs D = MakeDefs();
	AWECharacter* A = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	AWEWorkbench* Bench = SpawnBench(TW, FVector(150.f, 0.f, 0.f));
	AWEPickupItem* Pc = SpawnItem(TW, D.Pc, 1, FVector(60.f, -80.f, 30.f));
	AWEPickupItem* Box = SpawnItem(TW, D.Box, 1, FVector(60.f, 80.f, 30.f));
	// Without a controller the server-side view uses the actor rotation: look down at the bench top.
	A->SetActorRotation(FRotator(-30.f, 0.f, 0.f));

	TestTrue(TEXT("Aiming at the bench"), A->FindAimedWorkbench() == Bench);
	TestTrue(TEXT("Carry the box"), A->TryPickUpItem(Box));
	FText Reason;
	TestFalse(TEXT("Box cannot be disassembled"), Bench->CanPlaceItem(Box, Reason));
	A->DropHeldItem();
	TestNull(TEXT("Box was dropped normally, not placed"), Box->GetPlacedOn());

	TestTrue(TEXT("Carry the PC"), A->TryPickUpItem(Pc));
	A->DropHeldItem();
	TestTrue(TEXT("PC placed on the bench by aiming"), Bench->GetPlacedItem() == Pc && Pc->GetPlacedOn() == Bench);
	TestNull(TEXT("Hands free"), A->GetHeldItem());

	AWEPickupItem* Second = SpawnItem(TW, D.Pc, 1, FVector(60.f, 0.f, 30.f));
	TestFalse(TEXT("Occupied bench refuses a second item"), Bench->CanPlaceItem(Second, Reason));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEDisOverflowTest, "WorkshopEmpire.Workshop.Disassembly.OverflowToWorld", Flags)
bool FWEDisOverflowTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	const FTestDefs D = MakeDefs();
	AWECharacter* A = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	Give(A, D.Screwdriver);
	AWEWorkbench* Bench = SpawnBench(TW, FVector(150.f, 0.f, 0.f));
	Bench->GetStorage()->Configure(FText::FromString(TEXT("Mini")), 1, 60.f);
	int32 Added = 0;
	Bench->GetStorage()->TryAddItem(FWEItemInstance::Make(D.Part), Added); // storage now full
	AWEPickupItem* Pc = SpawnItem(TW, D.Pc, 1, FVector(100.f, -100.f, 30.f));
	Bench->PlaceItem(Pc);
	Bench->TryStartStep(A);
	Bench->CompleteActiveStep();

	int32 ScrewsInWorld = 0;
	for (TActorIterator<AWEPickupItem> It(TW.World); It; ++It)
	{
		if (IsValid(*It) && It->GetItem().Definition == D.Screws)
		{
			ScrewsInWorld += It->GetItem().Quantity;
		}
	}
	TestEqual(TEXT("Outputs that do not fit land next to the bench"), ScrewsInWorld, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEDisSaveTest, "WorkshopEmpire.Save.WorkbenchProgress", Flags)
bool FWEDisSaveTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	const FTestDefs D = MakeDefs();
	AWECharacter* A = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	Give(A, D.Screwdriver);
	AWEWorkbench* Bench = SpawnBench(TW, FVector(150.f, 0.f, 0.f));
	AWEPickupItem* Pc = SpawnItem(TW, D.Pc, 1, FVector(100.f, -100.f, 30.f));
	Bench->PlaceItem(Pc);
	Bench->TryStartStep(A);
	Bench->CompleteActiveStep();

	UWESaveGame* Save = FWESaveSystem::Capture(TW.World);
	TestEqual(TEXT("Save version 2"), Save->SaveVersion, 2);
	Bench->ResetForLoad();
	FWESaveSystem::Apply(TW.World, Save);

	AWEPickupItem* Restored = Bench->GetPlacedItem();
	TestNotNull(TEXT("Item back on the bench"), Restored);
	TestTrue(TEXT("Progress restored"), Restored && Restored->GetItem().DisassemblyStep == 1);
	TestEqual(TEXT("Bench storage restored"), Bench->GetStorage()->CountItems(D.Screws), 4);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
