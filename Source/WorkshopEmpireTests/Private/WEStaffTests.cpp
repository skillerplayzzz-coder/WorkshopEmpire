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
#include "Logistics/WEForklift.h"
#include "Logistics/WEVehicle.h"
#include "Machines/WEMachine.h"
#include "Machines/WEMachineDefinition.h"
#include "Machines/WEPowerGridComponent.h"
#include "Player/WECharacter.h"
#include "Progression/WEProgressionComponent.h"
#include "Save/WESaveGame.h"
#include "Save/WESaveSystem.h"
#include "Staff/WEEmployee.h"
#include "Staff/WEStaffComponent.h"
#include "WETestItems.h"
#include "WETestWorld.h"
#include "Workshop/WEWorkshopLayoutComponent.h"
#include "World/WEBuildArea.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace WETestItems;

namespace WEStaffTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	UWEItemDefinition* Item(const TCHAR* Id) { return UWEItemDefinition::FindById(FName(Id)); }

	/** Garage, real data, 5000 EUR, one player; employees walk instantly (test worlds do not tick). */
	struct FStaffFixture
	{
		FWETestWorld TW;
		AWEGameState* GameState = nullptr;
		UWECompanyComponent* Company = nullptr;
		UWEStaffComponent* Staff = nullptr;
		UWEPowerGridComponent* Grid = nullptr;
		UWEWorkshopLayoutComponent* Layout = nullptr;
		AWECharacter* Player = nullptr;

		FStaffFixture()
		{
			AWEEmployee::bInstantTravel = true;
			GameState = TW.SpawnGameState<AWEGameState>();
			GameState->GetContracts()->SetCatalog(nullptr);
			Company = GameState->GetCompany();
			Staff = GameState->GetStaff();
			Grid = GameState->GetPowerGrid();
			Layout = GameState->GetLayout();
			Staff->SetRandomSeed(99);
			Staff->RefreshApplicants();
			Company->SetBalance(FWEMoney::FromEuros(5000));
			TW.Spawn<AWEBuildArea>(FVector(0.f, 0.f, 150.f));
			Player = SpawnPlayer(TW, FVector(0.f, -230.f, 100.f));
		}

		~FStaffFixture()
		{
			AWEEmployee::bInstantTravel = false;
		}

		AWEEmployee* Employ(EWEStaffRole Role, uint8 Skill = 1)
		{
			FWEEmployeeProfile Profile;
			Profile.Name = TEXT("Test Mitarbeiter");
			Profile.Role = Role;
			Profile.Skill = Skill;
			Profile.WageCentsPerMinute = 60;
			return Staff->SpawnEmployee(Profile, FTransform(FVector(250.f, 0.f, 100.f)));
		}

		template <typename T = AWEPlaceholderProp>
		T* Place(const TCHAR* Id, float X, float Y, float Yaw = 0.f)
		{
			AWEPlaceholderProp* Prop = nullptr;
			Layout->PlaceFurniture(Player, FName(Id), FTransform(FRotator(0.f, Yaw, 0.f), FVector(X, Y, 0.f)), FGuid::NewGuid(), &Prop);
			if (AWEMachine* Machine = Cast<AWEMachine>(Prop))
			{
				Machine->SetRandomSeed(4711);
			}
			return Cast<T>(Prop);
		}

		/** Staff and machines step together, like the two server timers. */
		void Run(float Seconds)
		{
			for (float T = 0.f; T < Seconds - KINDA_SMALL_NUMBER; T += UWEStaffComponent::StepSeconds)
			{
				Staff->Step(UWEStaffComponent::StepSeconds);
				Grid->StepMachines(UWEStaffComponent::StepSeconds);
			}
		}

		static void Fill(UWEInventoryComponent* Inventory, const TCHAR* ItemId, int32 Quantity, EWEItemCondition Condition = EWEItemCondition::Used, uint8 Quality = 50)
		{
			int32 Added = 0;
			Inventory->TryAddItem(FWEItemInstance::Make(Item(ItemId), Quantity, Condition, Quality), Added);
		}

		AWETraderTerminal* SpawnTerminal()
		{
			TArray<UWETraderDefinition*> Traders;
			for (const TCHAR* Name : { TEXT("Scrapyard"), TEXT("UsedGoods"), TEXT("ToolShop"), TEXT("SpareParts"), TEXT("Returns") })
			{
				Traders.Add(LoadObject<UWETraderDefinition>(nullptr, *FString::Printf(TEXT("/Game/WorkshopEmpire/Data/Economy/DA_Trader_%s.DA_Trader_%s"), Name, Name)));
			}
			AWETraderTerminal* Terminal = TW.Spawn<AWETraderTerminal>(FVector(-150.f, 262.f, 0.f));
			Terminal->SetTraders(Traders);
			return Terminal;
		}
	};
}

