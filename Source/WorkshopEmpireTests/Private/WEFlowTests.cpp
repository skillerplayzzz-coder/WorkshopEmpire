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
#include "Machines/WEConveyor.h"
#include "Machines/WEMachine.h"
#include "Machines/WEMachineDefinition.h"
#include "Machines/WEPowerGridComponent.h"
#include "Machines/WEProductionStorage.h"
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

namespace WEFlowTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	UWEItemDefinition* Item(const TCHAR* Id) { return UWEItemDefinition::FindById(FName(Id)); }

	/** Garage with the real catalog and machine data, 5000 EUR, one player. */
	struct FFlowFixture
	{
		FWETestWorld TW;
		AWEGameState* GameState = nullptr;
		UWEWorkshopLayoutComponent* Layout = nullptr;
		UWECompanyComponent* Company = nullptr;
		UWEPowerGridComponent* Grid = nullptr;
		AWECharacter* Player = nullptr;

		FFlowFixture()
		{
			GameState = TW.SpawnGameState<AWEGameState>();
			GameState->GetContracts()->SetCatalog(nullptr);
			Layout = GameState->GetLayout();
			Company = GameState->GetCompany();
			Grid = GameState->GetPowerGrid();
			Company->SetBalance(FWEMoney::FromEuros(5000));
			TW.Spawn<AWEBuildArea>(FVector(0.f, 0.f, 150.f));
			Player = SpawnPlayer(TW, FVector(0.f, -230.f, 100.f));
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

		void Run(float Seconds)
		{
			for (float T = 0.f; T < Seconds - KINDA_SMALL_NUMBER; T += UWEPowerGridComponent::StepSeconds)
			{
				Grid->StepMachines(UWEPowerGridComponent::StepSeconds);
			}
		}

		static void Fill(UWEInventoryComponent* Inventory, const TCHAR* ItemId, int32 Quantity, EWEItemCondition Condition = EWEItemCondition::Used)
		{
			int32 Added = 0;
			Inventory->TryAddItem(FWEItemInstance::Make(Item(ItemId), Quantity, Condition, 50), Added);
		}

		int32 CountWorldItems() const
		{
			int32 Count = 0;
			for (TActorIterator<AWEPickupItem> It(TW.World); It; ++It)
			{
				Count += IsValid(*It) && !It->IsActorBeingDestroyed() ? 1 : 0;
			}
			return Count;
		}
	};
}

