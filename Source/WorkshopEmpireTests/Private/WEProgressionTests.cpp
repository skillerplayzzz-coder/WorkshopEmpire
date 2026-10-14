#include "Misc/AutomationTest.h"

#include "Components/PointLightComponent.h"
#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WETraderDefinition.h"
#include "Economy/WETraderTerminal.h"
#include "Engine/PointLight.h"
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
#include "Workshop/WEDisassemblyDefinition.h"
#include "Workshop/WEWorkbench.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace WEProgressionTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	FWEUpgradeDefinition Upgrade(const TCHAR* Id, int64 Cents, const TCHAR* Requires = nullptr)
	{
		FWEUpgradeDefinition U;
		U.Id = FName(Id);
		U.DisplayName = FText::FromString(Id);
		U.Cost = FWEMoney::FromCents(Cents);
		U.RequiredUpgrade = Requires ? FName(Requires) : NAME_None;
		return U;
	}
}

using namespace WEProgressionTests;
using namespace WETestItems;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEUpgradeEffectsTest, "WorkshopEmpire.Progression.UpgradeEffectsAndRules", Flags)
bool FWEUpgradeEffectsTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	UWEProgressionComponent* Progression = GameState->GetProgression();

	UWEUpgradeCatalog* Catalog = NewObject<UWEUpgradeCatalog>(GetTransientPackage());
	FWEUpgradeDefinition Cabinet = Upgrade(TEXT("T_Cabinet"), 1000);
	Cabinet.BenchExtraSlots = 6;
	FWEUpgradeDefinition Belt = Upgrade(TEXT("T_Belt"), 1000);
	Belt.InventoryExtraKg = 10.f;
	FWEUpgradeDefinition Mat = Upgrade(TEXT("T_Mat"), 1000, TEXT("T_Cabinet"));
	Mat.WorkSpeedMultiplier = 1.25f;
	FWEUpgradeDefinition Leds = Upgrade(TEXT("T_Leds"), 1000);
	Leds.LightMultiplier = 2.f;
	FWEUpgradeDefinition Shelf = Upgrade(TEXT("T_Shelf"), 1000);
	Shelf.bUnlocksActors = true;
	Catalog->Upgrades = { Cabinet, Belt, Mat, Leds, Shelf };
	Progression->SetCatalog(Catalog);

	AWECharacter* Player = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	AWEWorkbench* Bench = TW.Spawn<AWEWorkbench>(FVector(150.f, 0.f, 0.f));
	APointLight* Lamp = TW.Spawn<APointLight>(FVector(0.f, 0.f, 280.f));
	Lamp->Tags.Add(TEXT("WE_WorkshopLight"));
	Lamp->GetLightComponent()->SetIntensity(30.f);
	Progression->ApplyEffects();

	const int32 BenchSlots = Bench->GetStorage()->GetNumSlots();
	TestEqual(TEXT("Player inventory starts at 25 kg"), Player->GetPlayerInventory()->GetMaxWeightKg(), 25.f);

	TestFalse(TEXT("Prerequisite missing: Mat refused"), Progression->GrantUpgrade(TEXT("T_Mat")));
	TestFalse(TEXT("Block reason names the prerequisite"), Progression->GetBlockReason(Mat).IsEmpty());
	TestTrue(TEXT("Cabinet"), Progression->GrantUpgrade(TEXT("T_Cabinet")));
	TestFalse(TEXT("No double purchase"), Progression->GrantUpgrade(TEXT("T_Cabinet")));
	TestTrue(TEXT("Mat now allowed"), Progression->GrantUpgrade(TEXT("T_Mat")));
	TestTrue(TEXT("Belt"), Progression->GrantUpgrade(TEXT("T_Belt")));
	TestTrue(TEXT("LEDs"), Progression->GrantUpgrade(TEXT("T_Leds")));
	TestFalse(TEXT("Unknown id refused"), Progression->GrantUpgrade(TEXT("Nope")));

	TestEqual(TEXT("Bench storage +6 slots"), Bench->GetStorage()->GetNumSlots(), BenchSlots + 6);
	TestEqual(TEXT("Inventory +10 kg"), Player->GetPlayerInventory()->GetMaxWeightKg(), 35.f);
	TestEqual(TEXT("Work speed x1.25"), Progression->GetWorkSpeedMultiplier(), 1.25f);
	TestEqual(TEXT("Lamp doubled"), Lamp->GetLightComponent()->Intensity, 60.f);
	Progression->ApplyEffects();
	TestEqual(TEXT("Re-applying does not compound"), Lamp->GetLightComponent()->Intensity, 60.f);

	// Work speed reaches the workbench.
	UWEItemDefinition* Pc = MakeDefinition(TEXT("P_Pc"), 9.f, false, 1, false);
	if (!Pc->Disassembly)
	{
		UWEDisassemblyDefinition* Dis = NewObject<UWEDisassemblyDefinition>(GetTransientPackage(), TEXT("P_PcDisassembly"));
		Dis->AddToRoot();
		FWEDisassemblyStep Step;
		Step.ActionText = FText::FromString(TEXT("Hand-Schritt"));
		Step.BaseSeconds = 2.5f;
		Dis->Steps = { Step };
		Pc->Disassembly = Dis;
	}
	Bench->PlaceItem(SpawnItem(TW, Pc, 1, FVector(100.f, -100.f, 30.f)));
	TestTrue(TEXT("Step"), Bench->TryStartStep(Player));
	TestEqual(TEXT("2.5 s / 1.25 = 2.0 s"), Bench->GetActiveStepDuration(), 2.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEUnlockTest, "WorkshopEmpire.Progression.FurnitureUnlock", Flags)
