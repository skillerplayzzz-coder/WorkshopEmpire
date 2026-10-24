#include "Misc/AutomationTest.h"

#include "Inventory/WEInventoryComponent.h"
#include "Framework/WEPlayerController.h"
#include "Interaction/WEInteractionComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "UI/WEInventorySlotWidget.h"
#include "WETestItems.h"
#include "WETestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace WEInventoryTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	/** A plain actor with an inventory (stands in for a shelf/workbench). */
	UWEInventoryComponent* SpawnContainer(FWETestWorld& TW, const FVector& Location, int32 Slots, float MaxKg)
	{
		AActor* Actor = TW.Spawn<AActor>(Location);
		USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
		Actor->SetRootComponent(Root);
		Root->RegisterComponent();
		Actor->SetActorLocation(Location);
		UWEInventoryComponent* Inventory = NewObject<UWEInventoryComponent>(Actor, TEXT("Storage"));
		Inventory->Configure(FText::FromString(TEXT("Testlager")), Slots, MaxKg);
		Inventory->RegisterComponent();
		return Inventory;
	}

	int32 SlotCount(const UWEInventoryComponent* Inventory)
	{
		return Inventory->GetAllSlots().Num();
	}
}

using namespace WEInventoryTests;
using namespace WETestItems;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEInvStackingTest, "WorkshopEmpire.Inventory.StackingAndSplitStacks", Flags)
bool FWEInvStackingTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	UWEInventoryComponent* Inv = SpawnContainer(TW, FVector::ZeroVector, 10, 100.f);
	UWEItemDefinition* Screws = MakeDefinition(TEXT("T_Screws50"), 0.01f, true, 50);

	const FWEItemInstance Incoming = FWEItemInstance::Make(Screws, 120);
	int32 Added = 0;
	TestEqual(TEXT("All added"), Inv->TryAddItem(Incoming, Added), EWEInventoryResult::Success);
	TestEqual(TEXT("120 added"), Added, 120);
	TestEqual(TEXT("Three stacks (50/50/20)"), SlotCount(Inv), 3);
	TestEqual(TEXT("Slot 0 full"), Inv->GetItemInSlot(0)->Quantity, 50);
	TestEqual(TEXT("Slot 2 rest"), Inv->GetItemInSlot(2)->Quantity, 20);
	TestTrue(TEXT("First stack keeps the instance id"), Inv->GetItemInSlot(0)->InstanceId == Incoming.InstanceId);
	TestTrue(TEXT("Other stacks get new ids"), Inv->GetItemInSlot(1)->InstanceId != Incoming.InstanceId);

	// Adding more fills the partial stack first.
	TestEqual(TEXT("Add 40 more"), Inv->TryAddItem(FWEItemInstance::Make(Screws, 40), Added), EWEInventoryResult::Success);
	TestEqual(TEXT("Partial stack now 50"), Inv->GetItemInSlot(2)->Quantity, 50);
	TestEqual(TEXT("New stack of 10"), Inv->GetItemInSlot(3)->Quantity, 10);
	TestEqual(TEXT("Total count 160"), Inv->CountItems(Screws), 160);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEInvConditionTest, "WorkshopEmpire.Inventory.DifferentConditionsDoNotStack", Flags)
bool FWEInvConditionTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	UWEInventoryComponent* Inv = SpawnContainer(TW, FVector::ZeroVector, 10, 100.f);
	UWEItemDefinition* Ram = MakeDefinition(TEXT("T_RamStack"), 0.05f, true, 10);
	int32 Added = 0;
	Inv->TryAddItem(FWEItemInstance::Make(Ram, 2, EWEItemCondition::New), Added);
	Inv->TryAddItem(FWEItemInstance::Make(Ram, 2, EWEItemCondition::Defective), Added);
	TestEqual(TEXT("Two separate stacks"), SlotCount(Inv), 2);

	UWEItemDefinition* Single = MakeDefinition(TEXT("T_NonStack"), 1.f, false, 1);
	Inv->TryAddItem(FWEItemInstance::Make(Single), Added);
	Inv->TryAddItem(FWEItemInstance::Make(Single), Added);
	TestEqual(TEXT("Non-stackable items use one slot each"), SlotCount(Inv), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEInvLimitsTest, "WorkshopEmpire.Inventory.SlotAndWeightLimits", Flags)
