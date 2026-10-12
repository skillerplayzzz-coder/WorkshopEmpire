#include "Framework/WEPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Dev/WEDevAutomationComponent.h"
#include "Framework/WEGameMode.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "UI/WEHUDWidget.h"
#include "UI/WEInventoryWidget.h"
#include "WELog.h"

#define LOCTEXT_NAMESPACE "WEPlayerController"

AWEPlayerController::AWEPlayerController()
{
	HUDWidgetClass = UWEHUDWidget::StaticClass();
	InventoryWidgetClass = UWEInventoryWidget::StaticClass();
}

void AWEPlayerController::BeginPlay()
{
	Super::BeginPlay();

	const TCHAR* NetModeName = TEXT("Unknown");
	switch (GetNetMode())
	{
	case NM_Standalone:      NetModeName = TEXT("Standalone"); break;
	case NM_DedicatedServer: NetModeName = TEXT("DedicatedServer"); break;
	case NM_ListenServer:    NetModeName = TEXT("ListenServer"); break;
	case NM_Client:          NetModeName = TEXT("Client"); break;
	default: break;
	}

	UE_LOG(LogWENet, Log, TEXT("%s BeginPlay (NetMode=%s, Local=%s)"),
		*GetName(), NetModeName, IsLocalController() ? TEXT("yes") : TEXT("no"));

	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);

		if (HUDWidgetClass)
		{
			HUDWidget = CreateWidget<UWEHUDWidget>(this, HUDWidgetClass);
			if (HUDWidget)
			{
				HUDWidget->AddToViewport(0);
			}
		}

		if (UWEDevAutomationComponent::IsRequested())
		{
			UWEDevAutomationComponent* Automation = NewObject<UWEDevAutomationComponent>(this, TEXT("DevAutomation"));
			Automation->RegisterComponent();
		}
	}
}

// --- Inventory UI --------------------------------------------------------------------------------

bool AWEPlayerController::IsInventoryOpen() const
{
	return InventoryWidget && InventoryWidget->IsInViewport();
}

void AWEPlayerController::ToggleInventory()
{
	if (IsInventoryOpen())
	{
		CloseInventory();
	}
	else
	{
		OpenInventory(nullptr);
	}
}

void AWEPlayerController::OpenInventory(UWEInventoryComponent* Container)
{
	const AWECharacter* WECharacter = GetPawn<AWECharacter>();
	UWEInventoryComponent* PlayerInventory = WECharacter ? WECharacter->GetPlayerInventory() : nullptr;
	if (!IsLocalController() || !PlayerInventory || !InventoryWidgetClass)
	{
		return;
	}
	if (!InventoryWidget)
	{
		InventoryWidget = CreateWidget<UWEInventoryWidget>(this, InventoryWidgetClass);
	}
	if (!InventoryWidget)
	{
		return;
	}
	InventoryWidget->Setup(PlayerInventory, Container);
	if (!InventoryWidget->IsInViewport())
	{
		InventoryWidget->AddToViewport(10);
	}

	FInputModeGameAndUI Mode;
	Mode.SetWidgetToFocus(InventoryWidget->TakeWidget());
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	SetShowMouseCursor(true);
	SetIgnoreLookInput(true);
	SetIgnoreMoveInput(true);
}

void AWEPlayerController::CloseInventory()
{
	if (InventoryWidget && InventoryWidget->IsInViewport())
	{
		InventoryWidget->RemoveFromParent();
	}
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
	ResetIgnoreLookInput();
	ResetIgnoreMoveInput();
}

void AWEPlayerController::ClientOpenContainer_Implementation(UWEInventoryComponent* Container)
{
	OpenInventory(Container);
}

void AWEPlayerController::ClientShowMessage_Implementation(const FText& Message)
{
	ShowLocalMessage(Message);
}

void AWEPlayerController::ShowLocalMessage(const FText& Message)
{
	if (HUDWidget)
	{
		HUDWidget->ShowMessage(Message);
	}
	UE_LOG(LogWorkshopEmpire, Display, TEXT("Message: %s"), *Message.ToString());
}

// --- Inventory requests ------------------------------------------------------------------------------

void AWEPlayerController::RequestMoveItem(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To, int32 ToSlot, int32 Quantity)
{
	ServerMoveItem(From, FromSlot, To, ToSlot, Quantity);
}

void AWEPlayerController::RequestQuickTransfer(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To)
{
	ServerQuickTransfer(From, FromSlot, To);
}

void AWEPlayerController::RequestDropFromInventory(UWEInventoryComponent* From, int32 Slot, int32 Quantity)
{
	ServerDropFromInventory(From, Slot, Quantity);
}

void AWEPlayerController::RequestSort(UWEInventoryComponent* Inventory)
{
	ServerSortInventory(Inventory);
}

void AWEPlayerController::ServerMoveItem_Implementation(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To, int32 ToSlot, int32 Quantity)
{
	ExecuteMoveItem(From, FromSlot, To, ToSlot, Quantity);
}

void AWEPlayerController::ServerQuickTransfer_Implementation(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To)
{
	ExecuteQuickTransfer(From, FromSlot, To);
}

