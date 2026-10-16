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
#include "Logistics/WEVehicle.h"
#include "Machines/WEMachine.h"
#include "Machines/WEMachineDefinition.h"
#include "Machines/WEPowerGridComponent.h"
#include "Player/WECharacter.h"
#include "Progression/WEProgressionComponent.h"
#include "Progression/WEUpgradeDefinition.h"
#include "Save/WESaveGame.h"
#include "Save/WESaveSystem.h"
#include "Staff/WEEmployee.h"
#include "Staff/WEStaffComponent.h"
#include "WETestItems.h"
#include "WETestWorld.h"
#include "Workshop/WEFurnitureDefinition.h"
#include "Workshop/WEWorkshopLayoutComponent.h"
#include "World/WEBuildArea.h"
#include "World/WEGarageBuilding.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace WETestItems;

namespace WEExpansionTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	UWEItemDefinition* Item(const TCHAR* Id) { return UWEItemDefinition::FindById(FName(Id)); }

	struct FExpansionFixture
	{
		FWETestWorld TW;
		AWEGameState* GameState = nullptr;
		UWECompanyComponent* Company = nullptr;
		UWEProgressionComponent* Progression = nullptr;
		UWEContractComponent* Contracts = nullptr;
		UWEWorkshopLayoutComponent* Layout = nullptr;
		UWEPowerGridComponent* Grid = nullptr;
		AWECharacter* Player = nullptr;

		FExpansionFixture()
		{
			GameState = TW.SpawnGameState<AWEGameState>();
			Company = GameState->GetCompany();
			Progression = GameState->GetProgression();
			Contracts = GameState->GetContracts();
			Layout = GameState->GetLayout();
			Grid = GameState->GetPowerGrid();
			Company->SetBalance(FWEMoney::FromEuros(200000));
			TW.Spawn<AWEBuildArea>(FVector(0.f, 0.f, 150.f));
			Player = SpawnPlayer(TW, FVector(0.f, -230.f, 100.f));
		}

		int32 FindTemplate(const TCHAR* TitlePart) const
		{
			const UWEContractCatalog* Catalog = LoadObject<UWEContractCatalog>(nullptr, TEXT("/Game/WorkshopEmpire/Data/Economy/DA_ContractCatalog.DA_ContractCatalog"));
			for (int32 i = 0; Catalog && i < Catalog->Templates.Num(); ++i)
			{
				if (Catalog->Templates[i].Title.ToString().Contains(TitlePart))
				{
					return i;
				}
			}
			return INDEX_NONE;
		}

		void Run(float Seconds)
		{
			for (float T = 0.f; T < Seconds - KINDA_SMALL_NUMBER; T += 0.5f)
			{
				Grid->StepMachines(0.5f);
				GameState->GetStaff()->Step(0.5f);
			}
		}
	};
}

