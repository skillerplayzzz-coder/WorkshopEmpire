// Quality update runs (-WEShots=QU... on L_Garage): customer order from the pallet into the hands and onto the bench,
// the inventory fallback, save/load mid-work, the PC work mode, the cars outside.
#include "Dev/WEDevAutomationComponent.h"

#include "EngineUtils.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WECompanyComponent.h"
#include "Items/WEItemTypes.h"
#include "TimerManager.h"
#include "Workshop/WEAssemblyDefinition.h"
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

void UWEDevAutomationComponent::RunQualityRepairFlow()
{
#if !UE_BUILD_SHIPPING
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	TSharedRef<FGuid> OrderId = MakeShared<FGuid>();
	TSharedRef<int64> BalanceBefore = MakeShared<int64>(0);
	Schedule(1.0f, [PC]()
	{
		if (!PC) { return; }
		PC->ConsoleCommand(TEXT("DisableAllScreenMessages"));
		PC->ServerDevGiveItem(TEXT("Screwdriver"), 1, EWEItemCondition::Used);
		PC->ServerDevGiveItem(TEXT("Multimeter"), 1, EWEItemCondition::Used);
		PC->ServerDevGiveItem(TEXT("GraphicsCard"), 1, EWEItemCondition::New);
		PC->ServerDevGiveItem(TEXT("ThermalPaste"), 1, EWEItemCondition::New);
		PC->ServerDevAddContractOffer(8); // "Büro-PC zur Reparatur"
	});
	Schedule(2.5f, [this, PC, OrderId]()
	{
		for (const FWEContract& K : UWEContractComponent::Get(this) ? UWEContractComponent::Get(this)->GetContracts() : TArray<FWEContract>())
		{
			if (K.Kind == EWEContractKind::Repair && K.State == EWEContractState::Offered && K.Item && K.Item->ItemId == TEXT("OldComputer"))
			{
				*OrderId = K.Id;
			}
		}
		if (PC && OrderId->IsValid()) { PC->RequestAcceptContract(FindTerminal(), *OrderId); }
	});
	// Pallet -> hands -> bench, with [E] like a player.
	Schedule(4.0f, [this, PC]()
	{
		if (const AWEPickupItem* Device = FindCustomerDevice(); Device && PC)
		{
			PC->ServerDevTeleport(Device->GetActorLocation() + FVector(-120.f, 60.f, 90.f), FRotator::ZeroRotator);
		}
	});
	Schedule(4.8f, [this]() { LookAt(FindCustomerDevice()); });
	Schedule(5.2f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->UpdateFocus(); C->GetInteractionComponent()->RequestPrimaryInteraction(); } });
	Schedule(6.0f, [this, PC]()
	{
		if (AWEWorkbench* B = FindBench(); B && PC) { PC->ServerDevTeleport(B->GetWorkSpotLocation() + B->GetActorForwardVector() * 110.f + FVector(0.f, 0.f, 10.f), FRotator::ZeroRotator); }
	});
	Schedule(6.8f, [this]() { if (AWEWorkbench* B = FindBench()) { LookAtPoint(B->GetWorkSpotLocation()); } });
	Schedule(7.2f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->UpdateFocus(); C->GetInteractionComponent()->RequestPrimaryInteraction(); } });
	// The customer's complaint: the graphics card is dead (as the diagnosis would find).
	Schedule(8.0f, [this, BalanceBefore]()
	{
		AWEWorkbench* B = FindBench();
		AWEPickupItem* Placed = B ? B->GetPlacedItem() : nullptr;
		if (!Placed) { LogCarry(TEXT("R0 NOT ON THE BENCH")); return; }
		FWEItemInstance Item = Placed->GetItem();
		Item.Repair.SetFlag(FWERepairState::FaultsRolled);
		Item.Repair.SetFlag(FWERepairState::Diagnosed);
		Item.Repair.Faults = 1 << 2; // "Grafikkarte defekt"
		Placed->SetItem(Item);
		*BalanceBefore = UWECompanyComponent::Get(this) ? UWECompanyComponent::Get(this)->GetBalance().GetCents() : 0;
		LogCarry(TEXT("R0 customer PC on the bench, GPU fault"));
		if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->UpdateFocus(); C->GetInteractionComponent()->RequestPrimaryInteraction(); }
	});
	Schedule(9.5f, [this]() { Shot(TEXT("01_Arbeitsmodus_Kunden_PC")); });
	// Part work: panel off, GPU out, new GPU in, panel on (timed steps, the same server functions the work view uses).
	struct FJob { bool bInstall; const TCHAR* Part; const TCHAR* Shot; };
	TSharedRef<TArray<FJob>> Jobs = MakeShared<TArray<FJob>>(TArray<FJob>{
		{ false, TEXT("SidePanel"), TEXT("02_Seitenteil_ab") }, { false, TEXT("GraphicsCard"), TEXT("03_Grafikkarte_raus") },
		{ true, TEXT("GraphicsCard"), TEXT("04_Neue_Grafikkarte") }, { false, TEXT("CpuCooler"), TEXT("05_Kuehler_ab") },
		{ true, TEXT("CpuCooler"), TEXT("06_Kuehler_mit_Paste") }, { true, TEXT("SidePanel"), TEXT("07_Seitenteil_zu") } });
	TSharedRef<int32> Next = MakeShared<int32>(0);
	TSharedRef<bool> bRunning = MakeShared<bool>(false);
	Schedule(10.0f, [this, Jobs, Next, bRunning, PC, OrderId, BalanceBefore]()
	{
		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Jobs, Next, bRunning, PC, OrderId, BalanceBefore]()
		{
			AWEWorkbench* B = FindBench();
			AWECharacter* C = GetCharacter();
			const UWEAssemblyDefinition* Asm = B ? B->GetAssembly() : nullptr;
			if (!B || !C || !Asm || B->IsBusy())
			{
				return;
			}
			if (*bRunning)
			{
				*bRunning = false;
				const FJob& Done = (*Jobs)[*Next];
				UE_LOG(LogWorkshopEmpire, Display, TEXT("QU R %s %s: removed=%d faults=%d needs test=%d"), Done.bInstall ? TEXT("installed") : TEXT("removed"), Done.Part,
					B->GetPlacedItem()->GetItem().RemovedParts, B->GetPlacedItem()->GetItem().Repair.Faults, B->GetPlacedItem()->GetItem().NeedsFunctionTest() ? 1 : 0);
				Shot(Done.Shot);
				++*Next;
				return;
			}
			if (!Jobs->IsValidIndex(*Next))
			{
				return;
			}
			const FJob& Job = (*Jobs)[*Next];
			const int32 Part = Asm->FindPart(Job.Part);
			FText Reason;
			const bool bOk = Job.bInstall ? B->CanStartInstall(C, Reason, Part) && B->TryStartInstall(C, Part) : B->CanStartStep(C, Reason, Part) && B->TryStartStep(C, Part);
			UE_LOG(LogWorkshopEmpire, Display, TEXT("QU R start %s %s: %s %s"), Job.bInstall ? TEXT("install") : TEXT("remove"), Job.Part, bOk ? TEXT("ok") : TEXT("REFUSED"), *Reason.ToString());
			*bRunning = bOk;
			if (!bOk) { ++*Next; }
		}), 0.3f, true);
	});
	// Function test, then off the bench and to the customer.
	Schedule(52.0f, [this]()
	{
		AWEWorkbench* B = FindBench();
		FText Reason;
		const FWERepairStepInfo Info = B ? B->GetNextRepairStep(Reason) : FWERepairStepInfo();
		const bool bOk = B && GetCharacter() && B->TryStartRepairStep(GetCharacter());
		UE_LOG(LogWorkshopEmpire, Display, TEXT("QU R function test (kind %d): %s %s"), static_cast<int32>(Info.Kind), bOk ? TEXT("started") : TEXT("REFUSED"), *Reason.ToString());
	});
	Schedule(58.0f, [this, PC]()
	{
		const AWEWorkbench* B = FindBench();
		const AWEPickupItem* P = B ? B->GetPlacedItem() : nullptr;
		UE_LOG(LogWorkshopEmpire, Display, TEXT("QU R after test: %s condition=%s faults=%d removed=%d"), P ? *P->GetDisplayName().ToString() : TEXT("-"),
			P ? *WEItems::GetConditionText(P->GetItem().Condition).ToString() : TEXT("-"), P ? P->GetItem().Repair.Faults : -1, P ? P->GetItem().RemovedParts : -1);
		Shot(TEXT("08_Getestet"));
		if (PC) { PC->CloseWorkView(); }
	});
	Schedule(59.0f, [this]()
	{
		if (AWEWorkbench* B = FindBench(); B && GetCharacter()) { B->TakeOffItem(GetCharacter()); }
		LogCarry(TEXT("R1 taken off the bench"));
		TeleportToTerminal(0.f);
	});
	Schedule(60.5f, [this, PC, OrderId]() { LookAt(FindTerminal()); if (PC) { PC->RequestDeliverContract(FindTerminal(), *OrderId); } });
	Schedule(62.0f, [this, OrderId, BalanceBefore]()
	{
		const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
		const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
		const bool bStillOpen = Contracts && Contracts->FindContract(*OrderId) != nullptr;
		UE_LOG(LogWorkshopEmpire, Display, TEXT("QU R delivered: order open=%d, balance %lld -> %lld cents, customer device left in world=%d, held=%s"),
			bStillOpen ? 1 : 0, *BalanceBefore, Company ? Company->GetBalance().GetCents() : -1, FindCustomerDevice() ? 1 : 0,
			GetCharacter() && GetCharacter()->GetHeldItem() ? *GetCharacter()->GetHeldItem()->GetDisplayName().ToString() : TEXT("-"));
		Shot(TEXT("09_Abgeliefert"));
	});
	Schedule(63.5f, [this]() { Quit(); });
#endif
}

void UWEDevAutomationComponent::RunQualityShots()
{
#if !UE_BUILD_SHIPPING
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	if (ShotName == TEXT("QUB"))
	{
		RunQualityRepairFlow();
		return;
	}
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
