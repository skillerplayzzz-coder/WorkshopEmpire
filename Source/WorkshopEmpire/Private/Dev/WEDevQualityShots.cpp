// Quality update runs (-WEShots=QU... on L_Garage): customer order from the pallet into the hands and onto the bench,
// the inventory fallback, save/load mid-work, the PC work mode, the cars outside.
#include "Dev/WEDevAutomationComponent.h"

#include "EngineUtils.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WETraderTerminal.h"
#include "Framework/WEGameMode.h"
#include "Framework/WEPlayerController.h"
#include "Interaction/WEInteractionComponent.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "WELog.h"
#include "Workshop/WEWorkbench.h"

void UWEDevAutomationComponent::LogCarry(const TCHAR* Step) const
{
#if !UE_BUILD_SHIPPING
	const AWECharacter* C = GetCharacter();
	const AWEWorkbench* B = FindBench();
	const AWEPickupItem* Device = FindCustomerDevice();
	TArray<FString> Prompts;
	if (C)
	{
		TArray<FWEInteractionPrompt> Lines;
		C->GetInteractionComponent()->UpdateFocus();
		C->GetInteractionComponent()->BuildPrompts(Lines);
		for (const FWEInteractionPrompt& L : Lines)
		{
			Prompts.Add(FString::Printf(TEXT("[%s] %s"), *L.KeyText.ToString(), *L.ActionText.ToString()));
		}
	}
	int32 CustomerInInventory = 0;
	if (const UWEInventoryComponent* Inv = GetMyInventory())
	{
		for (const FWEInventorySlotData& Slot : Inv->GetAllSlots())
		{
			CustomerInInventory += Slot.Item.IsCustomerItem() ? 1 : 0;
		}
	}
	FString Orders;
	if (const UWEContractComponent* Contracts = UWEContractComponent::Get(this))
	{
		for (const FWEContract& K : Contracts->GetContracts())
		{
			if (K.State == EWEContractState::Accepted && K.CustomerItemId.IsValid())
			{
				Orders += FString::Printf(TEXT("%s(device %s) "), *K.Title.ToString(), *K.CustomerItemId.ToString().Left(8));
			}
		}
	}
	UE_LOG(LogWorkshopEmpire, Display, TEXT("QU %s: held=%s focus=%s prompts={%s} device=%s id=%s holder=%s placed=%s at %s | bench=%s state='%s' | customer items in inventory=%d | orders: %s"),
		Step, C && C->GetHeldItem() ? *C->GetHeldItem()->GetDisplayName().ToString() : TEXT("-"),
		C ? *GetNameSafe(C->GetInteractionComponent()->GetFocusedActor()) : TEXT("-"), *FString::Join(Prompts, TEXT(" ; ")),
		Device ? *Device->GetDisplayName().ToString() : TEXT("none"), Device ? *Device->GetItem().InstanceId.ToString().Left(8) : TEXT("-"),
		Device ? *GetNameSafe(Device->GetHolder()) : TEXT("-"), Device && Device->GetPlacedOn() ? TEXT("bench") : TEXT("-"),
		Device ? *Device->GetActorLocation().ToCompactString() : TEXT("-"),
		B && B->GetPlacedItem() ? *B->GetPlacedItem()->GetDisplayName().ToString() : TEXT("free"), B && B->IsBusy() ? TEXT("busy") : TEXT("idle"),
		CustomerInInventory, *Orders);
#endif
}

