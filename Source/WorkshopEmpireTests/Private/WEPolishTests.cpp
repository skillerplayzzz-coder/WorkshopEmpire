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
#include "Machines/WEPowerGridComponent.h"
#include "Player/WECharacter.h"
#include "Progression/WEProgressionComponent.h"
#include "Progression/WEUpgradeDefinition.h"
#include "Save/WESaveGame.h"
#include "Save/WESaveSystem.h"
#include "Staff/WEStaffComponent.h"
#include "WETestItems.h"
#include "WETestWorld.h"
#include "World/WEBuildArea.h"
#include "World/WEGarageBuilding.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace WETestItems;

namespace WEPolishTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	/** Market with one trader that buys everything (tools too), and a player at the terminal. */
	struct FSaleFixture
	{
		FWETestWorld TW;
		AWEGameState* GameState = nullptr;
		UWECompanyComponent* Company = nullptr;
		AWETraderTerminal* Terminal = nullptr;
		AWECharacter* Player = nullptr;
		UWEItemDefinition* Screws = nullptr;
		UWEItemDefinition* Driver = nullptr;
		UWEItemDefinition* Rare = nullptr;

		FSaleFixture()
		{
			GameState = TW.SpawnGameState<AWEGameState>();
			Company = GameState->GetCompany();
			GameState->GetContracts()->SetCatalog(nullptr);
			GameState->GetContracts()->SetContracts({});
			Screws = MakeDefinition(TEXT("P_Screws"), 0.005f, true, 200, true, 100, EWEItemCategory::SparePart);
			Driver = MakeDefinition(TEXT("P_Driver"), 0.2f, false, 1, true, 800, EWEItemCategory::Tool);
			Driver->ToolType = EWEToolType::Screwdriver;
			Rare = MakeDefinition(TEXT("P_RareGpu"), 0.6f, false, 1, true, 30000, EWEItemCategory::Collectible);
			Rare->Rarity = EWEItemRarity::Rare;

			UWETraderDefinition* Trader = NewObject<UWETraderDefinition>(GetTransientPackage());
			Trader->DisplayName = FText::FromString(TEXT("Testhändler"));
			Trader->Buys = { { EWEItemCategory::SparePart, 1.f }, { EWEItemCategory::Tool, 1.f }, { EWEItemCategory::Collectible, 1.f } };
			Trader->RestockMinutes = 0.f;
			Terminal = TW.Spawn<AWETraderTerminal>(FVector(150.f, 0.f, 0.f));
			Terminal->SetTraders({ Trader });
			Company->SetBalance(FWEMoney::FromEuros(100));
			Player = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
		}

		int32 Give(UWEItemDefinition* Def, int32 Quantity)
		{
			int32 Added = 0;
			Player->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(Def, Quantity, EWEItemCondition::Used, 50), Added);
			for (const FWEInventorySlotData& Slot : Player->GetPlayerInventory()->GetAllSlots())
			{
				if (Slot.Item.Definition == Def)
				{
					return Slot.SlotIndex;
				}
			}
			return INDEX_NONE;
		}
	};
}

