// Quality update runs (-WEShots=QU... on L_Garage): customer order from the pallet into the hands and onto the bench,
// the inventory fallback, save/load mid-work, the PC work mode, the cars outside.
#include "Dev/WEDevAutomationComponent.h"

#include "EngineUtils.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WECourierComponent.h"
#include "Economy/WECompanyComponent.h"
#include "Items/WEItemTypes.h"
#include "TimerManager.h"
#include "RHI.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Workshop/WEAssemblyDefinition.h"
#include "Economy/WETraderTerminal.h"
#include "Framework/WEGameMode.h"
#include "Framework/WEPlayerController.h"
#include "Interaction/WEInteractionComponent.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Kismet/GameplayStatics.h"
#include "Online/WESessionSubsystem.h"
#include "Player/WECharacter.h"
#include "Progression/WETutorialComponent.h"
#include "WELog.h"
#include "Workshop/WEWorkbench.h"
#include "GameFramework/WorldSettings.h"
#include "UI/WETradeWidget.h"
#include "World/WEDayNightComponent.h"

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
	Schedule(7.2f, [this]()
	{
		// (Aim again right before [E]: the camera may still have been easing to the eye height after the teleport.)
		AWEWorkbench* B = FindBench();
		if (AWECharacter* C = GetCharacter(); C && B)
		{
			LookAtPoint(B->GetWorkSpotLocation());
			UE_LOG(LogWorkshopEmpire, Display, TEXT("QU R aiming at %s"), *GetNameSafe(C->FindAimedWorkbench()));
			C->GetInteractionComponent()->UpdateFocus();
			C->GetInteractionComponent()->RequestPrimaryInteraction();
		}
	});
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
				if (!Done.bInstall && FCString::Strcmp(Done.Part, TEXT("GraphicsCard")) == 0)
				{
					// Test 8 / save case: the customer PC half taken apart is saved and loaded - same state afterwards.
					const FWEItemInstance Before = B->GetPlacedItem()->GetItem();
					AWEGameMode* GM = GetWorld()->GetAuthGameMode<AWEGameMode>();
					const bool bSaved = GM && GM->SaveToSlot(TEXT("QUB_Mid"));
					const bool bLoaded = GM && GM->LoadFromSlot(TEXT("QUB_Mid"));
					const AWEWorkbench* B2 = FindBench();
					const AWEPickupItem* P2 = B2 ? B2->GetPlacedItem() : nullptr;
					const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
					bool bLinked = false;
					for (const FWEContract& K : Contracts ? Contracts->GetContracts() : TArray<FWEContract>())
					{
						bLinked |= P2 && K.CustomerItemId == P2->GetItem().InstanceId;
					}
					UE_LOG(LogWorkshopEmpire, Display, TEXT("QU R save/load half taken apart: saved %d loaded %d, on bench %d, same id %d, removed %d -> %d, faults %d -> %d, order linked %d"),
						bSaved ? 1 : 0, bLoaded ? 1 : 0, P2 ? 1 : 0, P2 && P2->GetItem().InstanceId == Before.InstanceId ? 1 : 0, Before.RemovedParts,
						P2 ? P2->GetItem().RemovedParts : -1, Before.Repair.Faults, P2 ? P2->GetItem().Repair.Faults : -1, bLinked ? 1 : 0);
					// The work view was closed by the load: open it again.
					if (AWECharacter* Me = GetCharacter(); Me && P2)
					{
						if (PC) { PC->OpenWorkView(const_cast<AWEWorkbench*>(B2), false); }
					}
				}
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

