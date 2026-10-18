#include "Misc/AutomationTest.h"

#include "Economy/WECompanyComponent.h"
#include "Framework/WEGameState.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "Progression/WETutorialComponent.h"
#include "Save/WESaveGame.h"
#include "Save/WESaveSystem.h"
#include "WETestItems.h"
#include "WETestWorld.h"
#include "Workshop/WEDisassemblyDefinition.h"
#include "Workshop/WEWorkbench.h"
#include "Workshop/WEWorkPlan.h"
#include "Workshop/WEWorkshopStyleComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace WETestItems;

namespace WEWorkViewTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	/** The test world does not tick: move its clock forward by hand (the server checks the time per task). */
	void Advance(UWorld* World, float Seconds)
	{
		// One world tick advances at most a fraction of a second: step in small ticks.
		for (float Done = 0.f; Done < Seconds; Done += 0.1f)
		{
			World->Tick(LEVELTICK_TimeOnly, 0.1f);
		}
	}

	void Give(AWECharacter* C, UWEItemDefinition* Def)
	{
		int32 Added = 0;
		C->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(Def), Added);
	}
}

using namespace WEWorkViewTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEWorkViewManualTest, "WorkshopEmpire.Workshop.WorkView.HandsOnSteps", Flags)
bool FWEWorkViewManualTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	UWEItemDefinition* Driver = MakeDefinition(TEXT("WV_Driver"), 0.2f);
	Driver->ToolType = EWEToolType::Screwdriver;
	UWEItemDefinition* Cutter = MakeDefinition(TEXT("WV_Cutter"), 0.2f);
	Cutter->ToolType = EWEToolType::SideCutter;
	UWEItemDefinition* Screws = MakeDefinition(TEXT("WV_Screws"), 0.005f, true, 200);
	UWEItemDefinition* Part = MakeDefinition(TEXT("WV_Part"), 0.6f);
	UWEItemDefinition* Device = MakeDefinition(TEXT("WV_Device"), 8.f, false, 1, false);
	if (!Device->Disassembly)
	{
		Device->Disassembly = NewObject<UWEDisassemblyDefinition>(GetTransientPackage(), TEXT("WV_DeviceDisassembly"));
		Device->Disassembly->AddToRoot();
	}
	auto MakeStep = [](const TCHAR* Text, EWEToolType Tool, float Seconds, UWEItemDefinition* Out, int32 Qty, bool bInherit)
	{
		FWEDisassemblyStep S;
		S.ActionText = FText::FromString(Text);
		S.RequiredTool = Tool;
		S.BaseSeconds = Seconds;
		FWEDisassemblyOutput O;
		O.Item = Out;
		O.MinQuantity = O.MaxQuantity = Qty;
		O.bInheritCondition = bInherit;
		S.Outputs = { O };
		return S;
	};
	Device->Disassembly->Steps = {
		MakeStep(TEXT("Rückwand abschrauben"), EWEToolType::Screwdriver, 4.f, Screws, 4, false),
		MakeStep(TEXT("Platine ausbauen"), EWEToolType::None, 2.f, Part, 1, true),
		MakeStep(TEXT("Kabel abtrennen"), EWEToolType::SideCutter, 2.f, Screws, 1, false),
	};

	AWECharacter* A = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	AWECharacter* B = SpawnPlayer(TW, FVector(0.f, 80.f, 100.f));
	Give(A, Driver);
	AWEWorkbench* Bench = TW.Spawn<AWEWorkbench>(FVector(150.f, 0.f, 0.f));
	Bench->SetRandomSeed(7);
	AWEPickupItem* Item = SpawnItem(TW, Device, 1, FVector(100.f, -100.f, 30.f), EWEItemCondition::Used);
	TestTrue(TEXT("Placed"), Bench->PlaceItem(Item));

	// The plan of step 1: screws (with the step's tool) before the cover (hands), the same on every machine.
	const FWEWorkPlan Plan = Bench->GetWorkPlan(false);
	TestTrue(TEXT("Plan has tasks"), Plan.Tasks.Num() >= 3);
	TestEqual(TEXT("Same plan again (server and client agree)"), WEWorkPlan::ForDisassembly(Device, 0).Tasks.Num(), Plan.Tasks.Num());
	int32 LidTask = INDEX_NONE;
	int32 Screw = INDEX_NONE;
	for (int32 i = 0; i < Plan.Tasks.Num(); ++i)
	{
		TestTrue(TEXT("Every task uses hands or the step's tool"), Plan.Tasks[i].Tool == EWEToolType::None || Plan.Tasks[i].Tool == EWEToolType::Screwdriver);
		if (Plan.Tasks[i].Kind == EWEWorkTaskKind::Lid) { LidTask = i; }
		if (Plan.Tasks[i].Kind == EWEWorkTaskKind::Screw && Screw == INDEX_NONE) { Screw = i; }
	}
	if (!TestTrue(TEXT("Cover and screws planned"), LidTask != INDEX_NONE && Screw != INDEX_NONE))
	{
		return false;
	}

	TestTrue(TEXT("Hands-on step starts"), Bench->TryStartManualStep(A, false));
	TestTrue(TEXT("Busy, by hand"), Bench->IsBusy() && Bench->IsManualWork());
	TestFalse(TEXT("Other player cannot work on it"), Bench->DoManualTask(B, Screw, EWEToolType::Screwdriver));
	TestFalse(TEXT("Cover before the screws is refused"), Bench->DoManualTask(A, LidTask, EWEToolType::None));
	TestFalse(TEXT("Hands on a screw are refused"), Bench->DoManualTask(A, Screw, EWEToolType::None));
	TestFalse(TEXT("Too fast is refused"), Bench->DoManualTask(A, Screw, EWEToolType::Screwdriver));
	Advance(TW.World, Plan.GetTaskSeconds(Screw, Bench->GetActiveStepDuration()));
	TestTrue(TEXT("Screw out"), Bench->DoManualTask(A, Screw, EWEToolType::Screwdriver));
	TestFalse(TEXT("Same screw twice is refused"), Bench->DoManualTask(A, Screw, EWEToolType::Screwdriver));
	TestEqual(TEXT("Nothing delivered yet"), Bench->GetStorage()->CountItems(Screws), 0);

	// Leaving the view drops the half-done step; the device keeps its progress.
	Bench->CancelManualStep(A);
	TestFalse(TEXT("Free again"), Bench->IsBusy());
	TestEqual(TEXT("Device still at step 0"), static_cast<int32>(Item->GetItem().DisassemblyStep), 0);

	// The whole step by hand, in order.
	TestTrue(TEXT("Start again"), Bench->TryStartManualStep(A, false));
	for (int32 Phase = 0; Phase < 4; ++Phase)
	{
		for (int32 i = 0; i < Plan.Tasks.Num(); ++i)
		{
			if (Plan.Tasks[i].Phase == Phase)
			{
				Advance(TW.World, Plan.GetTaskSeconds(i, Bench->GetActiveStepDuration()));
				TestTrue(*FString::Printf(TEXT("Task %d"), i), Bench->DoManualTask(A, i, Plan.Tasks[i].Tool));
			}
		}
	}
	TestFalse(TEXT("Step finished"), Bench->IsBusy());
	TestEqual(TEXT("Device at step 1"), static_cast<int32>(Item->GetItem().DisassemblyStep), 1);
	TestEqual(TEXT("4 screws delivered, like the timed step"), Bench->GetStorage()->CountItems(Screws), 4);

	// Step 3 needs the side cutter: without it neither way works; the timed path ("Automatisch") still does with it.
	Advance(TW.World, 1.f);
	TestTrue(TEXT("Step 2 by hand starts"), Bench->TryStartManualStep(A, false));
	const FWEWorkPlan Plan2 = Bench->GetWorkPlan(false);
	for (int32 Phase = 0; Phase < 4; ++Phase)
	{
		for (int32 i = 0; i < Plan2.Tasks.Num(); ++i)
		{
			if (Plan2.Tasks[i].Phase == Phase)
			{
				Advance(TW.World, Plan2.GetTaskSeconds(i, Bench->GetActiveStepDuration()));
				Bench->DoManualTask(A, i, Plan2.Tasks[i].Tool);
			}
		}
	}
	TestEqual(TEXT("Device at step 2"), static_cast<int32>(Item->GetItem().DisassemblyStep), 2);
	TestEqual(TEXT("Part delivered"), Bench->GetStorage()->CountItems(Part), 1);
	TestFalse(TEXT("No side cutter: no hands-on step"), Bench->TryStartManualStep(A, false));
	Give(A, Cutter);
	TestTrue(TEXT("Timed step still works"), Bench->TryStartStep(A));
	TestFalse(TEXT("Timed step is not hands-on"), Bench->IsManualWork());
	Bench->CompleteActiveStep();
	TestFalse(TEXT("Device used up"), IsValid(Item) && Bench->GetPlacedItem() == Item);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEWorkshopStyleTest, "WorkshopEmpire.Workshop.Style.ApplyAndSave", Flags)