bool FWEUnlockTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	UWEProgressionComponent* Progression = GameState->GetProgression();
	UWEUpgradeCatalog* Catalog = NewObject<UWEUpgradeCatalog>(GetTransientPackage());
	FWEUpgradeDefinition Shelf = Upgrade(TEXT("T_Shelf2"), 1000);
	Shelf.bUnlocksActors = true;
	Catalog->Upgrades = { Shelf };
	Progression->SetCatalog(Catalog);

	// A placed shelf that needs the upgrade (set before BeginPlay, like in the map).
	FActorSpawnParameters Params;
	Params.bDeferConstruction = true;
	AWEPlaceholderProp* Prop = TW.World->SpawnActor<AWEPlaceholderProp>(AWEPlaceholderProp::StaticClass(), FTransform(FVector(300.f, 0.f, 0.f)), Params);
	FProperty* RequiredProperty = AWEPlaceholderProp::StaticClass()->FindPropertyByName(TEXT("RequiredUpgrade"));
	*RequiredProperty->ContainerPtrToValuePtr<FName>(Prop) = TEXT("T_Shelf2");
	FProperty* StorageProperty = AWEPlaceholderProp::StaticClass()->FindPropertyByName(TEXT("bHasStorage"));
	*StorageProperty->ContainerPtrToValuePtr<bool>(Prop) = true;
	Prop->FinishSpawning(FTransform(FVector(300.f, 0.f, 0.f)));

	TestFalse(TEXT("Locked at start"), Prop->IsUnlocked());
	TestTrue(TEXT("Hidden"), Prop->IsHidden());
	TestFalse(TEXT("No storage while locked"), Prop->HasStorage());
	Progression->GrantUpgrade(TEXT("T_Shelf2"));
	TestTrue(TEXT("Unlocked after buying"), Prop->IsUnlocked());
	TestFalse(TEXT("Visible"), Prop->IsHidden());
	TestTrue(TEXT("Storage usable"), Prop->HasStorage());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEGuideTest, "WorkshopEmpire.Progression.GuideFollowsEvents", Flags)