using namespace WEExpansionTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEExpansionTierTest, "WorkshopEmpire.Expansion.TiersAndBuildings", Flags)
bool FWEExpansionTierTest::RunTest(const FString& Parameters)
{
	FExpansionFixture F;
	// A building, its build area and a lamp behind the "CommercialHall" upgrade, like in the map.
	const FName HallTag = UWEProgressionComponent::MakeUpgradeTag(TEXT("CommercialHall"));
	AWEGarageBuilding* Hall = F.TW.Spawn<AWEGarageBuilding>(FVector(1700.f, -200.f, 0.f));
	AWEBuildArea* HallArea = F.TW.Spawn<AWEBuildArea>(FVector(1700.f, -200.f, 150.f));
	Hall->Tags.Add(HallTag);
	HallArea->Tags.Add(HallTag);
	F.Progression->ApplyEffects();
	TestTrue(TEXT("Hall hidden before it is bought"), Hall->IsHidden() && !Hall->GetActorEnableCollision());
	FText Reason;
	TestFalse(TEXT("Nothing can be placed in the hall yet"), F.Layout->CanPlaceAt(FVector(100.f), FTransform(FVector(1700.f, -200.f, 0.f)), nullptr, Reason));

	TestEqual(TEXT("Tier 1: small garage"), F.Progression->GetCompanyTier(), 1);
	TestEqual(TEXT("6 employees"), F.GameState->GetStaff()->GetMaxEmployees(), 6);
	TestEqual(TEXT("11 kW"), F.Grid->GetCapacityKw(), 11.f);
	TestTrue(TEXT("Next tier names the extension"), F.Progression->GetNextTierText().ToString().Contains(TEXT("Werkstatt-Erweiterung")));
	FText Blocked = F.Progression->GetBlockReason(*F.Progression->GetCatalog()->Find(TEXT("CommercialHall")));
	TestFalse(TEXT("Hall needs the extension first"), Blocked.IsEmpty());

	TestTrue(TEXT("Buy the extension"), F.Progression->GrantUpgrade(TEXT("WorkshopExtension")));
	TestEqual(TEXT("Tier 2: big workshop"), F.Progression->GetCompanyTier(), 2);
	TestEqual(TEXT("8 employees"), F.GameState->GetStaff()->GetMaxEmployees(), 8);
	TestTrue(TEXT("Buy the hall"), F.Progression->GrantUpgrade(TEXT("CommercialHall")));
	TestEqual(TEXT("Tier 3: commercial hall"), F.Progression->GetCompanyTier(), 3);
	TestEqual(TEXT("12 employees"), F.GameState->GetStaff()->GetMaxEmployees(), 12);
	TestEqual(TEXT("11 + 5.5 + 22 kW"), F.Grid->GetCapacityKw(), 38.5f);
	TestFalse(TEXT("Hall visible"), Hall->IsHidden());
	TestTrue(TEXT("Hall collides"), Hall->GetActorEnableCollision());
	TestTrue(TEXT("Now furniture can stand in the hall"), F.Layout->CanPlaceAt(FVector(100.f), FTransform(FVector(1700.f, -200.f, 0.f)), nullptr, Reason));

	// Company value = account + half of buildings and furniture.
	const int64 Balance = F.Company->GetBalance().GetCents();
	const int64 Buildings = F.Progression->GetCatalog()->Find(TEXT("WorkshopExtension"))->Cost.GetCents() + F.Progression->GetCatalog()->Find(TEXT("CommercialHall"))->Cost.GetCents();
	TestEqual(TEXT("Company value"), F.Progression->GetCompanyValue().GetCents(), Balance + Buildings / 2);

	// Factory + 250,000 EUR company value = industrial company (GrantUpgrade does not charge, so lower the account first).
	F.Company->SetBalance(FWEMoney::FromEuros(10000));
	TestTrue(TEXT("Buy the factory"), F.Progression->GrantUpgrade(TEXT("FactoryHall")));
	TestEqual(TEXT("Tier 4: factory"), F.Progression->GetCompanyTier(), 4);
	TestTrue(TEXT("Next tier: company value"), F.Progression->GetNextTierText().ToString().Contains(TEXT("Firmenwert")));
	F.Company->SetBalance(FWEMoney::FromEuros(250000));
	TestEqual(TEXT("Tier 5: industrial company"), F.Progression->GetCompanyTier(), 5);
	TestTrue(TEXT("Top"), F.Progression->GetNextTierText().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEExpansionLockTest, "WorkshopEmpire.Expansion.LockedFurniture", Flags)
bool FWEExpansionLockTest::RunTest(const FString& Parameters)
{
	FExpansionFixture F;
	const FWEFurnitureDefinition* Furnace = F.Layout->FindDefinition(TEXT("Furnace"));
	if (!TestNotNull(TEXT("Furnace data"), Furnace))
	{
		return false;
	}
	TestTrue(TEXT("Locked without the hall"), F.Layout->GetLockReason(*Furnace).ToString().Contains(TEXT("Gewerbehalle")));
	TestEqual(TEXT("Placing refused"), F.Layout->PlaceFurniture(F.Player, TEXT("Furnace"), FTransform(FVector(100.f, 50.f, 0.f)), FGuid::NewGuid()), EWEBuildResult::Locked);
	const int64 Balance = F.Company->GetBalance().GetCents();
	TestEqual(TEXT("Nothing paid"), F.Company->GetBalance().GetCents(), Balance);
	F.Progression->GrantUpgrade(TEXT("WorkshopExtension"));
	F.Progression->GrantUpgrade(TEXT("CommercialHall"));
	TestTrue(TEXT("Unlocked"), F.Layout->GetLockReason(*Furnace).IsEmpty());
	TestEqual(TEXT("Placed"), F.Layout->PlaceFurniture(F.Player, TEXT("Furnace"), FTransform(FVector(100.f, 50.f, 0.f)), FGuid::NewGuid()), EWEBuildResult::Success);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEExpansionBooksTest, "WorkshopEmpire.Expansion.CompanyBooks", Flags)
bool FWEExpansionBooksTest::RunTest(const FString& Parameters)
{
	FExpansionFixture F;
	F.Company->SetBalance(FWEMoney::FromEuros(1000));
	TestEqual(TEXT("Sale"), UWECompanyComponent::Classify(TEXT("Verkauf: RAM an Gebrauchtbörse")), EWEBookingCategory::Sales);
	TestEqual(TEXT("Sell run"), UWECompanyComponent::Classify(TEXT("Verkaufsfahrt: Verkaufsfahrt (3 Stück)")), EWEBookingCategory::Sales);
	TestEqual(TEXT("Supply contract"), UWECompanyComponent::Classify(TEXT("Liefervertrag: X (Lieferung 1/3)")), EWEBookingCategory::Orders);
	TestEqual(TEXT("Wages"), UWECompanyComponent::Classify(TEXT("Löhne (3 Mitarbeiter)")), EWEBookingCategory::Wages);
	TestEqual(TEXT("Building"), UWECompanyComponent::Classify(TEXT("Werkstatt: Gewerbehalle")), EWEBookingCategory::Expansion);
	TestEqual(TEXT("Machine"), UWECompanyComponent::Classify(TEXT("Einrichtung: Schmelzofen")), EWEBookingCategory::Equipment);
	TestEqual(TEXT("Refund furniture"), UWECompanyComponent::Classify(TEXT("Rückbuchung Einrichtung")), EWEBookingCategory::Equipment);
	TestEqual(TEXT("Unknown"), UWECompanyComponent::Classify(TEXT("Irgendwas")), EWEBookingCategory::Other);

	F.Company->Deposit(FWEMoney::FromEuros(50), TEXT("Verkauf: Stahl an Schrotthandel"));
	F.Company->Deposit(FWEMoney::FromEuros(30), TEXT("Verkauf: Kupfer an Schrotthandel"));
	F.Company->TryWithdraw(FWEMoney::FromEuros(12), TEXT("Löhne (2 Mitarbeiter)"));
	F.Company->TryWithdraw(FWEMoney::FromEuros(5), TEXT("Stromrechnung (2.00 kWh)"));
	F.Company->Deposit(FWEMoney::FromEuros(100), TEXT("Industrieauftrag: Stahlwerk"));
	TestEqual(TEXT("Sales 80 EUR"), F.Company->GetCategoryTotalCents(EWEBookingCategory::Sales), 8000ll);
	TestEqual(TEXT("Wages -12 EUR"), F.Company->GetCategoryTotalCents(EWEBookingCategory::Wages), -1200ll);
	TestEqual(TEXT("Power -5 EUR"), F.Company->GetCategoryTotalCents(EWEBookingCategory::Power), -500ll);
	TestEqual(TEXT("Orders 100 EUR"), F.Company->GetCategoryTotalCents(EWEBookingCategory::Orders), 10000ll);
	TestEqual(TEXT("Last hour too"), F.Company->GetCategoryLastHourCents(EWEBookingCategory::Sales), 8000ll);

	// Loading keeps the totals.
	TArray<int64> Saved = F.Company->GetCategoryTotals();
	F.Company->SetCategoryTotals({});
	TestEqual(TEXT("Cleared"), F.Company->GetCategoryTotalCents(EWEBookingCategory::Sales), 0ll);
	F.Company->SetCategoryTotals(Saved);
	TestEqual(TEXT("Restored"), F.Company->GetCategoryTotalCents(EWEBookingCategory::Sales), 8000ll);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEExpansionOrdersTest, "WorkshopEmpire.Expansion.IndustrialOrders", Flags)
bool FWEExpansionOrdersTest::RunTest(const FString& Parameters)
{
	FExpansionFixture F;
	F.Contracts->SetContracts({});
	TestFalse(TEXT("Small garage: no industrial orders"), F.Contracts->AddOffer(INDEX_NONE, true));
	F.Progression->GrantUpgrade(TEXT("WorkshopExtension"));
	TestTrue(TEXT("Big workshop: industrial orders"), F.Contracts->AddOffer(INDEX_NONE, true));
	const FWEContract Offer = F.Contracts->GetContracts().Last();
	TestTrue(TEXT("It is industrial"), Offer.bIndustrial);
	F.Contracts->UpdateContracts();
	const int32 Industrial = F.Contracts->GetContracts().FilterByPredicate([](const FWEContract& C) { return C.bIndustrial; }).Num();
	TestEqual(TEXT("Industrial board: 2 offers"), Industrial, UWEContractComponent::MaxIndustrialOffers);
	TestEqual(TEXT("Normal board: 3 offers"), F.Contracts->GetContracts().Num() - Industrial, UWEContractComponent::MaxOffers);

	// Supply contract: three deliveries, each paid; a missed delivery costs a penalty.
	F.Contracts->SetContracts({});
	const int32 Supply = F.FindTemplate(TEXT("Liefervertrag Stahlwinkel"));
	if (!TestTrue(TEXT("Supply template"), Supply != INDEX_NONE) || !TestTrue(TEXT("Offer"), F.Contracts->AddOffer(Supply)))
	{
		return false;
	}
	const FGuid Id = F.Contracts->GetContracts()[0].Id;
	TestEqual(TEXT("3 rounds"), F.Contracts->GetContracts()[0].Rounds, 3);
	TestEqual(TEXT("Accept"), F.Contracts->Accept(Id, TEXT("Mike")), EWEContractResult::Success);
	AWEVehicle* Van = F.TW.Spawn<AWEVehicle>(FVector(660.f, -190.f, 0.f));
	UWEInventoryComponent* Cargo = Van->GetStorage();
	int32 Added = 0;
	Cargo->TryAddItem(FWEItemInstance::Make(Item(TEXT("PalletBrackets")), 1, EWEItemCondition::New, 70), Added);
	const FWEMoney Reward = F.Contracts->GetContracts()[0].Reward;
	const int64 Before = F.Company->GetBalance().GetCents();
	TestEqual(TEXT("First delivery"), F.Contracts->DeliverFromStorage(Id, Cargo, TEXT("Mike")), EWEContractResult::Success);
	TestEqual(TEXT("Paid"), F.Company->GetBalance().GetCents() - Before, Reward.GetCents());
	TestEqual(TEXT("Still running: 1 of 3"), F.Contracts->GetContracts()[0].RoundsDone, 1);
	TestEqual(TEXT("Pallet used"), Cargo->GetAllSlots().Num(), 0);
	TestEqual(TEXT("Nothing loaded: missing"), F.Contracts->DeliverFromStorage(Id, Cargo, TEXT("Mike")), EWEContractResult::MissingItems);

	TArray<FWEContract> Late = F.Contracts->GetContracts();
	Late[0].ExpiresAt = F.Contracts->GetServerTime() - 1.f;
	F.Contracts->SetContracts(Late);
	const int64 BeforePenalty = F.Company->GetBalance().GetCents();
	F.Contracts->UpdateContracts();
	TestEqual(TEXT("Penalty = a quarter of a delivery"), BeforePenalty - F.Company->GetBalance().GetCents(), Reward.GetCents() / 4);
	TestFalse(TEXT("Contract gone"), F.Contracts->FindContract(Id) != nullptr);
	TestEqual(TEXT("Penalty booked as orders"), F.Company->GetCategoryTotalCents(EWEBookingCategory::Orders), Reward.GetCents() - Reward.GetCents() / 4);

	// Delivery tour: the van brings the bales for the steel works.
	F.Contracts->SetContracts({});
	const int32 Steel = F.FindTemplate(TEXT("Stahlwerk"));
	F.Contracts->AddOffer(Steel);
	const FWEContract SteelOrder = F.Contracts->GetContracts()[0];
	F.Contracts->Accept(SteelOrder.Id, TEXT("Mike"));
	F.Progression->GrantUpgrade(TEXT("OldVan"));
	for (int32 i = 0; i < SteelOrder.Quantity; ++i)
	{
		Cargo->TryAddItem(FWEItemInstance::Make(Item(TEXT("SteelBale")), 1, EWEItemCondition::Used, 50), Added);
	}
	AWEEmployee::bInstantTravel = true;
	FWEEmployeeProfile Profile;
	Profile.Name = TEXT("Test Fahrer");
	Profile.Role = EWEStaffRole::Driver;
	AWEEmployee* Driver = F.GameState->GetStaff()->SpawnEmployee(Profile, FTransform(FVector(400.f, 0.f, 100.f)));
	Driver->AssignTask({ EWETaskKind::DriveVehicle, nullptr, Van, nullptr });
	F.Run(1.f);
	while (Van->GetSelectedTour() != FName(TEXT("DeliverOrders")))
	{
		Van->CycleTour();
	}
	TestTrue(TEXT("Delivery tour starts"), Van->StartTour(F.Player));
	F.Run(Van->GetTourSeconds(*Van->FindTour(TEXT("DeliverOrders"))) + 1.f);
	AWEEmployee::bInstantTravel = false;
	TestFalse(TEXT("Order delivered by the van"), F.Contracts->FindContract(SteelOrder.Id) != nullptr);
	TestEqual(TEXT("Bales gone"), Cargo->CountItems(Item(TEXT("SteelBale"))), 0);
	TestTrue(TEXT("Result names the order"), Van->GetLastResult().Contains(TEXT("Stahlwerk")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEExpansionMachinesTest, "WorkshopEmpire.Machines.FurnaceAndPacker", Flags)
bool FWEExpansionMachinesTest::RunTest(const FString& Parameters)
{
	FExpansionFixture F;
	F.Progression->GrantUpgrade(TEXT("WorkshopExtension"));
	F.Progression->GrantUpgrade(TEXT("CommercialHall"));
	AWEPlaceholderProp* FurnaceProp = nullptr;
	AWEPlaceholderProp* PackerProp = nullptr;
	F.Layout->PlaceFurniture(F.Player, TEXT("Furnace"), FTransform(FVector(-200.f, 50.f, 0.f)), FGuid::NewGuid(), &FurnaceProp);
	F.Layout->PlaceFurniture(F.Player, TEXT("Packer"), FTransform(FVector(150.f, 50.f, 0.f)), FGuid::NewGuid(), &PackerProp);
	AWEMachine* Furnace = Cast<AWEMachine>(FurnaceProp);
	AWEMachine* Packer = Cast<AWEMachine>(PackerProp);
	if (!TestNotNull(TEXT("Furnace"), Furnace) || !TestNotNull(TEXT("Packer"), Packer))
	{
		return false;
	}
	int32 Added = 0;
	Furnace->GetStorage()->TryAddItem(FWEItemInstance::Make(Item(TEXT("AluminiumScrap")), 10, EWEItemCondition::Used, 50), Added);
	Furnace->GetStorage()->TryAddItem(FWEItemInstance::Make(Item(TEXT("CopperGranulate")), 20, EWEItemCondition::Used, 50), Added);
	Furnace->SetPower(true);
	TestTrue(TEXT("15 kW furnace runs on the hall connection"), Furnace->IsRunning());
	F.Run(50.f);
	TestEqual(TEXT("2 aluminium ingots"), Furnace->GetOutputStorage()->CountItems(Item(TEXT("AluminiumIngot"))), 2);
	TestEqual(TEXT("2 copper ingots"), Furnace->GetOutputStorage()->CountItems(Item(TEXT("CopperIngot"))), 2);

	Packer->GetStorage()->TryAddItem(FWEItemInstance::Make(Item(TEXT("SteelBracketSet")), 20, EWEItemCondition::New, 70), Added);
	Packer->SetPower(true);
	F.Run(16.f);
	int32 Pallets = 0;
	for (TActorIterator<AWEPickupItem> It(F.TW.World); It; ++It)
	{
		Pallets += It->GetItem().Definition == Item(TEXT("PalletBrackets")) ? 1 : 0;
	}
	TestEqual(TEXT("One pallet in front of the packer (bulky)"), Pallets, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEExpansionSaveTest, "WorkshopEmpire.Save.ExpansionRoundTrip", Flags)
bool FWEExpansionSaveTest::RunTest(const FString& Parameters)
{
	FExpansionFixture F;
	F.Contracts->SetContracts({});
	F.Progression->GrantUpgrade(TEXT("WorkshopExtension"));
	const int32 Supply = F.FindTemplate(TEXT("Liefervertrag Stahlwinkel"));
	F.Contracts->AddOffer(Supply);
	const FGuid Id = F.Contracts->GetContracts()[0].Id;
	F.Contracts->Accept(Id, TEXT("Mike"));
	UWEInventoryComponent* Shelf = F.TW.Spawn<AWEPlaceholderProp>(FVector(0.f, 200.f, 0.f))->GetStorage();
	Shelf->SetAcceptsBulky(true);
	int32 Added = 0;
	Shelf->TryAddItem(FWEItemInstance::Make(Item(TEXT("PalletBrackets")), 1, EWEItemCondition::New, 70), Added);
	F.Contracts->DeliverFromStorage(Id, Shelf, TEXT("Mike"));
	F.Company->TryWithdraw(FWEMoney::FromEuros(7), TEXT("Löhne (1 Mitarbeiter)"));
	const int64 Wages = F.Company->GetCategoryTotalCents(EWEBookingCategory::Wages);

	UWESaveGame* Save = FWESaveSystem::Capture(F.TW.World);
	TestEqual(TEXT("Current version (11 added expansion)"), Save->SaveVersion, UWESaveGame::CurrentVersion);
	F.Contracts->SetContracts({});
	F.Progression->SetOwnedUpgrades({});
	F.Company->SetCategoryTotals({});
	FWESaveSystem::Apply(F.TW.World, Save);

	const FWEContract* Loaded = F.Contracts->FindContract(Id);
	if (!TestNotNull(TEXT("Supply contract back"), Loaded))
	{
		return false;
	}
	TestTrue(TEXT("Industrial"), Loaded->bIndustrial);
	TestEqual(TEXT("Rounds"), Loaded->Rounds, 3);
	TestEqual(TEXT("One delivery done"), Loaded->RoundsDone, 1);
	TestEqual(TEXT("Tier from the buildings"), F.Progression->GetCompanyTier(), 2);
	TestEqual(TEXT("Books restored"), F.Company->GetCategoryTotalCents(EWEBookingCategory::Wages), Wages);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEExpansionBalanceTest, "WorkshopEmpire.Economy.BalanceExpansion", Flags)
bool FWEExpansionBalanceTest::RunTest(const FString& Parameters)
{
	// Real data: what the new production and the industrial orders earn compared with selling raw.
	FExpansionFixture F;
	TArray<UWETraderDefinition*> Traders;
	for (const TCHAR* Name : { TEXT("Scrapyard"), TEXT("UsedGoods"), TEXT("ToolShop"), TEXT("SpareParts"), TEXT("Returns") })
	{
		Traders.Add(LoadObject<UWETraderDefinition>(nullptr, *FString::Printf(TEXT("/Game/WorkshopEmpire/Data/Economy/DA_Trader_%s.DA_Trader_%s"), Name, Name)));
	}
	AWETraderTerminal* Terminal = F.TW.Spawn<AWETraderTerminal>(FVector(-150.f, 262.f, 0.f));
	Terminal->SetTraders(Traders);
	auto Sell = [Terminal](const TCHAR* Id, int32 Quantity, EWEItemCondition Condition, uint8 Quality)
	{
		return Terminal->GetBestSellPrice(FWEItemInstance::Make(Item(Id), Quantity, Condition, Quality)).GetCents() / 100.0;
	};
	struct FStep { const TCHAR* Name; double Raw; double Made; };
	const FStep Steps[] = {
		{ TEXT("10 Aluminiumschrott -> 2 Aluminiumbarren"), Sell(TEXT("AluminiumScrap"), 10, EWEItemCondition::Used, 50), Sell(TEXT("AluminiumIngot"), 2, EWEItemCondition::New, 70) },
		{ TEXT("20 Kupfergranulat -> 2 Kupferbarren"), Sell(TEXT("CopperGranulate"), 20, EWEItemCondition::Used, 50), Sell(TEXT("CopperIngot"), 2, EWEItemCondition::New, 70) },
		{ TEXT("20 Stahlwinkel-Sets -> Palette"), Sell(TEXT("SteelBracketSet"), 20, EWEItemCondition::New, 70), Sell(TEXT("PalletBrackets"), 1, EWEItemCondition::New, 70) },
		{ TEXT("10 Verlängerungskabel -> Palette"), Sell(TEXT("ExtensionCable"), 10, EWEItemCondition::New, 70), Sell(TEXT("PalletCables"), 1, EWEItemCondition::New, 70) },
	};
	for (const FStep& Step : Steps)
	{
		AddInfo(FString::Printf(TEXT("%s: %.2f EUR -> %.2f EUR (x%.2f)"), Step.Name, Step.Raw, Step.Made, Step.Made / FMath::Max(Step.Raw, 0.01)));
		TestTrue(*FString::Printf(TEXT("%s adds value"), Step.Name), Step.Made > Step.Raw);
		TestTrue(*FString::Printf(TEXT("%s: no runaway (< 2x)"), Step.Name), Step.Made < Step.Raw * 2.0);
	}
	// Industrial orders pay more than the trader for the same goods.
	F.Progression->GrantUpgrade(TEXT("WorkshopExtension"));
	F.Progression->GrantUpgrade(TEXT("CommercialHall"));
	for (const TCHAR* Title : { TEXT("Stahlwerk"), TEXT("Kabelwerk"), TEXT("Liefervertrag Kabel") })
	{
		F.Contracts->SetContracts({});
		F.Contracts->AddOffer(F.FindTemplate(Title));
		const FWEContract& C = F.Contracts->GetContracts()[0];
		const double Direct = Terminal->GetBestSellPrice(FWEItemInstance::Make(C.Item, C.Quantity, EWEItemCondition::New, 70)).GetCents() / 100.0;
		AddInfo(FString::Printf(TEXT("%s: %d x %s, reward %.2f EUR per delivery (x%d), sold directly %.2f EUR"), *C.Title.ToString(), C.Quantity,
			*C.Item->DisplayName.ToString(), C.Reward.GetCents() / 100.0, C.Rounds, Direct));
		TestTrue(*FString::Printf(TEXT("%s pays more than the trader"), *C.Title.ToString()), C.Reward.GetCents() / 100.0 > Direct);
	}
	// Payback of the hall in furnace batches (the granulate supply from the shredder is the real limit).
	const double PerBatch = Sell(TEXT("CopperIngot"), 2, EWEItemCondition::New, 70) - Sell(TEXT("CopperGranulate"), 20, EWEItemCondition::Used, 50);
	AddInfo(FString::Printf(TEXT("Furnace: +%.2f EUR per copper batch -> hall 25,000 EUR + furnace 2,800 EUR = %.0f batches (%.0f kg granulate)"), PerBatch, 27800.0 / PerBatch, 27800.0 / PerBatch * 20.0 * Item(TEXT("CopperGranulate"))->WeightKg));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