bool FWEInvLimitsTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	UWEItemDefinition* Single = MakeDefinition(TEXT("T_Single1kg"), 1.f, false, 1);
	UWEItemDefinition* Bricks = MakeDefinition(TEXT("T_Bricks3kg"), 3.f, true, 10);
	UWEItemDefinition* Bulky = MakeDefinition(TEXT("T_BulkyLimits"), 5.f, false, 1, /*bFitsInInventory*/ false);

	UWEInventoryComponent* Small = SpawnContainer(TW, FVector::ZeroVector, 3, 100.f);
	int32 Added = 0;
	for (int32 i = 0; i < 3; ++i)
	{
		TestEqual(TEXT("Fits"), Small->TryAddItem(FWEItemInstance::Make(Single), Added), EWEInventoryResult::Success);
	}
	TestEqual(TEXT("4th item: no space"), Small->TryAddItem(FWEItemInstance::Make(Single), Added), EWEInventoryResult::NoSpace);
	TestEqual(TEXT("Nothing added"), Added, 0);

	UWEInventoryComponent* Light = SpawnContainer(TW, FVector::ZeroVector, 10, 10.f);
	TestEqual(TEXT("5 x 3 kg into 10 kg: partial"), Light->TryAddItem(FWEItemInstance::Make(Bricks, 5), Added), EWEInventoryResult::PartiallyAdded);
	TestEqual(TEXT("3 fit (9 kg)"), Added, 3);
	TestEqual(TEXT("Next one too heavy"), Light->TryAddItem(FWEItemInstance::Make(Bricks, 1), Added), EWEInventoryResult::TooHeavy);
	TestTrue(TEXT("Weight never exceeds the limit"), Light->GetTotalWeightKg() <= 10.f + KINDA_SMALL_NUMBER);

	TestEqual(TEXT("Bulky items are rejected"), Small->TryAddItem(FWEItemInstance::Make(Bulky), Added), EWEInventoryResult::InvalidItem);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEInvMoveTest, "WorkshopEmpire.Inventory.MoveMergeSwapSplit", Flags)
bool FWEInvMoveTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	UWEInventoryComponent* A = SpawnContainer(TW, FVector::ZeroVector, 6, 100.f);
	UWEInventoryComponent* B = SpawnContainer(TW, FVector::ZeroVector, 6, 2.f);
	UWEItemDefinition* Cable = MakeDefinition(TEXT("T_Cable"), 0.25f, true, 20);
	UWEItemDefinition* Tool = MakeDefinition(TEXT("T_Tool"), 0.5f, false, 1);
	UWEItemDefinition* Heavy = MakeDefinition(TEXT("T_Heavy"), 5.f, false, 1);

	int32 Added = 0;
	A->TryAddItem(FWEItemInstance::Make(Cable, 10), Added);   // slot 0
	A->TryAddItem(FWEItemInstance::Make(Tool), Added);        // slot 1
	A->TryAddItem(FWEItemInstance::Make(Heavy), Added);       // slot 2

	TestEqual(TEXT("Split 4 cables to empty slot 4"), A->MoveItem(0, A, 4, 4), EWEInventoryResult::Success);
	TestEqual(TEXT("Slot 0 has 6"), A->GetItemInSlot(0)->Quantity, 6);
	TestEqual(TEXT("Slot 4 has 4"), A->GetItemInSlot(4)->Quantity, 4);

	TestEqual(TEXT("Merge back"), A->MoveItem(4, A, 0, 0), EWEInventoryResult::Success);
	TestEqual(TEXT("Slot 0 has 10 again"), A->GetItemInSlot(0)->Quantity, 10);
	TestNull(TEXT("Slot 4 empty"), A->GetItemInSlot(4));

	TestEqual(TEXT("Swap cable and tool"), A->MoveItem(0, A, 1, 0), EWEInventoryResult::Success);
	TestTrue(TEXT("Tool now in slot 0"), A->GetItemInSlot(0)->Definition == Tool);
	TestTrue(TEXT("Cable now in slot 1"), A->GetItemInSlot(1)->Definition == Cable);

	TestEqual(TEXT("Move 4 cables (1 kg) to the 2 kg container"), A->MoveItem(1, B, 0, 4), EWEInventoryResult::Success);
	TestEqual(TEXT("Heavy item does not fit the 2 kg container"), A->MoveItem(2, B, 1, 0), EWEInventoryResult::TooHeavy);
	TestTrue(TEXT("Heavy item stayed"), A->GetItemInSlot(2) && A->GetItemInSlot(2)->Definition == Heavy);
	TestEqual(TEXT("Total cables conserved"), A->CountItems(Cable) + B->CountItems(Cable), 10);

	TestEqual(TEXT("Invalid source slot"), A->MoveItem(5, B, 2, 0), EWEInventoryResult::InvalidSlot);
	TestEqual(TEXT("Invalid target slot"), A->MoveItem(0, B, 99, 0), EWEInventoryResult::InvalidSlot);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEInvSortRemoveTest, "WorkshopEmpire.Inventory.SortAndRemove", Flags)
