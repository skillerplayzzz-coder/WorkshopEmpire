#include "Misc/AutomationTest.h"

#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "EngineUtils.h"
#include "Framework/WEGameState.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "Progression/WEProgressionComponent.h"
#include "Progression/WEUpgradeDefinition.h"
#include "Save/WESaveGame.h"
#include "Save/WESaveSystem.h"
#include "WETestItems.h"
#include "WETestWorld.h"
#include "Workshop/WEBuildComponent.h"
#include "Workshop/WEDisassemblyDefinition.h"
#include "Workshop/WEFurnitureDefinition.h"
#include "Workshop/WEWorkbench.h"
#include "Workshop/WEWorkshopLayoutComponent.h"
#include "World/WEBuildArea.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace WETestItems;

namespace WEWorkshopTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	/** Garage-sized build area, real furniture catalog, 500 EUR, one player near the origin. */
	struct FBuildFixture
	{
		FWETestWorld TW;
		AWEGameState* GameState = nullptr;
		UWEWorkshopLayoutComponent* Layout = nullptr;
		UWECompanyComponent* Company = nullptr;
		AWECharacter* Player = nullptr;

		FBuildFixture()
		{
			GameState = TW.SpawnGameState<AWEGameState>();
			GameState->GetContracts()->SetCatalog(nullptr);
			Layout = GameState->GetLayout();
			Company = GameState->GetCompany();
			Company->SetBalance(FWEMoney::FromEuros(500));
			TW.Spawn<AWEBuildArea>(FVector(0.f, 0.f, 150.f));
			Player = SpawnPlayer(TW, FVector(-300.f, -200.f, 100.f));
		}

		const FWEFurnitureDefinition* Def(const TCHAR* Id) const { return Layout->FindDefinition(FName(Id)); }

		static FTransform At(float X, float Y, float Yaw = 0.f) { return FTransform(FRotator(0.f, Yaw, 0.f), FVector(X, Y, 0.f)); }

		AWEPlaceholderProp* Place(const TCHAR* Id, const FTransform& Transform, EWEBuildResult* OutResult = nullptr)
		{
			AWEPlaceholderProp* Prop = nullptr;
			const EWEBuildResult Result = Layout->PlaceFurniture(Player, FName(Id), Transform, FGuid::NewGuid(), &Prop);
			if (OutResult)
			{
				*OutResult = Result;
			}
			return Prop;
		}
	};
}