void UWEDevAutomationComponent::RunQualityShots()
{
#if !UE_BUILD_SHIPPING
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	// --- Test A: repair order -> pallet -> in the hands -> on the bench (real focus and [E] like a player) -----------
	Schedule(1.0f, [PC]() { if (PC) { PC->ConsoleCommand(TEXT("DisableAllScreenMessages")); } });
	Schedule(2.0f, [this, PC]()
	{
		const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
		FGuid Chosen;
		for (const FWEContract& K : Contracts ? Contracts->GetContracts() : TArray<FWEContract>())
		{
			if (K.Kind == EWEContractKind::Repair && K.State == EWEContractState::Offered && K.Item && K.Item->ItemId == TEXT("OldComputer"))
			{
				Chosen = K.Id;
			}
		}
		if (!Chosen.IsValid() && PC)
		{
			PC->ServerDevAddContractOffer(8); // "Büro-PC zur Reparatur"
		}
	});
	Schedule(3.0f, [this, PC]()
	{
		const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
		FGuid Chosen;
		for (const FWEContract& K : Contracts ? Contracts->GetContracts() : TArray<FWEContract>())
		{
			if (K.Kind == EWEContractKind::Repair && K.State == EWEContractState::Offered && K.Item && K.Item->ItemId == TEXT("OldComputer"))
			{
				Chosen = K.Id;
			}
		}
		UE_LOG(LogWorkshopEmpire, Display, TEXT("QU accept repair order %s"), *Chosen.ToString());
		if (PC && Chosen.IsValid())
		{
			PC->RequestAcceptContract(FindTerminal(), Chosen);
		}
	});
	Schedule(5.0f, [this, PC]()
	{
		LogCarry(TEXT("A0 accepted"));
		if (const AWEPickupItem* Device = FindCustomerDevice(); Device && PC)
		{
			const AWEDeliveryZone* Zone = AWEDeliveryZone::Find(GetWorld());
			const FVector Center = Zone ? Zone->GetActorLocation() : Device->GetActorLocation();
			FVector Away = (GetCharacter() ? GetCharacter()->GetActorLocation() : Center + FVector(200.f, 0.f, 0.f)) - Center;
			Away.Z = 0.f;
			const FVector Spot = Device->GetActorLocation() + Away.GetSafeNormal() * 130.f;
			PC->ServerDevTeleport(FVector(Spot.X, Spot.Y, Center.Z + 100.f), (Device->GetActorLocation() - Spot).Rotation());
		}
	});
	Schedule(6.0f, [this]() { LookAt(FindCustomerDevice()); });
	Schedule(6.5f, [this]() { LogCarry(TEXT("A1 looking at the device on the pallet")); Shot(TEXT("01_Palette")); });
	Schedule(7.0f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->UpdateFocus(); C->GetInteractionComponent()->RequestPrimaryInteraction(); } });
	Schedule(8.0f, [this]() { LogCarry(TEXT("A2 after [E] on the device")); Shot(TEXT("02_In_der_Hand")); });
	Schedule(8.5f, [this, PC]()
	{
		if (AWEWorkbench* B = FindBench(); B && PC)
		{
			const FVector Spot = B->GetWorkSpotLocation();
			PC->ServerDevTeleport(Spot + B->GetActorForwardVector() * 110.f + FVector(0.f, 0.f, 10.f), FRotator::ZeroRotator);
		}
	});
	Schedule(9.2f, [this]() { if (AWEWorkbench* B = FindBench()) { LookAtPoint(B->GetWorkSpotLocation()); } });
	Schedule(9.6f, [this]() { LogCarry(TEXT("A3 carrying, aiming at the bench")); Shot(TEXT("03_Werkbank_anvisiert")); });
	Schedule(10.0f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->UpdateFocus(); C->GetInteractionComponent()->RequestPrimaryInteraction(); } });
	Schedule(11.0f, [this]() { if (const AWEPickupItem* D = FindCustomerDevice()) { LookAtPoint(D->GetActorLocation()); } });
	Schedule(11.5f, [this]() { LogCarry(TEXT("A4 after [E] at the bench")); Shot(TEXT("04_Auf_der_Werkbank")); });
	// Save with the PC on the bench, load, the order still points at it.
	Schedule(12.5f, [this]()
	{
		AWEGameMode* GM = GetWorld()->GetAuthGameMode<AWEGameMode>();
		const bool bSaved = GM && GM->SaveToSlot(TEXT("QUA"));
		const bool bLoaded = GM && GM->LoadFromSlot(TEXT("QUA"));
		UE_LOG(LogWorkshopEmpire, Display, TEXT("QU save %d load %d"), bSaved ? 1 : 0, bLoaded ? 1 : 0);
	});
	Schedule(14.0f, [this]() { LogCarry(TEXT("A5 after save/load with the PC on the bench")); });
	// Take it off again ([R]) and carry it.
	Schedule(14.5f, [this]()
	{
		if (AWECharacter* C = GetCharacter(); C && FindCustomerDevice())
		{
			LookAtPoint(FindCustomerDevice()->GetActorLocation());
			C->GetInteractionComponent()->RequestSecondaryInteractWith(FindCustomerDevice());
		}
	});
	Schedule(15.5f, [this]() { LogCarry(TEXT("A6 taken off the bench")); });
	// Save while carrying, load: the PC must not be lost.
	Schedule(16.0f, [this]()
	{
		AWEGameMode* GM = GetWorld()->GetAuthGameMode<AWEGameMode>();
		const bool bSaved = GM && GM->SaveToSlot(TEXT("QUA2"));
		const bool bLoaded = GM && GM->LoadFromSlot(TEXT("QUA2"));
		UE_LOG(LogWorkshopEmpire, Display, TEXT("QU save while carrying %d load %d"), bSaved ? 1 : 0, bLoaded ? 1 : 0);
	});
	Schedule(17.5f, [this]() { LogCarry(TEXT("A7 after save/load while carrying")); Shot(TEXT("05_Nach_Laden")); });
	// --- Inventory fallback: a radio in the pocket -> "In die Hand nehmen" -> onto the bench -------------------------
	Schedule(18.0f, [PC]() { if (PC) { PC->ServerDevGiveItem(TEXT("PortableRadio"), 1, EWEItemCondition::Damaged); } });
	Schedule(19.0f, [this, PC]()
	{
		const int32 Slot = FindMySlot(TEXT("PortableRadio"));
		UE_LOG(LogWorkshopEmpire, Display, TEXT("QU B0 radio in inventory slot %d"), Slot);
		if (PC && Slot != INDEX_NONE) { PC->RequestTakeIntoHands(GetMyInventory(), Slot); }
	});
	Schedule(20.0f, [this, PC]()
	{
		LogCarry(TEXT("B1 radio taken into the hands"));
		if (AWEWorkbench* B = FindBench(); B && PC)
		{
			PC->ServerDevTeleport(B->GetWorkSpotLocation() + B->GetActorForwardVector() * 110.f + FVector(0.f, 0.f, 10.f), FRotator::ZeroRotator);
		}
	});
	Schedule(20.8f, [this]() { if (AWEWorkbench* B = FindBench()) { LookAtPoint(B->GetWorkSpotLocation()); } });
	Schedule(21.3f, [this]() { LogCarry(TEXT("B2 radio aimed at the bench")); Shot(TEXT("06_Radio_Vorschau_gruen")); });
	Schedule(21.8f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->UpdateFocus(); C->GetInteractionComponent()->RequestPrimaryInteraction(); } });
	Schedule(22.8f, [this]()
	{
		const AWEWorkbench* B = FindBench();
		UE_LOG(LogWorkshopEmpire, Display, TEXT("QU B3 on the bench: %s"), B && B->GetPlacedItem() ? *B->GetPlacedItem()->GetDisplayName().ToString() : TEXT("nothing"));
		// Pick the customer PC up from the floor again and aim at the occupied bench: red preview, reason shown.
		if (AWECharacter* C = GetCharacter(); C && FindCustomerDevice())
		{
			C->GetInteractionComponent()->RequestInteractWith(FindCustomerDevice());
		}
	});
	Schedule(23.8f, [this]() { if (AWEWorkbench* B = FindBench()) { LookAtPoint(B->GetWorkSpotLocation()); } });
	Schedule(24.3f, [this]() { LogCarry(TEXT("B4 PC aimed at the occupied bench")); Shot(TEXT("07_Belegt_Vorschau_rot")); });
	Schedule(25.5f, [this]() { Quit(); });
#endif
}
