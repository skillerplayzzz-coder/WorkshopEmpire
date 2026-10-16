#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Interaction/WEInteractionComponent.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "Player/WEInputConfig.h"
#include "UI/WEHUDWidget.h"
#include "WETestItems.h"
#include "WETestWorld.h"
#include "World/WEGarageDoor.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace WEGameplayTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	bool IsPhysicsOn(const AWEPickupItem* Item)
	{
		const UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Item->GetRootComponent());
		return Mesh && Mesh->IsSimulatingPhysics() && Mesh->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics;
	}
}

using namespace WEGameplayTests;
using namespace WETestItems;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWECarryHandOverTest, "WorkshopEmpire.Gameplay.Carry.PickupConflictDropHandOver", Flags)
bool FWECarryHandOverTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWECharacter* A = TW.Spawn<AWECharacter>(FVector(0.f, 0.f, 100.f));
	AWECharacter* B = TW.Spawn<AWECharacter>(FVector(0.f, 150.f, 100.f));
	AWEPickupItem* Item = SpawnBulky(TW, FVector(120.f, 60.f, 140.f), TEXT("Computer"), 9.f);
	if (!TestNotNull(TEXT("A"), A) || !TestNotNull(TEXT("B"), B) || !TestNotNull(TEXT("Item"), Item))
	{
		return false;
	}

	TestTrue(TEXT("A picks up via interaction"), A->GetInteractionComponent()->ExecuteInteract(Item));
	TestTrue(TEXT("A holds item"), A->GetHeldItem() == Item);
	TestTrue(TEXT("Item holder is A"), Item->GetHolder() == A);
	TestTrue(TEXT("Item attached to A"), Item->GetAttachParentActor() == A);
	TestFalse(TEXT("Physics off while held"), IsPhysicsOn(Item));

	TestFalse(TEXT("B cannot take an item A is holding"), B->GetInteractionComponent()->ExecuteInteract(Item));
	TestNull(TEXT("B holds nothing"), B->GetHeldItem());
	TestTrue(TEXT("Holder still A"), Item->GetHolder() == A);

	A->DropHeldItem();
	TestNull(TEXT("A hands empty after drop"), A->GetHeldItem());
	TestNull(TEXT("Item has no holder"), Item->GetHolder());
	TestNull(TEXT("Item detached"), Item->GetAttachParentActor());
	TestTrue(TEXT("Physics back on"), IsPhysicsOn(Item));

	TestTrue(TEXT("B picks up after drop"), B->GetInteractionComponent()->ExecuteInteract(Item));
	TestTrue(TEXT("Holder is B"), Item->GetHolder() == B);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWECarrySecondItemTest, "WorkshopEmpire.Gameplay.Carry.HandsFull", Flags)
bool FWECarrySecondItemTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWECharacter* A = TW.Spawn<AWECharacter>(FVector(0.f, 0.f, 100.f));
	AWEPickupItem* First = SpawnBulky(TW, FVector(120.f, 0.f, 140.f), TEXT("Eins"), 2.f);
	AWEPickupItem* Second = SpawnBulky(TW, FVector(120.f, 40.f, 140.f), TEXT("Zwei"), 2.f);

	TestTrue(TEXT("First pickup"), A->TryPickUpItem(First));
	FText Reason;
	TestFalse(TEXT("Second rejected while hands full"), Second->CanBePickedUpBy(A, Reason));
	TestFalse(TEXT("TryPickUpItem rejects second"), A->TryPickUpItem(Second));
	TestTrue(TEXT("Still holding first"), A->GetHeldItem() == First);
	TestNull(TEXT("Second untouched"), Second->GetHolder());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWECarryWeightTest, "WorkshopEmpire.Gameplay.Carry.WeightLimit", Flags)
bool FWECarryWeightTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWECharacter* A = TW.Spawn<AWECharacter>(FVector(0.f, 0.f, 100.f));
	AWEPickupItem* Motor = SpawnBulky(TW, FVector(120.f, 0.f, 120.f), TEXT("Elektromotor"), 45.f);
	AWEPickupItem* Light = SpawnBulky(TW, FVector(120.f, 40.f, 120.f), TEXT("Kiste"), A->GetMaxCarryWeightKg());

	FText Reason;
	TestFalse(TEXT("45 kg motor is too heavy"), Motor->CanBePickedUpBy(A, Reason));
	TestTrue(TEXT("Reason mentions weight"), Reason.ToString().Contains(TEXT("45")));
	TestFalse(TEXT("Server rejects heavy pickup"), A->GetInteractionComponent()->ExecuteInteract(Motor));
	TestTrue(TEXT("Item exactly at the limit is allowed"), A->TryPickUpItem(Light));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWECarryDropOnLeaveTest, "WorkshopEmpire.Gameplay.Carry.DropWhenPlayerLeaves", Flags)
bool FWECarryDropOnLeaveTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWECharacter* A = TW.Spawn<AWECharacter>(FVector(0.f, 0.f, 100.f));
	AWEPickupItem* Item = SpawnBulky(TW, FVector(120.f, 0.f, 140.f), TEXT("Werkzeugkiste"), 6.f);

	TestTrue(TEXT("Test world has begun play"), A->HasActorBegunPlay());
	TestTrue(TEXT("Pickup"), A->TryPickUpItem(Item));
	A->Destroy(); // what happens when a player disconnects

	TestTrue(TEXT("Item still exists"), IsValid(Item));
	TestNull(TEXT("Item released"), Item->GetHolder());
	TestNull(TEXT("Item detached"), Item->GetAttachParentActor());
	TestTrue(TEXT("Physics back on"), IsPhysicsOn(Item));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEReachTest, "WorkshopEmpire.Gameplay.Interaction.ServerReachValidation", Flags)
bool FWEReachTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWECharacter* A = TW.Spawn<AWECharacter>(FVector(0.f, 0.f, 100.f));
	AWEPickupItem* Far = SpawnBulky(TW, FVector(1000.f, 0.f, 140.f), TEXT("WeitWeg"), 1.f);
	AWEPickupItem* Near = SpawnBulky(TW, FVector(150.f, 0.f, 140.f), TEXT("Nah"), 1.f);

	// The server logs a warning for suspicious out-of-reach requests; that is the expected outcome here.
	AddExpectedMessage(TEXT("rejected: out of reach"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);

	TestFalse(TEXT("Far item out of reach"), UWEInteractionComponent::IsWithinReach(A, Far, A->GetInteractionComponent()->GetInteractionRange()));
	TestFalse(TEXT("Server rejects far interaction"), A->GetInteractionComponent()->ExecuteInteract(Far));
	TestNull(TEXT("Far item not taken"), Far->GetHolder());
	TestTrue(TEXT("Near item in reach"), UWEInteractionComponent::IsWithinReach(A, Near, A->GetInteractionComponent()->GetInteractionRange()));
	TestTrue(TEXT("Server accepts near interaction"), A->GetInteractionComponent()->ExecuteInteract(Near));
	TestFalse(TEXT("Null target rejected"), A->GetInteractionComponent()->ExecuteInteract(nullptr));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEDropPlacementTest, "WorkshopEmpire.Gameplay.Carry.DropStopsBeforeWall", Flags)
bool FWEDropPlacementTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWECharacter* A = TW.Spawn<AWECharacter>(FVector(0.f, 0.f, 100.f));
	AWEPickupItem* Item = SpawnBulky(TW, FVector(100.f, 0.f, 140.f), TEXT("Kiste3"), 3.f);
	TestTrue(TEXT("Pickup"), A->TryPickUpItem(Item));

	const FTransform Free = A->ComputeDropTransform(Item);
	TestTrue(TEXT("Free drop is in front of the player"), Free.GetLocation().X > 90.f && Free.GetLocation().X < 150.f);

	// Aimed surface within reach (a closed door 200 cm ahead): item is placed right in front of it.
	AWEGarageDoor* FarWall = TW.Spawn<AWEGarageDoor>(FVector(200.f, 0.f, 0.f));
	const FTransform Aimed = A->ComputeDropTransform(Item);
	TestTrue(TEXT("Placed at the aimed surface"), Aimed.GetLocation().X > 150.f && Aimed.GetLocation().X < 200.f);

	// A wall closer than the free drop distance: the item must stay on the player's side.
	AWEGarageDoor* NearWall = TW.Spawn<AWEGarageDoor>(FVector(60.f, 0.f, 0.f));
	const FTransform Blocked = A->ComputeDropTransform(Item);
	TestTrue(TEXT("Drop stops before the wall"), Blocked.GetLocation().X < 60.f);
	TestTrue(TEXT("Walls exist"), FarWall != nullptr && NearWall != nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEDoorTest, "WorkshopEmpire.Gameplay.Door.ToggleViaInteraction", Flags)