bool FWEInvSortRemoveTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	UWEInventoryComponent* Inv = SpawnContainer(TW, FVector::ZeroVector, 8, 100.f);
	UWEItemDefinition* Screws = MakeDefinition(TEXT("T_ScrewsSort"), 0.01f, true, 50, true, 5, EWEItemCategory::SparePart);
	UWEItemDefinition* Ram = MakeDefinition(TEXT("T_RamSort"), 0.05f, false, 1, true, 2000, EWEItemCategory::Electronics);

	FWEItemInstance Part = FWEItemInstance::Make(Screws, 30);
	int32 Added = 0;
	Inv->TryAddItem(Part, Added);
	Inv->MoveItem(0, Inv, 5, 10);  // stacks 20 (slot 0) + 10 (slot 5)
	Inv->TryAddItem(FWEItemInstance::Make(Ram), Added);
	Inv->SortItems();

	TestEqual(TEXT("Screw stacks merged: 2 entries"), Inv->GetAllSlots().Num(), 2);
	TestTrue(TEXT("Electronics sorted first"), Inv->GetItemInSlot(0)->Definition == Ram);
	TestEqual(TEXT("30 screws in slot 1"), Inv->GetItemInSlot(1)->Quantity, 30);

	FWEItemInstance Removed;
	TestEqual(TEXT("Remove 12"), Inv->RemoveFromSlot(1, 12, Removed), EWEInventoryResult::Success);
	TestEqual(TEXT("Removed 12"), Removed.Quantity, 12);
	TestEqual(TEXT("18 left"), Inv->GetItemInSlot(1)->Quantity, 18);
	TestEqual(TEXT("Remove rest"), Inv->RemoveFromSlot(1, 99, Removed), EWEInventoryResult::Success);
	TestNull(TEXT("Slot empty"), Inv->GetItemInSlot(1));
	TestEqual(TEXT("Remove from empty slot fails"), Inv->RemoveFromSlot(1, 1, Removed), EWEInventoryResult::InvalidSlot);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEInvWorldRoundTripTest, "WorkshopEmpire.Inventory.WorldToInventory", Flags)
bool FWEInvWorldRoundTripTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWECharacter* Player = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	UWEInventoryComponent* Inv = Player->GetPlayerInventory();
	if (!TestNotNull(TEXT("Player inventory on PlayerState"), Inv))
	{
		return false;
	}

	UWEItemDefinition* Screws = MakeDefinition(TEXT("T_ScrewsWorld"), 0.01f, true, 100);
	AWEPickupItem* Stack = SpawnItem(TW, Screws, 20, FVector(120.f, 0.f, 140.f), EWEItemCondition::VeryGood);
	const FGuid OriginalId = Stack->GetItem().InstanceId;

	TestTrue(TEXT("Small item goes into the inventory"), Stack->GoesIntoInventory());
	TestTrue(TEXT("[E] stores it"), Player->GetInteractionComponent()->ExecuteInteract(Stack));
	TestFalse(TEXT("World actor removed"), IsValid(Stack));
	TestEqual(TEXT("20 screws in inventory"), Inv->CountItems(Screws), 20);
	TestTrue(TEXT("Instance id preserved"), Inv->GetItemInSlot(0)->InstanceId == OriginalId);
	TestEqual(TEXT("Condition preserved"), Inv->GetItemInSlot(0)->Condition, EWEItemCondition::VeryGood);

	// Partial pickup: only part of a stack fits by weight.
	UWEItemDefinition* Bricks = MakeDefinition(TEXT("T_BricksWorld"), 2.f, true, 50);
	AWEPickupItem* BrickStack = SpawnItem(TW, Bricks, 20, FVector(120.f, 40.f, 140.f));
	TestTrue(TEXT("Partial store succeeds"), Player->TryStoreWorldItem(BrickStack));
	const int32 Stored = Inv->CountItems(Bricks);
	TestTrue(TEXT("Some but not all stored"), Stored > 0 && Stored < 20);
	TestTrue(TEXT("Rest stays in the world"), IsValid(BrickStack) && BrickStack->GetItem().Quantity == 20 - Stored);
	TestTrue(TEXT("Inventory weight within limit"), Inv->GetTotalWeightKg() <= Inv->GetMaxWeightKg() + KINDA_SMALL_NUMBER);

	FText Reason;
	TestFalse(TEXT("Full inventory refuses more bricks"), BrickStack->CanInteract(Player, Reason));
	TestFalse(TEXT("Reason given"), Reason.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEInvCustomerDeviceHandsTest, "WorkshopEmpire.Inventory.CustomerDeviceIntoHands", Flags)