using namespace WEFlowTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEFlowLineTest, "WorkshopEmpire.Flow.AutomatedRecyclingLine", Flags)
bool FWEFlowLineTest::RunTest(const FString& Parameters)
{
	// Feed belt -> shredder -> belt -> sorting machine -> 2 m belt -> material store, no player in between.
	FFlowFixture F;
	AWEConveyor* Feed = F.Place<AWEConveyor>(TEXT("Conveyor"), -310.f, -110.f, 90.f);
	AWEMachine* Shredder = F.Place<AWEMachine>(TEXT("Shredder"), -310.f, 0.f);
	AWEConveyor* BeltA = F.Place<AWEConveyor>(TEXT("Conveyor"), -200.f, 0.f);
	AWEMachine* Sorter = F.Place<AWEMachine>(TEXT("Sorter"), -100.f, 0.f);
	AWEConveyor* BeltB = F.Place<AWEConveyor>(TEXT("ConveyorLong"), 50.f, 0.f);
	AWEProductionStorage* Store = F.Place<AWEProductionStorage>(TEXT("MaterialStore"), 200.f, 0.f);
	if (!TestNotNull(TEXT("Feed belt"), Feed) || !TestNotNull(TEXT("Shredder"), Shredder) || !TestNotNull(TEXT("Belt A"), BeltA)
		|| !TestNotNull(TEXT("Sorter"), Sorter) || !TestNotNull(TEXT("2 m belt"), BeltB) || !TestNotNull(TEXT("Material store"), Store))
	{
		return false;
	}
	TestEqual(TEXT("2 m belt"), BeltB->GetLength(), 200.f);
	TestTrue(TEXT("Feed ends in the shredder"), UWEPowerGridComponent::FindPortActor(F.TW.World, Feed->GetOutputPortPoint(), Feed) == Shredder);
	TestTrue(TEXT("Belt A starts at the shredder"), UWEPowerGridComponent::FindPortActor(F.TW.World, BeltA->GetInputPortPoint(), BeltA) == Shredder);
	TestTrue(TEXT("Belt A ends in the sorter"), UWEPowerGridComponent::FindPortActor(F.TW.World, BeltA->GetOutputPortPoint(), BeltA) == Sorter);
	TestTrue(TEXT("Belt B ends in the store"), UWEPowerGridComponent::FindPortActor(F.TW.World, BeltB->GetOutputPortPoint(), BeltB) == Store);
	Shredder->SetPower(true);
	Sorter->SetPower(true);

	// A defective PC onto the feed belt; from here on nobody touches anything.
	F.Player->SetActorLocation(FVector(-230.f, -130.f, 100.f));
	AWEPickupItem* Pc = SpawnItem(F.TW, Item(TEXT("OldComputer")), 1, FVector(-230.f, -60.f, 30.f), EWEItemCondition::Defective);
	TestTrue(TEXT("PC on the feed belt"), Feed->PutOnBelt(Pc, F.Player));
	TestEqual(TEXT("1 item on the belt"), Feed->GetItems().Num(), 1);
	F.Run(3.f);
	TestEqual(TEXT("Belt carried it into the shredder"), Feed->GetItems().Num(), 0);
	TestTrue(TEXT("Shredder runs on its own"), Shredder->IsRunning());
	TestTrue(TEXT("Shredder output is connected"), Shredder->IsOutputConnected());
	F.Run(60.f);

	const UWEInventoryComponent* Stored = Store->GetStorage();
	AddInfo(FString::Printf(TEXT("Store: e-scrap %d, steel %d, plastic %d, copper %d, alu %d; sorter input %d shredded"),
		Stored->CountItems(Item(TEXT("ElectronicScrap"))), Stored->CountItems(Item(TEXT("SteelScrap"))), Stored->CountItems(Item(TEXT("PlasticParts"))),
		Stored->CountItems(Item(TEXT("CopperGranulate"))), Stored->CountItems(Item(TEXT("AluminiumScrap"))), Sorter->GetStorage()->CountItems(Item(TEXT("ShredElectronics")))));
	TestTrue(TEXT("Sorted electronic scrap arrived in the store"), Stored->CountItems(Item(TEXT("ElectronicScrap"))) >= 3);
	TestTrue(TEXT("Sorted steel arrived in the store"), Stored->CountItems(Item(TEXT("SteelScrap"))) >= 2);
	TestEqual(TEXT("Shredder output emptied by the belt"), Shredder->GetOutputStorage()->GetAllSlots().Num(), 0);
	TestEqual(TEXT("Sorter output emptied by the belt"), Sorter->GetOutputStorage()->GetAllSlots().Num(), 0);
	TestEqual(TEXT("3 shredded units wait for the next batch of 5"), Sorter->GetStorage()->CountItems(Item(TEXT("ShredElectronics"))), 3);
	TestEqual(TEXT("Belts are empty"), BeltA->GetItems().Num() + BeltB->GetItems().Num(), 0);
	TestEqual(TEXT("Nothing fell on the floor"), F.CountWorldItems(), 0);
	TestTrue(TEXT("Conveyors draw power"), F.Grid->GetLoadKw() >= 3.f * AWEConveyor::PowerKw);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEFlowBeltTest, "WorkshopEmpire.Flow.ConveyorMovesBlocksAndJoins", Flags)