bool FWEGuideTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	UWEProgressionComponent* Progression = GameState->GetProgression();

	TestEqual(TEXT("Starts at step 0"), Progression->GetGuideStep(), 0);
	TestTrue(TEXT("Guide text shown"), Progression->GetGuideText().ToString().Contains(TEXT("Erste Schritte (1/6)")));
	Progression->NotifyEvent(EWEProgressEvent::ItemSold);
	TestEqual(TEXT("Out-of-order event ignored"), Progression->GetGuideStep(), 0);
	const EWEProgressEvent Order[] = { EWEProgressEvent::ToolStored, EWEProgressEvent::ItemPlacedOnBench, EWEProgressEvent::DisassemblyFinished,
		EWEProgressEvent::ItemSold, EWEProgressEvent::PurchaseMade, EWEProgressEvent::GameSaved };
	for (const EWEProgressEvent Event : Order)
	{
		Progression->NotifyEvent(Event);
	}
	TestTrue(TEXT("Guide finished"), Progression->IsGuideFinished());
	TestFalse(TEXT("Shows a next goal or completion text"), Progression->GetGuideText().ToString().Contains(TEXT("Erste Schritte")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWETransferAllTest, "WorkshopEmpire.Inventory.TransferAllKeepsWhatDoesNotFit", Flags)
bool FWETransferAllTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	UWEItemDefinition* Light = MakeDefinition(TEXT("TA_Light"), 1.f, true, 10);
	UWEItemDefinition* Heavy = MakeDefinition(TEXT("TA_Heavy"), 8.f, false, 1);
	AActor* Owner = TW.Spawn<AActor>(FVector::ZeroVector);
	UWEInventoryComponent* From = NewObject<UWEInventoryComponent>(Owner, TEXT("From"));
	UWEInventoryComponent* To = NewObject<UWEInventoryComponent>(Owner, TEXT("To"));
	From->Configure(FText::FromString(TEXT("Werkbank")), 10, 100.f);
	To->Configure(FText::FromString(TEXT("Inventar")), 10, 10.f);
	From->RegisterComponent();
	To->RegisterComponent();
	int32 Added = 0;
	From->TryAddItem(FWEItemInstance::Make(Light, 4), Added);
	From->TryAddItem(FWEItemInstance::Make(Heavy, 1), Added);
	From->TryAddItem(FWEItemInstance::Make(Heavy, 1), Added);

	const int32 Moved = From->TransferAllTo(To);
	TestTrue(TEXT("Something moved"), Moved > 0);
	TestEqual(TEXT("Light stack moved completely"), To->CountItems(Light), 4);
	TestTrue(TEXT("Target never over its weight limit"), To->GetTotalWeightKg() <= 10.f + KINDA_SMALL_NUMBER);
	TestTrue(TEXT("Rest stays in the source"), From->GetAllSlots().Num() > 0);
	TestEqual(TEXT("Nothing lost"), From->GetTotalWeightKg() + To->GetTotalWeightKg(), 20.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEVerticalSliceTest, "WorkshopEmpire.Loop.FullVerticalSlice", Flags)
bool FWEVerticalSliceTest::RunTest(const FString& Parameters)
{
	// The complete Phase 5 loop with the real game data, through the same server functions the
	// player's requests end up in: buy → carry → place → disassemble → take parts → sell → invest → save → load.
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	UWECompanyComponent* Company = GameState->GetCompany();
	UWEProgressionComponent* Progression = GameState->GetProgression();
	GameState->GetContracts()->SetCatalog(nullptr);
	Company->SetBalance(FWEMoney::FromEuros(250));
	if (!TestNotNull(TEXT("Real upgrade catalog loaded"), Progression->GetCatalog()))
	{
		return false;
	}

	const auto LoadTrader = [](const TCHAR* Name)
	{
		return LoadObject<UWETraderDefinition>(nullptr, *FString::Printf(TEXT("/Game/WorkshopEmpire/Data/Economy/%s.%s"), Name, Name));
	};
	AWETraderTerminal* Terminal = TW.Spawn<AWETraderTerminal>(FVector(0.f, 250.f, 0.f));
	Terminal->SetTraders({ LoadTrader(TEXT("DA_Trader_Scrapyard")), LoadTrader(TEXT("DA_Trader_UsedGoods")), LoadTrader(TEXT("DA_Trader_ToolShop")) });
	AWEDeliveryZone* Pallet = TW.Spawn<AWEDeliveryZone>(FVector(120.f, -150.f, 6.f));
	AWEWorkbench* Bench = TW.Spawn<AWEWorkbench>(FVector(150.f, 0.f, 0.f));
	Bench->SetRandomSeed(7);
	AWECharacter* Player = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	UWEInventoryComponent* Inventory = Player->GetPlayerInventory();

	auto OfferOf = [Terminal](const TCHAR* ItemId)
	{
		for (const FWEMarketOffer& Offer : Terminal->GetOffers())
		{
			// Regular offers only (tools new, PC defective); rotating extras may list other conditions.
			if (Offer.Item.Definition && Offer.Item.Definition->ItemId == FName(ItemId)
				&& (Offer.Item.Condition == EWEItemCondition::Defective || Offer.Item.Condition == EWEItemCondition::New))
			{
				return Offer.OfferIndex;
			}
		}
		return int32(INDEX_NONE);
	};

	// 1) Procure: tools and a defective PC.
	TestEqual(TEXT("Buy screwdriver"), Terminal->Buy(Player, OfferOf(TEXT("Screwdriver")), 1, FGuid::NewGuid()), EWETradeResult::Success);
	TestEqual(TEXT("Buy side cutter"), Terminal->Buy(Player, OfferOf(TEXT("SideCutter")), 1, FGuid::NewGuid()), EWETradeResult::Success);
	TestEqual(TEXT("Buy defective PC"), Terminal->Buy(Player, OfferOf(TEXT("OldComputer")), 1, FGuid::NewGuid()), EWETradeResult::Success);
	const int64 AfterBuying = Company->GetBalance().GetCents();

	AWEPickupItem* Pc = nullptr;
	for (TActorIterator<AWEPickupItem> It(TW.World); It; ++It)
	{
		const UWEItemDefinition* Def = It->GetItem().Definition;
		Pc = Def && Def->ItemId == TEXT("OldComputer") ? *It : Pc;
	}
	TestEqual(TEXT("Both tools went straight into the inventory"), Inventory->GetAllSlots().Num(), 2);
	TestEqual(TEXT("Guide: a bought tool counts as stored"), Progression->GetGuideStep(), 1);
	if (!TestNotNull(TEXT("PC delivered to the pallet"), Pc))
	{
		return false;
	}

	// 2) Carry it to the bench and disassemble it completely.
	Pc->SetActorLocation(Player->GetActorLocation() + FVector(80.f, 0.f, -40.f)); // walk over to the pallet
	TestTrue(TEXT("Carry PC"), Player->TryPickUpItem(Pc));
	Player->ReleaseHeldItem();
	TestTrue(TEXT("Place on bench"), Bench->PlaceItem(Pc));
	int32 Steps = 0;
	while (Bench->GetPlacedItem() && Steps < 20)
	{
		if (!TestTrue(TEXT("Next step can start"), Bench->TryStartStep(Player)))
		{
			break;
		}
		Bench->CompleteActiveStep();
		++Steps;
	}
	TestEqual(TEXT("7 steps"), Steps, 7);
	TestTrue(TEXT("Parts in the bench storage"), Bench->GetStorage()->GetAllSlots().Num() >= 5);

	// 3) Take everything and sell all parts (keep the tools).
	TestTrue(TEXT("Take all parts"), Bench->GetStorage()->TransferAllTo(Inventory) > 0);
	TestEqual(TEXT("Bench storage empty"), Bench->GetStorage()->GetAllSlots().Num(), 0);
	for (const FWEInventorySlotData& Slot : Inventory->GetAllSlots())
	{
		if (Slot.Item.Definition->ToolType == EWEToolType::None)
		{
			TestEqual(TEXT("Sell part"), Terminal->SellFromInventory(Player, Inventory, Slot.SlotIndex, 0, FGuid::NewGuid()), EWETradeResult::Success);
		}
	}
	const int64 AfterSelling = Company->GetBalance().GetCents();
	TestTrue(TEXT("Selling the parts paid more than the PC cost"), AfterSelling - AfterBuying > 0);

	// 4) Invest: the cheapest bench upgrade.
	const int32 SlotsBefore = Bench->GetStorage()->GetNumSlots();
	TestEqual(TEXT("Buy bench cabinet"), Terminal->BuyUpgrade(Player, TEXT("BenchCabinet"), FGuid::NewGuid()), EWETradeResult::Success);
	TestEqual(TEXT("Bench got +6 slots"), Bench->GetStorage()->GetNumSlots(), SlotsBefore + 6);
	TestEqual(TEXT("Guide reached the save step"), Progression->GetGuideStep(), 5);

	// Upgrade purchase refusals: owned, prerequisite missing, not affordable, replayed request. None may cost money.
	const int64 BeforeRefusals = Company->GetBalance().GetCents();
	TestEqual(TEXT("Already owned"), Terminal->BuyUpgrade(Player, TEXT("BenchCabinet"), FGuid::NewGuid()), EWETradeResult::OutOfStock);
	TestEqual(TEXT("Prerequisite missing"), Terminal->BuyUpgrade(Player, TEXT("ProBench"), FGuid::NewGuid()), EWETradeResult::OutOfStock);
	TestEqual(TEXT("Unknown upgrade"), Terminal->BuyUpgrade(Player, TEXT("Nope"), FGuid::NewGuid()), EWETradeResult::InvalidItem);
	Company->SetBalance(FWEMoney::FromEuros(10));
	TestEqual(TEXT("Not affordable"), Terminal->BuyUpgrade(Player, TEXT("SecondShelf"), FGuid::NewGuid()), EWETradeResult::NotEnoughMoney);
	Company->SetBalance(FWEMoney::FromCents(BeforeRefusals));
	const FGuid Replayed = FGuid::NewGuid();
	TestEqual(TEXT("LEDs"), Terminal->BuyUpgrade(Player, TEXT("LedLights"), Replayed), EWETradeResult::Success);
	Progression->SetOwnedUpgrades({ TEXT("BenchCabinet") });
	AddExpectedMessage(TEXT("duplicate or invalid request"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	TestEqual(TEXT("Replayed request id refused"), Terminal->BuyUpgrade(Player, TEXT("LedLights"), Replayed), EWETradeResult::Duplicate);
	TestEqual(TEXT("Only the LEDs were charged"), Company->GetBalance().GetCents(), BeforeRefusals - 3000);

	// 5) Save, lose everything, load.
	Progression->NotifyEvent(EWEProgressEvent::GameSaved);
	UWESaveGame* Save = FWESaveSystem::Capture(TW.World);
	const int64 SavedBalance = Company->GetBalance().GetCents();
	Company->SetBalance(FWEMoney::FromEuros(1));
	Progression->SetOwnedUpgrades({});
	Progression->SetGuideStep(0);
	TestEqual(TEXT("Upgrade really gone before loading"), Bench->GetStorage()->GetNumSlots(), SlotsBefore);
	FWESaveSystem::Apply(TW.World, Save);
	TestEqual(TEXT("Balance restored"), Company->GetBalance().GetCents(), SavedBalance);
	TestTrue(TEXT("Upgrade restored"), Progression->HasUpgrade(TEXT("BenchCabinet")));
	TestEqual(TEXT("Upgrade effect restored"), Bench->GetStorage()->GetNumSlots(), SlotsBefore + 6);
	TestTrue(TEXT("Guide restored as finished"), Progression->IsGuideFinished());

	AddInfo(FString::Printf(TEXT("Loop: start 250,00 EUR -> after buying tools+PC %.2f -> after selling parts %.2f (+%.2f) -> after upgrade %.2f EUR"),
		AfterBuying / 100.0, AfterSelling / 100.0, (AfterSelling - AfterBuying) / 100.0, SavedBalance / 100.0));
	TestTrue(TEXT("Pallet exists"), Pallet != nullptr);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