void AWEPlayerController::ServerDropFromInventory_Implementation(UWEInventoryComponent* From, int32 Slot, int32 Quantity)
{
	ExecuteDropFromInventory(From, Slot, Quantity);
}

void AWEPlayerController::ServerSortInventory_Implementation(UWEInventoryComponent* Inventory)
{
	if (CanUseInventory(Inventory))
	{
		Inventory->SortItems();
	}
}

bool AWEPlayerController::CanUseInventory(const UWEInventoryComponent* Inventory) const
{
	const AWECharacter* WECharacter = GetPawn<AWECharacter>();
	const bool bOk = Inventory && Inventory->CanBeAccessedBy(WECharacter);
	if (!bOk && Inventory)
	{
		UE_LOG(LogWENet, Warning, TEXT("%s denied access to inventory %s"), *GetName(), *GetPathNameSafe(Inventory));
	}
	return bOk;
}

bool AWEPlayerController::ExecuteMoveItem(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To, int32 ToSlot, int32 Quantity)
{
	if (!HasAuthority() || !CanUseInventory(From) || !CanUseInventory(To))
	{
		return false;
	}
	const EWEInventoryResult Result = From->MoveItem(FromSlot, To, ToSlot, Quantity);
	if (Result == EWEInventoryResult::TooHeavy)
	{
		ClientShowMessage(LOCTEXT("TooHeavy", "Zu schwer für dieses Lager"));
	}
	else if (Result == EWEInventoryResult::NoSpace)
	{
		ClientShowMessage(LOCTEXT("NoSpace", "Kein Platz"));
	}
	return Result == EWEInventoryResult::Success || Result == EWEInventoryResult::PartiallyAdded;
}

bool AWEPlayerController::ExecuteQuickTransfer(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To)
{
	if (!HasAuthority() || From == To || !CanUseInventory(From) || !CanUseInventory(To))
	{
		return false;
	}
	const FWEItemInstance* Source = From->GetItemInSlot(FromSlot);
	if (!Source)
	{
		return false;
	}
	const int32 Movable = To->GetAddableQuantity(*Source);
	if (Movable <= 0)
	{
		ClientShowMessage(LOCTEXT("TargetFull", "Kein Platz im Ziel"));
		return false;
	}
	FWEItemInstance Removed;
	From->RemoveFromSlot(FromSlot, Movable, Removed);
	int32 Added = 0;
	To->TryAddItem(Removed, Added);
	if (Added < Removed.Quantity)
	{
		// Should not happen (we checked first); never lose items: put the rest back.
		FWEItemInstance Rest = Removed;
		Rest.Quantity -= Added;
		int32 Back = 0;
		From->TryAddItem(Rest, Back);
	}
	return Added > 0;
}

bool AWEPlayerController::ExecuteDropFromInventory(UWEInventoryComponent* From, int32 Slot, int32 Quantity)
{
	AWECharacter* WECharacter = GetPawn<AWECharacter>();
	if (!HasAuthority() || !WECharacter || !CanUseInventory(From))
	{
		return false;
	}
	const FWEItemInstance* Source = From->GetItemInSlot(Slot);
	if (!Source)
	{
		return false;
	}
	const float Radius = Source->Definition ? Source->Definition->WorldScale.GetMax() * 50.f : 20.f;
	FWEItemInstance Removed;
	if (From->RemoveFromSlot(Slot, Quantity <= 0 ? Source->Quantity : Quantity, Removed) != EWEInventoryResult::Success)
	{
		return false;
	}
	const FTransform DropTransform = WECharacter->ComputeDropTransform(Radius, nullptr);
	AWEPickupItem* Spawned = AWEPickupItem::SpawnWorldItem(GetWorld(), Removed, DropTransform);
	if (!Spawned)
	{
		// Never lose the item if spawning fails.
		int32 Back = 0;
		From->TryAddItem(Removed, Back);
		return false;
	}
	UE_LOG(LogWENet, Log, TEXT("%s dropped %d x %s from inventory"), *GetName(), Removed.Quantity, *Removed.GetDisplayName().ToString());
	return true;
}

// --- Save / load -------------------------------------------------------------------------------------

void AWEPlayerController::RequestSave(const FString& Slot)
{
	AWEGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AWEGameMode>() : nullptr;
	if (!GameMode || !IsLocalController())
	{
		ShowLocalMessage(LOCTEXT("HostOnlySave", "Nur der Host kann speichern."));
		return;
	}
	GameMode->SaveToSlot(Slot);
}

void AWEPlayerController::RequestLoad(const FString& Slot)
{
	AWEGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AWEGameMode>() : nullptr;
	if (!GameMode || !IsLocalController())
	{
		ShowLocalMessage(LOCTEXT("HostOnlyLoad", "Nur der Host kann laden."));
		return;
	}
	CloseInventory();
	GameMode->LoadFromSlot(Slot);
}

void AWEPlayerController::WESave(const FString& Slot)
{
	RequestSave(Slot.IsEmpty() ? TEXT("Quicksave") : Slot);
}

void AWEPlayerController::WELoad(const FString& Slot)
{
	RequestLoad(Slot.IsEmpty() ? TEXT("Quicksave") : Slot);
}

#undef LOCTEXT_NAMESPACE