bool FWEFlowBeltTest::RunTest(const FString& Parameters)
{
	FFlowFixture F;
	AWEConveyor* A = F.Place<AWEConveyor>(TEXT("Conveyor"), 0.f, 100.f);
	AWEConveyor* B = F.Place<AWEConveyor>(TEXT("Conveyor"), 80.f, 150.f, 90.f); // corner: A's end meets B's start from the side
	if (!TestNotNull(TEXT("A"), A) || !TestNotNull(TEXT("B"), B))
	{
		return false;
	}
	TestTrue(TEXT("A hands over to B"), UWEPowerGridComponent::FindPortActor(F.TW.World, A->GetOutputPortPoint(), A) == B);
	TestEqual(TEXT("...at B's start"), B->ProjectToBelt(A->GetOutputPortPoint()), 0.f, 1.f);
	const FWEItemInstance Steel = FWEItemInstance::Make(Item(TEXT("SteelScrap")), 5, EWEItemCondition::Used, 50);
	TestTrue(TEXT("Item on A"), A->AddItemAt(Steel, 0.f));
	TestFalse(TEXT("No second item on the same spot"), A->AddItemAt(Steel, 10.f));
	F.Run(1.f);
	TestEqual(TEXT("60 cm/s"), A->GetItems()[0].Position, 60.f, 0.5f);
	TestTrue(TEXT("Second item"), A->AddItemAt(Steel, 0.f));
	F.Run(1.f);
	TestTrue(TEXT("Third item"), A->AddItemAt(Steel, 0.f));
	F.Run(6.f);
	// Nothing at B's end: the belt stops, items keep their distance.
	TestEqual(TEXT("All on B"), B->GetItems().Num(), 3);
	TestEqual(TEXT("Front waits at the end"), B->GetItems()[0].Position, 100.f, 0.5f);
	TestEqual(TEXT("Spacing kept"), B->GetItems()[0].Position - B->GetItems()[1].Position, AWEConveyor::ItemSpacing, 0.5f);
	FText Reason;
	TestFalse(TEXT("Belt with items cannot be moved"), UWEWorkshopLayoutComponent::CanPickUpFurniture(B, Reason));
	TestTrue(TEXT("...says so"), Reason.ToString().Contains(TEXT("Band")));

	// Stopped: nothing moves.
	B->SetPower(false);
	AWEPlaceholderProp* Crate = F.Place(TEXT("Crate"), 80.f, 240.f);
	if (!TestNotNull(TEXT("Crate at B's end"), Crate))
	{
		return false;
	}
	F.Run(3.f);
	TestEqual(TEXT("Stopped belt keeps its items"), B->GetItems().Num(), 3);
	TestEqual(TEXT("Stopped belt: no power"), F.Grid->GetLoadKw(), AWEConveyor::PowerKw, 0.001f);
	B->SetPower(true);
	F.Run(3.f);
	TestEqual(TEXT("Delivered into the crate"), Crate->GetStorage()->CountItems(Item(TEXT("SteelScrap"))), 15);
	TestEqual(TEXT("B empty"), B->GetItems().Num(), 0);
	TestTrue(TEXT("Now it can be moved"), UWEWorkshopLayoutComponent::CanPickUpFurniture(B, Reason));

	// A full shelf blocks again (shelves take no bulky items): the PC waits at the end.
	TestTrue(TEXT("Bulky PC on A"), A->AddItemAt(FWEItemInstance::Make(Item(TEXT("OldComputer")), 1, EWEItemCondition::Defective), 0.f));
	F.Run(5.f);
	TestEqual(TEXT("PC waits on B: the crate takes no bulky items"), B->CountUnits(Item(TEXT("OldComputer"))), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEFlowRecipeTest, "WorkshopEmpire.Flow.StoreFilterAndRecipes", Flags)
bool FWEFlowRecipeTest::RunTest(const FString& Parameters)
{
	// Material store -> belt -> assembly machine. The store's output filter and the machine's recipe choice
	// decide what is made.
	FFlowFixture F;
	AWEProductionStorage* Store = F.Place<AWEProductionStorage>(TEXT("MaterialStore"), -200.f, 0.f);
	AWEConveyor* Belt = F.Place<AWEConveyor>(TEXT("Conveyor"), -100.f, 0.f);
	AWEMachine* Assembler = F.Place<AWEMachine>(TEXT("Assembler"), 20.f, 0.f);
	if (!TestNotNull(TEXT("Store"), Store) || !TestNotNull(TEXT("Belt"), Belt) || !TestNotNull(TEXT("Assembler"), Assembler)
		|| !TestEqual(TEXT("3 production recipes"), Assembler->GetMachineDefinition()->Recipes.Num(), 3))
	{
		return false;
	}
	// [T] cycles: automatic -> each recipe -> automatic.
	FText Text, Reason;
	TestTrue(TEXT("[T] offered"), Assembler->GetSettingInteraction(F.Player, Text, Reason));
	TestTrue(TEXT("...shows the recipe"), Text.ToString().Contains(TEXT("Automatisch")));
	Assembler->CycleRecipe();
	TestEqual(TEXT("Recipe 1"), Assembler->GetSelectedRecipe(), 0);
	Assembler->CycleRecipe();
	Assembler->CycleRecipe();
	TestEqual(TEXT("Recipe 3"), Assembler->GetSelectedRecipe(), 2);
	Assembler->CycleRecipe();
	TestEqual(TEXT("Back to automatic"), Assembler->GetSelectedRecipe(), static_cast<int32>(INDEX_NONE));

	// Cables chosen: copper granulate also fits the wire spool, but only cables are made.
	TestTrue(TEXT("Choose cables"), Assembler->SelectRecipe(1));
	FFlowFixture::Fill(Store->GetStorage(), TEXT("CopperGranulate"), 8);
	FFlowFixture::Fill(Store->GetStorage(), TEXT("PlasticParts"), 6);
	FFlowFixture::Fill(Store->GetStorage(), TEXT("SteelScrap"), 6);
	TestFalse(TEXT("Steel is no cable ingredient"), Assembler->CanAcceptFromBelt(FWEItemInstance::Make(Item(TEXT("SteelScrap")), 6)));
	Assembler->SetPower(true);
	F.Run(40.f);
	TestEqual(TEXT("2 cables (plastic for two)"), Assembler->GetOutputStorage()->CountItems(Item(TEXT("ExtensionCable"))), 2);
	TestEqual(TEXT("No wire spool"), Assembler->GetOutputStorage()->CountItems(Item(TEXT("CopperWireSpool"))), 0);
	TestEqual(TEXT("4 granulate left in the machine"), Assembler->GetStorage()->CountItems(Item(TEXT("CopperGranulate"))), 4);
	TestEqual(TEXT("Store output 'everything' only sends what the machine takes: steel stays"), Store->GetStorage()->CountItems(Item(TEXT("SteelScrap"))), 6);
	TestEqual(TEXT("Belt not clogged"), Belt->GetItems().Num(), 0);
	TestTrue(TEXT("Line destination = assembler"), Belt->FindLineDestination() == Assembler);
	const FWEInventorySlotData* CableSlot = Assembler->GetOutputStorage()->GetAllSlots().FindByPredicate([](const FWEInventorySlotData& S) { return S.Item.Definition == Item(TEXT("ExtensionCable")); });
	TestTrue(TEXT("Products come out new"), CableSlot && CableSlot->Item.Condition == EWEItemCondition::New);

	// Store filter: only steel goes out; the granulate stays in the store.
	FFlowFixture::Fill(Store->GetStorage(), TEXT("CopperGranulate"), 4);
	Store->SetOutput(true, Item(TEXT("SteelScrap")));
	TestTrue(TEXT("[T] on the store names the filter"), Store->GetSettingInteraction(F.Player, Text, Reason) && Text.ToString().Contains(Item(TEXT("SteelScrap"))->DisplayName.ToString()));
	Assembler->SelectRecipe(2);
	F.Run(20.f);
	TestEqual(TEXT("One bracket set"), Assembler->GetOutputStorage()->CountItems(Item(TEXT("SteelBracketSet"))), 1);
	TestEqual(TEXT("Granulate stayed in the store"), Store->GetStorage()->CountItems(Item(TEXT("CopperGranulate"))), 4);

	// Cycling the store output: everything -> each material -> off -> everything.
	Store->SetOutput(true, nullptr);
	Store->CycleOutput();
	TestTrue(TEXT("First material"), Store->IsOutputOn() && Store->GetOutputFilter() != nullptr);
	Store->CycleOutput(); // only granulate is left in the store
	TestFalse(TEXT("Then off"), Store->IsOutputOn());
	F.Run(5.f);
	TestEqual(TEXT("Off: nothing leaves"), Store->GetStorage()->CountItems(Item(TEXT("CopperGranulate"))), 4);
	Store->CycleOutput();
	TestTrue(TEXT("Everything again"), Store->IsOutputOn() && !Store->GetOutputFilter());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEFlowPlayerTest, "WorkshopEmpire.Flow.PlayerAndBelt", Flags)
bool FWEFlowPlayerTest::RunTest(const FString& Parameters)
{
	FFlowFixture F;
	AWEConveyor* Belt = F.Place<AWEConveyor>(TEXT("ConveyorLong"), 0.f, -100.f);
	if (!TestNotNull(TEXT("Belt"), Belt))
	{
		return false;
	}
	F.Player->SetActorLocation(FVector(60.f, -180.f, 100.f));
	AWEPickupItem* Pc = SpawnItem(F.TW, Item(TEXT("OldComputer")), 1, F.Player->GetActorLocation() + FVector(0.f, 80.f, -40.f), EWEItemCondition::Defective);
	TestTrue(TEXT("Carry a PC"), F.Player->TryPickUpItem(Pc));
	FText Reason;
	TestTrue(TEXT("Can put it on the belt"), Belt->CanPutOnBelt(Pc, F.Player, Reason));
	TestTrue(TEXT("On the belt"), Belt->PutOnBelt(Pc, F.Player));
	TestNull(TEXT("Hands free"), F.Player->GetHeldItem());
	TestEqual(TEXT("Lies where the player stands (x = 60 -> 160 cm)"), Belt->GetItems()[0].Position, 160.f, 10.f);

	FWEItemInstance CustomerDevice = FWEItemInstance::Make(Item(TEXT("OldComputer")), 1, EWEItemCondition::Defective);
	CustomerDevice.Repair.SetFlag(FWERepairState::CustomerItem);
	AWEPickupItem* Customer = AWEPickupItem::SpawnWorldItem(F.TW.World, CustomerDevice, FTransform(FVector(100.f, -200.f, 30.f)));
	TestFalse(TEXT("Customer devices stay off the belt"), Belt->CanPutOnBelt(Customer, F.Player, Reason));

	F.Run(1.f);
	TestTrue(TEXT("[E] takes the front item"), Belt->CanInteract(F.Player, Reason));
	TestTrue(TEXT("Bulky: into the hands"), Belt->TakeFrontItem(F.Player));
	TestNotNull(TEXT("Carrying the PC again"), F.Player->GetHeldItem());
	TestEqual(TEXT("Belt empty"), Belt->GetItems().Num(), 0);
	F.Player->DropHeldItem();

	Belt->AddItemAt(FWEItemInstance::Make(Item(TEXT("SteelScrap")), 7, EWEItemCondition::Used, 50), 50.f);
	TestTrue(TEXT("Small stack into the inventory"), Belt->TakeFrontItem(F.Player));
	TestEqual(TEXT("7 steel in the inventory"), F.Player->GetPlayerInventory()->CountItems(Item(TEXT("SteelScrap"))), 7);

	FText Text;
	TestTrue(TEXT("[Q] stops the belt"), Belt->GetAlternativeInteraction(F.Player, Text, Reason) && Text.ToString().Contains(TEXT("anhalten")));
	Belt->InteractAlternative(F.Player);
	TestFalse(TEXT("Stopped"), Belt->IsOn());
	for (float Position = 0.f; Position <= 200.f; Position += AWEConveyor::ItemSpacing)
	{
		Belt->AddItemAt(FWEItemInstance::Make(Item(TEXT("PlasticParts")), 1, EWEItemCondition::Used, 50), Position);
	}
	AWEPickupItem* Box = SpawnItem(F.TW, Item(TEXT("CableBox")), 1, FVector(0.f, -200.f, 30.f));
	TestFalse(TEXT("Full belt"), Belt->CanPutOnBelt(Box, F.Player, Reason));
	TestTrue(TEXT("...says so"), Reason.ToString().Contains(TEXT("voll")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEFlowSaveTest, "WorkshopEmpire.Save.FlowRoundTrip", Flags)
bool FWEFlowSaveTest::RunTest(const FString& Parameters)
{
	FFlowFixture F;
	AWEProductionStorage* Store = F.Place<AWEProductionStorage>(TEXT("MaterialStore"), -200.f, 0.f);
	AWEConveyor* Belt = F.Place<AWEConveyor>(TEXT("Conveyor"), -100.f, 0.f);
	AWEMachine* Assembler = F.Place<AWEMachine>(TEXT("Assembler"), 20.f, 0.f);
	if (!Store || !Belt || !TestNotNull(TEXT("Assembler"), Assembler))
	{
		return false;
	}
	Belt->AddItemAt(FWEItemInstance::Make(Item(TEXT("SteelScrap")), 6, EWEItemCondition::Used, 50), 40.f);
	Belt->AddItemAt(FWEItemInstance::Make(Item(TEXT("OldComputer")), 1, EWEItemCondition::Defective), 0.f);
	Belt->SetPower(false);
	FFlowFixture::Fill(Store->GetStorage(), TEXT("CopperGranulate"), 5);
	Store->SetOutput(true, Item(TEXT("CopperGranulate")));
	Assembler->SelectRecipe(1);

	UWESaveGame* Save = FWESaveSystem::Capture(F.TW.World);
	TestEqual(TEXT("Current version (9 added conveyors)"), Save->SaveVersion, UWESaveGame::CurrentVersion);
	TestEqual(TEXT("Conveyor saved"), Save->Conveyors.Num(), 1);

	Belt->SetPower(true);
	F.Run(10.f);
	Store->SetOutput(false, nullptr);
	Assembler->SelectRecipe(INDEX_NONE);
	FWESaveSystem::Apply(F.TW.World, Save);

	AWEConveyor* LoadedBelt = nullptr;
	AWEProductionStorage* LoadedStore = nullptr;
	AWEMachine* LoadedAssembler = nullptr;
	for (TActorIterator<AWEPlaceholderProp> It(F.TW.World); It; ++It)
	{
		if (IsValid(*It) && !It->IsActorBeingDestroyed())
		{
			LoadedBelt = Cast<AWEConveyor>(*It) ? Cast<AWEConveyor>(*It) : LoadedBelt;
			LoadedStore = Cast<AWEProductionStorage>(*It) ? Cast<AWEProductionStorage>(*It) : LoadedStore;
			LoadedAssembler = Cast<AWEMachine>(*It) ? Cast<AWEMachine>(*It) : LoadedAssembler;
		}
	}
	if (!TestNotNull(TEXT("Belt"), LoadedBelt) || !TestNotNull(TEXT("Store"), LoadedStore) || !TestNotNull(TEXT("Assembler"), LoadedAssembler))
	{
		return false;
	}
	TestFalse(TEXT("Belt stopped as saved"), LoadedBelt->IsOn());
	TestEqual(TEXT("Both items back on the belt"), LoadedBelt->GetItems().Num(), 2);
	TestEqual(TEXT("Front item at 40 cm"), LoadedBelt->GetItems()[0].Position, 40.f, 0.01f);
	TestEqual(TEXT("PC back"), LoadedBelt->CountUnits(Item(TEXT("OldComputer"))), 1);
	TestTrue(TEXT("Store filter restored"), LoadedStore->IsOutputOn() && LoadedStore->GetOutputFilter() == Item(TEXT("CopperGranulate")));
	TestEqual(TEXT("Store contents restored"), LoadedStore->GetStorage()->CountItems(Item(TEXT("CopperGranulate"))), 5);
	TestEqual(TEXT("Recipe choice restored"), LoadedAssembler->GetSelectedRecipe(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEFlowBalanceTest, "WorkshopEmpire.Economy.BalanceProduction", Flags)
bool FWEFlowBalanceTest::RunTest(const FString& Parameters)
{
	// Real data: what a product earns compared with selling its material, minus the power for the cycle.
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	GameState->GetContracts()->SetCatalog(nullptr);
	TArray<UWETraderDefinition*> Traders;
	for (const TCHAR* Name : { TEXT("Scrapyard"), TEXT("UsedGoods"), TEXT("ToolShop"), TEXT("SpareParts"), TEXT("Returns") })
	{
		Traders.Add(LoadObject<UWETraderDefinition>(nullptr, *FString::Printf(TEXT("/Game/WorkshopEmpire/Data/Economy/DA_Trader_%s.DA_Trader_%s"), Name, Name)));
	}
	const UWEMachineDefinition* Assembler = LoadObject<UWEMachineDefinition>(nullptr, TEXT("/Game/WorkshopEmpire/Data/Machines/MD_Assembler.MD_Assembler"));
	if (!TestTrue(TEXT("Traders"), !Traders.Contains(nullptr)) || !TestNotNull(TEXT("Assembler data"), Assembler))
	{
		return false;
	}
	AWETraderTerminal* Terminal = TW.Spawn<AWETraderTerminal>(FVector::ZeroVector);
	Terminal->SetTraders(Traders);
	auto Sell = [Terminal](const UWEItemDefinition* Def, int32 Quantity, EWEItemCondition Condition, uint8 Quality)
	{
		return Def ? Terminal->GetBestSellPrice(FWEItemInstance::Make(const_cast<UWEItemDefinition*>(Def), Quantity, Condition, Quality)).GetCents() / 100.0 : 0.0;
	};
	for (const FWEMachineRecipe& Recipe : Assembler->Recipes)
	{
		const double Material = Sell(Recipe.InputItem, Recipe.InputQuantity, EWEItemCondition::Used, 50)
			+ Sell(Recipe.SecondInputItem, Recipe.SecondInputQuantity, EWEItemCondition::Used, 50);
		const FWEMachineOutput& Out = Recipe.Outputs[0];
		const double Product = Sell(Out.Item, 1, Out.Condition, static_cast<uint8>(Out.Quality));
		const double Power = Assembler->PowerKw * Recipe.BaseSeconds / 3600.0 * UWEPowerGridComponent::CentsPerKWh / 100.0;
		const double Gain = Product - Power - Material;
		AddInfo(FString::Printf(TEXT("%s: material %.2f EUR -> product %.2f EUR (power %.3f EUR), +%.2f EUR"),
			*Recipe.DisplayName.ToString(), Material, Product, Power, Gain));
		TestTrue(*FString::Printf(TEXT("%s adds value"), *Recipe.DisplayName.ToString()), Gain > 0.0);
		TestTrue(*FString::Printf(TEXT("%s: no runaway (< 2.5x the material)"), *Recipe.DisplayName.ToString()), Product < FMath::Max(Material, 0.5) * 2.5);
	}
	TestTrue(TEXT("Products are sellable"), Sell(Item(TEXT("ExtensionCable")), 1, EWEItemCondition::New, 70) > 0.0);
	TestEqual(TEXT("Sealed pallets are still not bought back"), Sell(Item(TEXT("ReturnPalletElectronics")), 1, EWEItemCondition::New, 50), 0.0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