bool FWEDoorTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWECharacter* A = TW.Spawn<AWECharacter>(FVector(0.f, 0.f, 100.f));
	AWEGarageDoor* Door = TW.Spawn<AWEGarageDoor>(FVector(150.f, 0.f, 0.f));

	TestFalse(TEXT("Door starts closed"), Door->IsOpen());
	TestTrue(TEXT("Open"), A->GetInteractionComponent()->ExecuteInteract(Door));
	TestTrue(TEXT("Door open"), Door->IsOpen());
	TestTrue(TEXT("Close"), A->GetInteractionComponent()->ExecuteInteract(Door));
	TestFalse(TEXT("Door closed"), Door->IsOpen());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEInputConfigTest, "WorkshopEmpire.Input.DefaultMapping", Flags)
bool FWEInputConfigTest::RunTest(const FString& Parameters)
{
	UWEInputConfig* Config = UWEInputConfig::CreateDefault(GetTransientPackage());
	if (!TestNotNull(TEXT("Config"), Config))
	{
		return false;
	}
	TestNotNull(TEXT("Move"), Config->MoveAction.Get());
	TestNotNull(TEXT("Look"), Config->LookAction.Get());
	TestNotNull(TEXT("Jump"), Config->JumpAction.Get());
	TestNotNull(TEXT("Sprint"), Config->SprintAction.Get());
	TestNotNull(TEXT("Crouch"), Config->CrouchAction.Get());
	TestNotNull(TEXT("Inventory"), Config->InventoryAction.Get());
	TestNotNull(TEXT("MappingContext"), Config->MappingContext.Get());
	TestEqual(TEXT("Interact on E"), Config->GetKeyForAction(Config->InteractAction), EKeys::E);
	TestEqual(TEXT("Examine on F"), Config->GetKeyForAction(Config->ExamineAction), EKeys::F);
	TestEqual(TEXT("Sprint on LeftShift"), Config->GetKeyForAction(Config->SprintAction), EKeys::LeftShift);
	TestEqual(TEXT("Jump on Space"), Config->GetKeyForAction(Config->JumpAction), EKeys::SpaceBar);
	TestEqual(TEXT("Inventory on Tab"), Config->GetKeyForAction(Config->InventoryAction), EKeys::Tab);
	TestEqual(TEXT("Quicksave on F5"), Config->GetKeyForAction(Config->QuickSaveAction), EKeys::F5);
	TestEqual(TEXT("Quickload on F9"), Config->GetKeyForAction(Config->QuickLoadAction), EKeys::F9);
	TestEqual(TEXT("Secondary on R"), Config->GetKeyForAction(Config->SecondaryAction), EKeys::R);
	TestEqual(TEXT("Repair on Q"), Config->GetKeyForAction(Config->AlternativeAction), EKeys::Q);
	TestEqual(TEXT("Build mode on B"), Config->GetKeyForAction(Config->BuildAction), EKeys::B);
	TestEqual(TEXT("Remove on X"), Config->GetKeyForAction(Config->RemoveAction), EKeys::X);
	TestEqual(TEXT("Setting on T"), Config->GetKeyForAction(Config->SettingAction), EKeys::T);
	TestEqual(TEXT("Mapping count"), Config->MappingContext->GetMappings().Num(), 22);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEPromptFormatTest, "WorkshopEmpire.UI.PromptFormat", Flags)
bool FWEPromptFormatTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Key + action"), UWEHUDWidget::FormatPrompt(FText::FromString(TEXT("E")), FText::FromString(TEXT("Aufheben"))).ToString(), FString(TEXT("[E] Aufheben")));
	TestEqual(TEXT("Info line without key"), UWEHUDWidget::FormatPrompt(FText::GetEmpty(), FText::FromString(TEXT("Zu schwer"))).ToString(), FString(TEXT("Zu schwer")));
	TestEqual(TEXT("Weight format"), AWEPickupItem::FormatWeight(45.f).ToString().Replace(TEXT(","), TEXT(".")), FString(TEXT("45.0 kg")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