using namespace WEPolishTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWESaleProtectionTest, "WorkshopEmpire.Items.SaleProtection", Flags)
bool FWESaleProtectionTest::RunTest(const FString& Parameters)
{
	FSaleFixture F;
	UWEInventoryComponent* Inv = F.Player->GetPlayerInventory();

	// Tools are personal equipment: never sold without a deliberate release.
	const int32 ToolSlot = F.Give(F.Driver, 1);
	TestTrue(TEXT("Tool is equipment"), Inv->GetItemInSlot(ToolSlot)->IsPersonalEquipment());
	TestTrue(TEXT("Tool protected by default"), Inv->GetItemInSlot(ToolSlot)->IsSaleProtected());
	const FWEMoney Before = F.Company->GetBalance();
	TestEqual(TEXT("Tool refused"), F.Terminal->SellFromInventory(F.Player, Inv, ToolSlot, 0, FGuid::NewGuid()), EWETradeResult::Protected);
	TestEqual(TEXT("Tool still there"), Inv->CountItems(F.Driver), 1);
	TestEqual(TEXT("No money booked"), F.Company->GetBalance(), Before);
	TestEqual(TEXT("Release for sale"), Inv->SetSlotMarks(ToolSlot, FWEItemInstance::ReleasedForSale, true), EWEInventoryResult::Success);
	TestEqual(TEXT("Released tool sells"), F.Terminal->SellFromInventory(F.Player, Inv, ToolSlot, 0, FGuid::NewGuid()), EWETradeResult::Success);
	TestEqual(TEXT("Tool gone"), Inv->CountItems(F.Driver), 0);

	// Favourite and locked goods.
	const int32 ScrewSlot = F.Give(F.Screws, 20);
	Inv->SetSlotMarks(ScrewSlot, FWEItemInstance::Favorite, true);
	TestEqual(TEXT("Favourite refused"), F.Terminal->SellFromInventory(F.Player, Inv, ScrewSlot, 5, FGuid::NewGuid()), EWETradeResult::Protected);
	Inv->SetSlotMarks(ScrewSlot, FWEItemInstance::Favorite, false);
	Inv->SetSlotMarks(ScrewSlot, FWEItemInstance::Locked, true);
	TestEqual(TEXT("Locked refused"), F.Terminal->SellFromInventory(F.Player, Inv, ScrewSlot, 5, FGuid::NewGuid()), EWETradeResult::Protected);
	TestEqual(TEXT("All 20 kept"), Inv->CountItems(F.Screws), 20);

	// Customer orders ignore protected items too.
	FWEContract Order;
	Order.Item = F.Screws;
	Order.Quantity = 5;
	Order.MinCondition = EWEItemCondition::Scrap;
	TestEqual(TEXT("Locked screws do not count for an order"), UWEContractComponent::CountDeliverable(Order, F.Player), 0);
	Inv->SetSlotMarks(ScrewSlot, FWEItemInstance::Locked, false);
	TestEqual(TEXT("Unlocked screws count"), UWEContractComponent::CountDeliverable(Order, F.Player), 20);
	TestEqual(TEXT("Unlocked sells"), F.Terminal->SellFromInventory(F.Player, Inv, ScrewSlot, 5, FGuid::NewGuid()), EWETradeResult::Success);

	// Marks keep stacks apart and survive saving.
	FWEItemInstance A = FWEItemInstance::Make(F.Screws, 3);
	FWEItemInstance B = A;
	B.Marks = FWEItemInstance::Locked;
	TestFalse(TEXT("Locked and loose screws do not stack"), A.CanStackWith(B));
	FWEItemInstance Loaded;
	TestTrue(TEXT("Saved item loads"), FWESaveSystem::FromSaved(FWESaveSystem::ToSaved(B), Loaded));
	TestTrue(TEXT("Lock survives saving"), Loaded.IsLocked());

	// A carried locked item cannot be sold either.
	FWEItemInstance HeldItem = FWEItemInstance::Make(F.Screws, 1);
	HeldItem.Marks = FWEItemInstance::Favorite;
	AWEPickupItem* Held = AWEPickupItem::SpawnWorldItem(F.TW.World, HeldItem, FTransform(FVector(40.f, 0.f, 100.f)));
	TestTrue(TEXT("Pick up"), Held && F.Player->TryPickUpItem(Held));
	TestEqual(TEXT("Carried favourite refused"), F.Terminal->SellHeldItem(F.Player, FGuid::NewGuid()), EWETradeResult::Protected);

	// Rare or expensive items ask first (UI), cheap ones do not.
	TestTrue(TEXT("Rare asks"), FWEItemInstance::Make(F.Rare).NeedsSaleConfirmation());
	TestFalse(TEXT("Screws do not ask"), FWEItemInstance::Make(F.Screws, 50).NeedsSaleConfirmation());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWESellBuildingTest, "WorkshopEmpire.Expansion.SellOldBuilding", Flags)