using namespace WEWorkshopTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEBuildPlaceTest, "WorkshopEmpire.Build.PlaceRulesAndCosts", Flags)
bool FWEBuildPlaceTest::RunTest(const FString& Parameters)
{
	FBuildFixture F;
	if (!TestNotNull(TEXT("Real catalog"), F.Layout->GetCatalog()) || !TestNotNull(TEXT("Heavy shelf"), F.Def(TEXT("HeavyShelf"))))
	{
		return false;
	}
	FText Reason;
	const FVector ShelfSize = F.Def(TEXT("HeavyShelf"))->Size;
	TestTrue(TEXT("Free spot inside: ok"), F.Layout->CanPlaceAt(ShelfSize, FBuildFixture::At(200.f, 150.f, 90.f), nullptr, Reason));
	TestFalse(TEXT("Outside the garage: refused"), F.Layout->CanPlaceAt(ShelfSize, FBuildFixture::At(600.f, 0.f), nullptr, Reason));
	TestTrue(TEXT("...with reason"), Reason.ToString().Contains(TEXT("Werkstatt")));
	TestFalse(TEXT("Half over the wall line: refused"), F.Layout->CanPlaceAt(ShelfSize, FBuildFixture::At(380.f, 0.f), nullptr, Reason));

	// Place: costs money, snaps to the grid and 15° steps.
	EWEBuildResult Result;
	AWEPlaceholderProp* Shelf = F.Place(TEXT("HeavyShelf"), FTransform(FRotator(0.f, 97.f, 0.f), FVector(203.7f, 148.2f, 0.f)), &Result);
	TestEqual(TEXT("Placed"), Result, EWEBuildResult::Success);
	if (!TestNotNull(TEXT("Shelf spawned"), Shelf))
	{
		return false;
	}
	TestEqual(TEXT("60 EUR paid"), F.Company->GetBalance().GetCents(), 44000ll);
	TestTrue(TEXT("Snapped to 10 cm"), Shelf->GetActorLocation().Equals(FVector(200.f, 150.f, 0.f), 0.01f));
	TestEqual(TEXT("Snapped to 15° (97 -> 90)"), static_cast<int32>(FMath::RoundToInt(Shelf->GetActorRotation().Yaw)), 90);
	TestTrue(TEXT("Furniture id"), Shelf->GetFurnitureId() == FName(TEXT("HeavyShelf")));
	TestTrue(TEXT("Has storage"), Shelf->HasStorage());
	TestEqual(TEXT("15 slots"), Shelf->GetStorage()->GetNumSlots(), 15);
	TestEqual(TEXT("150 kg"), Shelf->GetStorage()->GetMaxWeightKg(), 150.f);

	// Refusals cost nothing.
	F.Place(TEXT("HeavyShelf"), FBuildFixture::At(200.f, 150.f, 90.f), &Result);
	TestEqual(TEXT("Same spot: blocked"), Result, EWEBuildResult::Blocked);
	AWEPickupItem* Box = SpawnItem(F.TW, MakeDefinition(TEXT("WS_Box"), 2.f, false, 1, false), 1, FVector(-100.f, 100.f, 20.f));
	TestFalse(TEXT("Item in the way"), F.Layout->CanPlaceAt(ShelfSize, FBuildFixture::At(-100.f, 100.f), nullptr, Reason));
	TestTrue(TEXT("...says so"), Reason.ToString().Contains(TEXT("Gegenstand")));
	Box->Destroy();
	TestFalse(TEXT("Player in the way"), F.Layout->CanPlaceAt(ShelfSize, FBuildFixture::At(-300.f, -200.f), nullptr, Reason));
	F.Company->SetBalance(FWEMoney::FromEuros(10));
	F.Place(TEXT("LargeShelf"), FBuildFixture::At(0.f, -200.f, 90.f), &Result);
	TestEqual(TEXT("Too expensive"), Result, EWEBuildResult::NotEnoughMoney);
	TestEqual(TEXT("Balance unchanged"), F.Company->GetBalance().GetCents(), 1000ll);
	F.Company->SetBalance(FWEMoney::FromEuros(100));
	const FGuid Request = FGuid::NewGuid();
	TestEqual(TEXT("Crate"), F.Layout->PlaceFurniture(F.Player, TEXT("Crate"), FBuildFixture::At(0.f, -200.f), Request), EWEBuildResult::Success);
	AddExpectedMessage(TEXT("duplicate or invalid request"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	TestEqual(TEXT("Same request again: once"), F.Layout->PlaceFurniture(F.Player, TEXT("Crate"), FBuildFixture::At(100.f, -200.f), Request), EWEBuildResult::Duplicate);
	TestEqual(TEXT("Crate paid once"), F.Company->GetBalance().GetCents(), 8500ll);
	TestEqual(TEXT("Unknown furniture"), F.Layout->PlaceFurniture(F.Player, TEXT("Nope"), FBuildFixture::At(0.f, 0.f), FGuid::NewGuid()), EWEBuildResult::InvalidFurniture);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEBuildMoveRemoveTest, "WorkshopEmpire.Build.MoveAndRemove", Flags)
bool FWEBuildMoveRemoveTest::RunTest(const FString& Parameters)
{
	FBuildFixture F;
	AWEPlaceholderProp* Bench = F.Place(TEXT("LargeWorkbench"), FBuildFixture::At(-250.f, 150.f));
	AWEPlaceholderProp* Shelf = F.Place(TEXT("HeavyShelf"), FBuildFixture::At(250.f, -200.f, 90.f));
	if (!TestNotNull(TEXT("Bench"), Bench) || !TestNotNull(TEXT("Shelf"), Shelf) || !TestTrue(TEXT("It is a workbench"), Bench->IsA<AWEWorkbench>()))
	{
		return false;
	}
	TestEqual(TEXT("Move bench"), F.Layout->MoveFurniture(F.Player, Bench, FBuildFixture::At(-200.f, 150.f, 180.f)), EWEBuildResult::Success);
	TestTrue(TEXT("Moved"), Bench->GetActorLocation().Equals(FVector(-200.f, 150.f, 0.f), 0.01f));
	TestEqual(TEXT("Moving onto the shelf: blocked"), F.Layout->MoveFurniture(F.Player, Bench, FBuildFixture::At(250.f, -200.f)), EWEBuildResult::Blocked);
	TestTrue(TEXT("...stays where it was"), Bench->GetActorLocation().Equals(FVector(-200.f, 150.f, 0.f), 0.01f));

	// A device on the bench: neither move nor remove.
	AWEWorkbench* Workbench = Cast<AWEWorkbench>(Bench);
	UWEItemDefinition* DeviceDef = MakeDefinition(TEXT("WS_Device"), 5.f, false, 1, false);
	UWEDisassemblyDefinition* Dis = NewObject<UWEDisassemblyDefinition>(GetTransientPackage());
	Dis->AddToRoot();
	Dis->Steps.AddDefaulted();
	Dis->Steps[0].BaseSeconds = 2.4f;
	DeviceDef->Disassembly = Dis;
	AWEPickupItem* Device = SpawnItem(F.TW, DeviceDef, 1, FVector(-100.f, -100.f, 30.f));
	TestTrue(TEXT("Place device"), Workbench->PlaceItem(Device));
	TestEqual(TEXT("Busy bench cannot move"), F.Layout->MoveFurniture(F.Player, Bench, FBuildFixture::At(-200.f, 100.f)), EWEBuildResult::NotEmpty);

	// Large bench works 20 % faster: 2.4 s -> 2.0 s.
	F.Player->SetActorLocation(FVector(-60.f, 150.f, 100.f));
	TestTrue(TEXT("Start step"), Workbench->TryStartStep(F.Player));
	TestEqual(TEXT("Big bench: 2.4 s / 1.2 = 2.0 s"), Workbench->GetActiveStepDuration(), 2.f);
	Workbench->CompleteActiveStep();
	TestEqual(TEXT("Large bench storage: 36 slots"), Workbench->GetStorage()->GetNumSlots(), 36);

	// Storage not empty: no removing.
	int32 Added = 0;
	Shelf->GetStorage()->TryAddItem(FWEItemInstance::Make(MakeDefinition(TEXT("WS_Part"), 0.5f)), Added);
	TestEqual(TEXT("Full shelf cannot be removed"), F.Layout->RemoveFurniture(F.Player, Shelf, FGuid::NewGuid()), EWEBuildResult::NotEmpty);
	Shelf->GetStorage()->ClearAll();
	const int64 Before = F.Company->GetBalance().GetCents();
	TestEqual(TEXT("Empty shelf removed"), F.Layout->RemoveFurniture(F.Player, Shelf, FGuid::NewGuid()), EWEBuildResult::Success);
	TestEqual(TEXT("Half the price back (30 EUR)"), F.Company->GetBalance().GetCents(), Before + 3000);
	TestFalse(TEXT("Gone"), IsValid(Shelf) && !Shelf->IsActorBeingDestroyed());

	// Map furniture: move yes, sell no.
	AWEPlaceholderProp* MapShelf = F.TW.Spawn<AWEPlaceholderProp>(FVector(250.f, 200.f, 0.f));
	TestEqual(TEXT("Map furniture cannot be sold"), F.Layout->RemoveFurniture(F.Player, MapShelf, FGuid::NewGuid()), EWEBuildResult::NotMovable);
	TestEqual(TEXT("...but moved"), F.Layout->MoveFurniture(F.Player, MapShelf, FBuildFixture::At(250.f, -150.f)), EWEBuildResult::Success);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEBuildPreviewTest, "WorkshopEmpire.Build.PreviewAndSelection", Flags)
bool FWEBuildPreviewTest::RunTest(const FString& Parameters)
{
	FBuildFixture F;
	UWEBuildComponent* Build = F.Player->GetBuildComponent();
	if (!TestNotNull(TEXT("Build component"), Build))
	{
		return false;
	}
	Build->SetBuildMode(true);
	TestTrue(TEXT("Build mode on"), Build->IsActive());
	TestEqual(TEXT("Starts in edit mode"), Build->GetSelectedIndex(), INDEX_NONE);
	Build->CycleSelection(-1);
	TestEqual(TEXT("Wheel back from edit wraps to the last item"), Build->GetSelectedIndex(), F.Layout->GetCatalog()->Items.Num() - 1);
	Build->CycleSelection(1);
	TestEqual(TEXT("...and forward to edit"), Build->GetSelectedIndex(), INDEX_NONE);
	Build->CycleSelection(1);
	TestEqual(TEXT("First catalog item"), Build->GetSelectedIndex(), 0);
	TestEqual(TEXT("Menu: edit + all items"), Build->GetMenuLines().Num(), F.Layout->GetCatalog()->Items.Num() + 1);

	Build->SetAimOverride(FVector(104.f, 57.f, 0.f));
	Build->UpdatePreview();
	TestTrue(TEXT("Preview"), Build->HasPreview());
	TestTrue(TEXT("Free spot: green"), Build->IsPreviewValid());
	TestTrue(TEXT("Preview snapped"), Build->GetPreviewTransform().GetLocation().Equals(FVector(100.f, 60.f, 0.f), 0.01f));
	Build->Rotate(90.f);
	Build->Rotate(15.f);
	TestEqual(TEXT("Rotated 105°"), static_cast<int32>(FMath::RoundToInt(Build->GetPreviewTransform().Rotator().Yaw)), 105);

	F.Place(TEXT("HeavyShelf"), FBuildFixture::At(100.f, 60.f));
	Build->UpdatePreview();
	TestFalse(TEXT("Occupied: red"), Build->IsPreviewValid());
	TestTrue(TEXT("Status says why"), Build->GetStatusText().ToString().Contains(TEXT("Platz")));

	Build->SetAimOverride(FVector(700.f, 0.f, 0.f));
	Build->UpdatePreview();
	TestFalse(TEXT("Outside: red"), Build->IsPreviewValid());
	Build->SetBuildMode(false);
	TestFalse(TEXT("Off"), Build->IsActive());

	// Carrying something: no build mode.
	AWEPickupItem* Box = SpawnItem(F.TW, MakeDefinition(TEXT("WS_Carry"), 2.f, false, 1, false), 1, F.Player->GetActorLocation() + FVector(80.f, 0.f, -40.f));
	F.Player->TryPickUpItem(Box);
	Build->SetBuildMode(true);
	TestFalse(TEXT("Hands full: build mode refused"), Build->IsActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEBuildProgressionTest, "WorkshopEmpire.Build.NewBenchGetsUpgrades", Flags)
bool FWEBuildProgressionTest::RunTest(const FString& Parameters)
{
	FBuildFixture F;
	UWEProgressionComponent* Progression = F.GameState->GetProgression();
	if (!TestNotNull(TEXT("Upgrade catalog"), Progression->GetCatalog()))
	{
		return false;
	}
	TestTrue(TEXT("Bench cabinet"), Progression->GrantUpgrade(TEXT("BenchCabinet")));
	AWEPlaceholderProp* Bench = F.Place(TEXT("Workbench"), FBuildFixture::At(-250.f, 150.f));
	if (!TestNotNull(TEXT("Second bench"), Bench))
	{
		return false;
	}
	TestEqual(TEXT("New bench: 24 + 6 slots"), Bench->GetStorage()->GetNumSlots(), 30);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEBuildSaveTest, "WorkshopEmpire.Save.FurnitureRoundTrip", Flags)
bool FWEBuildSaveTest::RunTest(const FString& Parameters)
{
	FBuildFixture F;
	AWEPlaceholderProp* Shelf = F.Place(TEXT("LargeShelf"), FBuildFixture::At(250.f, -150.f, 90.f));
	AWEPlaceholderProp* Bench = F.Place(TEXT("Workbench"), FBuildFixture::At(-250.f, 150.f));
	AWEPlaceholderProp* MapProp = F.TW.Spawn<AWEPlaceholderProp>(FVector(0.f, 200.f, 0.f));
	if (!Shelf || !Bench || !TestNotNull(TEXT("Map prop"), MapProp))
	{
		return false;
	}
	UWEItemDefinition* Part = MakeDefinition(TEXT("WS_SavedPart"), 0.5f);
	int32 Added = 0;
	Shelf->GetStorage()->TryAddItem(FWEItemInstance::Make(Part, 1), Added);
	F.Layout->MoveFurniture(F.Player, MapProp, FBuildFixture::At(0.f, 100.f));
	const FName ShelfName = Shelf->GetFName();
	const FName BenchName = Bench->GetFName();

	UWESaveGame* Save = FWESaveSystem::Capture(F.TW.World);
	TestEqual(TEXT("Version 7"), Save->SaveVersion, UWESaveGame::CurrentVersion);
	TestTrue(TEXT("Furniture saved"), Save->bHasFurniture && Save->Furniture.Num() >= 3);

	// Change everything: sell the bench (empty), move the map prop back. Then load.
	F.Layout->RemoveFurniture(F.Player, Bench, FGuid::NewGuid());
	MapProp->SetActorLocation(FVector(0.f, 200.f, 0.f));
	FWESaveSystem::Apply(F.TW.World, Save);

	AWEPlaceholderProp* LoadedBench = nullptr;
	AWEPlaceholderProp* LoadedShelf = nullptr;
	for (TActorIterator<AWEPlaceholderProp> It(F.TW.World); It; ++It)
	{
		if (IsValid(*It) && !It->IsActorBeingDestroyed())
		{
			LoadedBench = It->GetFName() == BenchName ? *It : LoadedBench;
			LoadedShelf = It->GetFName() == ShelfName ? *It : LoadedShelf;
		}
	}
	if (!TestNotNull(TEXT("Sold bench is back"), LoadedBench) || !TestNotNull(TEXT("Shelf still there"), LoadedShelf))
	{
		return false;
	}
	TestTrue(TEXT("Bench is a workbench again"), LoadedBench->IsA<AWEWorkbench>());
	TestTrue(TEXT("Bench id"), LoadedBench->GetFurnitureId() == FName(TEXT("Workbench")));
	TestTrue(TEXT("Bench at its place"), LoadedBench->GetActorLocation().Equals(FVector(-250.f, 150.f, 0.f), 0.01f));
	TestEqual(TEXT("Shelf contents restored"), LoadedShelf->GetStorage()->CountItems(Part), 1);
	TestEqual(TEXT("Large shelf: 25 slots"), LoadedShelf->GetStorage()->GetNumSlots(), 25);
	TestTrue(TEXT("Moved map furniture restored"), MapProp->GetActorLocation().Equals(FVector(0.f, 100.f, 0.f), 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWENewToolsTest, "WorkshopEmpire.Build.NewToolsAndHandTruck", Flags)
bool FWENewToolsTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	TW.SpawnGameState<AWEGameState>();
	AWECharacter* Player = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	UWEItemDefinition* Truck = UWEItemDefinition::FindById(TEXT("HandTruck"));
	UWEItemDefinition* Ratchet = UWEItemDefinition::FindById(TEXT("BitRatchet"));
	UWEItemDefinition* Screwdriver = UWEItemDefinition::FindById(TEXT("Screwdriver"));
	UWEItemDefinition* Motor = UWEItemDefinition::FindById(TEXT("ElectricMotor"));
	if (!TestNotNull(TEXT("Hand truck"), Truck) || !TestNotNull(TEXT("Ratchet"), Ratchet) || !TestNotNull(TEXT("Motor"), Motor))
	{
		return false;
	}
	AWEPickupItem* MotorItem = SpawnItem(TW, Motor, 1, Player->GetActorLocation() + FVector(80.f, 0.f, -40.f), EWEItemCondition::Defective);
	TestFalse(TEXT("45 kg motor: too heavy by hand"), Player->TryPickUpItem(MotorItem));
	int32 Added = 0;
	Player->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(Truck), Added);
	TestEqual(TEXT("Hand truck: 30 + 30 kg"), Player->GetMaxCarryWeightKg(), 60.f);
	TestTrue(TEXT("Now the motor can be moved"), Player->TryPickUpItem(MotorItem));

	Player->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(Screwdriver), Added);
	TestEqual(TEXT("Basic screwdriver"), AWEWorkbench::FindToolSpeed(Player, EWEToolType::Screwdriver), 1.f);
	Player->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(Ratchet), Added);
	TestEqual(TEXT("Best tool counts: ratchet 1.6"), AWEWorkbench::FindToolSpeed(Player, EWEToolType::Screwdriver), 1.6f);
	TestEqual(TEXT("Digital multimeter x2"), UWEItemDefinition::FindById(TEXT("DigitalMultimeter"))->ToolSpeedMultiplier, 2.f);
	TestEqual(TEXT("Soldering station x2"), UWEItemDefinition::FindById(TEXT("SolderingStation"))->ToolSpeedMultiplier, 2.f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