bool FWEWorkshopStyleTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	UWECompanyComponent* Company = GameState->GetCompany();
	UWEWorkshopStyleComponent* Style = GameState->GetWorkshopStyle();
	AWECharacter* Player = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	if (!TestNotNull(TEXT("Style component"), Style))
	{
		return false;
	}
	Company->SetBalance(FWEMoney::FromEuros(100));
	const TArray<FWEStyleOption>& Walls = UWEWorkshopStyleComponent::GetOptions(EWEStyleCategory::WallPaint);
	TestTrue(TEXT("Several wall colours"), Walls.Num() >= 8);
	TestEqual(TEXT("Not enough money"), Style->Apply(Player, TEXT("Garage"), EWEStyleCategory::WallPaint, 4, FGuid::NewGuid()), EWEStyleResult::NotEnoughMoney);
	Company->SetBalance(FWEMoney::FromEuros(5000));
	const int64 Before = Company->GetBalance().GetCents();
	TestEqual(TEXT("Paint the garage"), Style->Apply(Player, TEXT("Garage"), EWEStyleCategory::WallPaint, 4, FGuid::NewGuid()), EWEStyleResult::Success);
	TestEqual(TEXT("Paid the price"), Before - Company->GetBalance().GetCents(), Walls[4].CostCents); // garage: price factor 1
	TestEqual(TEXT("Same again: nothing to do"), Style->Apply(Player, TEXT("Garage"), EWEStyleCategory::WallPaint, 4, FGuid::NewGuid()), EWEStyleResult::AlreadySet);
	TestEqual(TEXT("Zone not owned is refused"), Style->Apply(Player, TEXT("FactoryHall"), EWEStyleCategory::WallPaint, 1, FGuid::NewGuid()), EWEStyleResult::Locked);
	int32 HighTier = INDEX_NONE;
	for (int32 i = 0; i < Walls.Num(); ++i)
	{
		if (Walls[i].MinTier > 1) { HighTier = i; break; }
	}
	TestTrue(TEXT("Colours of a higher tier are locked at tier 1"), HighTier != INDEX_NONE
		&& Style->Apply(Player, TEXT("Garage"), EWEStyleCategory::WallPaint, HighTier, FGuid::NewGuid()) == EWEStyleResult::Locked);
	TestEqual(TEXT("Floor"), Style->Apply(Player, TEXT("Garage"), EWEStyleCategory::Floor, 1, FGuid::NewGuid()), EWEStyleResult::Success);
	TestEqual(TEXT("Door"), Style->Apply(Player, TEXT("Garage"), EWEStyleCategory::Door, 2, FGuid::NewGuid()), EWEStyleResult::Success);
	TestEqual(TEXT("Booked as equipment"), UWECompanyComponent::Classify(TEXT("Gestaltung: Garage – Wandfarbe")), EWEBookingCategory::Equipment);

	// Tutorial in the same save.
	UWETutorialComponent* Tutorial = GameState->GetTutorial();
	Tutorial->Start();
	Tutorial->NotifyEvent(EWETutorialEvent::Moved);
	const int32 TutorialStep = Tutorial->GetStep();

	UWESaveGame* Save = FWESaveSystem::Capture(TW.World);
	TestEqual(TEXT("Save version 14"), Save->SaveVersion, UWESaveGame::CurrentVersion);
	const FWEZoneStyle Saved = Style->GetZoneStyle(TEXT("Garage"));
	Style->SetAllStyles({});
	Tutorial->Skip();
	TestEqual(TEXT("Cleared"), Style->GetZoneStyle(TEXT("Garage")).WallPaint, static_cast<uint8>(0));
	FWESaveSystem::Apply(TW.World, Save);
	TestTrue(TEXT("Look restored"), Style->GetZoneStyle(TEXT("Garage")) == Saved);
	TestTrue(TEXT("Tutorial restored"), Tutorial->IsActive() && Tutorial->GetStep() == TutorialStep);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWETutorialFlowTest, "WorkshopEmpire.Progression.Tutorial.Flow", Flags)