bool FWESellBuildingTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	UWECompanyComponent* Company = GameState->GetCompany();
	UWEProgressionComponent* Progression = GameState->GetProgression();
	Company->SetBalance(FWEMoney::FromEuros(10000));
	TW.Spawn<AWEDeliveryZone>(FVector(320.f, -220.f, 6.f));
	AWETraderTerminal* Terminal = TW.Spawn<AWETraderTerminal>(FVector(-150.f, 262.f, 0.f));
	AWECharacter* Player = SpawnPlayer(TW, FVector(-150.f, 150.f, 100.f));

	// Extension and hall with their build areas, tagged like in the map.
	const FName ExtTag = UWEProgressionComponent::MakeUpgradeTag(TEXT("WorkshopExtension"));
	AWEGarageBuilding* Extension = TW.Spawn<AWEGarageBuilding>(FVector(0.f, -760.f, 0.f));
	AWEBuildArea* ExtArea = TW.Spawn<AWEBuildArea>(FVector(0.f, -760.f, 150.f));
	Extension->Tags.Add(ExtTag);
	ExtArea->Tags.Add(ExtTag);
	const FName HallTag = UWEProgressionComponent::MakeUpgradeTag(TEXT("CommercialHall"));
	AWEBuildArea* HallArea = TW.Spawn<AWEBuildArea>(FVector(1700.f, -200.f, 150.f));
	HallArea->Tags.Add(HallTag);

	TestTrue(TEXT("Buy the extension"), Progression->GrantUpgrade(TEXT("WorkshopExtension")));
	TestFalse(TEXT("The only bigger building cannot be sold"), Progression->CanOfferBuildingSale(TEXT("WorkshopExtension")));
	TestTrue(TEXT("Buy the hall"), Progression->GrantUpgrade(TEXT("CommercialHall")));
	TestTrue(TEXT("Now the extension can go"), Progression->CanOfferBuildingSale(TEXT("WorkshopExtension")));
	TestFalse(TEXT("The biggest building stays"), Progression->CanOfferBuildingSale(TEXT("CommercialHall")));
	const int32 StaffBefore = GameState->GetStaff()->GetMaxEmployees();
	const float PowerBefore = GameState->GetPowerGrid()->GetCapacityKw();

	// Something left inside blocks the sale until it is moved out.
	UWEItemDefinition* Scrap = MakeDefinition(TEXT("P_Scrap"), 2.f, false, 1, true, 100, EWEItemCategory::ScrapMetal);
	AWEPickupItem* Left = SpawnItem(TW, Scrap, 1, FVector(100.f, -800.f, 30.f));
	TestTrue(TEXT("Item is inside"), Progression->IsInsideBuilding(TEXT("WorkshopExtension"), Left->GetActorLocation()));
	TestTrue(TEXT("Blocked by the item"), Progression->GetBuildingSaleBlockers(TEXT("WorkshopExtension")).Num() == 1);
	const int64 BalanceBefore = Company->GetBalance().GetCents();
	TestEqual(TEXT("Terminal refuses"), Terminal->SellBuilding(Player, TEXT("WorkshopExtension"), FGuid::NewGuid()), EWETradeResult::Protected);
	TestEqual(TEXT("No money"), Company->GetBalance().GetCents(), BalanceBefore);
	TestEqual(TEXT("Clear: one item moved"), Progression->ClearBuildingForSale(TEXT("WorkshopExtension")), 1);
	TestTrue(TEXT("Item kept, now outside"), IsValid(Left) && !Progression->IsInsideBuilding(TEXT("WorkshopExtension"), Left->GetActorLocation()));
	TestEqual(TEXT("No blockers left"), Progression->GetBuildingSaleBlockers(TEXT("WorkshopExtension")).Num(), 0);

	// Sale: 35 % back, building gone, power and staff places reduced, tier unchanged.
	const FWEUpgradeDefinition* Ext = Progression->GetCatalog()->Find(TEXT("WorkshopExtension"));
	TestEqual(TEXT("Sold"), Terminal->SellBuilding(Player, TEXT("WorkshopExtension"), FGuid::NewGuid()), EWETradeResult::Success);
	TestEqual(TEXT("35 % of the price"), Company->GetBalance().GetCents() - BalanceBefore, FMath::RoundToInt64(Ext->Cost.GetCents() * 0.35));
	TestFalse(TEXT("Upgrade gone"), Progression->HasUpgrade(TEXT("WorkshopExtension")));
	TestTrue(TEXT("Building hidden"), Extension->IsHidden() && !Extension->GetActorEnableCollision());
	TestEqual(TEXT("Tier stays 3"), Progression->GetCompanyTier(), 3);
	TestEqual(TEXT("Staff places reduced"), GameState->GetStaff()->GetMaxEmployees(), StaffBefore - Ext->StaffSlots);
	TestEqual(TEXT("Power reduced"), GameState->GetPowerGrid()->GetCapacityKw(), PowerBefore - Ext->PowerCapacityKw);
	TestEqual(TEXT("Booked as expansion"), UWECompanyComponent::Classify(TEXT("Gebäude verkauft: Werkstatt-Erweiterung")), EWEBookingCategory::Expansion);
	TestTrue(TEXT("It can be bought again"), Progression->GetBlockReason(*Ext).IsEmpty());
	return true;
}

#endif