void UWEDevAutomationComponent::RunQualityNewGameFlow()
{
#if !UE_BUILD_SHIPPING
	// Test 1: a new company from the main menu with the tutorial; every tutorial step through the normal player requests
	// (no dev money, no dev items); save; back to the menu; continue - the same company must come back.
	static const FString Company = TEXT("QU_Test1");
	struct FSnapshot { int64 Balance = -1; int32 TutorialStep = -1; int32 Slots = -1; int32 Tools = -1; };
	static FSnapshot Saved;
	static int32 GameVisits = 0;
	UWESessionSubsystem* Session = UWESessionSubsystem::Get(this);
	const int32 Visit = Session ? Session->DevMenuVisits : 0;
	if (IsInMenu())
	{
		Schedule(1.f, [Visit]() { UE_LOG(LogWorkshopEmpire, Display, TEXT("QU T menu visit %d"), Visit); });
		if (GameVisits == 0)
		{
			// (An old test company would be loaded instead of a new one.)
			Schedule(1.5f, []() { UE_LOG(LogWorkshopEmpire, Display, TEXT("QU T old slot removed: %d"), UGameplayStatics::DeleteGameInSlot(Company, 0) ? 1 : 0); });
			Schedule(3.f, [Session]() { if (Session) { Session->HostNewGame(Company, /*bTutorial*/ true, /*bLAN*/ true); } });
		}
		else
		{
			Schedule(3.f, [Session]() { if (Session) { Session->HostGame(Company, /*bLAN*/ true); } });
		}
		return;
	}
	++GameVisits;
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Snapshot = [this]()
	{
		FSnapshot S;
		const UWECompanyComponent* Co = UWECompanyComponent::Get(this);
		const UWETutorialComponent* Tutorial = UWETutorialComponent::Get(this);
		const UWEInventoryComponent* Mine = GetMyInventory();
		S.Balance = Co ? Co->GetBalance().GetCents() : -1;
		S.TutorialStep = Tutorial ? Tutorial->GetStep() : -1;
		S.Slots = Mine ? Mine->GetAllSlots().Num() : -1;
		S.Tools = 0;
		for (const FWEInventorySlotData& Slot : Mine ? Mine->GetAllSlots() : TArray<FWEInventorySlotData>())
		{
			S.Tools += Slot.Item.Definition && Slot.Item.Definition->ToolType != EWEToolType::None ? 1 : 0;
		}
		return S;
	};
	auto Log = [this, Snapshot](const FString& What)
	{
		const UWETutorialComponent* Tutorial = UWETutorialComponent::Get(this);
		const FSnapshot S = Snapshot();
		UE_LOG(LogWorkshopEmpire, Display, TEXT("QU T %s: tutorial state=%d step=%d/%d, balance=%lld cents, inventory slots=%d tools=%d"), *What,
			Tutorial ? static_cast<int32>(Tutorial->GetState()) : -1, S.TutorialStep, Tutorial ? Tutorial->NumSteps() : -1, S.Balance, S.Slots, S.Tools);
	};
	if (GameVisits > 1)
	{
		// Continued from the menu: the saved company.
		Schedule(4.f, [this, Snapshot, Log]()
		{
			const FSnapshot S = Snapshot();
			Log(TEXT("continued"));
			UE_LOG(LogWorkshopEmpire, Display, TEXT("QU T compare saved -> continued: balance %lld -> %lld, tutorial step %d -> %d, slots %d -> %d, tools %d -> %d: %s"),
				Saved.Balance, S.Balance, Saved.TutorialStep, S.TutorialStep, Saved.Slots, S.Slots, Saved.Tools, S.Tools,
				Saved.Balance == S.Balance && Saved.TutorialStep == S.TutorialStep && Saved.Slots == S.Slots && Saved.Tools == S.Tools ? TEXT("SAME") : TEXT("DIFFERENT"));
			Shot(TEXT("09_Fortgesetzt"));
		});
		Schedule(6.f, [this]() { Quit(); });
		return;
	}
	Schedule(1.f, [PC]() { if (PC) { PC->ConsoleCommand(TEXT("DisableAllScreenMessages")); } });
	Schedule(3.f, [this, Log]() { Log(TEXT("new game")); Shot(TEXT("01_Neues_Spiel")); });
	// Each tutorial step: an action (a few timed parts), then wait until the step counter moves on.
	// (Until: the runner waits until it returns true before the next part - e.g. the courier.)
	struct FAct { float Delay; TFunction<void()> Do; TFunction<bool()> Until; };
	auto Aim = [this](const AActor* Target)
	{
		LookAt(Target);
		if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->UpdateFocus(); C->GetInteractionComponent()->RequestPrimaryInteraction(); }
	};
	auto AimBench = [this]()
	{
		if (AWEWorkbench* B = FindBench(); B && GetCharacter())
		{
			LookAtPoint(B->GetWorkSpotLocation());
			GetCharacter()->GetInteractionComponent()->UpdateFocus();
			GetCharacter()->GetInteractionComponent()->RequestPrimaryInteraction();
		}
	};
	TSharedRef<TArray<TArray<FAct>>> Steps = MakeShared<TArray<TArray<FAct>>>();
	// 0 walk to the bench
	Steps->Add({ { 0.f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(250.f, 0.f, 100.f), FRotator(0.f, 180.f, 0.f)); } } },
		{ 0.8f, [this]() { TeleportToBench(); } } });
	// 1 the screwdriver from the bench
	Steps->Add({ { 0.f, [this]() { TeleportToBench(); } }, { 0.8f, [this, Aim]() { Aim(FindItemById(TEXT("Screwdriver"))); } } });
	// 2 inventory
	Steps->Add({ { 0.f, [PC]() { if (PC) { PC->OpenInventory(); } } }, { 0.8f, [PC]() { if (PC) { PC->CloseInventory(); } } } });
	// 3 the old computer onto the bench
	Steps->Add({ { 0.f, [this, PC]() { if (const AWEPickupItem* Pc = FindItemById(TEXT("OldComputer")); Pc && PC) { PC->ServerDevTeleport(Pc->GetActorLocation() + FVector(110.f, 0.f, 80.f), FRotator(0.f, 180.f, 0.f)); } } },
		{ 0.8f, [this, Aim]() { Aim(FindItemById(TEXT("OldComputer"))); } },
		{ 1.0f, [this]() { if (AWEWorkbench* B = FindBench(); B && Cast<AWEPlayerController>(GetOwner())) { Cast<AWEPlayerController>(GetOwner())->ServerDevTeleport(B->GetWorkSpotLocation() + B->GetActorForwardVector() * 110.f + FVector(0.f, 0.f, 10.f), FRotator::ZeroRotator); } } },
		{ 0.8f, [AimBench]() { AimBench(); } } });
	// 4 work view
	Steps->Add({ { 0.f, [AimBench]() { AimBench(); } } });
	// 5 one disassembly step (the first part the screwdriver can take off)
	Steps->Add({ { 0.f, [this]() {
		AWEWorkbench* B = FindBench();
		AWECharacter* C = GetCharacter();
		const UWEAssemblyDefinition* Asm = B ? B->GetAssembly() : nullptr;
		for (int32 Part = 0; Asm && C && Part < Asm->Parts.Num(); ++Part)
		{
			FText Reason;
			if (B->CanStartStep(C, Reason, Part) && B->TryStartStep(C, Part))
			{
				UE_LOG(LogWorkshopEmpire, Display, TEXT("QU T taking off %s"), *Asm->Parts[Part].DisplayName.ToString());
				return;
			}
		}
		UE_LOG(LogWorkshopEmpire, Display, TEXT("QU T NO PART could be taken off"));
	} } });
	// 6 the workshop PC
	Steps->Add({ { 0.f, [PC]() { if (PC) { PC->CloseWorkView(); } } }, { 0.5f, [this]() { TeleportToTerminal(0.f); } },
		{ 0.8f, [this, PC]() { LookAt(FindTerminal()); if (PC) { PC->OpenTrade(FindTerminal()); } } } });
	// 7 sell the part that came off (it lies in the bench's parts storage; the market lists that too)
	Steps->Add({ { 0.f, [this, PC]() {
		UWEInventoryComponent* Parts = FindBench() ? FindBench()->GetStorage() : nullptr;
		for (const FWEInventorySlotData& Slot : Parts ? Parts->GetAllSlots() : TArray<FWEInventorySlotData>())
		{
			if (PC && Slot.Item.Definition && Slot.Item.Definition->ToolType == EWEToolType::None)
			{
				UE_LOG(LogWorkshopEmpire, Display, TEXT("QU T selling %s from the bench"), *Slot.Item.GetDisplayName().ToString());
				PC->RequestSell(FindTerminal(), Parts, Slot.SlotIndex, 0);
			}
		}
		SellAllParts();
	} } });
	// 8 accept an order
	Steps->Add({ { 0.f, [this, PC]() {
		for (const FWEContract& K : UWEContractComponent::Get(this) ? UWEContractComponent::Get(this)->GetContracts() : TArray<FWEContract>())
		{
			if (K.State == EWEContractState::Offered && PC)
			{
				PC->RequestAcceptContract(FindTerminal(), K.Id);
				UE_LOG(LogWorkshopEmpire, Display, TEXT("QU T accepting order %s"), *K.Title.ToString());
				return;
			}
		}
		UE_LOG(LogWorkshopEmpire, Display, TEXT("QU T no order offered"));
	} } });
	// 9 buy a multimeter, wait for the courier, open the parcel, take the multimeter, check the computer on the bench with it
	auto FindParcel = [this]() -> AWEPickupItem*
	{
		for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It)
		{
			if (UWECourierComponent::IsParcel(It->GetItem()) && !It->IsActorBeingDestroyed()) { return *It; }
		}
		return nullptr;
	};
	Steps->Add({ { 0.f, [this, PC]() { const int32 Offer = FindOfferIndex(TEXT("Multimeter")); UE_LOG(LogWorkshopEmpire, Display, TEXT("QU T buying multimeter (offer %d)"), Offer);
			if (PC && Offer != INDEX_NONE) { PC->RequestBuy(FindTerminal(), Offer, 1); } } },
		{ 1.0f, [PC]() { if (PC) { PC->CloseTrade(); } } },
		{ 0.3f, []() {}, [this, FindParcel]() { return FindParcel() != nullptr || FindMySlot(TEXT("Multimeter")) != INDEX_NONE; } },
		{ 0.5f, [this, PC, FindParcel]() { if (AWEPickupItem* P = FindParcel(); P && PC) { UE_LOG(LogWorkshopEmpire, Display, TEXT("QU T parcel arrived")); PC->ServerDevTeleport(P->GetActorLocation() + FVector(-90.f, 0.f, 80.f), FRotator::ZeroRotator); } } },
		{ 0.8f, [this, FindParcel]() { if (AWEPickupItem* P = FindParcel(); P && GetCharacter()) { LookAt(P); GetCharacter()->GetInteractionComponent()->UpdateFocus(); GetCharacter()->GetInteractionComponent()->RequestAlternativeInteraction(); } } },
		{ 1.0f, [this, Aim]() { if (FindMySlot(TEXT("Multimeter")) == INDEX_NONE) { Aim(FindItemById(TEXT("Multimeter"))); } } },
		{ 1.0f, [this]() { UE_LOG(LogWorkshopEmpire, Display, TEXT("QU T multimeter in the inventory: %d"), FindMySlot(TEXT("Multimeter")) != INDEX_NONE ? 1 : 0); } },
		{ 0.3f, [this]() { if (AWEWorkbench* B = FindBench(); B && Cast<AWEPlayerController>(GetOwner())) { Cast<AWEPlayerController>(GetOwner())->ServerDevTeleport(B->GetWorkSpotLocation() + B->GetActorForwardVector() * 110.f + FVector(0.f, 0.f, 10.f), FRotator::ZeroRotator); } } },
		{ 0.8f, [this]() { FText Reason; AWEWorkbench* B = FindBench(); const bool bOk = B && GetCharacter() && B->TryStartRepairStep(GetCharacter());
			if (B) { B->GetNextRepairStep(Reason); } UE_LOG(LogWorkshopEmpire, Display, TEXT("QU T check with multimeter: %s %s"), bOk ? TEXT("started") : TEXT("REFUSED"), *Reason.ToString()); } } });
	// 10 save
	Steps->Add({ { 0.f, [PC]() { if (PC) { PC->RequestSaveCurrent(); } } } });
	TSharedRef<int32> Index = MakeShared<int32>(0);
	TSharedRef<int32> Part = MakeShared<int32>(0);
	TSharedRef<double> Waiting = MakeShared<double>(0.0);
	TSharedRef<double> NextAt = MakeShared<double>(0.0);
	TSharedRef<int32> StartStep = MakeShared<int32>(-1);
	Schedule(4.f, [this, Steps, Index, Part, Waiting, NextAt, StartStep, Log, Snapshot, Session]()
	{
		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Steps, Index, Part, Waiting, NextAt, StartStep, Log, Snapshot, Session]()
		{
			const UWETutorialComponent* Tutorial = UWETutorialComponent::Get(this);
			const double Now = GetWorld()->GetTimeSeconds();
			if (!Tutorial || !Steps->IsValidIndex(*Index))
			{
				return;
			}
			if (*StartStep < 0)
			{
				*StartStep = Tutorial->GetStep();
				*Part = 0;
				*NextAt = Now;
				*Waiting = Now;
			}
			const TArray<FAct>& Acts = (*Steps)[*Index];
			if (Acts.IsValidIndex(*Part) && Now >= *NextAt)
			{
				if (*Part > 0 && Acts[*Part - 1].Until && !Acts[*Part - 1].Until() && Now - *Waiting < 240.0)
				{
					return; // still waiting for the previous part's condition
				}
				Acts[*Part].Do();
				++*Part;
				*NextAt = Now + (Acts.IsValidIndex(*Part) ? Acts[*Part].Delay : 0.f);
				return;
			}
			if (Acts.Num() > 0 && Acts.Last().Until && !Acts.Last().Until() && Now - *Waiting < 240.0)
			{
				return;
			}
			const bool bAdvanced = Tutorial->GetStep() > *StartStep || !Tutorial->IsActive();
			// (The courier takes a few minutes in game time: the multimeter step may wait longer.)
			if (bAdvanced || Now - *Waiting > (*Index == 9 ? 240.0 : 25.0))
			{
				Log(FString::Printf(TEXT("step %d %s"), *Index, bAdvanced ? TEXT("done") : TEXT("NOT DONE (timeout)")));
				Shot(FString::Printf(TEXT("%02d_Schritt_%d"), *Index + 2, *Index));
				++*Index;
				*StartStep = -1;
				if (!Steps->IsValidIndex(*Index))
				{
					Saved = Snapshot();
					Log(TEXT("all steps, saved"));
					FTimerHandle Leave;
					GetWorld()->GetTimerManager().SetTimer(Leave, FTimerDelegate::CreateWeakLambda(this, [Session]() { if (Session) { Session->LeaveToMainMenu(); } }), 2.f, false);
				}
			}
		}), 0.25f, true);
	});
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
	if (ShotName == TEXT("QUT"))
	{
		RunQualityNewGameFlow();
		return;
	}
	if (ShotName == TEXT("QUK"))
	{
		// --- Mike's list: radio into the hands, customer device never in the pocket, hand-in hints, night skip, fast-forward ---
		auto Inv = [this](const TCHAR* Id) { const int32 Slot = FindMySlot(Id); return Slot == INDEX_NONE ? 0 : 1; };
		auto Held = [this]() { return GetCharacter() && GetCharacter()->GetHeldItem() ? GetCharacter()->GetHeldItem()->GetDisplayName().ToString() : FString(TEXT("-")); };
		auto Press = [this](bool bSecondary)
		{
			if (AWECharacter* C = GetCharacter())
			{
				C->GetInteractionComponent()->UpdateFocus();
				bSecondary ? C->GetInteractionComponent()->RequestSecondaryInteraction() : C->GetInteractionComponent()->RequestPrimaryInteraction();
			}
		};
		TSharedRef<TWeakObjectPtr<AWEPickupItem>> Radio = MakeShared<TWeakObjectPtr<AWEPickupItem>>();
		TSharedRef<FGuid> OrderId = MakeShared<FGuid>();
		Schedule(1.0f, [PC]() { if (PC) { PC->ConsoleCommand(TEXT("DisableAllScreenMessages")); } });
		Schedule(2.0f, [this, PC, Radio]()
		{
			const FWEItemInstance Own = FWEItemInstance::Make(UWEItemDefinition::FindById(TEXT("PortableRadio")), 1, EWEItemCondition::Defective, 35);
			*Radio = AWEPickupItem::SpawnWorldItem(GetWorld(), Own, FTransform(FVector(120.f, -150.f, 30.f)));
			if (PC) { PC->ServerDevTeleport(FVector(0.f, -150.f, 100.f), FRotator::ZeroRotator); }
		});
		Schedule(3.0f, [this, Radio]() { LookAt(Radio->Get()); });
		Schedule(3.3f, [Press]() { Press(false); });
		Schedule(4.0f, [this, Held, Inv]() { UE_LOG(LogWorkshopEmpire, Display, TEXT("QU K own radio [E]: held=%s, in inventory=%d (expect held)"), *Held(), Inv(TEXT("PortableRadio"))); Shot(TEXT("01_Radio_in_der_Hand")); });
		Schedule(4.5f, [this]() { if (AWECharacter* C = GetCharacter()) { C->DropHeldItem(); } });
		Schedule(5.5f, [this, Radio]() { LookAt(Radio->Get()); });
		Schedule(5.8f, [Press]() { Press(true); });
		Schedule(6.5f, [Held, Inv]() { UE_LOG(LogWorkshopEmpire, Display, TEXT("QU K own radio [R]: held=%s, in inventory=%d (expect pocketed)"), *Held(), Inv(TEXT("PortableRadio"))); });
		// A customer's radio.
		Schedule(7.0f, [this, PC]()
		{
			const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
			const UWEContractCatalog* Catalog = Contracts ? Contracts->GetCatalog() : nullptr;
			for (int32 i = 0; Catalog && i < Catalog->Templates.Num(); ++i)
			{
				if (Catalog->Templates[i].Kind == EWEContractKind::Repair && Catalog->Templates[i].Item && Catalog->Templates[i].Item->ItemId == TEXT("PortableRadio"))
				{
					if (PC) { PC->ServerDevAddContractOffer(i); }
					break;
				}
			}
		});
		Schedule(8.0f, [this, PC, OrderId]()
		{
			for (const FWEContract& K : UWEContractComponent::Get(this) ? UWEContractComponent::Get(this)->GetContracts() : TArray<FWEContract>())
			{
				if (K.Kind == EWEContractKind::Repair && K.State == EWEContractState::Offered && K.Item && K.Item->ItemId == TEXT("PortableRadio")) { *OrderId = K.Id; }
			}
			if (PC && OrderId->IsValid()) { PC->RequestAcceptContract(FindTerminal(), *OrderId); }
		});
		Schedule(10.0f, [this, PC]()
		{
			AWEPickupItem* Device = FindCustomerDevice();
			int32 Added = 0;
			const EWEInventoryResult Result = Device && GetMyInventory() ? GetMyInventory()->TryAddItem(Device->GetItem(), Added) : EWEInventoryResult::InvalidItem;
			UE_LOG(LogWorkshopEmpire, Display, TEXT("QU K customer radio straight into the pocket: result %d, added %d (expect refused)"), static_cast<int32>(Result), Added);
			if (Device && PC) { PC->ServerDevTeleport(Device->GetActorLocation() + FVector(-110.f, 0.f, 80.f), FRotator::ZeroRotator); }
		});
		Schedule(11.0f, [this]() { LookAt(FindCustomerDevice()); });
		Schedule(11.3f, [Press]() { Press(false); });
		Schedule(12.0f, [this, Held, OrderId]()
		{
			const FWEContract* K = UWEContractComponent::Get(this) ? UWEContractComponent::Get(this)->FindContract(*OrderId) : nullptr;
			UE_LOG(LogWorkshopEmpire, Display, TEXT("QU K customer radio [E]: held=%s; hint '%s'"), *Held(), K ? *UWEContractComponent::GetDeliveryHint(*K, GetCharacter()).ToString() : TEXT("-"));
			// As if repaired at the bench, but the function test not done yet.
			if (AWEPickupItem* Device = GetCharacter() ? GetCharacter()->GetHeldItem() : nullptr)
			{
				FWEItemInstance Item = Device->GetItem();
				Item.Repair.Faults = 0;
				Item.Condition = EWEItemCondition::Used;
				Item.Repair.SetFlag(FWERepairState::NeedsTest);
				Device->SetItem(Item);
			}
			UE_LOG(LogWorkshopEmpire, Display, TEXT("QU K repaired, untested: hint '%s'"), K ? *UWEContractComponent::GetDeliveryHint(*K, GetCharacter()).ToString() : TEXT("-"));
			if (AWEPickupItem* Device = GetCharacter() ? GetCharacter()->GetHeldItem() : nullptr)
			{
				FWEItemInstance Item = Device->GetItem();
				Item.Repair.Flags &= ~FWERepairState::NeedsTest;
				Device->SetItem(Item);
			}
			UE_LOG(LogWorkshopEmpire, Display, TEXT("QU K tested: hint '%s', deliverable %d"), K ? *UWEContractComponent::GetDeliveryHint(*K, GetCharacter()).ToString() : TEXT("-"),
				K ? UWEContractComponent::CountDeliverable(*K, GetCharacter()) : -1);
			TeleportToTerminal(0.f);
		});
		// Like a player: the radio in the hands, [E] on the workshop PC (that used to put the radio down instead).
		Schedule(13.0f, [this]() { LookAt(FindTerminal()); });
		Schedule(13.2f, [Press]() { Press(false); });
		Schedule(13.6f, [this, PC, Held]()
		{
			UE_LOG(LogWorkshopEmpire, Display, TEXT("QU K [E] on the PC while carrying: PC open=%d, held=%s (expect open, still held)"),
				PC && PC->GetTradeWidget() && PC->GetTradeWidget()->IsInViewport() ? 1 : 0, *Held());
		});
		// Mike: hand it in even when it lies somewhere in the garage - put it down on the floor first.
		Schedule(14.0f, [this, PC, OrderId, Held]()
		{
			if (PC) { PC->CloseTrade(); }
			if (AWECharacter* C = GetCharacter()) { C->DropHeldItem(); }
			const FWEContract* K = UWEContractComponent::Get(this) ? UWEContractComponent::Get(this)->FindContract(*OrderId) : nullptr;
			UE_LOG(LogWorkshopEmpire, Display, TEXT("QU K radio put down: held=%s, deliverable %d (expect 1 from the floor)"), *Held(), K ? UWEContractComponent::CountDeliverable(*K, GetCharacter()) : -1);
		});
		Schedule(14.6f, [this, PC]() { if (PC) { PC->OpenTrade(FindTerminal()); } });
		Schedule(15.0f, [PC]() { if (PC && PC->GetTradeWidget()) { PC->GetTradeWidget()->ShowPage(4); } });
		Schedule(15.6f, [this]() { Shot(TEXT("02_Auftrag_Abliefern")); });
		Schedule(16.0f, [this, PC, OrderId]()
		{
			const int64 Before = UWECompanyComponent::Get(this) ? UWECompanyComponent::Get(this)->GetBalance().GetCents() : 0;
			if (PC) { PC->RequestDeliverContract(FindTerminal(), *OrderId); }
			FTimerHandle H;
			GetWorld()->GetTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [this, Before, OrderId]()
			{
				const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
				UE_LOG(LogWorkshopEmpire, Display, TEXT("QU K delivered: order open=%d, balance %lld -> %lld"), Contracts && Contracts->FindContract(*OrderId) ? 1 : 0, Before,
					UWECompanyComponent::Get(this) ? UWECompanyComponent::Get(this)->GetBalance().GetCents() : -1);
			}), 1.f, false);
		});
		// Night skip and fast-forward.
		Schedule(16.5f, [this, PC]()
		{
			if (PC) { PC->CloseTrade(); }
			UWEDayNightComponent* Clock = UWEDayNightComponent::Get(this);
			if (Clock) { Clock->SetTime(Clock->GetDay(), 14.f * 60.f); }
			if (PC) { PC->RequestSkipNight(); }
		});
		Schedule(17.0f, [this, PC]()
		{
			UWEDayNightComponent* Clock = UWEDayNightComponent::Get(this);
			UE_LOG(LogWorkshopEmpire, Display, TEXT("QU K skip at 14:00 (refused): day %d %s"), Clock ? Clock->GetDay() : -1, Clock ? *Clock->GetClockText().ToString() : TEXT("-"));
			if (Clock) { Clock->SetTime(Clock->GetDay(), 21.f * 60.f); }
			if (PC) { PC->OpenPauseMenu(); }
		});
		Schedule(18.0f, [this]() { Shot(TEXT("03_Pausenmenue_Nacht")); });
		Schedule(18.3f, [this, PC]()
		{
			const UWEDayNightComponent* Clock = UWEDayNightComponent::Get(this);
			UE_LOG(LogWorkshopEmpire, Display, TEXT("QU K before skip: day %d %s"), Clock ? Clock->GetDay() : -1, Clock ? *Clock->GetClockText().ToString() : TEXT("-"));
			if (PC) { PC->ClosePauseMenu(); PC->RequestSkipNight(); }
		});
		Schedule(19.0f, [this, PC]()
		{
			const UWEDayNightComponent* Clock = UWEDayNightComponent::Get(this);
			UE_LOG(LogWorkshopEmpire, Display, TEXT("QU K after skip: day %d %s"), Clock ? Clock->GetDay() : -1, Clock ? *Clock->GetClockText().ToString() : TEXT("-"));
			if (PC) { PC->RequestTimeSpeed(4.f); }
		});
		Schedule(19.5f, [this]()
		{
			const UWEDayNightComponent* Clock = UWEDayNightComponent::Get(this);
			UE_LOG(LogWorkshopEmpire, Display, TEXT("QU K speed: dilation %.1f, clock %s"), GetWorld()->GetWorldSettings()->TimeDilation, Clock ? *Clock->GetClockText().ToString() : TEXT("-"));
			Shot(TEXT("04_Zeitraffer_HUD"));
		});
		// (Timers run in game time: 8 s at 4x are 2 real seconds.)
		Schedule(27.5f, [this, PC]()
		{
			const UWEDayNightComponent* Clock = UWEDayNightComponent::Get(this);
			UE_LOG(LogWorkshopEmpire, Display, TEXT("QU K 8 game seconds later at 4x: clock %s"), Clock ? *Clock->GetClockText().ToString() : TEXT("-"));
			if (PC) { PC->RequestTimeSpeed(1.f); }
		});
		Schedule(28.5f, [this]() { UE_LOG(LogWorkshopEmpire, Display, TEXT("QU K speed back: dilation %.1f"), GetWorld()->GetWorldSettings()->TimeDilation); Quit(); });
		return;
	}
	if (ShotName.StartsWith(TEXT("QUE")))
	{
		// --- Audit views of the garage (fixed cameras): fuse box, PC desk lamp, bench, walls, door, yard -----------------
		struct FView { FVector From; FVector At; const TCHAR* Name; };
		TArray<FView> Views = {
			{ FVector(-180.f, 205.f, 165.f), FVector(-398.f, 205.f, 150.f), TEXT("01_Sicherungskasten") },
			{ FVector(-300.f, 60.f, 190.f), FVector(-398.f, 205.f, 150.f), TEXT("02_Sicherungskasten_seitlich") },
			{ FVector(-200.f, -60.f, 175.f), FVector(-355.f, -40.f, 95.f), TEXT("04_Werkbank") },
			{ FVector(250.f, 0.f, 200.f), FVector(-400.f, 0.f, 120.f), TEXT("05_Garage_Rueckwand") },
			{ FVector(-300.f, 250.f, 210.f), FVector(400.f, -200.f, 100.f), TEXT("06_Garage_zum_Tor") },
			{ FVector(-300.f, -250.f, 210.f), FVector(300.f, 280.f, 100.f), TEXT("07_Garage_Ecke") },
			{ FVector(0.f, 0.f, 280.f), FVector(-200.f, 0.f, 0.f), TEXT("08_Garage_oben") },
			{ FVector(900.f, 600.f, 220.f), FVector(400.f, 0.f, 120.f), TEXT("09_Hof_Tor") },
			{ FVector(-5000.f, -1850.f, 260.f), FVector(-7150.f, -1850.f, 40.f), TEXT("10_Erdhaufen") },
			{ FVector(2500.f, -2300.f, 180.f), FVector(-1000.f, -1850.f, 120.f), TEXT("11_Strasse") },
		};
		Schedule(1.0f, [PC]() { if (PC) { PC->ConsoleCommand(TEXT("DisableAllScreenMessages")); } });
		// The PC desk: from its front, wherever the terminal stands.
		Schedule(1.5f, [this]()
		{
			if (const AWETraderTerminal* T = FindTerminal())
			{
				UE_LOG(LogWorkshopEmpire, Display, TEXT("QU terminal at %s yaw %.0f"), *T->GetActorLocation().ToCompactString(), T->GetActorRotation().Yaw);
			}
		});
		float At = 2.5f;
		auto Cam = [this, PC](const FVector& From, const FVector& Target)
		{
			if (!CatCamera.IsValid())
			{
				FActorSpawnParameters Params;
				Params.ObjectFlags |= RF_Transient;
				CatCamera = GetWorld()->SpawnActor<ACameraActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
				CatCamera->GetCameraComponent()->SetFieldOfView(75.f);
				CatCamera->GetCameraComponent()->bConstrainAspectRatio = false;
			}
			CatCamera->SetActorLocationAndRotation(From, (Target - From).Rotation());
			if (PC) { PC->SetViewTarget(CatCamera.Get()); }
		};
		for (const FView& V : Views)
		{
			Schedule(At, [Cam, V]() { Cam(V.From, V.At); });
			Schedule(At + 0.9f, [this, V]() { Shot(V.Name); });
			At += 1.5f;
		}
		Schedule(At, [this, Cam]()
		{
			if (const AWETraderTerminal* T = FindTerminal())
			{
				const FTransform X = T->GetActorTransform();
				Cam(X.TransformPosition(FVector(120.f, -40.f, 150.f)), X.TransformPosition(FVector(-28.f, -72.f, 100.f)));
			}
		});
		Schedule(At + 0.9f, [this]() { Shot(TEXT("03_PC_Lampe")); });
		Schedule(At + 1.5f, [this, Cam]()
		{
			if (const AWETraderTerminal* T = FindTerminal())
			{
				const FTransform X = T->GetActorTransform();
				Cam(X.TransformPosition(FVector(-20.f, -170.f, 130.f)), X.TransformPosition(FVector(-28.f, -72.f, 100.f)));
			}
		});
		Schedule(At + 2.4f, [this]() { Shot(TEXT("03b_PC_Lampe_seitlich")); });
		Schedule(At + 3.5f, [this]() { Quit(); });
		return;
	}
	if (ShotName == TEXT("QUD"))
	{
		// --- The parked cars where they stand in the map: several views, eye height and from above, plus the frame rate ---
		TSharedRef<TArray<FTransform>> Cars = MakeShared<TArray<FTransform>>();
		Schedule(1.0f, [this, PC, Cars]()
		{
			if (PC) { PC->ConsoleCommand(TEXT("DisableAllScreenMessages")); }
			int32 Count = 0;
			for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
			{
				const UStaticMesh* Mesh = It->GetStaticMeshComponent()->GetStaticMesh();
				if (Mesh && Mesh->GetName().StartsWith(TEXT("SM_Gen_Car_")))
				{
					Cars->Add(It->GetActorTransform());
					++Count;
					const FBox Box = It->GetComponentsBoundingBox();
					if (Count <= 30)
					{
						UE_LOG(LogWorkshopEmpire, Display, TEXT("QU parked %s at %s yaw %.0f, bottom z %.1f"), *Mesh->GetName(), *It->GetActorLocation().ToCompactString(), It->GetActorRotation().Yaw, Box.Min.Z);
					}
				}
			}
			UE_LOG(LogWorkshopEmpire, Display, TEXT("QU parked cars: %d"), Count);
		});
		auto ViewAt = [this, PC, Cars](int32 Index, const FVector& Offset, const FString& Name, float Delay)
		{
			Schedule(Delay, [this, PC, Cars, Index, Offset]()
			{
				if (!Cars->IsValidIndex(Index)) { return; }
				const FTransform& T = (*Cars)[Index];
				if (!CatCamera.IsValid())
				{
					FActorSpawnParameters Params;
					Params.ObjectFlags |= RF_Transient;
					CatCamera = GetWorld()->SpawnActor<ACameraActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
					CatCamera->GetCameraComponent()->SetFieldOfView(70.f);
					CatCamera->GetCameraComponent()->bConstrainAspectRatio = false;
				}
				const FVector At = T.GetLocation() + FVector(0.f, 0.f, 70.f);
				const FVector From = T.GetLocation() + T.GetRotation().RotateVector(Offset);
				CatCamera->SetActorLocationAndRotation(From, (At - From).Rotation());
				if (PC) { PC->SetViewTarget(CatCamera.Get()); }
			});
			Schedule(Delay + 1.0f, [this, Name]() { Shot(Name); });
		};
		ViewAt(0, FVector(520.f, 420.f, 160.f), TEXT("01_Vorne_schraeg"), 2.0f);
		ViewAt(0, FVector(-560.f, -380.f, 150.f), TEXT("02_Hinten_schraeg"), 4.0f);
		ViewAt(3, FVector(0.f, 650.f, 165.f), TEXT("03_Seite_Augenhoehe"), 6.0f);
		ViewAt(6, FVector(900.f, 900.f, 700.f), TEXT("04_Von_oben"), 8.0f);
		ViewAt(10, FVector(2200.f, 300.f, 170.f), TEXT("05_Strasse_lang"), 10.0f);
		ViewAt(15, FVector(450.f, -450.f, 170.f), TEXT("06_Weiterer"), 12.0f);
		// Frame rate looking down the street with many cars in view.
		TSharedRef<uint64> Frames = MakeShared<uint64>(0);
		TSharedRef<double> Start = MakeShared<double>(0.0);
		Schedule(10.5f, [Frames, Start]() { *Frames = GFrameCounter; *Start = FPlatformTime::Seconds(); });
		Schedule(11.5f, [this, Frames, Start]()
		{
			UE_LOG(LogWorkshopEmpire, Display, TEXT("PERF street with cars: %.1f fps, GPU %.2f ms"), (GFrameCounter - *Frames) / FMath::Max(FPlatformTime::Seconds() - *Start, 0.001),
				FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));
			// The same view without the cars: what they cost.
			for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
			{
				const UStaticMesh* Mesh = It->GetStaticMeshComponent()->GetStaticMesh();
				if (Mesh && Mesh->GetName().StartsWith(TEXT("SM_Gen_Car_"))) { It->SetActorHiddenInGame(true); }
			}
		});
		Schedule(12.5f, [Frames, Start]() { *Frames = GFrameCounter; *Start = FPlatformTime::Seconds(); });
		// Where the GPU time of the street view goes (-WEProfileGPU: ProfileGPU dump into the log, cars hidden).
		Schedule(13.6f, [PC]() { if (PC && FParse::Param(FCommandLine::Get(), TEXT("WEProfileGPU"))) { PC->ConsoleCommand(TEXT("ProfileGPU")); } });
		Schedule(13.5f, [Frames, Start]()
		{
			UE_LOG(LogWorkshopEmpire, Display, TEXT("PERF street without cars: %.1f fps, GPU %.2f ms"), (GFrameCounter - *Frames) / FMath::Max(FPlatformTime::Seconds() - *Start, 0.001),
				FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));
		});
		Schedule(14.5f, [this]() { Quit(); });
		return;
	}
	if (ShotName == TEXT("QUC") || ShotName == TEXT("QUM"))
	{
		// --- The new parked cars (QUM: the machine models): side by side on the street, then each one close up ----------
		static const TCHAR* CarList[] = { TEXT("Car_SedanOld"), TEXT("Car_Wagon"), TEXT("Car_Pickup"), TEXT("Car_PickupOld"), TEXT("Car_Suv"), TEXT("Car_Hatch") };
		static const TCHAR* MachineList[] = { TEXT("M_Shredder"), TEXT("M_Sorter"), TEXT("M_Press"), TEXT("M_Assembler"), TEXT("M_Furnace"), TEXT("M_Refinery") };
		const TCHAR** Cars = ShotName == TEXT("QUM") ? MachineList : CarList;
		Schedule(1.0f, [this, PC, Cars]()
		{
			if (PC) { PC->ConsoleCommand(TEXT("DisableAllScreenMessages")); }
			for (int32 i = 0; i < 6; ++i)
			{
				UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_%s.SM_Gen_%s"), Cars[i], Cars[i]));
				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(-1500.f + i * 650.f, -1850.f, 2.f), FRotator(0.f, 15.f, 0.f), Params);
				if (A && Mesh)
				{
					A->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
					A->GetStaticMeshComponent()->SetStaticMesh(Mesh);
				}
				UE_LOG(LogWorkshopEmpire, Display, TEXT("QU car %s: %s"), Cars[i], Mesh ? TEXT("ok") : TEXT("MISSING"));
			}
		});
		auto View = [this, PC](const FVector& From, const FVector& At, const FString& Name, float Delay)
		{
			Schedule(Delay, [this, PC, From, At, Name]()
			{
				if (!CatCamera.IsValid())
				{
					FActorSpawnParameters Params;
					Params.ObjectFlags |= RF_Transient;
					CatCamera = GetWorld()->SpawnActor<ACameraActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
					CatCamera->GetCameraComponent()->SetFieldOfView(55.f);
					CatCamera->GetCameraComponent()->bConstrainAspectRatio = false;
				}
				CatCamera->SetActorLocationAndRotation(From, (At - From).Rotation());
				if (PC) { PC->SetViewTarget(CatCamera.Get()); }
			});
			Schedule(Delay + 0.8f, [this, Name]() { Shot(Name); });
		};
		View(FVector(125.f, -850.f, 260.f), FVector(125.f, -1850.f, 60.f), TEXT("01_Reihe"), 2.5f);
		for (int32 i = 0; i < 6; ++i)
		{
			const FVector C(-1500.f + i * 650.f, -1850.f, 70.f);
			View(C + FVector(420.f, 380.f, 90.f), C, FString::Printf(TEXT("%02d_%s"), i + 2, Cars[i]), 4.f + i * 1.6f);
		}
		View(FVector(-200.f - 480.f, -1850.f - 380.f, 320.f), FVector(-200.f, -1850.f, 60.f), TEXT("08_Pickup_Ladeflaeche"), 4.f + 6 * 1.6f);
		Schedule(16.f, [this]() { Quit(); });
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