bool FWETutorialFlowTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	UWETutorialComponent* Tutorial = GameState->GetTutorial();
	TestEqual(TEXT("Off by default"), Tutorial->GetState(), EWETutorialState::Off);
	Tutorial->NotifyEvent(EWETutorialEvent::Moved);
	TestEqual(TEXT("Events do nothing while off"), Tutorial->GetStep(), 0);
	Tutorial->Start();
	TestTrue(TEXT("Active"), Tutorial->IsActive());
	// Only the event the current step waits for moves it on; each step once.
	const TArray<FWETutorialStep>& Steps = UWETutorialComponent::GetSteps();
	TestTrue(TEXT("Steps cover moving, inventory, disassembly, repair, trade, orders, saving"), Steps.Num() >= 10);
	Tutorial->NotifyEvent(EWETutorialEvent::GameSaved);
	TestEqual(TEXT("Wrong event ignored"), Tutorial->GetStep(), 0);
	for (int32 i = 0; i < Steps.Num(); ++i)
	{
		Tutorial->NotifyEvent(Steps[i].Event);
		TestEqual(*FString::Printf(TEXT("Step %d done"), i), Tutorial->GetStep(), FMath::Min(i + 1, Steps.Num()));
	}
	TestEqual(TEXT("Completed"), Tutorial->GetState(), EWETutorialState::Completed);
	Tutorial->Start();
	TestTrue(TEXT("Can be restarted"), Tutorial->IsActive() && Tutorial->GetStep() == 0);
	Tutorial->Skip();
	TestEqual(TEXT("Skipped"), Tutorial->GetState(), EWETutorialState::Skipped);
	return true;
}

#endif