bool FWEInvCustomerDeviceHandsTest::RunTest(const FString& Parameters)
{
	// Quality update: a small customer device is carried (never pocketed); anything in the pocket can go into the hands.
	FWETestWorld TW;
	AWECharacter* Player = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	AWEPlayerState* PS = Player->GetPlayerState<AWEPlayerState>();
	AWEPlayerController* PC = TW.Spawn<AWEPlayerController>(FVector::ZeroVector);
	PC->Possess(Player);
	Player->SetPlayerState(PS);
	PC->PlayerState = PS;
	UWEInventoryComponent* Inv = Player->GetPlayerInventory();
	if (!TestNotNull(TEXT("Player inventory"), Inv))
	{
		return false;
	}
	UWEItemDefinition* Radio = MakeDefinition(TEXT("T_RadioHands"), 2.5f, false, 1);
	FWEItemInstance Device = FWEItemInstance::Make(Radio, 1, EWEItemCondition::Defective, 40);
	Device.Repair.SetFlag(FWERepairState::CustomerItem);
	AWEPickupItem* Customer = AWEPickupItem::SpawnWorldItem(TW.World, Device, FTransform(FVector(120.f, 0.f, 140.f)));
	TestFalse(TEXT("Customer device does not go into the inventory"), Customer->GoesIntoInventory());
	TestTrue(TEXT("[E] carries it"), Player->GetInteractionComponent()->ExecuteInteract(Customer));
	TestTrue(TEXT("In the hands"), Player->GetHeldItem() == Customer);
	TestEqual(TEXT("Nothing in the inventory"), Inv->CountItems(Radio), 0);

	// Hands full: taking another item out of the pocket is refused and nothing is lost.
	int32 Added = 0;
	Inv->TryAddItem(FWEItemInstance::Make(Radio, 1, EWEItemCondition::Used, 50), Added);
	if (PC)
	{
		TestFalse(TEXT("Hands full refuses"), PC->ExecuteTakeIntoHands(Inv, 0));
		TestEqual(TEXT("Still in the inventory"), Inv->CountItems(Radio), 1);
		Player->DropHeldItem();
		TestTrue(TEXT("Into the hands from the inventory"), PC->ExecuteTakeIntoHands(Inv, 0));
		TestTrue(TEXT("Holding the radio"), Player->GetHeldItem() && Player->GetHeldItem()->GetItem().Definition == Radio);
		TestEqual(TEXT("Removed from the inventory"), Inv->CountItems(Radio), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEInvStoreRaceTest, "WorkshopEmpire.Inventory.TwoPlayersSameItem", Flags)
bool FWEInvStoreRaceTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWECharacter* A = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	AWECharacter* B = SpawnPlayer(TW, FVector(0.f, 150.f, 100.f));
	UWEItemDefinition* Ram = MakeDefinition(TEXT("T_RamRace"), 0.05f, false, 1);
	AWEPickupItem* Item = SpawnItem(TW, Ram, 1, FVector(120.f, 60.f, 140.f));

	// Both requests arrive in the same server frame: the first wins, the second finds nothing.
	const bool bA = A->GetInteractionComponent()->ExecuteInteract(Item);
	const bool bB = B->GetInteractionComponent()->ExecuteInteract(Item);
	TestTrue(TEXT("A got it"), bA);
	TestFalse(TEXT("B was refused"), bB);
	TestEqual(TEXT("Exactly one copy exists"), A->GetPlayerInventory()->CountItems(Ram) + B->GetPlayerInventory()->CountItems(Ram), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEInvAccessTest, "WorkshopEmpire.Inventory.AccessRules", Flags)
bool FWEInvAccessTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWECharacter* A = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	AWECharacter* B = SpawnPlayer(TW, FVector(0.f, 150.f, 100.f));
	UWEInventoryComponent* Near = SpawnContainer(TW, FVector(150.f, 0.f, 100.f), 4, 50.f);
	UWEInventoryComponent* Far = SpawnContainer(TW, FVector(2000.f, 0.f, 100.f), 4, 50.f);

	TestTrue(TEXT("Own inventory"), A->GetPlayerInventory()->CanBeAccessedBy(A));
	TestFalse(TEXT("Other player's inventory is private"), B->GetPlayerInventory()->CanBeAccessedBy(A));
	TestTrue(TEXT("Container in reach"), Near->CanBeAccessedBy(A));
	TestFalse(TEXT("Container out of reach"), Far->CanBeAccessedBy(A));
	TestFalse(TEXT("No character"), Near->CanBeAccessedBy(nullptr));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEItemValueTest, "WorkshopEmpire.Items.ValueAndTooltip", Flags)
bool FWEItemValueTest::RunTest(const FString& Parameters)
{
	UWEItemDefinition* Gpu = MakeDefinition(TEXT("T_GpuValue"), 0.8f, false, 1, true, 10000);
	const FWEMoney New = FWEItemInstance::Make(Gpu, 1, EWEItemCondition::New, 50).GetEstimatedValue();
	const FWEMoney Used = FWEItemInstance::Make(Gpu, 1, EWEItemCondition::Used, 50).GetEstimatedValue();
	const FWEMoney Scrap = FWEItemInstance::Make(Gpu, 1, EWEItemCondition::Scrap, 50).GetEstimatedValue();
	TestTrue(TEXT("New > Used > Scrap"), New > Used && Used > Scrap);
	TestEqual(TEXT("New, quality 50: 100 EUR"), New.GetCents(), int64(10000));
	TestTrue(TEXT("Higher quality is worth more"), FWEItemInstance::Make(Gpu, 1, EWEItemCondition::Used, 90).GetEstimatedValue() > Used);
	TestTrue(TEXT("Stack value scales"), FWEItemInstance::Make(Gpu, 3, EWEItemCondition::New, 50).GetEstimatedValue().GetCents() == 30000);

	const FString Tooltip = UWEInventorySlotWidget::BuildTooltip(FWEItemInstance::Make(Gpu, 1, EWEItemCondition::Damaged, 40)).ToString();
	TestTrue(TEXT("Tooltip has condition"), Tooltip.Contains(TEXT("Beschädigt")));
	TestTrue(TEXT("Tooltip has weight"), Tooltip.Contains(TEXT("kg")));
	TestTrue(TEXT("Tooltip has value"), Tooltip.Contains(TEXT("€")));
	TestEqual(TEXT("FindById finds the definition"), UWEItemDefinition::FindById(TEXT("T_GpuValue")), Gpu);
	TestNull(TEXT("Unknown id"), UWEItemDefinition::FindById(TEXT("DoesNotExist_123")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