using namespace WEStaffTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEStaffHireTest, "WorkshopEmpire.Staff.HireFireAndPayroll", Flags)
bool FWEStaffHireTest::RunTest(const FString& Parameters)
{
	FStaffFixture F;
	if (!TestNotNull(TEXT("Staff catalog"), F.Staff->GetCatalog()) || !TestEqual(TEXT("3 applicants"), F.Staff->GetApplicants().Num(), 3))
	{
		return false;
	}
	TSet<EWEStaffRole> Roles;
	for (const FWEEmployeeProfile& A : F.Staff->GetApplicants())
	{
		Roles.Add(A.Role);
		TestTrue(TEXT("Applicant has a name"), A.Name.Len() > 3);
		TestTrue(TEXT("Wage by role and skill"), A.WageCentsPerMinute >= 60 && A.WageCentsPerMinute <= 120);
	}
	TestEqual(TEXT("Every role among the applicants"), Roles.Num(), 3);

	const FString FirstName = F.Staff->GetApplicants()[0].Name;
	AWEEmployee* Hired = nullptr;
	const FGuid Request = FGuid::NewGuid();
	TestEqual(TEXT("Hire"), F.Staff->Hire(F.Player, 0, Request, &Hired), EWEStaffResult::Success);
	if (!TestNotNull(TEXT("Employee spawned"), Hired))
	{
		return false;
	}
	TestEqual(TEXT("Same person"), Hired->GetProfile().Name, FirstName);
	TestNotEqual(TEXT("Applicant replaced"), F.Staff->GetApplicants()[0].Name, FirstName);
	AddExpectedMessage(TEXT("duplicate or invalid request"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	TestEqual(TEXT("Same request twice: once"), F.Staff->Hire(F.Player, 0, Request), EWEStaffResult::Duplicate);
	TestEqual(TEXT("One employee"), F.Staff->GetEmployees().Num(), 1);
	TestEqual(TEXT("Waits for a task"), Hired->GetState(), EWEEmployeeState::Idle);

	// Wages: a minute of work is paid at the payroll.
	F.Run(60.f);
	const int64 Before = F.Company->GetBalance().GetCents();
	TestTrue(TEXT("Payroll paid"), F.Staff->Payroll());
	TestEqual(TEXT("One minute of wage"), Before - F.Company->GetBalance().GetCents(), static_cast<int64>(Hired->GetProfile().WageCentsPerMinute));
	// Paused time is not paid.
	Hired->SetPaused(true);
	F.Run(60.f);
	TestEqual(TEXT("Pause unpaid"), Hired->GetUnpaidSeconds(), 0.f);
	Hired->SetPaused(false);

	// No money: nobody works until the debt is paid.
	F.Run(120.f);
	F.Company->SetBalance(FWEMoney::FromCents(10));
	TestFalse(TEXT("Payroll fails"), F.Staff->Payroll());
	TestEqual(TEXT("Unpaid: stops working"), Hired->GetState(), EWEEmployeeState::Unpaid);
	TestTrue(TEXT("Debt kept"), F.Staff->GetOwedWages().GetCents() > 0);
	TestEqual(TEXT("Balance untouched"), F.Company->GetBalance().GetCents(), 10ll);
	F.Run(60.f);
	TestEqual(TEXT("No new wage while not working"), Hired->GetUnpaidSeconds(), 0.f);
	F.Company->SetBalance(FWEMoney::FromEuros(100));
	TestTrue(TEXT("Debt paid"), F.Staff->Payroll());
	TestTrue(TEXT("Works again"), Hired->GetState() != EWEEmployeeState::Unpaid);
	TestTrue(TEXT("Debt cleared"), F.Staff->GetOwedWages().IsZero());

	// Limit and firing.
	for (int32 i = 0; i < 8; ++i)
	{
		F.Staff->Hire(F.Player, i % 3, FGuid::NewGuid());
	}
	TestEqual(TEXT("At most 6"), F.Staff->GetEmployees().Num(), 6);
	TestTrue(TEXT("Fire"), F.Staff->Fire(Hired, F.Player));
	TestEqual(TEXT("5 left"), F.Staff->GetEmployees().Num(), 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEStaffTransportTest, "WorkshopEmpire.Staff.WarehouseTransport", Flags)
bool FWEStaffTransportTest::RunTest(const FString& Parameters)
{
	FStaffFixture F;
	AWEPlaceholderProp* Shelf = F.Place(TEXT("HeavyShelf"), -250.f, -150.f, 90.f);
	AWEMachine* Press = F.Place<AWEMachine>(TEXT("Press"), 150.f, 150.f);
	AWEEmployee* Worker = F.Employ(EWEStaffRole::WarehouseWorker);
	if (!TestNotNull(TEXT("Shelf"), Shelf) || !TestNotNull(TEXT("Press"), Press) || !TestNotNull(TEXT("Worker"), Worker))
	{
		return false;
	}
	FStaffFixture::Fill(Shelf->GetStorage(), TEXT("SteelScrap"), 30);
	FStaffFixture::Fill(Shelf->GetStorage(), TEXT("PlasticParts"), 10);
	TestTrue(TEXT("Shelf is a valid source"), Worker->IsValidSource(Shelf));
	TestTrue(TEXT("Press is a valid target"), Worker->IsValidTarget(Press));
	TestFalse(TEXT("Wrong task kind for the role"), Worker->AssignTask({ EWETaskKind::OperateMachine, nullptr, Press, nullptr }));

	FWEEmployeeTask Task;
	Task.Kind = EWETaskKind::Transport;
	Task.Source = Shelf;
	Task.Target = Press;
	TestTrue(TEXT("Assign transport"), Worker->AssignTask(Task));
	TestTrue(TEXT("Task text"), Worker->GetTaskText().ToString().Contains(TEXT("Transport")));
	F.Run(2.f);
	TestEqual(TEXT("Carries 25 kg (25 steel)"), Worker->GetCarried().Quantity, 25);
	F.Run(15.f);
	TestEqual(TEXT("All steel in the press"), Press->GetStorage()->CountItems(Item(TEXT("SteelScrap"))), 30);
	TestEqual(TEXT("Plastic stays: the press does not use it"), Shelf->GetStorage()->CountItems(Item(TEXT("PlasticParts"))), 10);
	TestEqual(TEXT("Nothing left that fits: waits"), Worker->GetState(), EWEEmployeeState::Waiting);

	Press->SetPower(true);
	F.Run(10.f);
	TestEqual(TEXT("The press makes a bale from what he brought"), Press->GetStorage()->CountItems(Item(TEXT("SteelScrap"))), 10);

	// [T] filter: everything -> each material -> everything.
	FStaffFixture::Fill(Shelf->GetStorage(), TEXT("SteelScrap"), 5);
	Worker->CycleFilter();
	TestTrue(TEXT("Filter set"), Worker->GetTask().Filter != nullptr);
	Worker->CycleFilter();
	Worker->CycleFilter();
	TestNull(TEXT("Back to everything"), Worker->GetTask().Filter.Get());

	// Back to a storage: the worker clears the press output.
	AWEPlaceholderProp* Crate = F.Place(TEXT("Crate"), -100.f, 200.f);
	FStaffFixture::Fill(Press->GetOutputStorage(), TEXT("AluBale"), 1, EWEItemCondition::Used);
	F.Grid->StepMachines(0.f);
	Task.Source = Press;
	Task.Target = Crate;
	TestTrue(TEXT("Machine output -> crate"), Worker->AssignTask(Task));
	F.Run(8.f);
	TestEqual(TEXT("Crate takes no bulky bale: he waits"), Worker->GetState(), EWEEmployeeState::Waiting);
	Task.Target = Shelf;
	Worker->AssignTask(Task);
	FStaffFixture::Fill(Press->GetOutputStorage(), TEXT("AluminiumScrap"), 4);
	F.Run(8.f);
	TestEqual(TEXT("Small products moved to the shelf"), Shelf->GetStorage()->CountItems(Item(TEXT("AluminiumScrap"))), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEStaffOperatorTest, "WorkshopEmpire.Staff.MachineOperator", Flags)
bool FWEStaffOperatorTest::RunTest(const FString& Parameters)
{
	FStaffFixture F;
	AWEMachine* Sorter = F.Place<AWEMachine>(TEXT("Sorter"), 0.f, 150.f);
	AWEPlaceholderProp* Store = F.Place(TEXT("LargeShelf"), 250.f, -150.f, 90.f);
	AWEEmployee* Operator = F.Employ(EWEStaffRole::MachineOperator);
	if (!TestNotNull(TEXT("Sorter"), Sorter) || !TestNotNull(TEXT("Store"), Store) || !TestNotNull(TEXT("Operator"), Operator))
	{
		return false;
	}
	FStaffFixture::Fill(Sorter->GetStorage(), TEXT("ShredMetal"), 20);
	FStaffFixture::Fill(Store->GetStorage(), TEXT("MaintenanceKit"), 1, EWEItemCondition::New, 80);
	Sorter->RestoreState(false, 20, INDEX_NONE, 0.f, 0.f, {});
	TestFalse(TEXT("Sorter off"), Sorter->IsOn());
	TestTrue(TEXT("Operate with a store"), Operator->AssignTask({ EWETaskKind::OperateMachine, Store, Sorter, nullptr }));

	F.Run(1.f);
	TestTrue(TEXT("Operator switched it on"), Sorter->IsOn());
	TestEqual(TEXT("Fetched a kit and maintained the machine"), static_cast<int32>(Sorter->GetMaintenance()), 100);
	TestEqual(TEXT("Kit used from the store"), Store->GetStorage()->CountItems(Item(TEXT("MaintenanceKit"))), 0);
	F.Run(7.f);
	TestEqual(TEXT("Operator bonus active"), Sorter->GetOperatorBonus(), AWEEmployee::OperatorBonus);
	TestEqual(TEXT("Cycle 15 % faster (5 s / 1.15)"), Sorter->GetStatus().CycleSeconds, 5.f / AWEEmployee::OperatorBonus, 0.05f);
	F.Run(40.f);
	TestEqual(TEXT("All shredded material sorted"), Sorter->GetStorage()->CountItems(Item(TEXT("ShredMetal"))), 0);
	TestTrue(TEXT("Products carried to the store"), Store->GetStorage()->CountItems(Item(TEXT("SteelScrap"))) >= 9);
	TestEqual(TEXT("Output cleared"), Sorter->GetOutputStorage()->GetAllSlots().Num(), 0);

	Operator->SetPaused(true);
	TestEqual(TEXT("Paused: bonus off"), Sorter->GetOperatorBonus(), 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEStaffVanTest, "WorkshopEmpire.Logistics.VanTours", Flags)
bool FWEStaffVanTest::RunTest(const FString& Parameters)
{
	FStaffFixture F;
	AWETraderTerminal* Terminal = F.SpawnTerminal();
	AWEVehicle* Van = F.TW.Spawn<AWEVehicle>(FVector(660.f, -190.f, 0.f));
	UWEProgressionComponent* Progression = F.GameState->GetProgression();
	if (!TestNotNull(TEXT("Van"), Van) || !TestNotNull(TEXT("Tours"), F.Staff->GetTourCatalog()))
	{
		return false;
	}
	Progression->ApplyEffects();
	TestFalse(TEXT("Hidden until bought"), Van->IsUnlocked());
	TestTrue(TEXT("Buy the old van"), Progression->GrantUpgrade(TEXT("OldVan")));
	TestTrue(TEXT("Van there"), Van->IsUnlocked());
	TestEqual(TEXT("20 cargo slots"), Van->GetStorage()->GetNumSlots(), 20);
	TestTrue(TEXT("Needs a driver"), Van->GetStartBlockReason().ToString().Contains(TEXT("Fahrer")));

	AWEEmployee* Driver = F.Employ(EWEStaffRole::Driver);
	TestTrue(TEXT("Assign the van"), Driver->AssignTask({ EWETaskKind::DriveVehicle, nullptr, Van, nullptr }));
	F.Run(1.f);
	TestTrue(TEXT("Driver at the van"), Driver->IsReadyToDrive());
	TestTrue(TEXT("Empty cargo: no sell run"), Van->GetStartBlockReason().ToString().Contains(TEXT("leer")));

	// Sell run: cargo sold directly with 15 % bonus, minus fuel.
	const FWEItemInstance Steel = FWEItemInstance::Make(Item(TEXT("SteelBale")), 1, EWEItemCondition::Used, 50);
	const FWEItemInstance Pc = FWEItemInstance::Make(Item(TEXT("OldComputer")), 1, EWEItemCondition::Defective, 40);
	int32 Added = 0;
	for (int32 i = 0; i < 3; ++i) { Van->GetStorage()->TryAddItem(Steel, Added); }
	Van->GetStorage()->TryAddItem(Pc, Added);
	const int64 Expected = FMath::RoundToInt64(Terminal->GetBestSellPrice(Steel).GetCents() * 1.15) * 3 + FMath::RoundToInt64(Terminal->GetBestSellPrice(Pc).GetCents() * 1.15);
	const FWETourDefinition* SellRun = Van->FindTour(TEXT("SellRun"));
	if (!TestNotNull(TEXT("Sell run"), SellRun) || !TestTrue(TEXT("Sell run selected"), Van->GetSelectedTour() == FName(TEXT("SellRun"))))
	{
		return false;
	}
	const int64 Start = F.Company->GetBalance().GetCents();
	TestTrue(TEXT("Start"), Van->StartTour(F.Player));
	// Phase 15: it drives off visibly first (animation), but blocks nobody from the start.
	TestTrue(TEXT("Van away"), Van->IsAway() && !Van->GetActorEnableCollision());
	TestEqual(TEXT("Driver on tour"), Driver->GetState(), EWEEmployeeState::Away);
	TestEqual(TEXT("Fuel paid"), Start - F.Company->GetBalance().GetCents(), SellRun->FuelCost.GetCents());
	F.Run(SellRun->Minutes * 60.f - 5.f);
	TestTrue(TEXT("Still away"), Van->IsAway());
	F.Run(6.f);
	TestFalse(TEXT("Back"), Van->IsAway());
	TestEqual(TEXT("Sold with bonus"), F.Company->GetBalance().GetCents() - (Start - SellRun->FuelCost.GetCents()), Expected);
	TestEqual(TEXT("Cargo empty"), Van->GetStorage()->GetAllSlots().Num(), 0);
	TestTrue(TEXT("Driver back"), Driver->GetState() != EWEEmployeeState::Away && !Driver->IsHidden());

	// Pickup: bulk purchase paid at departure, arrives in the cargo.
	Van->CycleTour();
	const FWETourDefinition* Pickup = Van->FindTour(Van->GetSelectedTour());
	TestTrue(TEXT("Pickup tour"), Pickup && Pickup->Kind == EWETourKind::Pickup);
	F.Run(1.f);
	const int64 BeforePickup = F.Company->GetBalance().GetCents();
	TestTrue(TEXT("Start pickup"), Van->StartTour(F.Player));
	TestEqual(TEXT("Lot + fuel paid"), BeforePickup - F.Company->GetBalance().GetCents(), Pickup->PickupPrice.GetCents() + Pickup->FuelCost.GetCents());
	F.Run(Pickup->Minutes * 60.f + 1.f);
	TestEqual(TEXT("4 PCs in the cargo"), Van->GetStorage()->CountItems(Item(TEXT("OldComputer"))), 4);
	TestEqual(TEXT("4 cable boxes"), Van->GetStorage()->CountItems(Item(TEXT("CableBox"))), 4);
	TestEqual(TEXT("Unload the bulky lot behind the van"), Van->UnloadBulky(), 11);
	TestEqual(TEXT("Cargo empty"), Van->GetStorage()->GetAllSlots().Num(), 0);

	// The better van: bigger and faster.
	TestTrue(TEXT("Modern van"), Progression->GrantUpgrade(TEXT("ModernVan")));
	TestEqual(TEXT("30 slots"), Van->GetStorage()->GetNumSlots(), 30);
	TestEqual(TEXT("Tours 60 % faster"), Van->GetTourSeconds(*SellRun), SellRun->Minutes * 60.f / 1.6f, 0.1f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEForkliftTest, "WorkshopEmpire.Logistics.ForkliftCarriesHeavy", Flags)
bool FWEForkliftTest::RunTest(const FString& Parameters)
{
	FStaffFixture F;
	AWEForklift* Forklift = F.TW.Spawn<AWEForklift>(FVector(0.f, 0.f, 0.f));
	UWEItemDefinition* MotorDef = Item(TEXT("IndustrialMotorLarge"));
	if (!TestNotNull(TEXT("Forklift"), Forklift) || !TestNotNull(TEXT("Heavy motor data"), MotorDef))
	{
		return false;
	}
	F.GameState->GetProgression()->ApplyEffects();
	FText Reason;
	TestFalse(TEXT("Not bought: not usable"), Forklift->CanInteract(F.Player, Reason));
	F.GameState->GetProgression()->GrantUpgrade(TEXT("Forklift"));
	F.Player->SetActorLocation(FVector(-60.f, -80.f, 100.f));
	AWEPickupItem* Motor = SpawnItem(F.TW, MotorDef, 1, FVector(60.f, -80.f, 40.f), EWEItemCondition::Defective);
	TestFalse(TEXT("180 kg: too heavy by hand"), F.Player->TryPickUpItem(Motor));

	TestTrue(TEXT("Climb on"), Forklift->Mount(F.Player));
	TestTrue(TEXT("Driving"), F.Player->GetForklift() == Forklift && Forklift->GetDriver() == F.Player);
	TestEqual(TEXT("Lifts 1500 kg"), F.Player->GetMaxCarryWeightKg(), AWEForklift::LiftKg);
	TestFalse(TEXT("Forklift collision off while driven"), Forklift->GetActorEnableCollision());
	TestTrue(TEXT("Lift the motor"), F.Player->TryPickUpItem(Motor));
	TestTrue(TEXT("On the forks"), Motor->GetAttachParentActor() == Forklift);

	// Into the shredder (it takes 300 kg).
	AWEMachine* Shredder = F.Place<AWEMachine>(TEXT("Shredder"), 200.f, 150.f);
	if (TestNotNull(TEXT("Shredder"), Shredder))
	{
		TestTrue(TEXT("Shredder takes the big motor"), Shredder->CanInsertItem(Motor, Reason));
	}

	TestTrue(TEXT("Get off"), Forklift->Dismount());
	TestNull(TEXT("Not driving"), F.Player->GetForklift());
	TestNull(TEXT("Load set down"), F.Player->GetHeldItem());
	TestTrue(TEXT("Forklift collides again"), Forklift->GetActorEnableCollision());
	TestNull(TEXT("Forklift stays (detached)"), Forklift->GetAttachParentActor());
	TestEqual(TEXT("Normal carry limit"), F.Player->GetMaxCarryWeightKg(), 30.f);

	AWEPickupItem* Box = SpawnItem(F.TW, Item(TEXT("CableBox")), 1, F.Player->GetActorLocation() + FVector(60.f, 0.f, -40.f));
	F.Player->TryPickUpItem(Box);
	TestFalse(TEXT("Hands full: cannot climb on"), Forklift->CanInteract(F.Player, Reason));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEStaffSaveTest, "WorkshopEmpire.Save.StaffRoundTrip", Flags)
bool FWEStaffSaveTest::RunTest(const FString& Parameters)
{
	FStaffFixture F;
	AWEPlaceholderProp* Shelf = F.Place(TEXT("HeavyShelf"), -250.f, -150.f, 90.f);
	AWEMachine* Press = F.Place<AWEMachine>(TEXT("Press"), 150.f, 150.f);
	AWEVehicle* Van = F.TW.Spawn<AWEVehicle>(FVector(660.f, -190.f, 0.f));
	F.GameState->GetProgression()->GrantUpgrade(TEXT("OldVan"));
	AWEEmployee* Worker = F.Employ(EWEStaffRole::WarehouseWorker, 2);
	AWEEmployee* Driver = F.Employ(EWEStaffRole::Driver);
	if (!Shelf || !Press || !Van || !TestNotNull(TEXT("Worker"), Worker) || !TestNotNull(TEXT("Driver"), Driver))
	{
		return false;
	}
	FStaffFixture::Fill(Shelf->GetStorage(), TEXT("SteelScrap"), 40);
	FWEEmployeeTask Task;
	Task.Kind = EWETaskKind::Transport;
	Task.Source = Shelf;
	Task.Target = Press;
	Task.Filter = Item(TEXT("SteelScrap"));
	Worker->AssignTask(Task);
	Driver->AssignTask({ EWETaskKind::DriveVehicle, nullptr, Van, nullptr });
	F.Run(1.5f);
	TestTrue(TEXT("Worker carries steel"), Worker->GetCarried().IsValid());
	int32 Added = 0;
	Van->GetStorage()->TryAddItem(FWEItemInstance::Make(Item(TEXT("SteelBale")), 1, EWEItemCondition::Used, 50), Added);
	TestTrue(TEXT("Van leaves"), Van->StartTour(F.Player));
	F.Run(30.f);
	const float Remaining = Van->GetRemainingSeconds();
	const FName WorkerName = Worker->GetFName();
	const int32 Carried = Worker->GetCarried().Quantity;
	const FString Applicant = F.Staff->GetApplicants()[1].Name;

	UWESaveGame* Save = FWESaveSystem::Capture(F.TW.World);
	TestEqual(TEXT("Current version (10 added staff)"), Save->SaveVersion, UWESaveGame::CurrentVersion);
	TestEqual(TEXT("2 employees saved"), Save->Employees.Num(), 2);

	F.Staff->Fire(Worker, F.Player);
	F.Run(Remaining + 1.f);
	TestFalse(TEXT("Van back before loading"), Van->IsAway());
	F.Staff->RefreshApplicants();
	FWESaveSystem::Apply(F.TW.World, Save);

	AWEEmployee* LoadedWorker = nullptr;
	AWEEmployee* LoadedDriver = nullptr;
	for (AWEEmployee* E : F.Staff->GetEmployees())
	{
		(E->GetFName() == WorkerName ? LoadedWorker : LoadedDriver) = E;
	}
	if (!TestNotNull(TEXT("Fired worker is back"), LoadedWorker) || !TestNotNull(TEXT("Driver"), LoadedDriver))
	{
		return false;
	}
	TestEqual(TEXT("Skill kept"), static_cast<int32>(LoadedWorker->GetProfile().Skill), 2);
	TestTrue(TEXT("Task restored"), LoadedWorker->GetTask().Source == Shelf && LoadedWorker->GetTask().Target == Press);
	TestTrue(TEXT("Filter restored"), LoadedWorker->GetTask().Filter == Item(TEXT("SteelScrap")));
	TestEqual(TEXT("Still carries"), LoadedWorker->GetCarried().Quantity, Carried);
	TestTrue(TEXT("Van away again"), Van->IsAway());
	TestEqual(TEXT("Tour time restored"), Van->GetRemainingSeconds(), Remaining, 0.5f);
	TestEqual(TEXT("Driver on tour"), LoadedDriver->GetState(), EWEEmployeeState::Away);
	TestEqual(TEXT("Applicants restored"), F.Staff->GetApplicants()[1].Name, Applicant);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEStaffBalanceTest, "WorkshopEmpire.Economy.BalanceStaffAndTours", Flags)
bool FWEStaffBalanceTest::RunTest(const FString& Parameters)
{
	// Real data: is a bulk pickup cheaper than buying at the market, without a resell loop? Does a
	// worker earn his wage feeding the press?
	FStaffFixture F;
	AWETraderTerminal* Terminal = F.SpawnTerminal();
	const FWETourDefinition* Lot = F.Staff->GetTourCatalog() ? F.Staff->GetTourCatalog()->Find(TEXT("PickupScrapLot")) : nullptr;
	const FWETourDefinition* Motors = F.Staff->GetTourCatalog() ? F.Staff->GetTourCatalog()->Find(TEXT("PickupMotors")) : nullptr;
	if (!TestNotNull(TEXT("Lot"), Lot) || !TestNotNull(TEXT("Motors"), Motors))
	{
		return false;
	}
	const UWEMarketComponent* Market = F.GameState->GetMarket();
	auto Measure = [&](const FWETourDefinition& Tour, double& OutMarketBuy, double& OutResale)
	{
		OutMarketBuy = 0.0;
		OutResale = 0.0;
		for (const FWETourItem& Entry : Tour.PickupItems)
		{
			const FWEItemInstance One = FWEItemInstance::Make(Entry.Item, 1, Entry.Condition, static_cast<uint8>(Entry.Quality));
			OutMarketBuy += UWEMarketComponent::GetBuyPrice(One, 1.f, Market).GetCents() / 100.0 * Entry.Quantity;
			OutResale += Terminal->GetBestSellPrice(One).GetCents() / 100.0 * Entry.Quantity;
		}
	};
	for (const FWETourDefinition* Tour : { Lot, Motors })
	{
		double Buy = 0.0, Resale = 0.0;
		Measure(*Tour, Buy, Resale);
		const double Cost = (Tour->PickupPrice.GetCents() + Tour->FuelCost.GetCents()) / 100.0;
		AddInfo(FString::Printf(TEXT("%s: costs %.2f EUR (lot + fuel); same goods at the market %.2f EUR; sold on directly %.2f EUR (sell run %.2f EUR)"),
			*Tour->DisplayName.ToString(), Cost, Buy, Resale, Resale * 1.15));
		TestTrue(*FString::Printf(TEXT("%s is cheaper than the market"), *Tour->DisplayName.ToString()), Cost < Buy);
		TestTrue(*FString::Printf(TEXT("%s: no buy-and-resell loop"), *Tour->DisplayName.ToString()), Resale * 1.15 < Cost);
	}

	// A warehouse worker feeds the press from a shelf for 10 minutes; bales sold on a sell run.
	AWEPlaceholderProp* Shelf = F.Place(TEXT("HeavyShelf"), -250.f, -150.f, 90.f);
	AWEMachine* Press = F.Place<AWEMachine>(TEXT("Press"), 150.f, 150.f);
	AWEEmployee* Worker = F.Employ(EWEStaffRole::WarehouseWorker);
	if (!Shelf || !Press || !Worker)
	{
		return false;
	}
	FStaffFixture::Fill(Shelf->GetStorage(), TEXT("SteelScrap"), 50);
	FStaffFixture::Fill(Shelf->GetStorage(), TEXT("SteelScrap"), 50);
	FStaffFixture::Fill(Shelf->GetStorage(), TEXT("SteelScrap"), 50);
	Worker->AssignTask({ EWETaskKind::Transport, Shelf, Press, nullptr });
	Press->SetPower(true);
	F.Run(600.f);
	int32 Bales = 0;
	for (TActorIterator<AWEPickupItem> It(F.TW.World); It; ++It)
	{
		Bales += It->GetItem().Definition == Item(TEXT("SteelBale")) ? 1 : 0;
	}
	const FWEItemInstance OneBale = FWEItemInstance::Make(Item(TEXT("SteelBale")), 1, EWEItemCondition::Used, 50);
	const FWEItemInstance OneScrap = FWEItemInstance::Make(Item(TEXT("SteelScrap")), 20, EWEItemCondition::Used, 50);
	const double Added = Bales * (Terminal->GetBestSellPrice(OneBale).GetCents() - Terminal->GetBestSellPrice(OneScrap).GetCents()) / 100.0;
	const double Wage = Worker->GetProfile().WageCentsPerMinute * 10 / 100.0;
	AddInfo(FString::Printf(TEXT("Worker feeding the press, 10 min: %d bales, value added %.2f EUR, wage %.2f EUR"), Bales, Added, Wage));
	TestTrue(TEXT("The worker keeps the press busy (7 bales from 150 steel)"), Bales == 7);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
