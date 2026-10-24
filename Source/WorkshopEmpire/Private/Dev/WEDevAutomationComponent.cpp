#include "Dev/WEDevAutomationComponent.h"

#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/WEInteractionComponent.h"
#include "Items/WEPickupItem.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Player/WECharacter.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "WELog.h"
#include "World/WEGarageDoor.h"
#include "Framework/WEPlayerController.h"
#include "Framework/WEPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Inventory/WEInventoryComponent.h"
#include "World/WEPlaceholderProp.h"
#include "Workshop/WEWorkbench.h"
#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WETraderTerminal.h"
#include "UObject/UObjectIterator.h"
#include "Components/LightComponent.h"
#include "Engine/Light.h"
#include "Items/WEItemDefinition.h"
#include "Workshop/WERepairDefinition.h"
#include "Economy/WEAuctionComponent.h"
#include "UI/WETradeWidget.h"
#include "World/WEScrapPile.h"
#include "Workshop/WEBuildComponent.h"
#include "Workshop/WEFurnitureDefinition.h"
#include "Workshop/WEWorkshopLayoutComponent.h"
#include "Save/WESaveSystem.h"
#include "HAL/FileManager.h"
#include "UI/WEInventoryWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "InputKeyEventArgs.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "Progression/WEMilestoneComponent.h"
#include "Progression/WEProgressionComponent.h"
#include "Machines/WEMachine.h"
#include "Machines/WEMachineDefinition.h"
#include "Machines/WEPowerGridComponent.h"
#include "Machines/WEConveyor.h"
#include "Machines/WEProductionStorage.h"
#include "Staff/WEEmployee.h"
#include "Staff/WEStaffComponent.h"
#include "Logistics/WEVehicle.h"
#include "Logistics/WEForklift.h"
#include "World/WEGarageBuilding.h"
#include "Fx/WEFx.h"
#include "Engine/GameViewportClient.h"
#include "RHI.h"
#include "HAL/PlatformTime.h"
#include "Framework/WEMenuGameMode.h"
#include "Framework/WEGameMode.h"
#include "Online/WESessionSubsystem.h"
#include "UI/WEMenuWidgets.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "UI/WETradeWidget.h"

UWEDevAutomationComponent::UWEDevAutomationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UWEDevAutomationComponent::IsRequested()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	FString Value;
	return FParse::Value(FCommandLine::Get(), TEXT("WESmokeRole="), Value) || FParse::Value(FCommandLine::Get(), TEXT("WEShots="), Value);
#endif
}

void UWEDevAutomationComponent::BeginPlay()
{
	Super::BeginPlay();

#if !UE_BUILD_SHIPPING
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	FParse::Value(FCommandLine::Get(), TEXT("WESmokeRole="), Role);
	FParse::Value(FCommandLine::Get(), TEXT("WEShots="), ShotName);
	// Phase 17.5: the older scripts press [E] at the bench for timed steps; only the P175 runs use the work view.
	if ((!Role.IsEmpty() || !ShotName.IsEmpty()) && !ShotName.StartsWith(TEXT("P175")) && !ShotName.StartsWith(TEXT("P18R")) && !ShotName.StartsWith(TEXT("P19")) && !ShotName.StartsWith(TEXT("P20")) && !ShotName.StartsWith(TEXT("P21")) && !ShotName.StartsWith(TEXT("QU")))
	{
		if (IConsoleVariable* WorkView = IConsoleManager::Get().FindConsoleVariable(TEXT("we.WorkView")))
		{
			WorkView->Set(0);
		}
	}
	// Phase 17: headless test clients would run at hundreds of fps and overflow the movement prediction
	// ("Hit limit of 96 saved moves"); a real player's frame rate is far lower.
	if (!Role.IsEmpty() && (GUsingNullRHI || !FApp::CanEverRender()))
	{
		if (IConsoleVariable* MaxFps = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS")))
		{
			MaxFps->Set(120.f);
		}
	}

	if (Role == TEXT("A"))
	{
		RunSmokeRoleA();
	}
	else if (Role == TEXT("B"))
	{
		RunSmokeRoleB();
	}
	else if (Role == TEXT("C"))
	{
		RunSmokeRoleC();
	}
	else if (Role == TEXT("D"))
	{
		RunSmokeRoleD();
	}
	else if (Role == TEXT("E"))
	{
		RunSmokeRoleE();
	}
	else if (Role == TEXT("F"))
	{
		RunSmokeRoleF();
	}
	else if (Role == TEXT("G"))
	{
		RunSmokeRoleG();
	}
	else if (Role == TEXT("H"))
	{
		RunSmokeRoleH();
	}
	else if (Role == TEXT("I"))
	{
		RunSmokeRoleI();
	}
	else if (Role == TEXT("J"))
	{
		RunSmokeRoleJ();
	}
	else if (Role == TEXT("K"))
	{
		RunSmokeRoleK();
	}
	else if (Role == TEXT("L"))
	{
		RunSmokeRoleL();
	}
	else if (Role == TEXT("M"))
	{
		RunSmokeRoleM();
	}
	else if (Role == TEXT("N"))
	{
		RunSmokeRoleN();
	}
	else if (Role == TEXT("O"))
	{
		RunSmokeRoleO();
	}
	else if (Role == TEXT("P"))
	{
		RunSmokeRoleP();
	}
	else if (Role == TEXT("Q"))
	{
		RunSmokeRoleQ();
	}
	else if (Role == TEXT("R"))
	{
		RunSmokeRoleR();
	}
	else if (Role == TEXT("S"))
	{
		RunSmokeRoleS();
	}
	else if (Role == TEXT("T"))
	{
		RunSmokeRoleT();
	}
	else if (Role == TEXT("U"))
	{
		RunSmokeRoleU();
	}
	else if (Role == TEXT("V"))
	{
		RunSmokeRoleV();
	}
	else if (Role.StartsWith(TEXT("M")) && Role.Len() == 2)
	{
		RunCoopRole();
	}
	else if (Role == TEXT("SH") || Role == TEXT("SC"))
	{
		RunSteamRole();
	}
	else if (ShotName.StartsWith(TEXT("P18R")))
	{
		RunReassemblyShots();
	}
	else if (ShotName.StartsWith(TEXT("P21")))
	{
		RunPhase21Shots();
	}
	else if (ShotName.StartsWith(TEXT("QU")))
	{
		RunQualityShots();
	}
	else if (ShotName.StartsWith(TEXT("P20")))
	{
		RunPhase20Shots();
	}
	else if (ShotName.StartsWith(TEXT("P19")))
	{
		RunPhase19Shots();
	}
	else if (ShotName.StartsWith(TEXT("P175")))
	{
		RunPolishShots();
	}
	else if (ShotName.StartsWith(TEXT("P17UI")))
	{
		RunUiInputShots();
	}
	else if (ShotName.StartsWith(TEXT("P17")))
	{
		RunStressShots();
	}
	else if (ShotName.StartsWith(TEXT("P16")))
	{
		RunBalanceShots();
	}
	else if (ShotName.StartsWith(TEXT("P15")))
	{
		RunVisualShots();
	}
	else if (ShotName.StartsWith(TEXT("P13")))
	{
		RunCoopShots();
	}
	else if (Role == TEXT("W"))
	{
		RunSmokeRoleW();
	}
	else if (Role == TEXT("X"))
	{
		RunSmokeRoleX();
	}
	else if (Role == TEXT("Z"))
	{
		RunSmokeRoleZ();
	}
	else if (Role == TEXT("Y1") || Role == TEXT("Y2"))
	{
		RunSmokeRoleP21(Role == TEXT("Y1"));
	}
	else if (ShotName.StartsWith(TEXT("P12")))
	{
		RunExpansionShots();
	}
	else if (ShotName.StartsWith(TEXT("P11")))
	{
		RunStaffShots();
	}
	else if (ShotName.StartsWith(TEXT("P10")))
	{
		RunFlowShots();
	}
	else if (ShotName.StartsWith(TEXT("P9")))
	{
		RunMachineShots();
	}
	else if (ShotName.StartsWith(TEXT("P8")))
	{
		RunBuildShots();
	}
	else if (ShotName.StartsWith(TEXT("P7")))
	{
		RunProcurementShots();
	}
	else if (ShotName.StartsWith(TEXT("P6")))
	{
		RunRepairShots();
	}
	else if (ShotName.StartsWith(TEXT("P5")))
	{
		RunLoopShots();
	}
	else if (ShotName.StartsWith(TEXT("P4")))
	{
		RunEconomyShots();
	}
	else if (ShotName.StartsWith(TEXT("P3")))
	{
		RunWorkbenchShots();
	}
	else if (ShotName.StartsWith(TEXT("P2")))
	{
		RunInventoryShots();
	}
	else if (!ShotName.IsEmpty())
	{
		RunShots();
	}
#endif
}

void UWEDevAutomationComponent::Schedule(float Delay, TFunction<void()> Action)
{
	FTimerHandle& Handle = Timers.AddDefaulted_GetRef();
	// (Weak: the timer manager belongs to the game instance and outlives a map change - an action scheduled for the
	// old player controller must not run after it is gone; it crashed the P13 client back in the menu.)
	GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, MoveTemp(Action)), Delay, false);
}

AWECharacter* UWEDevAutomationComponent::GetCharacter() const
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	return PC ? Cast<AWECharacter>(PC->GetPawn()) : nullptr;
}

AActor* UWEDevAutomationComponent::FindItemByName(const FString& Substring) const
{
	for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It)
	{
		if (It->GetDisplayName().ToString().Contains(Substring))
		{
			return *It;
		}
	}
	return nullptr;
}

AActor* UWEDevAutomationComponent::FindDoor() const
{
	TActorIterator<AWEGarageDoor> It(GetWorld());
	return It ? *It : nullptr;
}

void UWEDevAutomationComponent::LogState(const TCHAR* Step) const
{
	const AWECharacter* Me = GetCharacter();
	const AWEPickupItem* Computer = Cast<AWEPickupItem>(FindItemByName(TEXT("Computer")));
	const AWEGarageDoor* Door = Cast<AWEGarageDoor>(FindDoor());

	UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] %s: Me=%s MyHeld=%s Computer.Holder=%s Computer.Loc=%s Door=%s"),
		*Role, Step,
		*GetNameSafe(Me),
		Me && Me->GetHeldItem() ? *Me->GetHeldItem()->GetDisplayName().ToString() : TEXT("none"),
		Computer ? *GetNameSafe(Computer->GetHolder()) : TEXT("<no computer>"),
		Computer ? *Computer->GetActorLocation().ToCompactString() : TEXT("-"),
		Door ? (Door->IsOpen() ? TEXT("open") : TEXT("closed")) : TEXT("<no door>"));
}

void UWEDevAutomationComponent::LookYawPitch(float Yaw, float Pitch) const
{
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		PC->SetControlRotation(FRotator(Pitch, Yaw, 0.f));
	}
}

void UWEDevAutomationComponent::LookAt(const AActor* Target) const
{
	const AWECharacter* Character = GetCharacter();
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!Character || !PC || !Target)
	{
		return;
	}
	const FVector Eye = Character->GetFirstPersonCamera()->GetComponentLocation();
	PC->SetControlRotation((Target->GetComponentsBoundingBox().GetCenter() - Eye).Rotation());
}

void UWEDevAutomationComponent::Shot(const FString& Suffix) const
{
	const FString File = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"), FString::Printf(TEXT("%s_%s.png"), *ShotName, *Suffix));
	FScreenshotRequest::RequestScreenshot(File, /*bShowUI*/ true, /*bAddFilenameSuffix*/ false);
	UE_LOG(LogWorkshopEmpire, Display, TEXT("SHOT %s"), *File);
}

void UWEDevAutomationComponent::Quit() const
{
	UE_LOG(LogWorkshopEmpire, Display, TEXT("Dev automation finished, quitting."));
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		PC->ConsoleCommand(TEXT("quit"));
	}
}

// --- Scenarios -----------------------------------------------------------------------------------

void UWEDevAutomationComponent::RunSmokeRoleA()
{
	Schedule(8.f, [this]()
	{
		LogState(TEXT("A0 start"));
		// The door is ~5 m away: the server must reject this (reach validation).
		if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestInteractWith(FindDoor()); }
	});
	Schedule(10.f, [this]()
	{
		if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestInteractWith(FindItemByName(TEXT("Computer"))); }
	});
	Schedule(12.f, [this]() { LogState(TEXT("A1 after pickup")); });
	Schedule(20.f, [this]()
	{
		if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestPrimaryInteraction(); }
	});
	Schedule(22.f, [this]() { LogState(TEXT("A2 after drop")); });
	Schedule(30.f, [this]() { LogState(TEXT("A3 final")); });
	Schedule(33.f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleB()
{
	Schedule(11.f, [this]() { LogState(TEXT("B0 sees A holding")); });
	Schedule(14.f, [this]()
	{
		// Conflict: A is holding the computer, the server must refuse.
		if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestInteractWith(FindItemByName(TEXT("Computer"))); }
	});
	Schedule(16.f, [this]() { LogState(TEXT("B1 after conflicting pickup")); });
	Schedule(23.f, [this]() { LogState(TEXT("B2 sees dropped")); });
	Schedule(25.f, [this]()
	{
		if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestInteractWith(FindItemByName(TEXT("Computer"))); }
	});
	Schedule(27.f, [this]() { LogState(TEXT("B3 after hand-over pickup")); });
	Schedule(34.f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunShots()
{
	Schedule(5.0f, [this]() { LookYawPitch(180.f, -12.f); });
	Schedule(6.0f, [this]() { Shot(TEXT("01_Werkbank")); });
	Schedule(7.0f, [this]() { LookYawPitch(0.f, -5.f); });
	Schedule(8.0f, [this]() { Shot(TEXT("02_Tor")); });
	Schedule(9.0f, [this]() { LookYawPitch(90.f, -8.f); });
	Schedule(10.0f, [this]() { Shot(TEXT("03_Seite")); });
	Schedule(11.0f, [this]() { LookAt(FindItemByName(TEXT("Computer"))); });
	Schedule(12.0f, [this]() { Shot(TEXT("04_Fokus")); });
	Schedule(12.5f, [this]()
	{
		if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestInteractWith(FindItemByName(TEXT("Computer"))); }
	});
	Schedule(13.5f, [this]() { LookYawPitch(180.f, -5.f); });
	Schedule(14.5f, [this]() { Shot(TEXT("05_Getragen")); });
	Schedule(15.0f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->ToggleExamine(); } });
	Schedule(16.0f, [this]() { Shot(TEXT("06_Untersuchen")); });
	Schedule(16.5f, [this]()
	{
		if (AWECharacter* C = GetCharacter())
		{
			C->GetInteractionComponent()->CloseExamine();
			LookYawPitch(180.f, -35.f);
			C->GetInteractionComponent()->RequestPrimaryInteraction();
		}
	});
	Schedule(18.5f, [this]() { Shot(TEXT("07_Abgelegt")); });
	Schedule(19.0f, [this]() { LookAt(FindItemByName(TEXT("Motor"))); });
	Schedule(20.0f, [this]() { Shot(TEXT("08_ZuSchwer")); });
	Schedule(21.5f, [this]() { Quit(); });
}

// --- Phase 2: inventory scenarios ----------------------------------------------------------------

UWEInventoryComponent* UWEDevAutomationComponent::GetMyInventory() const
{
	const AWECharacter* C = GetCharacter();
	return C ? C->GetPlayerInventory() : nullptr;
}

UWEInventoryComponent* UWEDevAutomationComponent::FindStorage(const FString& NameSubstring) const
{
	for (TActorIterator<AWEPlaceholderProp> It(GetWorld()); It; ++It)
	{
		if (It->HasStorage() && It->GetStorage()->GetDisplayName().ToString().Contains(NameSubstring))
		{
			return It->GetStorage();
		}
	}
	return nullptr;
}

UWEInventoryComponent* UWEDevAutomationComponent::FindOtherPlayerInventory() const
{
	const UWEInventoryComponent* Mine = GetMyInventory();
	if (const AGameStateBase* GS = GetWorld()->GetGameState())
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			if (const AWEPlayerState* WEPS = Cast<AWEPlayerState>(PS); WEPS && WEPS->GetInventory() != Mine)
			{
				return WEPS->GetInventory();
			}
		}
	}
	return nullptr;
}

FString UWEDevAutomationComponent::DescribeInventory(const UWEInventoryComponent* Inventory)
{
	if (!Inventory)
	{
		return TEXT("<none>");
	}
	TArray<FString> Parts;
	for (const FWEInventorySlotData& Slot : Inventory->GetAllSlots())
	{
		Parts.Add(FString::Printf(TEXT("[%d] %s x%d"), Slot.SlotIndex, *Slot.Item.GetDisplayName().ToString(), Slot.Item.Quantity));
	}
	return FString::Printf(TEXT("{%s} %.2fkg"), *FString::Join(Parts, TEXT(", ")), Inventory->GetTotalWeightKg());
}

int32 UWEDevAutomationComponent::FindSlotWith(const UWEInventoryComponent* Inventory, const FString& ItemName) const
{
	if (Inventory)
	{
		for (const FWEInventorySlotData& Slot : Inventory->GetAllSlots())
		{
			if (Slot.Item.GetDisplayName().ToString().Contains(ItemName))
			{
				return Slot.SlotIndex;
			}
		}
	}
	return INDEX_NONE;
}

int32 UWEDevAutomationComponent::CountEverywhere(const FString& ItemName) const
{
	int32 Total = 0;
	auto CountIn = [&ItemName](const UWEInventoryComponent* Inv)
	{
		int32 N = 0;
		for (const FWEInventorySlotData& Slot : Inv->GetAllSlots())
		{
			N += Slot.Item.GetDisplayName().ToString().Contains(ItemName) ? Slot.Item.Quantity : 0;
		}
		return N;
	};
	for (TObjectIterator<UWEInventoryComponent> It; It; ++It)
	{
		if (It->GetWorld() == GetWorld() && !It->IsTemplate())
		{
			Total += CountIn(*It);
		}
	}
	for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && It->GetItem().GetDisplayName().ToString().Contains(ItemName))
		{
			Total += It->GetItem().Quantity;
		}
	}
	return Total;
}

void UWEDevAutomationComponent::LogInventoryState(const TCHAR* Step) const
{
	UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] %s: Mine=%s Other=%s Bench=%s Blechschrauben.Total=%d RAM.Total=%d"),
		*Role, Step, *DescribeInventory(GetMyInventory()), *DescribeInventory(FindOtherPlayerInventory()),
		*DescribeInventory(FindStorage(TEXT("Werkbank"))), CountEverywhere(TEXT("Blechschrauben")), CountEverywhere(TEXT("Arbeitsspeicher")));
}

void UWEDevAutomationComponent::StoreItem(const FString& ItemName)
{
	if (AWECharacter* C = GetCharacter())
	{
		AActor* Target = FindItemByName(ItemName);
		UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] request store '%s' -> %s"), *Role, *ItemName, *GetNameSafe(Target));
		C->GetInteractionComponent()->RequestInteractWith(Target);
	}
}

void UWEDevAutomationComponent::RunSmokeRoleC()
{
	Schedule(8.f, [this]() { LogInventoryState(TEXT("C0 start")); StoreItem(TEXT("Blechschrauben")); });
	Schedule(11.f, [this]()
	{
		LogInventoryState(TEXT("C1 after storing screws"));
		AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
		UWEInventoryComponent* Mine = GetMyInventory();
		UWEInventoryComponent* Bench = FindStorage(TEXT("Werkbank"));
		if (PC && Mine && Bench) { PC->RequestMoveItem(Mine, FindSlotWith(Mine, TEXT("Blechschrauben")), Bench, 0, 5); }
	});
	Schedule(14.f, [this]() { LogInventoryState(TEXT("C2 after moving 5 to bench")); });
	Schedule(16.f, [this]()
	{
		AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
		UWEInventoryComponent* Mine = GetMyInventory();
		if (PC && Mine) { PC->RequestDropFromInventory(Mine, FindSlotWith(Mine, TEXT("Blechschrauben")), 3); }
	});
	Schedule(19.f, [this]() { LogInventoryState(TEXT("C3 after dropping 3")); });
	Schedule(24.f, [this]() { StoreItem(TEXT("Arbeitsspeicher")); }); // same moment as D: only one may get it
	Schedule(28.f, [this]() { LogInventoryState(TEXT("C4 after RAM race")); });
	Schedule(32.f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleD()
{
	Schedule(10.f, [this]() { LogInventoryState(TEXT("D0 start")); StoreItem(TEXT("Kupferkabel")); });
	Schedule(15.f, [this]() { LogInventoryState(TEXT("D1 sees bench")); });
	Schedule(17.f, [this]()
	{
		// Must be refused: another player's inventory is private.
		AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
		UWEInventoryComponent* Other = FindOtherPlayerInventory();
		UWEInventoryComponent* Mine = GetMyInventory();
		if (PC && Other && Mine) { PC->RequestMoveItem(Other, FindSlotWith(Other, TEXT("Blechschrauben")), Mine, 10, 0); }
	});
	Schedule(20.f, [this]() { LogInventoryState(TEXT("D2 after trying to take from other player")); });
	Schedule(21.f, [this]() { StoreItem(TEXT("Blechschrauben")); }); // the 3 dropped by C
	Schedule(24.f, [this]() { StoreItem(TEXT("Arbeitsspeicher")); });
	Schedule(28.f, [this]() { LogInventoryState(TEXT("D3 after RAM race")); });
	Schedule(33.f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunInventoryShots()
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	Schedule(3.5f, [this]() { LookYawPitch(180.f, -25.f); });
	Schedule(4.5f, [this]() { Shot(TEXT("00_Werkbank_Kleinteile")); });
	Schedule(5.0f, [this]() { StoreItem(TEXT("Blechschrauben")); StoreItem(TEXT("Kupferkabel")); });
	Schedule(5.5f, [this]() { StoreItem(TEXT("Arbeitsspeicher")); StoreItem(TEXT("Schraubendreher")); });
	Schedule(6.5f, [this]() { LookAt(FindItemByName(TEXT("Computer"))); });
	Schedule(7.5f, [this]() { Shot(TEXT("01_Fokus_Tragen")); LogInventoryState(TEXT("S1 stored")); });
	Schedule(8.0f, [this, PC]() { if (PC) { PC->OpenInventory(nullptr); } });
	Schedule(9.0f, [this]() { Shot(TEXT("02_Inventar")); });
	Schedule(9.5f, [this, PC]() { if (PC) { PC->CloseInventory(); PC->OpenInventory(FindStorage(TEXT("Werkbank"))); } });
	Schedule(10.0f, [this, PC]()
	{
		UWEInventoryComponent* Mine = GetMyInventory();
		UWEInventoryComponent* Bench = FindStorage(TEXT("Werkbank"));
		if (PC && Mine && Bench) { PC->RequestMoveItem(Mine, FindSlotWith(Mine, TEXT("Kupferkabel")), Bench, 0, 0); }
	});
	Schedule(11.0f, [this]() { Shot(TEXT("03_Werkbanklager")); LogInventoryState(TEXT("S2 moved cable to bench")); });
	Schedule(11.5f, [this, PC]() { if (PC) { PC->CloseInventory(); PC->RequestSave(TEXT("DevTest")); } });
	Schedule(12.5f, [this, PC]()
	{
		// Change the world after saving: drop the screws, take the cable back.
		UWEInventoryComponent* Mine = GetMyInventory();
		if (PC && Mine) { PC->RequestDropFromInventory(Mine, FindSlotWith(Mine, TEXT("Blechschrauben")), 0); }
	});
	Schedule(14.0f, [this]() { LogInventoryState(TEXT("S3 changed after save")); });
	Schedule(14.5f, [this, PC]() { if (PC) { PC->RequestLoad(TEXT("DevTest")); } });
	Schedule(16.0f, [this]() { LogInventoryState(TEXT("S4 after load")); Shot(TEXT("04_Geladen")); });
	Schedule(17.0f, [this, PC]() { if (PC) { PC->OpenInventory(nullptr); } });
	Schedule(18.0f, [this]() { Shot(TEXT("05_Inventar_nach_Laden")); });
	Schedule(19.0f, [this]() { Quit(); });
}
// --- Phase 3: workbench scenarios ----------------------------------------------------------------

AWEWorkbench* UWEDevAutomationComponent::FindBench() const
{
	TActorIterator<AWEWorkbench> It(GetWorld());
	return It ? *It : nullptr;
}

void UWEDevAutomationComponent::LookAtPoint(const FVector& Point) const
{
	const AWECharacter* Character = GetCharacter();
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (Character && PC)
	{
		PC->SetControlRotation((Point - Character->GetFirstPersonCamera()->GetComponentLocation()).Rotation());
	}
}

void UWEDevAutomationComponent::LogBenchState(const TCHAR* Step) const
{
	const AWEWorkbench* Bench = FindBench();
	const AWEPickupItem* Placed = Bench ? Bench->GetPlacedItem() : nullptr;
	UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] %s: Placed=%s Step=%d/%d Busy=%d Worker=%s Progress=%.2f Bench=%s Mine=%s"),
		*Role, Step,
		Placed ? *Placed->GetDisplayName().ToString() : TEXT("none"),
		Bench ? Bench->GetNextStepIndex() : -1, Bench ? Bench->GetNumSteps() : 0,
		Bench && Bench->IsBusy() ? 1 : 0, Bench ? *GetNameSafe(Bench->GetWorker()) : TEXT("-"),
		Bench ? Bench->GetProgress() : 0.f,
		*DescribeInventory(Bench ? Bench->GetStorage() : nullptr), *DescribeInventory(GetMyInventory()));
}

void UWEDevAutomationComponent::StartAutoSteps()
{
	GetWorld()->GetTimerManager().SetTimer(AutoStepTimer, this, &UWEDevAutomationComponent::AutoStepTick, 0.5f, true);
}

void UWEDevAutomationComponent::AutoStepTick()
{
	AWEWorkbench* Bench = FindBench();
	AWECharacter* C = GetCharacter();
	if (!Bench || !C)
	{
		return;
	}
	AWEPickupItem* Placed = Bench->GetPlacedItem();
	if (!Placed)
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoStepTimer);
		LogBenchState(TEXT("auto-steps finished"));
		return;
	}
	if (!Bench->IsBusy())
	{
		LookAtPoint(Placed->GetActorLocation());
		LogBenchState(TEXT("auto-step start"));
		C->GetInteractionComponent()->RequestInteractWith(Placed);
	}
}

void UWEDevAutomationComponent::RunSmokeRoleE()
{
	Schedule(8.f, [this]() { StoreItem(TEXT("Schraubendreher")); StoreItem(TEXT("Seitenschneider")); });
	Schedule(10.f, [this]()
	{
		if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestInteractWith(FindItemByName(TEXT("Computer"))); }
	});
	Schedule(11.f, [this]() { TeleportToBench(); }); // the spawn point is random; some are out of reach of the bench
	Schedule(12.f, [this]() { if (AWEWorkbench* B = FindBench()) { LookAtPoint(B->GetWorkSpotLocation()); } });
	Schedule(13.5f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestPrimaryInteraction(); } });
	Schedule(15.f, [this]() { LogBenchState(TEXT("E1 placed")); StartAutoSteps(); });
	Schedule(48.f, [this]() { LogBenchState(TEXT("E2 final")); });
	Schedule(52.f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleF()
{
	Schedule(16.f, [this]() { LogBenchState(TEXT("F0 sees bench")); });
	Schedule(18.f, [this]()
	{
		// Bench is busy with E's job (and F has no tool): must be refused.
		AWEWorkbench* B = FindBench();
		AWECharacter* C = GetCharacter();
		if (B && C && B->GetPlacedItem()) { C->GetInteractionComponent()->RequestInteractWith(B->GetPlacedItem()); }
	});
	Schedule(19.f, [this]() { LogBenchState(TEXT("F1 after trying to start a step")); });
	Schedule(20.f, [this]()
	{
		// Taking the item off while someone works on it: must be refused.
		AWEWorkbench* B = FindBench();
		AWECharacter* C = GetCharacter();
		if (B && C && B->GetPlacedItem()) { C->GetInteractionComponent()->RequestSecondaryInteractWith(B->GetPlacedItem()); }
	});
	Schedule(21.f, [this]() { LogBenchState(TEXT("F2 after trying to take it off")); });
	Schedule(48.f, [this]() { LogBenchState(TEXT("F3 final")); });
	Schedule(53.f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunWorkbenchShots()
{
	Schedule(4.0f, [this]() { StoreItem(TEXT("Schraubendreher")); StoreItem(TEXT("Seitenschneider")); });
	Schedule(5.0f, [this]() { LookAt(FindItemByName(TEXT("Computer"))); });
	Schedule(6.0f, [this]() { Shot(TEXT("01_Computer")); });
	Schedule(6.5f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestInteractWith(FindItemByName(TEXT("Computer"))); } });
	Schedule(7.5f, [this]() { if (AWEWorkbench* B = FindBench()) { LookAtPoint(B->GetWorkSpotLocation()); } });
	Schedule(8.5f, [this]() { Shot(TEXT("02_Platzieren")); });
	Schedule(9.0f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestPrimaryInteraction(); } });
	Schedule(10.0f, [this]() { if (AWEWorkbench* B = FindBench(); B && B->GetPlacedItem()) { LookAtPoint(B->GetPlacedItem()->GetActorLocation()); } });
	Schedule(11.0f, [this]() { Shot(TEXT("03_Erster_Schritt")); LogBenchState(TEXT("S1 placed")); });
	Schedule(11.5f, [this]() { StartAutoSteps(); });
	Schedule(13.0f, [this]() { Shot(TEXT("04_Fortschritt")); });
	Schedule(19.0f, [this]() { Shot(TEXT("05_Mitten_drin")); LogBenchState(TEXT("S2 midway")); });
	Schedule(36.0f, [this]()
	{
		LogBenchState(TEXT("S3 done"));
		if (AWEWorkbench* B = FindBench()) { LookAtPoint(B->GetWorkSpotLocation()); }
	});
	Schedule(37.0f, [this]() { Shot(TEXT("06_Zerlegt")); });
	Schedule(37.5f, [this]()
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner()); PC && FindBench()) { PC->OpenInventory(FindBench()->GetStorage()); }
	});
	Schedule(38.5f, [this]() { Shot(TEXT("07_Teile_im_Werkbanklager")); });
	Schedule(39.5f, [this]() { Quit(); });
}
// --- Phase 4: economy scenarios ------------------------------------------------------------------

AWETraderTerminal* UWEDevAutomationComponent::FindTerminal() const
{
	TActorIterator<AWETraderTerminal> It(GetWorld());
	return It ? *It : nullptr;
}

void UWEDevAutomationComponent::TeleportToTerminal(float SideOffset) const
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	const AWETraderTerminal* Terminal = FindTerminal();
	if (PC && Terminal)
	{
		// Stand 1.1 m in front of the terminal, facing it.
		const FVector Front = Terminal->GetActorLocation() + Terminal->GetActorForwardVector() * 110.f
			+ Terminal->GetActorRightVector() * SideOffset + FVector(0.f, 0.f, 100.f);
		const FRotator Facing = (Terminal->GetActorLocation() + FVector(0.f, 0.f, 90.f) - Front).Rotation();
		PC->ServerDevTeleport(Front, Facing);
	}
}

int32 UWEDevAutomationComponent::FindOfferIndex(const FString& ItemName) const
{
	if (const AWETraderTerminal* Terminal = FindTerminal())
	{
		for (const FWEMarketOffer& Offer : Terminal->GetOffers())
		{
			if (Offer.Item.GetDisplayName().ToString().Contains(ItemName))
			{
				return Offer.OfferIndex;
			}
		}
	}
	return INDEX_NONE;
}

void UWEDevAutomationComponent::LogEconomyState(const TCHAR* Step) const
{
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
	const AWETraderTerminal* Terminal = FindTerminal();
	TArray<FString> Stock;
	if (Terminal)
	{
		for (const int32 V : Terminal->GetStockRemaining())
		{
			Stock.Add(FString::FromInt(V));
		}
	}
	TArray<FString> Orders;
	if (Contracts)
	{
		for (const FWEContract& C : Contracts->GetContracts())
		{
			Orders.Add(FString::Printf(TEXT("%s:%s%s"), *C.Title.ToString(),
				C.State == EWEContractState::Accepted ? TEXT("accepted by ") : TEXT("offered"), *C.AcceptedBy));
		}
	}
	UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] %s: Balance=%s Mine=%s Stock=[%s] Motors=%d Screws.Total=%d Orders={%s}"),
		*Role, Step, Company ? *Company->GetBalance().ToDisplayString() : TEXT("?"), *DescribeInventory(GetMyInventory()),
		*FString::Join(Stock, TEXT(",")), CountEverywhere(TEXT("Elektromotor")), CountEverywhere(TEXT("Blechschrauben")),
		*FString::Join(Orders, TEXT(" | ")));
}

void UWEDevAutomationComponent::RunSmokeRoleG()
{
	Schedule(8.f, [this]() { StoreItem(TEXT("Blechschrauben")); });
	Schedule(10.f, [this]() { TeleportToTerminal(-40.f); });
	Schedule(12.f, [this]() { LogEconomyState(TEXT("G0 at terminal")); });
	Schedule(13.f, [this]()
	{
		// Same request id sent twice: the server must book it once.
		AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
		UWEInventoryComponent* Mine = GetMyInventory();
		if (PC && Mine) { PC->DevRequestSellTwice(FindTerminal(), Mine, FindSlotWith(Mine, TEXT("Blechschrauben")), 10); }
	});
	Schedule(16.f, [this]() { LogEconomyState(TEXT("G1 after double-sent sale of 10 screws")); });
	Schedule(20.f, [this]()
	{
		// Race: both players buy the single motor at the same moment.
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner())) { PC->RequestBuy(FindTerminal(), FindOfferIndex(TEXT("Elektromotor")), 1); }
	});
	Schedule(24.f, [this]() { LogEconomyState(TEXT("G2 after motor race")); });
	Schedule(26.f, [this]()
	{
		const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
		AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
		if (PC && Contracts && Contracts->GetContracts().Num() > 0) { PC->RequestAcceptContract(FindTerminal(), Contracts->GetContracts()[0].Id); }
	});
	Schedule(28.f, [this]() { LogEconomyState(TEXT("G3 after accepting an order")); });
	Schedule(32.f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleH()
{
	Schedule(10.f, [this]() { TeleportToTerminal(40.f); });
	Schedule(12.f, [this]() { LogEconomyState(TEXT("H0 at terminal")); });
	Schedule(16.f, [this]() { LogEconomyState(TEXT("H1 sees G's sale")); });
	Schedule(19.5f, [this]()
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner())) { PC->RequestBuy(FindTerminal(), FindOfferIndex(TEXT("Elektromotor")), 1); }
	});
	Schedule(24.f, [this]() { LogEconomyState(TEXT("H2 after motor race")); });
	Schedule(28.f, [this]() { LogEconomyState(TEXT("H3 sees G's order")); });
	Schedule(33.f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunEconomyShots()
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	Schedule(3.0f, [this]() { StoreItem(TEXT("Blechschrauben")); StoreItem(TEXT("Kupferkabel")); StoreItem(TEXT("Arbeitsspeicher")); });
	Schedule(4.0f, [this]() { TeleportToTerminal(0.f); LogEconomyState(TEXT("S0 start")); });
	Schedule(5.5f, [this]() { LookAt(FindTerminal()); });
	Schedule(6.5f, [this]() { Shot(TEXT("01_Terminal")); });
	Schedule(7.0f, [this, PC]() { if (PC) { PC->OpenTrade(FindTerminal()); } });
	Schedule(8.0f, [this]() { Shot(TEXT("02_Marktplatz")); });
	Schedule(8.5f, [this, PC]()
	{
		UWEInventoryComponent* Mine = GetMyInventory();
		if (PC && Mine)
		{
			PC->RequestSell(FindTerminal(), Mine, FindSlotWith(Mine, TEXT("Blechschrauben")), 0);
			PC->RequestSell(FindTerminal(), Mine, FindSlotWith(Mine, TEXT("Kupferkabel")), 2);
		}
	});
	Schedule(10.0f, [this]() { Shot(TEXT("03_Verkauft")); LogEconomyState(TEXT("S1 sold")); });
	Schedule(10.5f, [this, PC]()
	{
		if (PC)
		{
			PC->RequestBuy(FindTerminal(), FindOfferIndex(TEXT("Alter Computer")), 1);
			PC->RequestBuy(FindTerminal(), FindOfferIndex(TEXT("Akkuschrauber")), 1);
		}
	});
	Schedule(12.0f, [this]() { Shot(TEXT("04_Gekauft")); LogEconomyState(TEXT("S2 bought")); });
	Schedule(12.5f, [this, PC]()
	{
		const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
		if (PC && Contracts && Contracts->GetContracts().Num() > 0) { PC->RequestAcceptContract(FindTerminal(), Contracts->GetContracts()[0].Id); }
	});
	Schedule(13.5f, [this]() { Shot(TEXT("05_Auftrag_angenommen")); LogEconomyState(TEXT("S3 order accepted")); });
	Schedule(14.0f, [this, PC]()
	{
		if (PC) { PC->CloseTrade(); }
		if (const AWEDeliveryZone* Zone = AWEDeliveryZone::Find(GetWorld())) { LookAtPoint(Zone->GetActorLocation()); }
	});
	Schedule(15.5f, [this]() { Shot(TEXT("06_Lieferpalette")); });
	Schedule(16.5f, [this]() { Quit(); });
}

// --- Phase 5: full loop and progression ----------------------------------------------------------

void UWEDevAutomationComponent::LogProgressState(const TCHAR* Step) const
{
	const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	const AWEPlaceholderProp* Shelf2 = nullptr;
	for (TActorIterator<AWEPlaceholderProp> It(GetWorld()); It; ++It)
	{
		Shelf2 = It->GetRequiredUpgrade() == TEXT("SecondShelf") ? *It : Shelf2;
	}
	float Lamp = -1.f;
	for (TActorIterator<ALight> It(GetWorld()); It && Lamp < 0.f; ++It)
	{
		Lamp = It->ActorHasTag(TEXT("WE_WorkshopLight")) && It->GetLightComponent() ? It->GetLightComponent()->Intensity : Lamp;
	}
	TArray<FString> Owned;
	if (Progression)
	{
		for (const FName& Id : Progression->GetOwnedUpgrades())
		{
			Owned.Add(Id.ToString());
		}
	}
	const AWEWorkbench* Bench = FindBench();
	const UWEInventoryComponent* Mine = GetMyInventory();
	UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] %s: Balance=%s Upgrades=[%s] Guide=%d Shelf2=%s Lamp=%.1f MyMaxKg=%.1f BenchSlots=%d Mine=%s"),
		*Role, Step, Company ? *Company->GetBalance().ToDisplayString() : TEXT("?"), *FString::Join(Owned, TEXT(",")),
		Progression ? Progression->GetGuideStep() : -1,
		Shelf2 ? (Shelf2->IsUnlocked() && !Shelf2->IsHidden() ? TEXT("visible") : TEXT("locked")) : TEXT("<none>"),
		Lamp, Mine ? Mine->GetMaxWeightKg() : 0.f, Bench ? Bench->GetStorage()->GetNumSlots() : 0, *DescribeInventory(Mine));
}

void UWEDevAutomationComponent::SellAllParts()
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	UWEInventoryComponent* Mine = GetMyInventory();
	if (!PC || !Mine)
	{
		return;
	}
	for (const FWEInventorySlotData& Slot : Mine->GetAllSlots())
	{
		if (Slot.Item.Definition && Slot.Item.Definition->ToolType == EWEToolType::None)
		{
			PC->RequestSell(FindTerminal(), Mine, Slot.SlotIndex, 0);
		}
	}
}

void UWEDevAutomationComponent::RunLoopShots()
{
	// The complete vertical slice in one unattended run, through normal player requests.
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	Schedule(3.0f, [this]() { StoreItem(TEXT("Schraubendreher")); StoreItem(TEXT("Seitenschneider")); });
	Schedule(4.5f, [this]() { LookAt(FindItemByName(TEXT("Computer"))); LogProgressState(TEXT("S0 tools stored")); });
	Schedule(5.5f, [this]() { Shot(TEXT("01_Leitfaden")); });
	Schedule(6.0f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestInteractWith(FindItemByName(TEXT("Computer"))); } });
	Schedule(6.5f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(-250.f, 0.f, 100.f), FRotator(0.f, 180.f, 0.f)); } }); // walk to the bench
	Schedule(7.5f, [this]() { if (AWEWorkbench* B = FindBench()) { LookAtPoint(B->GetWorkSpotLocation()); } });
	Schedule(8.0f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestPrimaryInteraction(); } });
	Schedule(9.0f, [this]() { Shot(TEXT("02_Auf_der_Werkbank")); LogBenchState(TEXT("S1 placed")); StartAutoSteps(); });
	Schedule(38.0f, [this, PC]()
	{
		LogBenchState(TEXT("S2 disassembled"));
		if (AWEWorkbench* B = FindBench(); B && PC) { LookAtPoint(B->GetWorkSpotLocation()); PC->OpenInventory(B->GetStorage()); }
	});
	Schedule(39.0f, [this]() { Shot(TEXT("03_Werkbanklager")); });
	Schedule(39.5f, [this, PC]() { if (AWEWorkbench* B = FindBench(); B && PC) { PC->RequestTakeAll(B->GetStorage()); } });
	Schedule(40.5f, [this]() { Shot(TEXT("04_Alles_genommen")); LogProgressState(TEXT("S3 took all")); });
	Schedule(41.0f, [this, PC]() { if (PC) { PC->CloseInventory(); } TeleportToTerminal(0.f); });
	Schedule(42.5f, [this, PC]() { LookAt(FindTerminal()); if (PC) { PC->OpenTrade(FindTerminal()); } });
	Schedule(43.5f, [this]() { Shot(TEXT("05_Markt_mit_Upgrades")); SellAllParts(); });
	Schedule(45.0f, [this]() { Shot(TEXT("06_Teile_verkauft")); LogProgressState(TEXT("S4 sold parts")); });
	Schedule(45.5f, [this, PC]()
	{
		if (PC)
		{
			PC->RequestBuyUpgrade(FindTerminal(), TEXT("LedLights"));
			PC->RequestBuyUpgrade(FindTerminal(), TEXT("SecondShelf"));
		}
	});
	Schedule(47.0f, [this]() { Shot(TEXT("07_Upgrades_gekauft")); LogProgressState(TEXT("S5 bought upgrades")); });
	Schedule(47.5f, [this, PC]()
	{
		if (PC) { PC->CloseTrade(); }
		LookAtPoint(FVector(150.f, -272.f, 110.f));
	});
	Schedule(49.0f, [this]() { Shot(TEXT("08_Regal2_und_LED")); });
	Schedule(49.5f, [this, PC]() { if (PC) { PC->RequestSave(TEXT("DevP5")); } });
	Schedule(50.5f, [this]() { Shot(TEXT("09_Gespeichert_Leitfaden_fertig")); LogProgressState(TEXT("S6 saved")); });
	Schedule(51.0f, [this, PC]() { if (PC) { PC->RequestLoad(TEXT("DevP5")); } });
	Schedule(53.0f, [this]() { LookAtPoint(FVector(150.f, -272.f, 110.f)); LogProgressState(TEXT("S7 after load")); });
	Schedule(54.0f, [this]() { Shot(TEXT("10_Nach_dem_Laden")); });
	Schedule(55.0f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleI()
{
	// Listen-server host: buys two upgrades and saves; must later see the client's purchase.
	Schedule(10.f, [this]() { TeleportToTerminal(-40.f); });
	Schedule(12.f, [this]() { LogProgressState(TEXT("I0 host at terminal")); });
	Schedule(50.f, [this]()
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner()))
		{
			PC->RequestBuyUpgrade(FindTerminal(), TEXT("LedLights"));
			PC->RequestBuyUpgrade(FindTerminal(), TEXT("SecondShelf"));
		}
	});
	Schedule(53.f, [this]() { LogProgressState(TEXT("I1 host bought LED+shelf")); });
	Schedule(56.f, [this]() { if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner())) { PC->RequestSave(TEXT("DevNetP5")); } });
	Schedule(58.f, [this]() { LogProgressState(TEXT("I2 host saved")); });
	Schedule(78.f, [this]() { LogProgressState(TEXT("I3 host final")); });
	Schedule(80.f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleJ()
{
	// Client: must see the host's upgrades (shelf, lamps), cannot buy one twice, and its own purchase reaches everyone.
	Schedule(8.f, [this]() { TeleportToTerminal(40.f); });
	Schedule(10.f, [this]() { LogProgressState(TEXT("J0 client joined")); });
	Schedule(30.f, [this]() { LogProgressState(TEXT("J1 client sees host upgrades")); });
	Schedule(32.f, [this]()
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner()))
		{
			PC->RequestBuyUpgrade(FindTerminal(), TEXT("LedLights")); // already owned: refused
			PC->RequestBuyUpgrade(FindTerminal(), TEXT("ToolBelt"));
		}
	});
	Schedule(35.f, [this]() { LogProgressState(TEXT("J2 client bought tool belt")); });
	Schedule(40.f, [this]() { Quit(); });
}

// --- Phase 6: repair -----------------------------------------------------------------------------

void UWEDevAutomationComponent::TeleportToBench(float SideOffset) const
{
	if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner()))
	{
		PC->ServerDevTeleport(FVector(-250.f, SideOffset, 100.f), FRotator(0.f, 180.f, 0.f));
	}
}

AWEPickupItem* UWEDevAutomationComponent::FindCustomerDevice() const
{
	for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It)
	{
		if (It->GetItem().IsCustomerItem())
		{
			return *It;
		}
	}
	return nullptr;
}

int32 UWEDevAutomationComponent::FindOfferFor(const UWEItemDefinition* Def) const
{
	if (const AWETraderTerminal* Terminal = FindTerminal())
	{
		for (const FWEMarketOffer& Offer : Terminal->GetOffers())
		{
			if (Offer.Item.Definition == Def && Offer.Remaining > 0)
			{
				return Offer.OfferIndex;
			}
		}
	}
	return INDEX_NONE;
}

void UWEDevAutomationComponent::LogRepairState(const TCHAR* Step) const
{
	const AWEWorkbench* Bench = FindBench();
	const AWEPickupItem* Placed = Bench ? Bench->GetPlacedItem() : nullptr;
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
	TArray<FString> Orders;
	if (Contracts)
	{
		for (const FWEContract& C : Contracts->GetContracts())
		{
			if (C.Kind == EWEContractKind::Repair)
			{
				Orders.Add(FString::Printf(TEXT("%s:%s%s"), *C.Title.ToString(), C.State == EWEContractState::Accepted ? TEXT("accepted by ") : TEXT("offered"), *C.AcceptedBy));
			}
		}
	}
	const AWEPickupItem* Customer = FindCustomerDevice();
	UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] %s: Placed=%s Cond=%s Q=%d Repair='%s' Next='%s' Busy=%d Worker=%s Balance=%s Customer=%s RepairOrders={%s} Mine=%s Bench=%s"),
		*Role, Step,
		Placed ? *Placed->GetDisplayName().ToString() : TEXT("none"),
		Placed ? *WEItems::GetConditionText(Placed->GetItem().Condition).ToString() : TEXT("-"),
		Placed ? Placed->GetItem().Quality : 0,
		Placed ? *AWEWorkbench::GetRepairStatusText(Placed->GetItem()).ToString() : TEXT("-"),
		Bench ? *Bench->GetNextRepairPrompt().ToString() : TEXT("-"),
		Bench && Bench->IsBusy() ? 1 : 0, Bench ? *GetNameSafe(Bench->GetWorker()) : TEXT("-"),
		Company ? *Company->GetBalance().ToDisplayString() : TEXT("?"),
		Customer ? *FString::Printf(TEXT("%s(%s)"), *Customer->GetDisplayName().ToString(), *WEItems::GetConditionText(Customer->GetItem().Condition).ToString()) : TEXT("none"),
		*FString::Join(Orders, TEXT(" | ")), *DescribeInventory(GetMyInventory()), *DescribeInventory(Bench ? Bench->GetStorage() : nullptr));
}

void UWEDevAutomationComponent::BuyPartsForPlacedItem()
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	const AWEWorkbench* Bench = FindBench();
	const AWEPickupItem* Placed = Bench ? Bench->GetPlacedItem() : nullptr;
	const UWEItemDefinition* Def = Placed ? Placed->GetItem().Definition.Get() : nullptr;
	if (!PC || !Def || !Def->CanBeRepaired())
	{
		return;
	}
	const FWERepairState& State = Placed->GetItem().Repair;
	for (int32 i = 0; i < Def->Repair->Faults.Num(); ++i)
	{
		if (!FWERepairState::HasBit(State.Faults, i) || FWERepairState::HasBit(State.Replaced, i))
		{
			continue;
		}
		const UWEItemDefinition* Part = Def->Repair->Faults[i].Part;
		const int32 Offer = FindOfferFor(Part);
		UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] buy spare part %s -> offer %d"), *Role, *GetNameSafe(Part), Offer);
		if (Offer != INDEX_NONE)
		{
			PC->RequestBuy(FindTerminal(), Offer, 1);
		}
	}
}

void UWEDevAutomationComponent::StartAutoRepair(TFunction<void()> OnDone)
{
	OnAutoRepairDone = MoveTemp(OnDone);
	bAutoRepairSawBroken = false;
	GetWorld()->GetTimerManager().SetTimer(AutoRepairTimer, this, &UWEDevAutomationComponent::AutoRepairTick, 0.5f, true);
}

void UWEDevAutomationComponent::AutoRepairTick()
{
	AWEWorkbench* Bench = FindBench();
	AWECharacter* C = GetCharacter();
	AWEPickupItem* Placed = Bench ? Bench->GetPlacedItem() : nullptr;
	if (!Bench || !C || !Placed)
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoRepairTimer);
		return;
	}
	if (Bench->IsBusy())
	{
		return;
	}
	if (bAutoRepairSawBroken && !Placed->GetItem().IsBroken())
	{
		bAutoRepairSawBroken = false;
		Shot(TEXT("08_Funktionstest_bestanden"));
		LogRepairState(TEXT("auto-repair: test passed"));
	}
	bAutoRepairSawBroken |= Placed->GetItem().IsBroken();
	FText Reason;
	if (!Bench->CanStartRepairStep(C, Reason))
	{
		if (Bench->GetNextRepairStep(Reason).Kind == EWERepairStepKind::None || !Reason.ToString().Contains(TEXT("Arbeite")))
		{
			GetWorld()->GetTimerManager().ClearTimer(AutoRepairTimer);
			UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] auto-repair finished: %s"), *Role, *Reason.ToString());
			if (OnAutoRepairDone)
			{
				TFunction<void()> Done = MoveTemp(OnAutoRepairDone);
				Done();
			}
		}
		return;
	}
	LookAtPoint(Placed->GetActorLocation());
	LogRepairState(TEXT("auto-repair step"));
	C->GetInteractionComponent()->RequestAlternativeInteractWith(Placed);
}

void UWEDevAutomationComponent::RunRepairShots()
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Placed = [this]() -> AWEPickupItem* { const AWEWorkbench* B = FindBench(); return B ? B->GetPlacedItem() : nullptr; };
	Schedule(3.0f, [this]() { StoreItem(TEXT("Schraubendreher")); StoreItem(TEXT("Seitenschneider")); });
	Schedule(4.0f, [this]() { TeleportToTerminal(0.f); });
	Schedule(5.5f, [this, PC]() { LookAt(FindTerminal()); if (PC) { PC->OpenTrade(FindTerminal()); } });
	Schedule(6.5f, [this, PC]()
	{
		Shot(TEXT("01_Markt_Ersatzteile"));
		if (PC)
		{
			PC->RequestBuy(FindTerminal(), FindOfferFor(UWEItemDefinition::FindById(TEXT("Multimeter"))), 1);
			PC->RequestBuy(FindTerminal(), FindOfferFor(UWEItemDefinition::FindById(TEXT("CleaningKit"))), 1);
		}
	});
	Schedule(8.0f, [this, PC]()
	{
		LogRepairState(TEXT("S0 bought multimeter"));
		if (PC) { PC->CloseTrade(); PC->ServerDevTeleport(FVector(-190.f, -40.f, 100.f), FRotator(0.f, 180.f, 0.f)); }
	});
	Schedule(9.0f, [this]() { LookAt(FindItemByName(TEXT("Computer"))); });
	Schedule(9.5f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestInteractWith(FindItemByName(TEXT("Computer"))); } });
	Schedule(10.5f, [this]() { TeleportToBench(); });
	Schedule(11.5f, [this]() { if (AWEWorkbench* B = FindBench()) { LookAtPoint(B->GetWorkSpotLocation()); } });
	Schedule(12.0f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestPrimaryInteraction(); } });
	Schedule(13.0f, [this, Placed]() { if (AWEPickupItem* P = Placed()) { LookAtPoint(P->GetActorLocation()); } });
	Schedule(13.5f, [this]() { Shot(TEXT("02_Werkbank_Q_Reparieren")); LogRepairState(TEXT("S1 placed")); });
	Schedule(14.0f, [this, Placed]() { if (AWECharacter* C = GetCharacter(); C && Placed()) { C->GetInteractionComponent()->RequestAlternativeInteractWith(Placed()); } });
	Schedule(16.0f, [this]() { Shot(TEXT("03_Fehlersuche_laeuft")); });
	Schedule(18.8f, [this]() { Shot(TEXT("04_Diagnose")); LogRepairState(TEXT("S2 diagnosed")); });
	Schedule(19.3f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->ToggleExamine(); } });
	Schedule(20.3f, [this]() { Shot(TEXT("05_Untersuchen_Fehler")); });
	Schedule(20.8f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->CloseExamine(); } TeleportToTerminal(0.f); });
	Schedule(22.3f, [this, PC]() { LookAt(FindTerminal()); if (PC) { PC->OpenTrade(FindTerminal()); } BuyPartsForPlacedItem(); });
	Schedule(23.8f, [this, PC]() { Shot(TEXT("06_Ersatzteil_gekauft")); LogRepairState(TEXT("S3 bought parts")); if (PC) { PC->CloseTrade(); } });
	Schedule(24.5f, [this]() { TeleportToBench(); });
	Schedule(25.5f, [this, PC, Placed]()
	{
		if (AWEPickupItem* P = Placed()) { LookAtPoint(P->GetActorLocation()); }
		Schedule(4.5f, [this]() { Shot(TEXT("07_Reparatur_laeuft")); LogRepairState(TEXT("S4 repairing")); });
		StartAutoRepair([this, PC, Placed]()
		{
			LogRepairState(TEXT("S5 repaired and refurbished"));
			Schedule(0.5f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->ToggleExamine(); } });
			Schedule(1.5f, [this]() { Shot(TEXT("09_Untersuchen_repariert")); });
			Schedule(2.0f, [this, Placed]()
			{
				if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->CloseExamine(); if (Placed()) { C->GetInteractionComponent()->RequestSecondaryInteractWith(Placed()); } }
			});
			Schedule(3.0f, [this]() { TeleportToTerminal(0.f); });
			Schedule(4.5f, [this, PC]() { LookAt(FindTerminal()); if (PC) { PC->OpenTrade(FindTerminal()); } });
			Schedule(5.5f, [this]() { Shot(TEXT("10_Verkauf_reparierter_PC")); LogRepairState(TEXT("S6 at terminal with repaired PC")); });
			Schedule(6.0f, [PC, this]() { if (PC) { PC->RequestSellHeld(FindTerminal()); } });
			Schedule(7.5f, [this]() { Shot(TEXT("11_Verkauft")); LogRepairState(TEXT("S7 sold")); });
			Schedule(8.5f, [this]() { Quit(); });
		});
	});
	Schedule(110.f, [this]() { LogRepairState(TEXT("timeout")); Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleK()
{
	// Repair order end to end over the network: accept, carry, diagnose, buy part, repair, deliver.
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	Schedule(8.f, [this]() { StoreItem(TEXT("Schraubendreher")); StoreItem(TEXT("Seitenschneider")); });
	Schedule(10.f, [this]() { TeleportToTerminal(-40.f); });
	Schedule(12.f, [this, PC]()
	{
		if (PC)
		{
			PC->ServerDevAddContractOffer(8); // "Büro-PC zur Reparatur"
			PC->RequestBuy(FindTerminal(), FindOfferFor(UWEItemDefinition::FindById(TEXT("Multimeter"))), 1);
		}
	});
	Schedule(14.f, [this, PC]()
	{
		// The order added above is the last repair offer in the list.
		const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
		FGuid Chosen;
		for (const FWEContract& C : Contracts ? Contracts->GetContracts() : TArray<FWEContract>())
		{
			Chosen = C.Kind == EWEContractKind::Repair && C.State == EWEContractState::Offered ? C.Id : Chosen;
		}
		if (PC && Chosen.IsValid())
		{
			PC->RequestAcceptContract(FindTerminal(), Chosen);
		}
	});
	Schedule(16.f, [this, PC]()
	{
		LogRepairState(TEXT("K0 accepted repair order"));
		if (const AWEPickupItem* Device = FindCustomerDevice(); Device && PC)
		{
			const FVector Spot = Device->GetActorLocation() + FVector(-140.f, 0.f, 0.f);
			PC->ServerDevTeleport(FVector(Spot.X, Spot.Y, 100.f), FRotator(0.f, 0.f, 0.f));
		}
	});
	Schedule(17.5f, [this]() { LookAt(FindCustomerDevice()); });
	Schedule(18.f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestInteractWith(FindCustomerDevice()); } });
	Schedule(19.5f, [this]() { TeleportToBench(); });
	Schedule(20.5f, [this]() { if (AWEWorkbench* B = FindBench()) { LookAtPoint(B->GetWorkSpotLocation()); } });
	Schedule(21.f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestPrimaryInteraction(); } });
	Schedule(22.5f, [this]()
	{
		LogRepairState(TEXT("K1 placed customer device"));
		const AWEWorkbench* B = FindBench();
		if (AWECharacter* C = GetCharacter(); C && B && B->GetPlacedItem())
		{
			LookAtPoint(B->GetPlacedItem()->GetActorLocation());
			C->GetInteractionComponent()->RequestAlternativeInteractWith(B->GetPlacedItem());
		}
	});
	Schedule(28.f, [this]() { LogRepairState(TEXT("K2 diagnosed")); TeleportToTerminal(-40.f); });
	Schedule(30.f, [this]() { BuyPartsForPlacedItem(); });
	Schedule(32.f, [this]() { TeleportToBench(); });
	Schedule(33.f, [this, PC]()
	{
		StartAutoRepair([this, PC]()
		{
			LogRepairState(TEXT("K3 repaired"));
			Schedule(0.5f, [this]()
			{
				const AWEWorkbench* B = FindBench();
				if (AWECharacter* C = GetCharacter(); C && B && B->GetPlacedItem()) { C->GetInteractionComponent()->RequestSecondaryInteractWith(B->GetPlacedItem()); }
			});
			Schedule(2.0f, [this]() { TeleportToTerminal(-40.f); });
			Schedule(4.0f, [this, PC]()
			{
				const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
				for (const FWEContract& C : Contracts ? Contracts->GetContracts() : TArray<FWEContract>())
				{
					if (C.Kind == EWEContractKind::Repair && C.State == EWEContractState::Accepted && PC)
					{
						PC->RequestDeliverContract(FindTerminal(), C.Id);
					}
				}
			});
			Schedule(6.5f, [this]() { LogRepairState(TEXT("K4 delivered")); });
			Schedule(9.0f, [this]() { Quit(); });
		});
	});
	Schedule(95.f, [this]() { LogRepairState(TEXT("K timeout")); Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleL()
{
	// Second player: sees the order and the repair, cannot interfere while the bench is busy.
	Schedule(10.f, [this]() { TeleportToTerminal(40.f); });
	Schedule(16.f, [this]() { LogRepairState(TEXT("L0 sees order")); });
	Schedule(22.f, [this]() { TeleportToBench(70.f); });
	Schedule(24.f, [this]()
	{
		LogRepairState(TEXT("L1 at bench while K diagnoses"));
		const AWEWorkbench* B = FindBench();
		if (AWECharacter* C = GetCharacter(); C && B && B->GetPlacedItem())
		{
			LookAtPoint(B->GetPlacedItem()->GetActorLocation());
			C->GetInteractionComponent()->RequestAlternativeInteractWith(B->GetPlacedItem()); // busy: must be refused
			C->GetInteractionComponent()->RequestSecondaryInteractWith(B->GetPlacedItem()); // take off while busy: refused
		}
	});
	Schedule(26.f, [this]() { LogRepairState(TEXT("L2 after trying to interfere")); });
	Schedule(40.f, [this]() { LogRepairState(TEXT("L3 sees repair")); });
	Schedule(70.f, [this]() { LogRepairState(TEXT("L4 final")); });
	Schedule(73.f, [this]() { Quit(); });
}

// --- Phase 7: procurement ------------------------------------------------------------------------

AWEPickupItem* UWEDevAutomationComponent::FindItemById(const TCHAR* ItemId) const
{
	for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && !It->IsActorBeingDestroyed() && It->GetItem().Definition && It->GetItem().Definition->ItemId == FName(ItemId))
		{
			return *It;
		}
	}
	return nullptr;
}

void UWEDevAutomationComponent::LogAuctionState(const TCHAR* Step) const
{
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	const UWEAuctionComponent* Auctions = UWEAuctionComponent::Get(this);
	TArray<FString> Lots;
	TArray<FString> Results;
	if (Auctions)
	{
		for (const FWEAuctionLot& Lot : Auctions->GetLots())
		{
			Lots.Add(FString::Printf(TEXT("%s:%s:%s%s"), *Lot.Title.ToString(), *Lot.CurrentBid.ToDisplayString(), Lot.bCompanyLeads ? TEXT("WE ") : TEXT(""), *Lot.LeaderName));
		}
		Results = Auctions->GetRecentResults();
	}
	int32 OnPallet = 0;
	if (const AWEDeliveryZone* Zone = AWEDeliveryZone::Find(GetWorld()))
	{
		for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It)
		{
			OnPallet += IsValid(*It) && FVector::Dist2D(It->GetActorLocation(), Zone->GetActorLocation()) < 250.f ? 1 : 0;
		}
	}
	UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] %s: Balance=%s ItemsAtPallet=%d Lots={%s} LastResult='%s' Mine=%s"),
		*Role, Step, Company ? *Company->GetBalance().ToDisplayString() : TEXT("?"), OnPallet, *FString::Join(Lots, TEXT(" | ")),
		Results.Num() > 0 ? *Results[0] : TEXT("-"), *DescribeInventory(GetMyInventory()));
}

void UWEDevAutomationComponent::BidUntilLeading(int32 Tries)
{
	// One bid per half second on the first lot until the company leads (the rival answers at once).
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	const UWEAuctionComponent* Auctions = UWEAuctionComponent::Get(this);
	if (!PC || !Auctions || Auctions->GetLots().Num() == 0 || Tries <= 0)
	{
		return;
	}
	// The cheapest lot (a short bidding war is enough for the test).
	const FWEAuctionLot* Cheapest = &Auctions->GetLots()[0];
	for (const FWEAuctionLot& Candidate : Auctions->GetLots())
	{
		Cheapest = Candidate.EstimatedValue < Cheapest->EstimatedValue ? &Candidate : Cheapest;
	}
	const FWEAuctionLot& Lot = *Cheapest;
	if (Lot.bCompanyLeads)
	{
		LogAuctionState(TEXT("leading"));
		return;
	}
	PC->RequestBid(FindTerminal(), Lot.Id);
	Schedule(0.5f, [this, Tries]() { BidUntilLeading(Tries - 1); });
}

void UWEDevAutomationComponent::RunProcurementShots()
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Trade = [PC]() { return PC ? PC->GetTradeWidget() : nullptr; };
	Schedule(3.0f, [this]() { TeleportToTerminal(0.f); });
	Schedule(4.5f, [this, PC]() { LookAt(FindTerminal()); if (PC) { PC->OpenTrade(FindTerminal()); } });
	Schedule(5.5f, [this, PC]()
	{
		Shot(TEXT("01_Marktplatz_Paletten"));
		LogAuctionState(TEXT("S0 at terminal"));
		if (PC) { PC->RequestBuy(FindTerminal(), FindOfferFor(UWEItemDefinition::FindById(TEXT("ReturnPalletElectronics"))), 1); }
	});
	Schedule(6.5f, [Trade]() { if (UWETradeWidget* W = Trade()) { W->ShowPage(1); } });
	Schedule(7.5f, [this]() { Shot(TEXT("02_Auktionen")); BidUntilLeading(12); });
	Schedule(8.3f, [this]() { Shot(TEXT("03_Ueberboten")); });
	Schedule(15.0f, [this]() { Shot(TEXT("04_Auktion_fuehrend")); LogAuctionState(TEXT("S1 leading")); });
	Schedule(15.5f, [PC]() { if (PC) { PC->ServerDevEndAuctions(); } });
	Schedule(17.0f, [this]() { Shot(TEXT("05_Auktion_gewonnen")); LogAuctionState(TEXT("S2 auction won")); });
	Schedule(17.5f, [this, PC]()
	{
		if (PC) { PC->CloseTrade(); }
		if (const AWEDeliveryZone* Zone = AWEDeliveryZone::Find(GetWorld()); Zone && PC)
		{
			PC->ServerDevTeleport(Zone->GetActorLocation() + FVector(-170.f, 60.f, 95.f), FRotator(0.f, -20.f, 0.f));
		}
	});
	Schedule(19.0f, [this]() { LookAt(FindItemById(TEXT("ReturnPalletElectronics"))); });
	Schedule(20.0f, [this]() { Shot(TEXT("06_Retourenpalette")); });
	Schedule(20.5f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestAlternativeInteractWith(FindItemById(TEXT("ReturnPalletElectronics"))); } });
	Schedule(21.5f, [this]() { Shot(TEXT("07_Palette_geoeffnet")); LogAuctionState(TEXT("S3 pallet opened")); });
	Schedule(22.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(480.f, 60.f, 100.f), FRotator(-10.f, 35.f, 0.f)); } });
	Schedule(23.5f, [this]()
	{
		TActorIterator<AWEScrapPile> It(GetWorld());
		LookAt(It ? *It : nullptr);
	});
	Schedule(24.5f, [this]() { Shot(TEXT("08_Schrotthaufen")); });
	Schedule(25.5f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleM()
{
	// Bids on an auction until the company leads, then ends it (dev) and sees the win delivered.
	Schedule(10.f, [this]() { TeleportToTerminal(-40.f); });
	Schedule(12.f, [this]() { LogAuctionState(TEXT("M0 at terminal")); BidUntilLeading(12); });
	Schedule(20.f, [this]() { LogAuctionState(TEXT("M1 after bidding")); });
	Schedule(24.f, [this]() { if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner())) { PC->ServerDevEndAuctions(); } });
	Schedule(26.f, [this]() { LogAuctionState(TEXT("M2 after the auction ended")); });
	Schedule(34.f, [this]() { LogAuctionState(TEXT("M3 final")); });
	Schedule(36.f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleN()
{
	// Sees the company lead, cannot bid against the own company, buys and opens a return pallet.
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	Schedule(10.f, [this]() { TeleportToTerminal(40.f); });
	Schedule(20.5f, [this, PC]()
	{
		LogAuctionState(TEXT("N0 sees the company lead"));
		const UWEAuctionComponent* Auctions = UWEAuctionComponent::Get(this);
		// Bid on the lot the company already leads: must be refused.
		for (const FWEAuctionLot& Lot : Auctions ? Auctions->GetLots() : TArray<FWEAuctionLot>())
		{
			if (Lot.bCompanyLeads && PC) { PC->RequestBid(FindTerminal(), Lot.Id); }
		}
	});
	Schedule(21.5f, [this, PC]()
	{
		LogAuctionState(TEXT("N1 after trying to bid against the own company"));
		if (PC) { PC->RequestBuy(FindTerminal(), FindOfferFor(UWEItemDefinition::FindById(TEXT("ReturnPalletTools"))), 1); }
	});
	Schedule(23.f, [this, PC]()
	{
		if (const AWEDeliveryZone* Zone = AWEDeliveryZone::Find(GetWorld()); Zone && PC)
		{
			PC->ServerDevTeleport(Zone->GetActorLocation() + FVector(-170.f, 60.f, 95.f), FRotator(0.f, -20.f, 0.f));
		}
	});
	Schedule(28.f, [this]()
	{
		LookAt(FindItemById(TEXT("ReturnPalletTools")));
		if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestAlternativeInteractWith(FindItemById(TEXT("ReturnPalletTools"))); }
	});
	Schedule(30.f, [this]() { LogAuctionState(TEXT("N2 after opening the pallet")); });
	Schedule(35.f, [this]() { LogAuctionState(TEXT("N3 final")); });
	Schedule(37.f, [this]() { Quit(); });
}

// --- Phase 8: build mode -------------------------------------------------------------------------

UWEBuildComponent* UWEDevAutomationComponent::GetBuild() const
{
	const AWECharacter* C = GetCharacter();
	return C ? C->GetBuildComponent() : nullptr;
}

AWEPlaceholderProp* UWEDevAutomationComponent::FindFurniture(const TCHAR* FurnitureId) const
{
	for (TActorIterator<AWEPlaceholderProp> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && !It->IsActorBeingDestroyed() && It->GetFurnitureId() == FName(FurnitureId))
		{
			return *It;
		}
	}
	return nullptr;
}

int32 UWEDevAutomationComponent::FurnitureIndex(const TCHAR* FurnitureId) const
{
	const UWEWorkshopLayoutComponent* Layout = UWEWorkshopLayoutComponent::Get(this);
	const UWEFurnitureCatalog* Catalog = Layout ? Layout->GetCatalog() : nullptr;
	return Catalog ? Catalog->Items.IndexOfByPredicate([FurnitureId](const FWEFurnitureDefinition& D) { return D.Id == FName(FurnitureId); }) : INDEX_NONE;
}

void UWEDevAutomationComponent::LogFurniture(const TCHAR* Step) const
{
	TArray<FString> Parts;
	for (TActorIterator<AWEPlaceholderProp> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && !It->IsActorBeingDestroyed() && !It->IsHidden())
		{
			const FVector L = It->GetActorLocation();
			Parts.Add(FString::Printf(TEXT("%s@(%.0f,%.0f,%.0f°)%s"), *It->GetFurnitureName().ToString(), L.X, L.Y, It->GetActorRotation().Yaw,
				It->GetStorage() && It->HasStorage() ? *FString::Printf(TEXT("[%d Pl.]"), It->GetStorage()->GetNumSlots()) : TEXT("")));
		}
	}
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	const UWEBuildComponent* Build = GetBuild();
	UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] %s: Balance=%s Furniture={%s} Build=%s Preview=%s Status='%s'"), *Role, Step,
		Company ? *Company->GetBalance().ToDisplayString() : TEXT("?"), *FString::Join(Parts, TEXT(" ")),
		Build && Build->IsActive() ? TEXT("on") : TEXT("off"), Build && Build->HasPreview() ? (Build->IsPreviewValid() ? TEXT("green") : TEXT("red")) : TEXT("-"),
		Build ? *Build->GetStatusText().ToString() : TEXT("-"));
}

void UWEDevAutomationComponent::RunBuildShots()
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	Schedule(3.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(-60.f, -20.f, 100.f), FRotator(0.f, 30.f, 0.f)); } });
	Schedule(4.0f, [this]() { if (UWEBuildComponent* B = GetBuild()) { B->SetBuildMode(true); B->SelectIndex(FurnitureIndex(TEXT("Workbench"))); B->Rotate(90.f); } LookAtPoint(FVector(150.f, 150.f, 0.f)); });
	Schedule(5.0f, [this]() { Shot(TEXT("01_Baumodus_Vorschau")); LogFurniture(TEXT("S0 preview")); });
	Schedule(5.5f, [this]() { LookAtPoint(FVector(-90.f, 230.f, 0.f)); }); // at the market terminal: blocked
	Schedule(6.5f, [this]() { Shot(TEXT("02_Vorschau_blockiert")); LogFurniture(TEXT("S1 blocked preview")); });
	Schedule(7.0f, [this]() { LookAtPoint(FVector(150.f, 150.f, 0.f)); });
	Schedule(7.5f, [this]() { if (UWEBuildComponent* B = GetBuild()) { B->Confirm(); } });
	Schedule(9.0f, [this]() { Shot(TEXT("03_Werkbank_aufgestellt")); LogFurniture(TEXT("S2 placed second bench")); });
	Schedule(9.5f, [this]() { if (UWEBuildComponent* B = GetBuild()) { B->SelectIndex(FurnitureIndex(TEXT("Crate"))); } LookAtPoint(FVector(0.f, 150.f, 0.f)); });
	Schedule(10.5f, [this]() { if (UWEBuildComponent* B = GetBuild()) { B->Confirm(); } });
	Schedule(11.5f, [this]() { if (UWEBuildComponent* B = GetBuild()) { B->SelectIndex(INDEX_NONE); } if (AWEPlaceholderProp* Crate = FindFurniture(TEXT("Crate"))) { LookAt(Crate); } });
	Schedule(12.5f, [this]() { Shot(TEXT("04_Bearbeiten")); LogFurniture(TEXT("S3 edit mode on crate")); if (UWEBuildComponent* B = GetBuild()) { B->Confirm(); } });
	Schedule(13.5f, [this]() { LookAtPoint(FVector(60.f, 60.f, 0.f)); });
	Schedule(14.5f, [this]() { Shot(TEXT("05_Verschieben")); LogFurniture(TEXT("S4 moving crate")); if (UWEBuildComponent* B = GetBuild()) { B->Confirm(); } });
	Schedule(16.0f, [this]() { LogFurniture(TEXT("S5 crate moved")); if (AWEPlaceholderProp* Crate = FindFurniture(TEXT("Crate"))) { LookAt(Crate); } });
	Schedule(17.0f, [this]() { if (UWEBuildComponent* B = GetBuild()) { B->Remove(); } });
	Schedule(18.0f, [this]() { Shot(TEXT("06_Abgebaut")); LogFurniture(TEXT("S6 crate removed")); });
	Schedule(18.5f, [this, PC]() { if (UWEBuildComponent* B = GetBuild()) { B->SetBuildMode(false); } if (PC) { PC->RequestSave(TEXT("DevP8")); } });
	Schedule(19.5f, [this]() { if (AWEPlaceholderProp* Bench = FindFurniture(TEXT("Workbench"))) { Bench->SetActorLocation(FVector(150.f, 0.f, 0.f)); } }); // change, then load
	Schedule(20.0f, [PC]() { if (PC) { PC->RequestLoad(TEXT("DevP8")); } });
	Schedule(21.5f, [this]() { LookAtPoint(FVector(150.f, 150.f, 40.f)); LogFurniture(TEXT("S7 after load")); });
	Schedule(22.5f, [this]() { Shot(TEXT("07_Nach_dem_Laden")); });
	Schedule(23.5f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleO()
{
	// Places a bench and a crate; later sees what the other player changed.
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	Schedule(10.f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(-60.f, -20.f, 100.f), FRotator(0.f, 30.f, 0.f)); } });
	Schedule(12.f, [this]()
	{
		if (UWEBuildComponent* B = GetBuild())
		{
			B->SetBuildMode(true);
			B->SelectIndex(FurnitureIndex(TEXT("Workbench")));
			B->Rotate(90.f);
			B->SetAimOverride(FVector(150.f, 150.f, 0.f));
			B->Confirm();
		}
	});
	Schedule(13.f, [this]()
	{
		if (UWEBuildComponent* B = GetBuild())
		{
			B->SelectIndex(FurnitureIndex(TEXT("Crate")));
			B->SetAimOverride(FVector(0.f, 150.f, 0.f));
			B->Confirm();
			B->SetBuildMode(false);
		}
	});
	Schedule(15.f, [this]() { LogFurniture(TEXT("O1 placed bench and crate")); });
	Schedule(27.f, [this]() { LogFurniture(TEXT("O2 sees P's changes")); });
	Schedule(30.f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleP()
{
	// Sees O's furniture, cannot place on top of it, moves O's crate and sells it.
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	Schedule(10.f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(-60.f, -80.f, 100.f), FRotator(0.f, 30.f, 0.f)); } });
	Schedule(16.f, [this]()
	{
		LogFurniture(TEXT("P0 sees O's furniture"));
		if (UWEBuildComponent* B = GetBuild())
		{
			B->SetBuildMode(true);
			B->SelectIndex(FurnitureIndex(TEXT("Crate")));
			B->SetAimOverride(FVector(150.f, 150.f, 0.f));
			B->UpdatePreview();
		}
		LogFurniture(TEXT("P1 preview on O's bench"));
	});
	Schedule(17.f, [PC]()
	{
		// Bypass the (red) preview: the server must refuse on its own.
		if (PC) { PC->ServerPlaceFurniture(TEXT("Crate"), FTransform(FVector(150.f, 150.f, 0.f)), FGuid::NewGuid()); }
	});
	Schedule(19.f, [this, PC]()
	{
		LogFurniture(TEXT("P2 after forced place on the bench"));
		if (AWEPlaceholderProp* Crate = FindFurniture(TEXT("Crate")); Crate && PC) { PC->ServerMoveFurniture(Crate, FTransform(FVector(0.f, 60.f, 0.f))); }
	});
	Schedule(21.f, [this, PC]()
	{
		LogFurniture(TEXT("P3 moved O's crate"));
		if (AWEPlaceholderProp* Crate = FindFurniture(TEXT("Crate")); Crate && PC) { PC->ServerRemoveFurniture(Crate, FGuid::NewGuid()); }
	});
	Schedule(23.f, [this]() { if (UWEBuildComponent* B = GetBuild()) { B->SetBuildMode(false); } LogFurniture(TEXT("P4 sold the crate")); });
	Schedule(31.f, [this]() { Quit(); });
}

// --- Phase 9: machines -----------------------------------------------------------------------------

AActor* UWEDevAutomationComponent::FindNearestItem(const FString& Substring) const
{
	const AWECharacter* Me = GetCharacter();
	AActor* Best = nullptr;
	for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It)
	{
		if (Me && It->GetDisplayName().ToString().Contains(Substring)
			&& (!Best || FVector::DistSquared(It->GetActorLocation(), Me->GetActorLocation()) < FVector::DistSquared(Best->GetActorLocation(), Me->GetActorLocation())))
		{
			Best = *It;
		}
	}
	return Best;
}
AWEMachine* UWEDevAutomationComponent::FindMachine(const TCHAR* FurnitureId) const
{
	return Cast<AWEMachine>(FindFurniture(FurnitureId));
}

void UWEDevAutomationComponent::LogMachines(const TCHAR* Step) const
{
	TArray<FString> Parts;
	for (TActorIterator<AWEMachine> It(GetWorld()); It; ++It)
	{
		const AWEMachine* M = *It;
		if (!IsValid(M) || M->IsActorBeingDestroyed() || !M->GetMachineDefinition())
		{
			continue;
		}
		int32 In = 0, Out = 0;
		for (const FWEInventorySlotData& Slot : M->GetStorage()->GetAllSlots()) { In += Slot.Item.Quantity; }
		for (const FWEInventorySlotData& Slot : M->GetOutputStorage()->GetAllSlots()) { Out += Slot.Item.Quantity; }
		Parts.Add(FString::Printf(TEXT("%s[%s %d%% Wartung=%d In=%d Proc=%d Out=%d]"), *M->GetMachineDefinition()->DisplayName.ToString(),
			*M->GetStatusText().ToString(), FMath::RoundToInt(M->GetProgress() * 100.f), static_cast<int32>(M->GetMaintenance()), In, M->GetProcessing().Num(), Out));
	}
	TArray<FString> Bales;
	for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && It->GetItem().Definition && It->GetItem().Definition->ItemId.ToString().EndsWith(TEXT("Bale")))
		{
			Bales.Add(It->GetDisplayName().ToString());
		}
	}
	TArray<FString> Mine;
	if (const AWECharacter* Me = GetCharacter(); Me && Me->GetPlayerInventory())
	{
		for (const FWEInventorySlotData& Slot : Me->GetPlayerInventory()->GetAllSlots())
		{
			Mine.Add(FString::Printf(TEXT("%dx%s"), Slot.Item.Quantity, *Slot.Item.Definition->ItemId.ToString()));
		}
	}
	const UWEPowerGridComponent* Grid = UWEPowerGridComponent::Get(this);
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] %s: Balance=%s Power=%.1f/%.0f kW Unbilled=%.4f kWh LastBill=%s Machines={%s} Bales={%s} Mine={%s}"), *Role, Step,
		Company ? *Company->GetBalance().ToDisplayString() : TEXT("?"), Grid ? Grid->GetLoadKw() : -1.f, Grid ? Grid->GetCapacityKw() : -1.f,
		Grid ? Grid->GetUnbilledKWh() : -1.f, Grid ? *Grid->GetLastBill().ToDisplayString() : TEXT("-"),
		*FString::Join(Parts, TEXT(" ")), *FString::Join(Bales, TEXT(",")), *FString::Join(Mine, TEXT(" ")));
}

void UWEDevAutomationComponent::PlaceMachines()
{
	// Along the +Y wall, fronts facing into the garage: shredder, sorting machine, press.
	UWEBuildComponent* B = GetBuild();
	if (!B)
	{
		return;
	}
	B->SetBuildMode(true);
	B->SelectIndex(FurnitureIndex(TEXT("Shredder")));
	B->Rotate(-90.f);
	B->SetAimOverride(FVector(-20.f, 210.f, 0.f));
	B->Confirm();
	B->SelectIndex(FurnitureIndex(TEXT("Sorter")));
	B->SetAimOverride(FVector(140.f, 200.f, 0.f));
	B->Confirm();
	B->SelectIndex(FurnitureIndex(TEXT("Press")));
	B->SetAimOverride(FVector(310.f, 200.f, 0.f));
	B->Confirm();
	B->SetBuildMode(false);
}

void UWEDevAutomationComponent::TransferToMachine(const TCHAR* ItemId, AWEMachine* Machine)
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	const AWECharacter* Me = GetCharacter();
	if (!PC || !Me || !Machine || !Me->GetPlayerInventory())
	{
		return;
	}
	for (const FWEInventorySlotData& Slot : Me->GetPlayerInventory()->GetAllSlots())
	{
		if (Slot.Item.Definition && Slot.Item.Definition->ItemId == FName(ItemId))
		{
			PC->RequestQuickTransfer(Me->GetPlayerInventory(), Slot.SlotIndex, Machine->GetStorage());
			return;
		}
	}
}

void UWEDevAutomationComponent::RunMachineShots()
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Interaction = [this]() { AWECharacter* C = GetCharacter(); return C ? C->GetInteractionComponent() : nullptr; };
	Schedule(3.0f, [PC]() { if (PC) { PC->ServerDevAddMoney(2000); PC->ServerDevTeleport(FVector(140.f, -60.f, 100.f), FRotator(0.f, 90.f, 0.f)); } });
	Schedule(4.0f, [this]() { PlaceMachines(); });
	Schedule(5.0f, [this]() { LookAtPoint(FVector(140.f, 200.f, 50.f)); LogMachines(TEXT("S0 machines placed")); LogFurniture(TEXT("S0")); });
	Schedule(5.5f, [this]() { Shot(TEXT("01_Maschinen_aufgestellt")); });
	Schedule(6.0f, [PC]()
	{
		if (PC)
		{
			PC->ServerDevTeleport(FVector(-20.f, 40.f, 100.f), FRotator(0.f, 90.f, 0.f));
			PC->ServerDevGiveItem(TEXT("MaintenanceKit"), 1, EWEItemCondition::New);
			PC->ServerDevGiveItem(TEXT("ShredMetal"), 10, EWEItemCondition::Used);
			PC->ServerDevGiveItem(TEXT("OldComputer"), 1, EWEItemCondition::Defective);
		}
	});
	Schedule(7.0f, [this, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(FindNearestItem(TEXT("Computer"))); } });
	Schedule(8.0f, [this]() { LookAtPoint(FVector(-20.f, 200.f, 90.f)); });
	Schedule(9.0f, [this]() { Shot(TEXT("02_PC_in_den_Schredder")); LogMachines(TEXT("S1 carrying the PC")); });
	Schedule(9.5f, [Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->UpdateFocus(); I->RequestPrimaryInteraction(); } });
	Schedule(10.5f, [this, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestAlternativeInteractWith(FindMachine(TEXT("Shredder"))); } });
	Schedule(12.0f, [this]() { Shot(TEXT("03_Schredder_laeuft")); LogMachines(TEXT("S2 shredder running")); });
	Schedule(12.5f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(140.f, 40.f, 100.f), FRotator(0.f, 90.f, 0.f)); } });
	Schedule(13.5f, [this, Interaction]()
	{
		TransferToMachine(TEXT("ShredMetal"), FindMachine(TEXT("Sorter")));
		if (UWEInteractionComponent* I = Interaction()) { I->RequestAlternativeInteractWith(FindMachine(TEXT("Sorter"))); }
	});
	Schedule(14.5f, [this]() { LookAtPoint(FVector(140.f, 200.f, 70.f)); });
	Schedule(15.5f, [this]() { Shot(TEXT("04_Strom_ueberlastet")); LogMachines(TEXT("S3 sorter waits for power")); });
	Schedule(26.0f, [this]() { Shot(TEXT("05_Sortiermaschine_laeuft")); LogMachines(TEXT("S4 shredder done, sorter running")); });
	Schedule(26.5f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(-20.f, 40.f, 100.f), FRotator(0.f, 90.f, 0.f)); } });
	Schedule(27.5f, [this, Interaction]()
	{
		LookAtPoint(FVector(-20.f, 200.f, 90.f));
		if (UWEInteractionComponent* I = Interaction()) { I->RequestSecondaryInteractWith(FindMachine(TEXT("Shredder"))); }
	});
	Schedule(28.5f, [this]() { Shot(TEXT("06_Warten_moeglich")); LogMachines(TEXT("S5 took shredded material")); });
	Schedule(29.0f, [this, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestExtraInteractWith(FindMachine(TEXT("Shredder"))); } });
	Schedule(29.6f, [this, PC]() { LogMachines(TEXT("S6 maintained")); if (PC) { PC->ServerDevTeleport(FVector(140.f, 40.f, 100.f), FRotator(0.f, 90.f, 0.f)); } });
	Schedule(30.2f, [this]() { TransferToMachine(TEXT("ShredElectronics"), FindMachine(TEXT("Sorter"))); }); // the sorter works through it later
	Schedule(30.6f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(310.f, 30.f, 100.f), FRotator(0.f, 90.f, 0.f)); PC->ServerDevGiveItem(TEXT("SteelScrap"), 20, EWEItemCondition::Used); } });
	Schedule(31.5f, [this, Interaction]()
	{
		TransferToMachine(TEXT("SteelScrap"), FindMachine(TEXT("Press")));
		if (UWEInteractionComponent* I = Interaction()) { I->RequestAlternativeInteractWith(FindMachine(TEXT("Press"))); }
	});
	Schedule(41.5f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(230.f, -40.f, 100.f), FRotator(0.f, 70.f, 0.f)); } });
	Schedule(42.2f, [this]() { LookAtPoint(FVector(300.f, 110.f, 30.f)); LogMachines(TEXT("S7 bale pressed")); });
	Schedule(43.0f, [this]() { Shot(TEXT("07_Stahlballen")); });
	Schedule(43.5f, [PC]() { if (PC) { PC->RequestSave(TEXT("DevP9")); } });
	Schedule(44.5f, [this, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestAlternativeInteractWith(FindMachine(TEXT("Sorter"))); } }); // change, then load
	Schedule(45.5f, [PC]() { if (PC) { PC->RequestLoad(TEXT("DevP9")); } });
	Schedule(47.0f, [this]() { LookAtPoint(FVector(140.f, 200.f, 70.f)); LogMachines(TEXT("S8 after load")); });
	Schedule(48.0f, [this]() { Shot(TEXT("08_Nach_dem_Laden")); });
	Schedule(49.0f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleQ()
{
	// Operator: places the machines, puts a PC into the shredder and switches it on.
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Interaction = [this]() { AWECharacter* C = GetCharacter(); return C ? C->GetInteractionComponent() : nullptr; };
	Schedule(10.f, [PC]() { if (PC) { PC->ServerDevAddMoney(2000); PC->ServerDevTeleport(FVector(140.f, -60.f, 100.f), FRotator(0.f, 90.f, 0.f)); } });
	Schedule(11.f, [this]() { PlaceMachines(); });
	Schedule(12.f, [PC]()
	{
		if (PC)
		{
			PC->ServerDevTeleport(FVector(-20.f, 40.f, 100.f), FRotator(0.f, 90.f, 0.f));
			PC->ServerDevGiveItem(TEXT("OldComputer"), 1, EWEItemCondition::Defective);
		}
	});
	Schedule(13.f, [this, Interaction]() { LogMachines(TEXT("Q0 placed")); if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(FindNearestItem(TEXT("Computer"))); } });
	Schedule(14.f, [this]() { LookAtPoint(FVector(-20.f, 200.f, 90.f)); });
	Schedule(15.f, [Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->UpdateFocus(); I->RequestPrimaryInteraction(); } });
	Schedule(16.f, [this, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestAlternativeInteractWith(FindMachine(TEXT("Shredder"))); } });
	Schedule(17.5f, [this]() { LogMachines(TEXT("Q1 shredder running")); });
	Schedule(40.f, [this]() { LogMachines(TEXT("Q2 sees R's actions")); });
	Schedule(42.f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleR()
{
	// Second player: sees the running shredder, starts the sorter (waits: power), takes the products.
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Interaction = [this]() { AWECharacter* C = GetCharacter(); return C ? C->GetInteractionComponent() : nullptr; };
	Schedule(10.f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(140.f, -120.f, 100.f), FRotator(0.f, 90.f, 0.f)); } });
	Schedule(17.f, [this, PC]()
	{
		LogMachines(TEXT("R0 sees Q's shredder"));
		if (PC)
		{
			PC->ServerDevTeleport(FVector(140.f, 40.f, 100.f), FRotator(0.f, 90.f, 0.f));
			PC->ServerDevGiveItem(TEXT("ShredMetal"), 10, EWEItemCondition::Used);
		}
	});
	Schedule(18.5f, [this, Interaction]()
	{
		TransferToMachine(TEXT("ShredMetal"), FindMachine(TEXT("Sorter")));
		if (UWEInteractionComponent* I = Interaction()) { I->RequestAlternativeInteractWith(FindMachine(TEXT("Sorter"))); }
	});
	Schedule(20.f, [this]() { LogMachines(TEXT("R1 sorter waits for power")); });
	Schedule(31.f, [this, PC]()
	{
		LogMachines(TEXT("R2 shredder done, sorter runs"));
		if (PC) { PC->ServerDevTeleport(FVector(40.f, 60.f, 100.f), FRotator(0.f, 90.f, 0.f)); }
	});
	Schedule(32.f, [this, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestSecondaryInteractWith(FindMachine(TEXT("Shredder"))); } });
	Schedule(33.5f, [this]() { LogMachines(TEXT("R3 took the shredded material")); });
	Schedule(38.f, [this]() { Quit(); });
}

// --- Phase 10: production line ------------------------------------------------------------------------

void UWEDevAutomationComponent::PlaceFurnitureAt(const TCHAR* FurnitureId, float X, float Y, float Yaw)
{
	if (UWEBuildComponent* B = GetBuild())
	{
		B->SelectIndex(FurnitureIndex(FurnitureId));
		B->SetRotation(Yaw);
		B->SetAimOverride(FVector(X, Y, 0.f));
		B->Confirm();
	}
}

void UWEDevAutomationComponent::PlaceProductionLine()
{
	// Feed belt from the front into the shredder, then along y = 130: belt -> sorting machine -> belt -> material store;
	// from the store a belt runs to the assembly machine near the delivery pallet.
	UWEBuildComponent* B = GetBuild();
	if (!B)
	{
		return;
	}
	B->SetBuildMode(true);
	PlaceFurnitureAt(TEXT("Conveyor"), -70.f, 20.f, 90.f);
	PlaceFurnitureAt(TEXT("Shredder"), -70.f, 130.f, 0.f);
	PlaceFurnitureAt(TEXT("Conveyor"), 35.f, 130.f, 0.f);
	PlaceFurnitureAt(TEXT("Sorter"), 135.f, 130.f, 0.f);
	PlaceFurnitureAt(TEXT("Conveyor"), 235.f, 130.f, 0.f);
	PlaceFurnitureAt(TEXT("MaterialStore"), 335.f, 130.f, 0.f);
	PlaceFurnitureAt(TEXT("Conveyor"), 335.f, 20.f, -90.f);
	PlaceFurnitureAt(TEXT("Assembler"), 310.f, -90.f, 90.f);
	B->SetRotation(0.f);
	B->SetBuildMode(false);
}

AWEConveyor* UWEDevAutomationComponent::FindConveyorNear(const FVector& Point) const
{
	AWEConveyor* Best = nullptr;
	for (TActorIterator<AWEConveyor> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && (!Best || FVector::Dist2D(It->GetActorLocation(), Point) < FVector::Dist2D(Best->GetActorLocation(), Point)))
		{
			Best = *It;
		}
	}
	return Best;
}

void UWEDevAutomationComponent::LogFlow(const TCHAR* Step) const
{
	TArray<FString> Belts;
	for (TActorIterator<AWEConveyor> It(GetWorld()); It; ++It)
	{
		TArray<FString> Entries;
		for (const FWEConveyorItem& Entry : It->GetItems())
		{
			Entries.Add(FString::Printf(TEXT("%dx%s@%.0f"), Entry.Item.Quantity, Entry.Item.Definition ? *Entry.Item.Definition->ItemId.ToString() : TEXT("?"), Entry.Position));
		}
		const FVector L = It->GetActorLocation();
		Belts.Add(FString::Printf(TEXT("Band(%.0f,%.0f)[%s]{%s}"), L.X, L.Y, It->IsOn() ? TEXT("an") : TEXT("aus"), *FString::Join(Entries, TEXT(","))));
	}
	TArray<FString> Stores;
	for (TActorIterator<AWEProductionStorage> It(GetWorld()); It; ++It)
	{
		TArray<FString> Content;
		for (const FWEInventorySlotData& Slot : It->GetStorage()->GetAllSlots())
		{
			Content.Add(FString::Printf(TEXT("%dx%s"), Slot.Item.Quantity, *Slot.Item.Definition->ItemId.ToString()));
		}
		Stores.Add(FString::Printf(TEXT("Lager[Ausgabe=%s]{%s}"), *It->GetOutputText().ToString(), *FString::Join(Content, TEXT(" "))));
	}
	TArray<FString> Machines;
	for (TActorIterator<AWEMachine> It(GetWorld()); It; ++It)
	{
		if (!It->GetMachineDefinition())
		{
			continue;
		}
		TArray<FString> Out;
		for (const FWEInventorySlotData& Slot : It->GetOutputStorage()->GetAllSlots())
		{
			Out.Add(FString::Printf(TEXT("%dx%s"), Slot.Item.Quantity, *Slot.Item.Definition->ItemId.ToString()));
		}
		int32 In = 0;
		for (const FWEInventorySlotData& Slot : It->GetStorage()->GetAllSlots()) { In += Slot.Item.Quantity; }
		Machines.Add(FString::Printf(TEXT("%s[%s | Rezept=%s In=%d Out={%s}]"), *It->GetMachineDefinition()->DisplayName.ToString(), *It->GetStatusText().ToString(),
			*It->GetSelectedRecipeText().ToString(), In, *FString::Join(Out, TEXT(" "))));
	}
	const UWEPowerGridComponent* Grid = UWEPowerGridComponent::Get(this);
	UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] %s: Power=%.2f/%.0f kW Belts={%s} Stores={%s} Machines={%s}"), *Role, Step,
		Grid ? Grid->GetLoadKw() : -1.f, Grid ? Grid->GetCapacityKw() : -1.f, *FString::Join(Belts, TEXT(" ")), *FString::Join(Stores, TEXT(" ")), *FString::Join(Machines, TEXT(" ")));
}

void UWEDevAutomationComponent::RunFlowShots()
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Interaction = [this]() { AWECharacter* C = GetCharacter(); return C ? C->GetInteractionComponent() : nullptr; };
	auto Store = [this]() { TActorIterator<AWEProductionStorage> It(GetWorld()); return It ? *It : nullptr; };
	Schedule(3.0f, [PC]() { if (PC) { PC->ServerDevAddMoney(3000); PC->ServerDevTeleport(FVector(40.f, -60.f, 100.f), FRotator(0.f, 90.f, 0.f)); } });
	Schedule(4.0f, [this]() { PlaceProductionLine(); });
	Schedule(5.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(60.f, -200.f, 100.f), FRotator(0.f, 90.f, 0.f)); } });
	Schedule(5.5f, [this]() { LookAtPoint(FVector(130.f, 100.f, 20.f)); LogFurniture(TEXT("S0")); LogFlow(TEXT("S0 line placed")); });
	Schedule(6.5f, [this]() { Shot(TEXT("01_Linie_aufgebaut")); });
	Schedule(7.0f, [this, PC, Interaction]()
	{
		if (PC)
		{
			PC->ServerDevTeleport(FVector(-170.f, -80.f, 100.f), FRotator(0.f, 0.f, 0.f));
			PC->ServerDevGiveItem(TEXT("OldComputer"), 2, EWEItemCondition::Defective);
		}
	});
	Schedule(7.5f, [this, Interaction]()
	{
		if (UWEInteractionComponent* I = Interaction())
		{
			I->RequestAlternativeInteractWith(FindMachine(TEXT("Shredder")));
			I->RequestAlternativeInteractWith(FindMachine(TEXT("Sorter")));
		}
	});
	Schedule(8.0f, [this, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(FindNearestItem(TEXT("Computer"))); } });
	Schedule(8.5f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(-150.f, 10.f, 100.f), FRotator(0.f, 0.f, 0.f)); } });
	Schedule(9.0f, [this]() { LookAtPoint(FVector(-70.f, 20.f, 45.f)); });
	Schedule(9.5f, [this]() { Shot(TEXT("02_PC_aufs_Band")); });
	Schedule(10.0f, [Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->UpdateFocus(); I->RequestPrimaryInteraction(); } });
	Schedule(10.5f, [this]() { LogFlow(TEXT("S1 first PC on the feed belt")); });
	Schedule(11.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(-170.f, -80.f, 100.f), FRotator(0.f, 0.f, 0.f)); } });
	Schedule(11.5f, [this, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(FindNearestItem(TEXT("Computer"))); } });
	Schedule(12.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(-150.f, 10.f, 100.f), FRotator(0.f, 0.f, 0.f)); } });
	Schedule(12.5f, [this]() { LookAtPoint(FVector(-70.f, 20.f, 45.f)); });
	Schedule(13.0f, [Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->UpdateFocus(); I->RequestPrimaryInteraction(); } });
	Schedule(13.5f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(-180.f, 40.f, 100.f), FRotator(0.f, 45.f, 0.f)); } });
	Schedule(14.0f, [this]() { LookAtPoint(FVector(-70.f, 130.f, 60.f)); LogFlow(TEXT("S2 both PCs fed")); });
	Schedule(14.5f, [this]() { Shot(TEXT("03_Schredder_vom_Band")); });
	Schedule(24.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(35.f, -20.f, 100.f), FRotator(0.f, 90.f, 0.f)); } });
	Schedule(24.5f, [this]() { LookAtPoint(FVector(35.f, 130.f, 45.f)); });
	Schedule(25.6f, [this]() { Shot(TEXT("04_Schreddergut_auf_dem_Band")); LogFlow(TEXT("S3 shredded material on its way")); });
	Schedule(58.0f, [this, PC]() { LogFlow(TEXT("S4 sorted into the store")); if (PC) { PC->ServerDevTeleport(FVector(250.f, 30.f, 100.f), FRotator(0.f, 45.f, 0.f)); } });
	Schedule(58.5f, [this, Store]() { if (AWEProductionStorage* S = Store()) { LookAt(S); } });
	Schedule(59.5f, [this, Store, Interaction]()
	{
		// [T] until the store sends out only steel.
		AWEProductionStorage* S = Store();
		UWEInteractionComponent* I = Interaction();
		UWEItemDefinition* Steel = UWEItemDefinition::FindById(TEXT("SteelScrap"));
		for (int32 i = 0; i < 8 && S && I && S->GetOutputFilter() != Steel; ++i)
		{
			I->RequestSettingInteractWith(S);
		}
	});
	Schedule(60.0f, [this]() { Shot(TEXT("05_Materiallager_Ausgabe")); LogFlow(TEXT("S5 store sends steel")); });
	Schedule(60.5f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(120.f, -60.f, 100.f), FRotator(0.f, 0.f, 0.f)); } });
	Schedule(61.0f, [this, Interaction]()
	{
		AWEMachine* Assembler = FindMachine(TEXT("Assembler"));
		LookAt(Assembler);
		if (UWEInteractionComponent* I = Interaction())
		{
			for (int32 i = 0; i < 3; ++i) { I->RequestSettingInteractWith(Assembler); }
			I->RequestAlternativeInteractWith(Assembler);
		}
	});
	Schedule(62.0f, [this]() { Shot(TEXT("06_Fertigungsautomat_Rezept")); });
	Schedule(63.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(250.f, 20.f, 100.f), FRotator(0.f, 0.f, 0.f)); } });
	Schedule(63.5f, [this, Interaction]() { LookAtPoint(FVector(335.f, 20.f, 40.f)); if (UWEInteractionComponent* I = Interaction()) { I->UpdateFocus(); I->ToggleExamine(); } });
	Schedule(64.5f, [this]() { Shot(TEXT("07_Band_untersuchen")); });
	Schedule(65.0f, [Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->CloseExamine(); } });
	Schedule(80.0f, [this, PC]() { LogFlow(TEXT("S6 products made")); if (PC) { PC->ServerDevTeleport(FVector(120.f, -60.f, 100.f), FRotator(0.f, 0.f, 0.f)); } });
	Schedule(80.5f, [this]() { LookAt(FindMachine(TEXT("Assembler"))); });
	Schedule(81.5f, [this]() { Shot(TEXT("08_Stahlwinkel_fertig")); });
	Schedule(82.0f, [PC]() { if (PC) { PC->RequestSave(TEXT("DevP10")); } });
	Schedule(83.0f, [this, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestAlternativeInteractWith(FindConveyorNear(FVector(335.f, 20.f, 0.f))); } }); // change, then load
	Schedule(84.0f, [PC]() { if (PC) { PC->RequestLoad(TEXT("DevP10")); } });
	Schedule(85.5f, [this]() { LookAtPoint(FVector(130.f, 100.f, 20.f)); LogFlow(TEXT("S7 after load")); });
	Schedule(86.5f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(60.f, -200.f, 100.f), FRotator(0.f, 90.f, 0.f)); } });
	Schedule(87.0f, [this]() { LookAtPoint(FVector(130.f, 100.f, 20.f)); });
	Schedule(88.0f, [this]() { Shot(TEXT("09_Nach_dem_Laden")); });
	Schedule(89.0f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleS()
{
	// Builds the line, feeds a PC, switches the machines on; later sees what T changed.
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Interaction = [this]() { AWECharacter* C = GetCharacter(); return C ? C->GetInteractionComponent() : nullptr; };
	Schedule(10.f, [PC]() { if (PC) { PC->ServerDevAddMoney(3000); PC->ServerDevTeleport(FVector(40.f, -60.f, 100.f), FRotator(0.f, 90.f, 0.f)); } });
	Schedule(11.f, [this]() { PlaceProductionLine(); });
	Schedule(12.f, [PC]()
	{
		if (PC)
		{
			PC->ServerDevTeleport(FVector(-170.f, -80.f, 100.f), FRotator(0.f, 0.f, 0.f));
			PC->ServerDevGiveItem(TEXT("OldComputer"), 1, EWEItemCondition::Defective);
		}
	});
	Schedule(13.f, [this, Interaction]()
	{
		LogFlow(TEXT("S0 line placed"));
		if (UWEInteractionComponent* I = Interaction())
		{
			I->RequestAlternativeInteractWith(FindMachine(TEXT("Shredder")));
			I->RequestAlternativeInteractWith(FindMachine(TEXT("Sorter")));
			I->RequestInteractWith(FindNearestItem(TEXT("Computer")));
		}
	});
	Schedule(14.f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(-150.f, 10.f, 100.f), FRotator(0.f, 0.f, 0.f)); } });
	Schedule(14.5f, [this]() { LookAtPoint(FVector(-70.f, 20.f, 45.f)); });
	Schedule(15.5f, [Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->UpdateFocus(); I->RequestPrimaryInteraction(); } });
	Schedule(16.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(140.f, 30.f, 100.f), FRotator(0.f, 90.f, 0.f)); } });
	Schedule(17.0f, [this, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestAlternativeInteractWith(FindMachine(TEXT("Sorter"))); } });
	Schedule(18.0f, [this]() { LogFlow(TEXT("S1 PC fed, shredder and sorter on")); });
	Schedule(38.f, [this]() { LogFlow(TEXT("S2 sees T's changes")); });
	Schedule(46.f, [this]() { LogFlow(TEXT("S3 final")); });
	Schedule(48.f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleT()
{
	// Watches the material move on the belts, changes the store output and stops/starts a belt.
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Interaction = [this]() { AWECharacter* C = GetCharacter(); return C ? C->GetInteractionComponent() : nullptr; };
	Schedule(10.f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(100.f, -60.f, 100.f), FRotator(0.f, 90.f, 0.f)); } });
	Schedule(17.f, [this]() { LogFlow(TEXT("T0 sees the line")); });
	for (const float At : { 30.5f, 31.0f, 31.5f, 32.0f })
	{
		Schedule(At, [this, At]() { LogFlow(*FString::Printf(TEXT("T1 belts at %.1f s"), At)); });
	}
	Schedule(33.f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(250.f, 30.f, 100.f), FRotator(0.f, 45.f, 0.f)); } });
	Schedule(34.f, [this, Interaction]()
	{
		TActorIterator<AWEProductionStorage> It(GetWorld());
		if (UWEInteractionComponent* I = Interaction(); I && It) { I->RequestSettingInteractWith(*It); }
	});
	Schedule(35.f, [this, Interaction]()
	{
		LogFlow(TEXT("T2 store output changed"));
		if (UWEInteractionComponent* I = Interaction()) { I->RequestAlternativeInteractWith(FindConveyorNear(FVector(235.f, 130.f, 0.f))); }
	});
	Schedule(36.5f, [this]() { LogFlow(TEXT("T3 stopped the belt to the store")); });
	Schedule(40.f, [this, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestAlternativeInteractWith(FindConveyorNear(FVector(235.f, 130.f, 0.f))); } });
	Schedule(42.f, [this]() { LogFlow(TEXT("T4 started it again")); });
	Schedule(47.f, [this]() { Quit(); });
}

// --- Phase 11: staff and logistics ------------------------------------------------------------------

AWEEmployee* UWEDevAutomationComponent::FindEmployee(uint8 InRole) const
{
	for (TActorIterator<AWEEmployee> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && static_cast<uint8>(It->GetProfile().Role) == InRole)
		{
			return *It;
		}
	}
	return nullptr;
}

void UWEDevAutomationComponent::LogStaff(const TCHAR* Step) const
{
	TArray<FString> People;
	for (TActorIterator<AWEEmployee> It(GetWorld()); It; ++It)
	{
		const FVector L = It->GetActorLocation();
		People.Add(FString::Printf(TEXT("%s(%s)@(%.0f,%.0f)[%s | %s | trägt %d %s]"), *It->GetProfile().Name, *It->GetRoleText().ToString(), L.X, L.Y,
			*It->GetTaskText().ToString(), *It->GetStateText().ToString(), It->GetCarried().IsValid() ? It->GetCarried().Quantity : 0,
			It->GetCarried().Definition ? *It->GetCarried().Definition->ItemId.ToString() : TEXT("-")));
	}
	FString Van = TEXT("-");
	for (TActorIterator<AWEVehicle> It(GetWorld()); It; ++It)
	{
		Van = FString::Printf(TEXT("%s unlocked=%d away=%d back_in=%.0fs cargo=%d tour=%s last='%s'"), *It->GetStats().DisplayName.ToString(), It->IsUnlocked() ? 1 : 0,
			It->IsAway() ? 1 : 0, It->GetSecondsToReturn(), It->GetStorage()->GetAllSlots().Num(), *It->GetSelectedTour().ToString(), *It->GetLastResult());
	}
	FString Forklift = TEXT("-");
	for (TActorIterator<AWEForklift> It(GetWorld()); It; ++It)
	{
		const FVector L = It->GetActorLocation();
		const FVector D = It->GetDriver() ? It->GetDriver()->GetActorLocation() : FVector::ZeroVector;
		Forklift = FString::Printf(TEXT("unlocked=%d driver=%s@(%.0f,%.0f) attached=%d @(%.0f,%.0f) load=%s"), It->IsUnlocked() ? 1 : 0, *GetNameSafe(It->GetDriver()), D.X, D.Y, It->GetAttachParentActor() ? 1 : 0, L.X, L.Y,
			It->GetDriver() && It->GetDriver()->GetHeldItem() ? *It->GetDriver()->GetHeldItem()->GetDisplayName().ToString() : TEXT("-"));
	}
	const UWEStaffComponent* Staff = UWEStaffComponent::Get(this);
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] %s: Balance=%s Wages/min=%s Owed=%s Applicants=%d Staff={%s} Van={%s} Forklift={%s}"), *Role, Step,
		Company ? *Company->GetBalance().ToDisplayString() : TEXT("?"), Staff ? *Staff->GetWagesPerMinute().ToDisplayString() : TEXT("?"),
		Staff ? *Staff->GetOwedWages().ToDisplayString() : TEXT("?"), Staff ? Staff->GetApplicants().Num() : -1,
		*FString::Join(People, TEXT(" ")), *Van, *Forklift);
}

void UWEDevAutomationComponent::BuyUpgradeAtTerminal(const TCHAR* UpgradeId)
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	if (PC)
	{
		PC->RequestBuyUpgrade(const_cast<AWETraderTerminal*>(FindTerminal()), FName(UpgradeId));
	}
}

void UWEDevAutomationComponent::TransferToStorage(const TCHAR* ItemId, UWEInventoryComponent* Storage)
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	const AWECharacter* Me = GetCharacter();
	if (!PC || !Me || !Storage || !Me->GetPlayerInventory())
	{
		return;
	}
	for (const FWEInventorySlotData& Slot : Me->GetPlayerInventory()->GetAllSlots())
	{
		if (Slot.Item.Definition && Slot.Item.Definition->ItemId == FName(ItemId))
		{
			PC->RequestQuickTransfer(Me->GetPlayerInventory(), Slot.SlotIndex, Storage);
			return;
		}
	}
}

void UWEDevAutomationComponent::RunStaffShots()
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Interaction = [this]() { AWECharacter* C = GetCharacter(); return C ? C->GetInteractionComponent() : nullptr; };
	auto Van = [this]() { TActorIterator<AWEVehicle> It(GetWorld()); return It ? *It : nullptr; };
	auto Forklift = [this]() { TActorIterator<AWEForklift> It(GetWorld()); return It ? *It : nullptr; };
	// Money, the van and the forklift, three employees (every role is among the first applicants).
	Schedule(3.0f, [this, PC]() { if (PC) { PC->ServerDevAddMoney(8000); } TeleportToTerminal(0.f); });
	Schedule(4.0f, [this, PC]()
	{
		BuyUpgradeAtTerminal(TEXT("Forklift"));
		BuyUpgradeAtTerminal(TEXT("OldVan"));
		if (PC)
		{
			AWETraderTerminal* Terminal = const_cast<AWETraderTerminal*>(FindTerminal());
			PC->RequestHire(Terminal, 0);
			PC->RequestHire(Terminal, 1);
			PC->RequestHire(Terminal, 2);
		}
	});
	Schedule(5.0f, [this, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(const_cast<AWETraderTerminal*>(FindTerminal())); } });
	Schedule(6.0f, [PC]() { if (PC && PC->GetTradeWidget()) { PC->GetTradeWidget()->ShowPage(2); } });
	Schedule(7.0f, [this]() { Shot(TEXT("01_Personal_und_Logistik")); LogStaff(TEXT("S0 hired")); });
	Schedule(7.5f, [PC]() { if (PC) { PC->CloseTrade(); } });
	// A small line: shelf with steel, press, shredder; the door open for the van.
	Schedule(8.0f, [this, PC]()
	{
		if (PC) { PC->ServerDevTeleport(FVector(-60.f, -60.f, 100.f), FRotator(0.f, -90.f, 0.f)); }
		if (UWEBuildComponent* B = GetBuild())
		{
			B->SetBuildMode(true);
			PlaceFurnitureAt(TEXT("HeavyShelf"), 60.f, -210.f, 90.f);
			PlaceFurnitureAt(TEXT("Press"), 60.f, -60.f, 0.f);
			PlaceFurnitureAt(TEXT("Shredder"), 150.f, 170.f, 0.f);
			LogFurniture(TEXT("S0b line placed"));
			B->SetBuildMode(false);
		}
	});
	Schedule(8.5f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(320.f, 0.f, 100.f), FRotator(0.f, 0.f, 0.f)); } });
	Schedule(9.0f, [this, PC, Interaction]()
	{
		if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(FindDoor()); }
		if (PC) { PC->ServerDevTeleport(FVector(-60.f, -60.f, 100.f), FRotator(0.f, -90.f, 0.f)); PC->ServerDevGiveItem(TEXT("SteelScrap"), 25, EWEItemCondition::Used); }
	});
	Schedule(10.0f, [this, PC]()
	{
		if (AWEPlaceholderProp* Shelf = FindFurniture(TEXT("HeavyShelf"))) { TransferToStorage(TEXT("SteelScrap"), Shelf->GetStorage()); }
		if (PC) { PC->ServerDevGiveItem(TEXT("SteelScrap"), 20, EWEItemCondition::Used); }
	});
	Schedule(11.0f, [this, PC, Van]()
	{
		if (AWEPlaceholderProp* Shelf = FindFurniture(TEXT("HeavyShelf"))) { TransferToStorage(TEXT("SteelScrap"), Shelf->GetStorage()); }
		AWEPlaceholderProp* Shelf = FindFurniture(TEXT("HeavyShelf"));
		AWEPlaceholderProp* Press = FindFurniture(TEXT("Press"));
		if (PC)
		{
			PC->RequestAssignTask(FindEmployee(static_cast<uint8>(EWEStaffRole::WarehouseWorker)), Shelf, Press);
			PC->RequestAssignTask(FindEmployee(static_cast<uint8>(EWEStaffRole::MachineOperator)), Shelf, Press);
			PC->RequestAssignTask(FindEmployee(static_cast<uint8>(EWEStaffRole::Driver)), nullptr, Van());
		}
	});
	// The assignment prompt (a second worker would be assigned like this).
	Schedule(12.0f, [this, Interaction]()
	{
		if (UWEInteractionComponent* I = Interaction()) { I->StartAssignment(FindEmployee(static_cast<uint8>(EWEStaffRole::WarehouseWorker))); }
		if (AWEPlaceholderProp* Shelf = FindFurniture(TEXT("HeavyShelf"))) { LookAt(Shelf); }
	});
	Schedule(13.0f, [this]() { Shot(TEXT("02_Aufgabe_zuweisen")); });
	Schedule(13.6f, [Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->CancelAssignment(); } });
	Schedule(16.0f, [this, PC]() { LogStaff(TEXT("S1 working")); if (PC) { PC->ServerDevTeleport(FVector(-100.f, -40.f, 100.f), FRotator(0.f, -30.f, 0.f)); } });
	Schedule(17.0f, [this]() { if (AWEEmployee* W = FindEmployee(static_cast<uint8>(EWEStaffRole::WarehouseWorker))) { LookAt(W); } });
	Schedule(17.5f, [this]() { Shot(TEXT("03_Lagerarbeiter")); LogStaff(TEXT("S2 worker")); });
	// Forklift: the 180-kg motor into the shredder.
	Schedule(18.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(-60.f, 60.f, 100.f), FRotator(0.f, 0.f, 0.f)); PC->ServerDevGiveItem(TEXT("IndustrialMotorLarge"), 1, EWEItemCondition::Defective); } });
	Schedule(19.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(-130.f, -215.f, 100.f), FRotator(0.f, 180.f, 0.f)); } });
	Schedule(20.0f, [this, Interaction, Forklift]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(Forklift()); } });
	Schedule(21.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(-80.f, 60.f, 100.f), FRotator(0.f, 0.f, 0.f)); } });
	Schedule(22.0f, [this]() { if (AActor* Motor = FindNearestItem(TEXT("Industriemotor"))) { LookAt(Motor); } });
	Schedule(22.5f, [this, Interaction]() { LogStaff(TEXT("S3 on the forklift")); if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(FindNearestItem(TEXT("Industriemotor"))); } });
	Schedule(23.0f, [this]() { LookYawPitch(0.f, -8.f); });
	Schedule(23.5f, [this]() { Shot(TEXT("04_Gabelstapler")); });
	Schedule(24.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(150.f, 20.f, 100.f), FRotator(0.f, 90.f, 0.f)); } });
	Schedule(25.0f, [this]() { LookAtPoint(FVector(150.f, 140.f, 80.f)); });
	Schedule(25.5f, [this]() { Shot(TEXT("05_Motor_zum_Schredder")); });
	Schedule(26.0f, [Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->UpdateFocus(); I->RequestPrimaryInteraction(); } });
	Schedule(27.0f, [this]() { LogStaff(TEXT("S4 motor in the shredder")); LogMachines(TEXT("S4 machines")); if (AWECharacter* C = GetCharacter()) { C->RequestExitForklift(); } });
	// Van: load a box, the driver takes it on a sell run.
	Schedule(28.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(490.f, 30.f, 100.f), FRotator(0.f, -90.f, 0.f)); PC->ServerDevGiveItem(TEXT("CableBox"), 1, EWEItemCondition::Used); } });
	Schedule(29.0f, [this, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(FindNearestItem(TEXT("Kabel"))); } });
	Schedule(29.6f, [this, PC, Van]()
	{
		if (AWEVehicle* V = Van(); V && PC && GetCharacter())
		{
			const FVector Dir = (V->GetActorLocation() - GetCharacter()->GetActorLocation()).GetSafeNormal2D();
			PC->ServerDevTeleport(V->GetActorLocation() - Dir * 190.f + FVector(0.f, 0.f, 100.f), Dir.Rotation());
		}
	});
	Schedule(30.0f, [this, Van]() { if (AWEVehicle* V = Van()) { LookAtPoint(V->GetActorLocation() + FVector(0.f, 0.f, 100.f)); } });
	Schedule(31.0f, [Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->UpdateFocus(); I->RequestPrimaryInteraction(); } });
	Schedule(36.0f, [this, Van]() { LogStaff(TEXT("S5 van loaded")); if (AWEVehicle* V = Van()) { LookAtPoint(V->GetActorLocation() + FVector(-120.f, 0.f, 110.f)); } });
	Schedule(37.0f, [this]() { Shot(TEXT("06_Tour_starten")); });
	Schedule(37.5f, [this, Interaction, Van]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestAlternativeInteractWith(Van()); } });
	Schedule(39.0f, [this]() { Shot(TEXT("07_Wagen_unterwegs")); LogStaff(TEXT("S6 van away")); });
	// Details of an employee; save, fire, load.
	Schedule(40.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(-100.f, -40.f, 100.f), FRotator(0.f, -30.f, 0.f)); } });
	Schedule(41.0f, [this, Interaction]()
	{
		if (AWEEmployee* W = FindEmployee(static_cast<uint8>(EWEStaffRole::MachineOperator))) { LookAt(W); }
		if (UWEInteractionComponent* I = Interaction()) { I->UpdateFocus(); I->ToggleExamine(); }
	});
	Schedule(42.0f, [this]() { Shot(TEXT("08_Mitarbeiter_Details")); });
	Schedule(42.5f, [this, PC, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->CloseExamine(); } if (PC) { PC->RequestSave(TEXT("DevP11")); } });
	Schedule(43.5f, [this, PC]() { if (PC) { PC->RequestStaffAction(nullptr, FindEmployee(static_cast<uint8>(EWEStaffRole::MachineOperator)), AWEPlayerController::StaffFire); } });
	Schedule(44.5f, [this, PC]() { LogStaff(TEXT("S7 operator fired")); if (PC) { PC->RequestLoad(TEXT("DevP11")); } });
	Schedule(46.0f, [this]() { LogStaff(TEXT("S8 after load")); if (AWEEmployee* W = FindEmployee(static_cast<uint8>(EWEStaffRole::MachineOperator))) { LookAt(W); } });
	Schedule(47.0f, [this]() { Shot(TEXT("09_Nach_dem_Laden")); });
	Schedule(48.0f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleU()
{
	// Buys van and forklift, hires, assigns the driver, loads the van and sends it off.
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Interaction = [this]() { AWECharacter* C = GetCharacter(); return C ? C->GetInteractionComponent() : nullptr; };
	auto Van = [this]() { TActorIterator<AWEVehicle> It(GetWorld()); return It ? *It : nullptr; };
	Schedule(10.f, [this, PC]() { if (PC) { PC->ServerDevAddMoney(8000); } TeleportToTerminal(-60.f); });
	Schedule(11.f, [this, PC]()
	{
		BuyUpgradeAtTerminal(TEXT("Forklift"));
		BuyUpgradeAtTerminal(TEXT("OldVan"));
		AWETraderTerminal* Terminal = const_cast<AWETraderTerminal*>(FindTerminal());
		if (PC) { PC->RequestHire(Terminal, 0); PC->RequestHire(Terminal, 1); PC->RequestHire(Terminal, 2); }
	});
	Schedule(13.f, [this, PC, Van, Interaction]()
	{
		LogStaff(TEXT("U0 hired"));
		if (PC)
		{
			PC->RequestAssignTask(FindEmployee(static_cast<uint8>(EWEStaffRole::Driver)), nullptr, Van());
			PC->ServerDevTeleport(FVector(330.f, 40.f, 100.f), FRotator(0.f, 0.f, 0.f));
		}
	});
	Schedule(14.f, [this, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(FindDoor()); } });
	Schedule(15.f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(560.f, -40.f, 100.f), FRotator(0.f, -90.f, 0.f)); PC->ServerDevGiveItem(TEXT("CableBox"), 1, EWEItemCondition::Used); } });
	Schedule(16.f, [this, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(FindNearestItem(TEXT("Kabel"))); } });
	// The van parks nose to the street since Phase 17.5: step up to its side and aim at its middle.
	Schedule(16.6f, [this, PC, Van]()
	{
		if (AWEVehicle* V = Van(); V && PC && GetCharacter())
		{
			const FVector Dir = (V->GetActorLocation() - GetCharacter()->GetActorLocation()).GetSafeNormal2D();
			const FVector At = V->GetActorLocation() - Dir * 190.f + FVector(0.f, 0.f, 100.f);
			PC->ServerDevTeleport(At, Dir.Rotation());
		}
	});
	Schedule(17.f, [this, Van]() { if (AWEVehicle* V = Van()) { LookAtPoint(V->GetActorLocation() + FVector(0.f, 0.f, 100.f)); } });
	Schedule(18.f, [Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->UpdateFocus(); I->RequestPrimaryInteraction(); } });
	Schedule(25.f, [this, Interaction, Van]() { LogStaff(TEXT("U1 van loaded, driver walking")); if (UWEInteractionComponent* I = Interaction()) { I->RequestAlternativeInteractWith(Van()); } });
	Schedule(27.f, [this]() { LogStaff(TEXT("U2 van started")); });
	Schedule(40.f, [this]() { LogStaff(TEXT("U3 sees V's changes")); });
	Schedule(43.f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleV()
{
	// Sees the staff, pauses the warehouse worker, drives the forklift around.
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Interaction = [this]() { AWECharacter* C = GetCharacter(); return C ? C->GetInteractionComponent() : nullptr; };
	auto Forklift = [this]() { TActorIterator<AWEForklift> It(GetWorld()); return It ? *It : nullptr; };
	Schedule(10.f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(100.f, -40.f, 100.f), FRotator(0.f, 90.f, 0.f)); } });
	Schedule(14.f, [this]() { LogStaff(TEXT("V0 sees the new staff")); });
	Schedule(20.f, [this]() { LogStaff(TEXT("V1 driver on his way")); });
	Schedule(28.f, [this]() { LogStaff(TEXT("V2 sees the van away")); });
	Schedule(29.f, [this, PC]()
	{
		AWEEmployee* Worker = FindEmployee(static_cast<uint8>(EWEStaffRole::WarehouseWorker));
		if (PC && Worker)
		{
			PC->ServerDevTeleport(Worker->GetActorLocation() + FVector(120.f, 0.f, 10.f), FRotator(0.f, 180.f, 0.f));
		}
	});
	Schedule(30.f, [this, PC]() { if (PC) { PC->RequestStaffAction(nullptr, FindEmployee(static_cast<uint8>(EWEStaffRole::WarehouseWorker)), AWEPlayerController::StaffPause); } });
	Schedule(31.f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(-130.f, -215.f, 100.f), FRotator(0.f, 180.f, 0.f)); } });
	Schedule(32.f, [this, Interaction, Forklift]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(Forklift()); } });
	Schedule(33.f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(100.f, 100.f, 100.f), FRotator(0.f, 0.f, 0.f)); } });
	Schedule(34.f, [this]() { LogStaff(TEXT("V3 drove the forklift")); });
	Schedule(35.f, [this]() { if (AWECharacter* C = GetCharacter()) { C->RequestExitForklift(); } });
	Schedule(36.5f, [this]() { LogStaff(TEXT("V4 got off")); });
	Schedule(44.f, [this]() { Quit(); });
}

// --- Phase 12: company expansion --------------------------------------------------------------------

AWEGarageDoor* UWEDevAutomationComponent::FindDoorNear(const FVector& Point) const
{
	AWEGarageDoor* Best = nullptr;
	for (TActorIterator<AWEGarageDoor> It(GetWorld()); It; ++It)
	{
		if (!Best || FVector::Dist2D(It->GetActorLocation(), Point) < FVector::Dist2D(Best->GetActorLocation(), Point))
		{
			Best = *It;
		}
	}
	return Best;
}

void UWEDevAutomationComponent::LogExpansion(const TCHAR* Step) const
{
	const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
	const UWEStaffComponent* Staff = UWEStaffComponent::Get(this);
	const UWEPowerGridComponent* Grid = UWEPowerGridComponent::Get(this);
	TArray<FString> Buildings;
	for (TActorIterator<AWEGarageBuilding> It(GetWorld()); It; ++It)
	{
		if (It->Tags.Num() > 0)
		{
			Buildings.Add(FString::Printf(TEXT("%s=%s"), *It->Tags[0].ToString(), It->IsHidden() ? TEXT("hidden") : TEXT("visible")));
		}
	}
	TArray<FString> Books;
	for (int32 i = 0; Company && i < static_cast<int32>(EWEBookingCategory::Count); ++i)
	{
		const int64 Cents = Company->GetCategoryTotalCents(static_cast<EWEBookingCategory>(i));
		if (Cents != 0)
		{
			Books.Add(FString::Printf(TEXT("%s:%.2f"), *UWECompanyComponent::GetCategoryName(static_cast<EWEBookingCategory>(i)).ToString(), Cents / 100.0));
		}
	}
	TArray<FString> Orders;
	for (const FWEContract& C : Contracts ? Contracts->GetContracts() : TArray<FWEContract>())
	{
		if (C.bIndustrial)
		{
			Orders.Add(FString::Printf(TEXT("'%s'[%s %d/%d]"), *C.Title.ToString(), C.State == EWEContractState::Accepted ? TEXT("accepted") : TEXT("offered"), C.RoundsDone, C.Rounds));
		}
	}
	const AWEMachine* Furnace = FindMachine(TEXT("Furnace"));
	UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] %s: Tier=%d(%s) Value=%s Balance=%s Power=%.1fkW MaxStaff=%d Buildings={%s} Books={%s} Industrial={%s} Furnace={%s %s ingots=%d}"),
		*Role, Step, Progression ? Progression->GetCompanyTier() : -1,
		Progression ? *UWEProgressionComponent::GetTierName(Progression->GetCompanyTier()).ToString() : TEXT("?"),
		Progression ? *Progression->GetCompanyValue().ToDisplayString() : TEXT("?"), Company ? *Company->GetBalance().ToDisplayString() : TEXT("?"),
		Grid ? Grid->GetCapacityKw() : -1.f, Staff ? Staff->GetMaxEmployees() : -1,
		*FString::Join(Buildings, TEXT(" ")), *FString::Join(Books, TEXT(" ")), *FString::Join(Orders, TEXT(" ")),
		Furnace ? TEXT("placed") : TEXT("-"), Furnace ? *Furnace->GetStatusText().ToString() : TEXT(""),
		Furnace && Furnace->GetOutputStorage() ? Furnace->GetOutputStorage()->CountItems(UWEItemDefinition::FindById(TEXT("AluminiumIngot"))) : 0);
}

// --- Phase 17: QA - a full late-game factory for performance, save/load at scale and stability -------

void UWEDevAutomationComponent::LogPerfWindow(const FString& Label, double Seconds, uint64 Frames)
{
	const FPlatformMemoryStats Mem = FPlatformMemory::GetStats();
	int32 Actors = 0;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		++Actors;
	}
	UE_LOG(LogWorkshopEmpire, Display, TEXT("PERF %s: %.1f fps (%.2f ms/frame), game %.2f ms, render %.2f ms, GPU %.2f ms, memory %.0f MB, actors %d, %d x %d"),
		*Label, Frames / FMath::Max(Seconds, 0.001), Seconds * 1000.0 / FMath::Max<uint64>(Frames, 1),
		FPlatformTime::ToMilliseconds(GGameThreadTime), FPlatformTime::ToMilliseconds(GRenderThreadTime), FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()),
		Mem.UsedPhysical / (1024.0 * 1024.0), Actors,
		GEngine && GEngine->GameViewport ? (int32)GEngine->GameViewport->Viewport->GetSizeXY().X : 0, GEngine && GEngine->GameViewport ? (int32)GEngine->GameViewport->Viewport->GetSizeXY().Y : 0);
}

FString UWEDevAutomationComponent::DescribeWorldCounts() const
{
	int32 Items = 0, Furniture = 0, Machines = 0, Running = 0, Employees = 0;
	for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It) { ++Items; }
	for (TActorIterator<AWEPlaceholderProp> It(GetWorld()); It; ++It) { Furniture += It->GetFurnitureId().IsNone() ? 0 : 1; }
	for (TActorIterator<AWEMachine> It(GetWorld()); It; ++It) { ++Machines; Running += It->GetStatus().State == EWEMachineState::Running ? 1 : 0; }
	for (TActorIterator<AWEEmployee> It(GetWorld()); It; ++It) { ++Employees; }
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	return FString::Printf(TEXT("items=%d furniture=%d machines=%d running=%d employees=%d balance=%s"), Items, Furniture, Machines, Running, Employees,
		Company ? *Company->GetBalance().ToDisplayString() : TEXT("-"));
}

void UWEDevAutomationComponent::RunStressShots()
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	// -WEStressMinutes=N: stability soak with the factory running N minutes before save/load.
	int32 Minutes = 0;
	FParse::Value(FCommandLine::Get(), TEXT("WEStressMinutes="), Minutes);
	const bool bLong = Minutes > 0;
	const float RunSeconds = bLong ? Minutes * 60.f : 40.f;
	// Everything a late-game company has: all buildings, truck, forklift.
	Schedule(3.0f, [this]()
	{
		UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
		for (const TCHAR* Id : { TEXT("LedLights"), TEXT("HeavyCurrent"), TEXT("Forklift"), TEXT("OldVan"), TEXT("ModernVan"), TEXT("WorkshopExtension"),
			TEXT("CommercialHall"), TEXT("Truck"), TEXT("FactoryHall") })
		{
			if (Progression) { Progression->GrantUpgrade(FName(Id)); }
		}
		if (UWECompanyComponent* Company = UWECompanyComponent::Get(this)) { Company->Deposit(FWEMoney::FromEuros(500000), TEXT("QA")); }
		UE_LOG(LogWorkshopEmpire, Display, TEXT("STRESS S0 buildings: %s"), *DescribeWorldCounts());
	});
	// 28 machines in the hall and the factory, each filled with a batch of its material and switched on.
	Schedule(4.0f, [this]()
	{
		UWEWorkshopLayoutComponent* Layout = UWEWorkshopLayoutComponent::Get(this);
		if (!Layout)
		{
			return;
		}
		struct FFill { const TCHAR* Machine; const TCHAR* Item; int32 Quantity; EWEItemCondition Condition; };
		const FFill Fills[] = {
			{ TEXT("Shredder"), TEXT("CableBox"), 6, EWEItemCondition::Used }, { TEXT("Sorter"), TEXT("ShredElectronics"), 40, EWEItemCondition::Used },
			{ TEXT("Press"), TEXT("SteelScrap"), 60, EWEItemCondition::Used }, { TEXT("Assembler"), TEXT("CopperGranulate"), 40, EWEItemCondition::Used },
			{ TEXT("Furnace"), TEXT("AluminiumScrap"), 50, EWEItemCondition::Used }, { TEXT("Refinery"), TEXT("ElectronicScrap"), 60, EWEItemCondition::Used } };
		auto Place = [&](const FFill& F, float X, float Y)
		{
			AWEPlaceholderProp* Prop = Layout->SpawnFurniture(FName(F.Machine), FTransform(FVector(X, Y, 0.f)));
			AWEMachine* Machine = Cast<AWEMachine>(Prop);
			const UWEItemDefinition* Def = UWEItemDefinition::FindById(FName(F.Item));
			if (!Machine || !Def)
			{
				return;
			}
			int32 Added = 0;
			for (int32 Left = F.Quantity; Left > 0;)
			{
				const int32 Stack = FMath::Min(Left, Def->GetMaxStackSize());
				Machine->GetStorage()->TryAddItem(FWEItemInstance::Make(const_cast<UWEItemDefinition*>(Def), Stack, F.Condition), Added);
				Left -= Stack;
			}
			Machine->SetPower(true);
		};
		for (int32 Row = 0; Row < 3; ++Row)
		{
			for (int32 Col = 0; Col < 6; ++Col)
			{
				Place(Fills[Col], 1150.f + Col * 210.f, 1050.f + Row * 520.f); // factory
			}
		}
		for (int32 Row = 0; Row < 2; ++Row)
		{
			for (int32 Col = 0; Col < 5; ++Col)
			{
				Place(Fills[Col], 1150.f + Col * 240.f, -700.f + Row * 560.f); // commercial hall
			}
		}
		UE_LOG(LogWorkshopEmpire, Display, TEXT("STRESS S1 machines: %s"), *DescribeWorldCounts());
	});
	// 20 employees: operators at the machines, the rest walking between them.
	Schedule(5.0f, [this]()
	{
		UWEStaffComponent* Staff = UWEStaffComponent::Get(this);
		TArray<AWEMachine*> Machines;
		for (TActorIterator<AWEMachine> It(GetWorld()); It; ++It) { Machines.Add(*It); }
		for (int32 i = 0; Staff && i < Staff->GetMaxEmployees() && Machines.Num() > 0; ++i)
		{
			FWEEmployeeProfile Profile;
			Profile.Name = FString::Printf(TEXT("QA-Mitarbeiter %d"), i + 1);
			Profile.Role = i % 2 == 0 ? EWEStaffRole::MachineOperator : EWEStaffRole::WarehouseWorker;
			Profile.WageCentsPerMinute = 70;
			AWEMachine* Machine = Machines[i % Machines.Num()];
			AWEEmployee* Employee = Staff->SpawnEmployee(Profile, FTransform(Machine->GetActorLocation() + FVector(-160.f, 0.f, 100.f)));
			if (!Employee)
			{
				continue;
			}
			FWEEmployeeTask Task;
			Task.Kind = Profile.Role == EWEStaffRole::MachineOperator ? EWETaskKind::OperateMachine : EWETaskKind::Transport;
			Task.Target = Profile.Role == EWEStaffRole::MachineOperator ? Machine : Machines[(i + 1) % Machines.Num()];
			Task.Source = Profile.Role == EWEStaffRole::MachineOperator ? nullptr : Machine;
			Employee->AssignTask(Task);
		}
		UE_LOG(LogWorkshopEmpire, Display, TEXT("STRESS S2 staff: %s"), *DescribeWorldCounts());
	});
	// 400 loose items in the yard (physics bodies at rest).
	Schedule(6.0f, [this]()
	{
		const TCHAR* Ids[] = { TEXT("SteelScrap"), TEXT("OldComputer"), TEXT("CrtMonitor"), TEXT("CableBox"), TEXT("ElectricMotor"), TEXT("PowerSupply"), TEXT("CopperCable") };
		FRandomStream Random(17);
		for (int32 i = 0; i < 400; ++i)
		{
			UWEItemDefinition* Def = UWEItemDefinition::FindById(FName(Ids[i % UE_ARRAY_COUNT(Ids)]));
			const FVector At(600.f + Random.FRandRange(0.f, 700.f), -500.f + Random.FRandRange(0.f, 1200.f), 60.f + (i % 4) * 40.f);
			if (Def) { AWEPickupItem::SpawnWorldItem(GetWorld(), FWEItemInstance::Make(Def, 1, EWEItemCondition::Defective), FTransform(At)); }
		}
		UE_LOG(LogWorkshopEmpire, Display, TEXT("STRESS S3 items: %s"), *DescribeWorldCounts());
	});
	// Measure from four places while everything runs.
	struct FView { FVector From; FVector Target; const TCHAR* Name; };
	const FView Views[] = {
		{ FVector(-250.f, -150.f, 100.f), FVector(300.f, 100.f, 120.f), TEXT("01_Garage") },
		{ FVector(900.f, -800.f, 200.f), FVector(1700.f, -300.f, 100.f), TEXT("02_Halle") },
		{ FVector(900.f, 900.f, 250.f), FVector(1800.f, 1800.f, 100.f), TEXT("03_Fabrik") },
		{ FVector(-300.f, 1200.f, 500.f), FVector(1200.f, 300.f, 0.f), TEXT("04_Gelaende") } };
	float T = 8.f;
	for (const FView& V : Views)
	{
		const FString Name(V.Name);
		const FVector From = V.From;
		const FVector Target = V.Target;
		Schedule(T, [PC, From]() { if (PC) { PC->ServerDevTeleport(From, FRotator::ZeroRotator); } });
		Schedule(T + 0.6f, [this, Target]() { LookAtPoint(Target); });
		Schedule(T + 1.5f, [this]() { PerfStart = FPlatformTime::Seconds(); PerfFrameStart = GFrameCounter; });
		Schedule(T + 4.5f, [this, Name]() { LogPerfWindow(Name, FPlatformTime::Seconds() - PerfStart, GFrameCounter - PerfFrameStart); Shot(Name); });
		T += 5.f;
	}
	// Long runs (stability): log every minute.
	const float RunEnd = T + RunSeconds;
	Schedule(T, [this]() { SoakTime = FPlatformTime::Seconds(); SoakFrame = GFrameCounter; });
	for (float At = T + 60.f; bLong && At < RunEnd; At += 60.f)
	{
		Schedule(At, [this, At]()
		{
			UE_LOG(LogWorkshopEmpire, Display, TEXT("STRESS minute %.0f: %s"), At / 60.f, *DescribeWorldCounts());
			LogPerfWindow(FString::Printf(TEXT("soak_%03.0f"), At / 60.f), FPlatformTime::Seconds() - SoakTime, GFrameCounter - SoakFrame);
			SoakTime = FPlatformTime::Seconds();
			SoakFrame = GFrameCounter;
		});
	}
	// Save and load the whole factory, compare.
	Schedule(RunEnd, [this]()
	{
		AWEGameMode* GM = GetWorld()->GetAuthGameMode<AWEGameMode>();
		const FString Before = DescribeWorldCounts();
		const double Start = FPlatformTime::Seconds();
		const bool bSaved = GM && GM->SaveToSlot(TEXT("QAStress"));
		const double SaveMs = (FPlatformTime::Seconds() - Start) * 1000.0;
		const int64 Bytes = IFileManager::Get().FileSize(*FWESaveSystem::GetSlotPath(TEXT("QAStress")));
		UE_LOG(LogWorkshopEmpire, Display, TEXT("STRESS S4 saved=%d in %.0f ms, %lld bytes: %s"), bSaved ? 1 : 0, SaveMs, Bytes, *Before);
		StressCountsBeforeLoad = Before;
	});
	Schedule(RunEnd + 2.f, [this]()
	{
		AWEGameMode* GM = GetWorld()->GetAuthGameMode<AWEGameMode>();
		const double Start = FPlatformTime::Seconds();
		const bool bLoaded = GM && GM->LoadFromSlot(TEXT("QAStress"));
		const double LoadMs = (FPlatformTime::Seconds() - Start) * 1000.0;
		UE_LOG(LogWorkshopEmpire, Display, TEXT("STRESS S5 loaded=%d in %.0f ms: %s"), bLoaded ? 1 : 0, LoadMs, *DescribeWorldCounts());
	});
	Schedule(RunEnd + 6.f, [this]()
	{
		// Items keep moving through the machines between save and load only by the 2 s, so counts must match closely.
		UE_LOG(LogWorkshopEmpire, Display, TEXT("STRESS S6 after load settle: %s | before save: %s"), *DescribeWorldCounts(), *StressCountsBeforeLoad);
		PerfStart = FPlatformTime::Seconds();
		PerfFrameStart = GFrameCounter;
	});
	Schedule(RunEnd + 9.f, [this]() { LogPerfWindow(TEXT("05_nach_Laden"), FPlatformTime::Seconds() - PerfStart, GFrameCounter - PerfFrameStart); Shot(TEXT("05_nach_Laden")); });
	Schedule(RunEnd + 10.f, [this]() { Quit(); });
}

// --- Phase 17: multiplayer stress - a client doing everything at once, for minutes ---------------------

void UWEDevAutomationComponent::RunSmokeRoleZ()
{
	// -WEStressMinutes=N (default 3): random actions twice a second, walking in between.
	int32 Minutes = 3;
	FParse::Value(FCommandLine::Get(), TEXT("WEStressMinutes="), Minutes);
	StressRandom.Initialize(static_cast<int32>(FCrc::StrCrc32(*GetNameSafe(GetOwner())) ^ static_cast<uint32>(FPlatformTime::Cycles())));
	Schedule(6.f, [this]()
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner())) { PC->ServerDevAddMoney(2000); }
		UE_LOG(LogWENet, Display, TEXT("SMOKE[Z] Z0 start: %s"), *DescribeStressState());
		FString Names;
		for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It) { Names += FString::Printf(TEXT("%s@%s, "), *It->GetItem().GetDisplayName().ToString(), *It->GetActorLocation().ToCompactString()); }
		UE_LOG(LogWENet, Display, TEXT("SMOKE[Z] items: %s"), *Names);
	});
	Schedule(7.f, [this]()
	{
		GetWorld()->GetTimerManager().SetTimer(StressActionTimer, this, &UWEDevAutomationComponent::StressAction, 0.5f, true);
		GetWorld()->GetTimerManager().SetTimer(StressMoveTimer, this, &UWEDevAutomationComponent::StressMove, 1.f / 30.f, true);
	});
	for (int32 M = 1; M <= Minutes; ++M)
	{
		Schedule(7.f + M * 60.f, [this, M]() { UE_LOG(LogWENet, Display, TEXT("SMOKE[Z] minute %d: actions=%d %s"), M, StressActions, *DescribeStressState()); });
	}
	Schedule(8.f + Minutes * 60.f, [this]()
	{
		GetWorld()->GetTimerManager().ClearTimer(StressActionTimer);
		GetWorld()->GetTimerManager().ClearTimer(StressMoveTimer);
		UE_LOG(LogWENet, Display, TEXT("SMOKE[Z] Z9 final: actions=%d %s"), StressActions, *DescribeStressState());
	});
	Schedule(11.f + Minutes * 60.f, [this]() { Quit(); });
}

FString UWEDevAutomationComponent::DescribeStressState() const
{
	const AWECharacter* C = GetCharacter();
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	int32 WorldItems = 0;
	for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It) { ++WorldItems; }
	const UWEInventoryComponent* Mine = GetMyInventory();
	const APlayerState* PS = C ? C->GetPlayerState() : nullptr;
	return FString::Printf(TEXT("balance=%s worldItems=%d mySlots=%d held=%s ping=%.0fms at=%s"), Company ? *Company->GetBalance().ToDisplayString() : TEXT("-"), WorldItems,
		Mine ? Mine->GetAllSlots().Num() : -1, C && C->GetHeldItem() ? *C->GetHeldItem()->GetDisplayName().ToString() : TEXT("-"),
		PS ? PS->GetPingInMilliseconds() : -1.f, C ? *C->GetActorLocation().ToCompactString() : TEXT("-"));
}

void UWEDevAutomationComponent::StressMove()
{
	if (AWECharacter* C = GetCharacter())
	{
		C->AddMovementInput(StressMoveDirection, 1.f);
	}
}

void UWEDevAutomationComponent::StressAction()
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	AWECharacter* C = GetCharacter();
	UWEInteractionComponent* I = C ? C->GetInteractionComponent() : nullptr;
	UWEInventoryComponent* Mine = GetMyInventory();
	if (!PC || !C || !I)
	{
		return;
	}
	++StressActions;
	StressMoveDirection = FVector(StressRandom.FRandRange(-1.f, 1.f), StressRandom.FRandRange(-1.f, 1.f), 0.f).GetSafeNormal();
	switch (StressRandom.RandRange(0, 9))
	{
	case 0: // pick up or drop
		if (C->GetHeldItem())
		{
			I->RequestPrimaryInteraction(); // [E] with something in the hands drops it
		}
		else
		{
			AWEPickupItem* Nearest = nullptr;
			float Best = 350.f;
			for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It)
			{
				const float D = FVector::Dist(It->GetActorLocation(), C->GetActorLocation());
				if (D < Best && !It->GetHolder()) { Best = D; Nearest = *It; }
			}
			if (Nearest) { I->RequestInteractWith(Nearest); }
		}
		break;
	case 1: // buy something small at the terminal (teleport there first)
		TeleportToTerminal(StressRandom.FRandRange(-60.f, 60.f));
		PC->RequestBuy(const_cast<AWETraderTerminal*>(FindTerminal()), StressRandom.RandRange(0, 8), 1);
		break;
	case 2: // sell a random slot
		if (Mine && Mine->GetAllSlots().Num() > 0)
		{
			TeleportToTerminal(StressRandom.FRandRange(-60.f, 60.f));
			const TArray<FWEInventorySlotData> Slots = Mine->GetAllSlots();
			const FWEInventorySlotData& Slot = Slots[StressRandom.RandRange(0, Slots.Num() - 1)];
			PC->RequestSell(const_cast<AWETraderTerminal*>(FindTerminal()), Mine, Slot.SlotIndex, 1);
		}
		break;
	case 3: // inventory shuffle and sort
		if (Mine && Mine->GetAllSlots().Num() > 0)
		{
			const FWEInventorySlotData Slot = Mine->GetAllSlots()[0];
			PC->RequestMoveItem(Mine, Slot.SlotIndex, Mine, StressRandom.RandRange(0, Mine->GetNumSlots() - 1), Slot.Item.Quantity);
			PC->RequestSort(Mine);
		}
		break;
	case 4: // drop something from the inventory
		if (Mine && Mine->GetAllSlots().Num() > 2)
		{
			PC->RequestDropFromInventory(Mine, Mine->GetAllSlots().Last().SlotIndex, 1);
		}
		break;
	case 5: // the door
		PC->ServerDevTeleport(FVector(330.f, StressRandom.FRandRange(-150.f, 150.f), 100.f), FRotator::ZeroRotator);
		I->RequestInteractWith(FindDoor());
		break;
	case 6: // a new random place in the garage or the yard
		PC->ServerDevTeleport(FVector(StressRandom.FRandRange(-300.f, 900.f), StressRandom.FRandRange(-250.f, 250.f), 100.f), FRotator(0.f, StressRandom.FRandRange(0.f, 360.f), 0.f));
		break;
	case 7: // something to carry around
		PC->ServerDevGiveItem(StressRandom.FRand() < 0.5f ? TEXT("SteelScrap") : TEXT("CopperCable"), StressRandom.RandRange(1, 5), EWEItemCondition::Used);
		break;
	default: // just walk (direction changed above)
		break;
	}
}

// --- Phase 17: UI and input test - every screen at the current resolution, every key through the real input path ---

void UWEDevAutomationComponent::PressKey(const FKey& Key, bool bDown)
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (PC && GEngine && GEngine->GameViewport)
	{
		// The same path as a real key: viewport -> PlayerController -> Enhanced Input.
		PC->InputKey(FInputKeyEventArgs(GEngine->GameViewport->Viewport, IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(), Key, bDown ? IE_Pressed : IE_Released, FPlatformTime::Cycles64()));
	}
}

void UWEDevAutomationComponent::TapKey(const FKey& Key)
{
	// Down now, up a few frames later - like a finger (down and up in one frame never triggers an action).
	PressKey(Key, true);
	Schedule(0.15f, [this, Key]() { PressKey(Key, false); });
}

void UWEDevAutomationComponent::PressSlateKey(const FKey& Key)
{
	// Keys that open widgets answer through Slate (the widget has the keyboard focus).
	if (FSlateApplication::IsInitialized())
	{
		const FKeyEvent Event(Key, FModifierKeysState(), 0, false, 0, 0);
		FSlateApplication::Get().ProcessKeyDownEvent(Event);
		FSlateApplication::Get().ProcessKeyUpEvent(Event);
	}
}

void UWEDevAutomationComponent::LogUiFit(const TCHAR* Name, UUserWidget* Widget)
{
	if (!Widget || !GEngine || !GEngine->GameViewport)
	{
		UE_LOG(LogWorkshopEmpire, Display, TEXT("UI %s: not open"), Name);
		return;
	}
	Widget->ForceLayoutPrepass();
	const FVector2D Desired = Widget->GetDesiredSize();
	FVector2D Viewport;
	GEngine->GameViewport->GetViewportSize(Viewport);
	const float Scale = UWidgetLayoutLibrary::GetViewportScale(Widget);
	const FVector2D Needed = Desired * Scale;
	const bool bFits = Needed.X <= Viewport.X + 1.f && Needed.Y <= Viewport.Y + 1.f;
	UE_LOG(LogWorkshopEmpire, Display, TEXT("UI %s: needs %.0f x %.0f px (DPI scale %.2f), viewport %.0f x %.0f -> %s"), Name, Needed.X, Needed.Y, Scale,
		Viewport.X, Viewport.Y, bFits ? TEXT("fits") : TEXT("TOO BIG"));
}

void UWEDevAutomationComponent::RunUiInputShots()
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Character = [this]() { return GetCharacter(); };
	auto Check = [](const TCHAR* What, bool bOk) { UE_LOG(LogWorkshopEmpire, Display, TEXT("INPUT %s: %s"), What, bOk ? TEXT("ok") : TEXT("FAIL")); };
	// In the main menu: the menu itself, then done.
	if (GetWorld()->GetMapName().Contains(TEXT("MainMenu")))
	{
		Schedule(3.0f, [this]()
		{
			TArray<UUserWidget*> Found;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Found, UUserWidget::StaticClass(), true);
			for (UUserWidget* W : Found) { LogUiFit(*W->GetClass()->GetName(), W); }
			Shot(TEXT("00_Hauptmenue"));
		});
		Schedule(4.0f, [this]() { Quit(); });
		return;
	}
	Schedule(3.0f, [this, PC]() { if (PC) { PC->ServerDevTeleport(FVector(-170.f, 0.f, 100.f), FRotator(0.f, 0.f, 0.f)); } });
	Schedule(4.0f, [this]() { Shot(TEXT("01_HUD")); });
	// Walking (W), sprinting (Shift), jumping (Space), crouching (Ctrl).
	Schedule(4.5f, [this, Character]() { StressMoveDirection = Character() ? Character()->GetActorLocation() : FVector::ZeroVector; PressKey(EKeys::W, true); });
	Schedule(5.5f, [this, Character, Check]()
	{
		PressKey(EKeys::W, false);
		Check(TEXT("W walks forward"), Character() && FVector::Dist2D(Character()->GetActorLocation(), StressMoveDirection) > 100.f);
	});
	Schedule(6.0f, [this, Character]() { StressMoveDirection = Character() ? Character()->GetActorLocation() : FVector::ZeroVector; PressKey(EKeys::S, true); PressKey(EKeys::LeftShift, true); });
	Schedule(6.6f, [this, Character, Check]()
	{
		Check(TEXT("S + Shift sprints backwards"), Character() && Character()->GetVelocity().Size2D() > 400.f);
		PressKey(EKeys::S, false);
		PressKey(EKeys::LeftShift, false);
	});
	Schedule(7.2f, [this]() { PressKey(EKeys::A, true); });
	Schedule(7.6f, [this, Character, Check]() { Check(TEXT("A strafes"), Character() && Character()->GetVelocity().Size2D() > 100.f); PressKey(EKeys::A, false); });
	Schedule(7.8f, [this]() { PressKey(EKeys::D, true); });
	Schedule(8.2f, [this, Character, Check]() { Check(TEXT("D strafes"), Character() && Character()->GetVelocity().Size2D() > 100.f); PressKey(EKeys::D, false); });
	Schedule(8.6f, [this]() { PressKey(EKeys::SpaceBar, true); });
	Schedule(8.75f, [this, Character, Check]() { Check(TEXT("Space jumps"), Character() && Character()->GetCharacterMovement()->IsFalling()); PressKey(EKeys::SpaceBar, false); });
	Schedule(9.6f, [this]() { PressKey(EKeys::LeftControl, true); });
	Schedule(10.0f, [this, Character, Check]() { Check(TEXT("Ctrl crouches"), Character() && Character()->bIsCrouched); PressKey(EKeys::LeftControl, false); });
	Schedule(10.6f, [Character, Check]() { Check(TEXT("Ctrl released stands up"), Character() && !Character()->bIsCrouched); });
	// Inventory (Tab, I), closed again with Tab through the widget.
	Schedule(11.0f, [this]() { TapKey(EKeys::Tab); });
	Schedule(11.5f, [this, PC, Check]()
	{
		Check(TEXT("Tab opens the inventory"), PC && PC->IsInventoryOpen());
		TArray<UUserWidget*> Found;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Found, UWEInventoryWidget::StaticClass(), false);
		LogUiFit(TEXT("Inventar"), Found.Num() > 0 ? Found[0] : nullptr);
		Shot(TEXT("02_Inventar"));
	});
	Schedule(12.2f, [this]() { PressSlateKey(EKeys::Tab); });
	Schedule(12.6f, [this, PC, Check]()
	{
		Check(TEXT("Tab in the inventory closes it"), PC && !PC->IsInventoryOpen());
		if (PC && PC->IsInventoryOpen()) { PC->CloseInventory(); }
		TapKey(EKeys::I);
	});
	Schedule(13.0f, [PC, Check]() { Check(TEXT("I opens the inventory"), PC && PC->IsInventoryOpen()); if (PC) { PC->CloseInventory(); } });
	// The market terminal with [E], all four pages, Esc closes.
	Schedule(13.5f, [this]() { TeleportToTerminal(0.f); });
	Schedule(14.3f, [this]() { if (AWECharacter* C = GetCharacter()) { if (UWEInteractionComponent* I = C->GetInteractionComponent()) { LookAt(FindTerminal()); I->UpdateFocus(); } } TapKey(EKeys::E); });
	for (int32 Page = 0; Page < 4; ++Page)
	{
		const float At = 15.0f + Page * 1.2f;
		Schedule(At, [this, PC, Page, Check]()
		{
			if (Page == 0) { Check(TEXT("E at the terminal opens the market"), PC && PC->IsTradeOpen()); }
			if (PC && PC->GetTradeWidget()) { PC->GetTradeWidget()->ShowPage(Page); }
		});
		Schedule(At + 0.8f, [this, PC, Page]()
		{
			LogUiFit(*FString::Printf(TEXT("Marktplatz Seite %d"), Page + 1), PC ? PC->GetTradeWidget() : nullptr);
			Shot(FString::Printf(TEXT("03_Marktplatz_%d"), Page + 1));
		});
	}
	Schedule(20.0f, [this]() { PressSlateKey(EKeys::Escape); });
	Schedule(20.5f, [PC, Check]() { Check(TEXT("Esc closes the market"), PC && !PC->IsTradeOpen()); if (PC && PC->IsTradeOpen()) { PC->CloseTrade(); } });
	// Build mode (B), turn (R), leave (B).
	Schedule(21.0f, [this, PC]() { if (PC) { PC->ServerDevTeleport(FVector(-100.f, 0.f, 100.f), FRotator(0.f, 0.f, 0.f)); } });
	Schedule(21.6f, [this]() { TapKey(EKeys::B); });
	Schedule(22.2f, [this, Check]()
	{
		Check(TEXT("B starts build mode"), GetBuild() && GetBuild()->IsActive());
		Shot(TEXT("04_Baumodus"));
	});
	Schedule(23.0f, [this]() { TapKey(EKeys::B); });
	Schedule(23.5f, [this, Check]() { Check(TEXT("B ends build mode"), GetBuild() && !GetBuild()->IsActive()); });
	// Esc: menu in the game.
	Schedule(24.0f, [this]() { TapKey(EKeys::Escape); });
	Schedule(24.6f, [this, PC, Check]()
	{
		Check(TEXT("Esc opens the menu"), PC && PC->IsPauseMenuOpen());
		LogUiFit(TEXT("Menue im Spiel"), PC ? PC->GetPauseMenu() : nullptr);
		Shot(TEXT("05_Menue"));
	});
	Schedule(25.4f, [this]() { PressSlateKey(EKeys::Escape); });
	Schedule(25.9f, [PC, Check]() { Check(TEXT("Esc in the menu closes it"), PC && !PC->IsPauseMenuOpen()); if (PC && PC->IsPauseMenuOpen()) { PC->ClosePauseMenu(); } });
	// F5 saves (host).
	Schedule(26.5f, [this]() { IFileManager::Get().Delete(*FWESaveSystem::GetSlotPath(TEXT("Quicksave")), false, true, true); TapKey(EKeys::F5); });
	Schedule(27.5f, [Check]() { Check(TEXT("F5 quick-saves"), IFileManager::Get().FileExists(*FWESaveSystem::GetSlotPath(TEXT("Quicksave")))); });
	// Examine (F) on the workbench.
	Schedule(28.0f, [this, PC]() { if (PC) { PC->ServerDevTeleport(FVector(-200.f, 0.f, 100.f), FRotator(0.f, 180.f, 0.f)); } });
	Schedule(28.6f, [this]() { LookAtPoint(FVector(-355.f, 0.f, 90.f)); if (AWECharacter* C = GetCharacter()) { if (UWEInteractionComponent* I = C->GetInteractionComponent()) { I->UpdateFocus(); } } TapKey(EKeys::F); });
	Schedule(29.3f, [this]() { Shot(TEXT("06_Untersuchen")); });
	Schedule(30.0f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunBalanceShots()
{
	// Phase 16: tiered tours, the truck, the refinery in the factory, new orders and the milestones.
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Interaction = [this]() { AWECharacter* C = GetCharacter(); return C ? C->GetInteractionComponent() : nullptr; };
	auto Terminal = [this]() { return const_cast<AWETraderTerminal*>(FindTerminal()); };
	auto Van = [this]() { TActorIterator<AWEVehicle> It(GetWorld()); return It ? *It : nullptr; };
	auto SelectTour = [Van](const TCHAR* Id)
	{
		AWEVehicle* V = Van();
		for (int32 i = 0; V && i < 12 && V->GetSelectedTour() != FName(Id); ++i)
		{
			V->CycleTour();
		}
	};
	auto LogVan = [Van](const TCHAR* Step)
	{
		if (const AWEVehicle* V = Van())
		{
			UE_LOG(LogWorkshopEmpire, Display, TEXT("BALANCE %s: %s speed=%.1f slots=%d tour=%s block=\"%s\" at %s hidden=%d"), Step, *V->GetStats().DisplayName.ToString(),
				V->GetStats().TourSpeed, V->GetStats().CargoSlots, *V->GetSelectedTour().ToString(), *V->GetStartBlockReason().ToString(), *V->GetActorLocation().ToString(), V->IsHidden() ? 1 : 0);
		}
	};
	Schedule(3.0f, [this, PC]() { if (PC) { PC->ServerDevAddMoney(200000); } TeleportToTerminal(0.f); });
	Schedule(4.0f, [this]() { BuyUpgradeAtTerminal(TEXT("OldVan")); BuyUpgradeAtTerminal(TEXT("WorkshopExtension")); });
	// The van with the big office clearance selected: still locked at tier 2.
	Schedule(5.0f, [PC, SelectTour]() { SelectTour(TEXT("PickupOfficeClearance")); if (PC) { PC->ServerDevTeleport(FVector(760.f, -470.f, 100.f), FRotator(0.f, 90.f, 0.f)); } });
	Schedule(5.8f, [this, Interaction, Van]() { if (AWEVehicle* V = Van()) { LookAtPoint(V->GetActorLocation() + FVector(0.f, 0.f, 110.f)); } if (UWEInteractionComponent* I = Interaction()) { I->UpdateFocus(); I->ToggleExamine(); } });
	Schedule(6.8f, [this, LogVan]() { Shot(TEXT("01_Tour_gesperrt")); LogVan(TEXT("tier 2")); });
	Schedule(7.2f, [Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->CloseExamine(); } });
	// Hall, truck and factory.
	Schedule(7.5f, [this]() { TeleportToTerminal(0.f); });
	Schedule(8.5f, [this]() { BuyUpgradeAtTerminal(TEXT("CommercialHall")); BuyUpgradeAtTerminal(TEXT("Truck")); BuyUpgradeAtTerminal(TEXT("FactoryHall")); });
	Schedule(9.5f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(760.f, -470.f, 100.f), FRotator(0.f, 90.f, 0.f)); } });
	Schedule(10.3f, [this, Interaction, Van]() { if (AWEVehicle* V = Van()) { LookAtPoint(V->GetActorLocation() + FVector(0.f, 0.f, 110.f)); } if (UWEInteractionComponent* I = Interaction()) { I->UpdateFocus(); I->ToggleExamine(); } });
	Schedule(11.3f, [this, LogVan]() { Shot(TEXT("02_Lkw")); LogVan(TEXT("tier 4 with truck")); });
	Schedule(11.7f, [Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->CloseExamine(); } });
	// The refinery in the factory, fed with electronic scrap.
	Schedule(12.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(880.f, 1650.f, 100.f), FRotator::ZeroRotator); } });
	Schedule(12.5f, [this, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(FindDoorNear(FVector(990.f, 1650.f, 0.f))); } });
	Schedule(14.5f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(1200.f, 1650.f, 100.f), FRotator::ZeroRotator); } });
	Schedule(15.0f, [this]()
	{
		if (UWEBuildComponent* B = GetBuild())
		{
			B->SetBuildMode(true);
			PlaceFurnitureAt(TEXT("Refinery"), 1500.f, 1300.f, 0.f);
			LogFurniture(TEXT("BALANCE refinery placed"));
			B->SetBuildMode(false);
		}
	});
	Schedule(16.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(1300.f, 1300.f, 100.f), FRotator::ZeroRotator); PC->ServerDevGiveItem(TEXT("ElectronicScrap"), 40, EWEItemCondition::Used); } });
	Schedule(17.0f, [this, Interaction]()
	{
		TransferToMachine(TEXT("ElectronicScrap"), FindMachine(TEXT("Refinery")));
		if (UWEInteractionComponent* I = Interaction()) { I->RequestAlternativeInteractWith(FindMachine(TEXT("Refinery"))); }
	});
	Schedule(19.0f, [this, Interaction]() { if (AWEMachine* M = FindMachine(TEXT("Refinery"))) { LookAt(M); } if (UWEInteractionComponent* I = Interaction()) { I->UpdateFocus(); I->ToggleExamine(); } });
	Schedule(20.0f, [this]() { Shot(TEXT("03_Edelmetall")); });
	Schedule(20.5f, [Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->CloseExamine(); } });
	Schedule(52.0f, [this]()
	{
		const AWEMachine* M = FindMachine(TEXT("Refinery"));
		FString Out;
		for (const FWEInventorySlotData& Slot : M ? M->GetOutputStorage()->GetAllSlots() : TArray<FWEInventorySlotData>())
		{
			Out += FString::Printf(TEXT("%s x%d, "), *Slot.Item.GetDisplayName().ToString(), Slot.Item.Quantity);
		}
		UE_LOG(LogWorkshopEmpire, Display, TEXT("BALANCE refinery output: %s"), *Out);
	});
	// Orders and milestones at the terminal.
	Schedule(53.0f, [this]() { TeleportToTerminal(0.f); });
	Schedule(54.0f, [Interaction, Terminal]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(Terminal()); } });
	Schedule(54.5f, [PC]() { if (PC && PC->GetTradeWidget()) { PC->GetTradeWidget()->ShowPage(0); } });
	Schedule(55.5f, [this]() { Shot(TEXT("04_Auftraege")); });
	Schedule(56.0f, [PC]() { if (PC && PC->GetTradeWidget()) { PC->GetTradeWidget()->ShowPage(3); } });
	Schedule(57.0f, [this]()
	{
		Shot(TEXT("05_Meilensteine"));
		if (const UWEMilestoneComponent* Milestones = UWEMilestoneComponent::Get(this))
		{
			UE_LOG(LogWorkshopEmpire, Display, TEXT("BALANCE milestones %d reached, machine cycles %d"), Milestones->GetCompleted().Num(), Milestones->GetStats().MachineCycles);
		}
	});
	Schedule(58.5f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunExpansionShots()
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Interaction = [this]() { AWECharacter* C = GetCharacter(); return C ? C->GetInteractionComponent() : nullptr; };
	auto Terminal = [this]() { return const_cast<AWETraderTerminal*>(FindTerminal()); };
	// Before: the small garage.
	Schedule(3.0f, [this, PC]() { LogExpansion(TEXT("S0 start")); if (PC) { PC->ServerDevAddMoney(150000); PC->ServerDevTeleport(FVector(620.f, 650.f, 100.f), FRotator::ZeroRotator); } });
	Schedule(4.0f, [this]() { LookAtPoint(FVector(1500.f, -200.f, 200.f)); });
	Schedule(4.5f, [this]() { Shot(TEXT("01_Hof_vorher")); });
	// Buy the workshop extension and the commercial hall at the terminal.
	Schedule(5.0f, [this]() { TeleportToTerminal(0.f); });
	Schedule(6.0f, [this]() { BuyUpgradeAtTerminal(TEXT("WorkshopExtension")); BuyUpgradeAtTerminal(TEXT("CommercialHall")); });
	Schedule(7.0f, [this, Interaction, Terminal]() { LogExpansion(TEXT("S1 bought extension and hall")); if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(Terminal()); } });
	Schedule(7.5f, [PC]() { if (PC && PC->GetTradeWidget()) { PC->GetTradeWidget()->ShowPage(3); } });
	Schedule(8.5f, [this]() { Shot(TEXT("02_Unternehmen")); });
	Schedule(9.0f, [PC]() { if (PC) { PC->CloseTrade(); PC->ServerDevTeleport(FVector(250.f, 1150.f, 100.f), FRotator::ZeroRotator); } });
	Schedule(10.0f, [this]() { LookAtPoint(FVector(1500.f, -200.f, 300.f)); });
	Schedule(10.5f, [this]() { Shot(TEXT("03_Gewerbehalle_aussen")); });
	Schedule(11.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(800.f, -1400.f, 100.f), FRotator(0.f, 150.f, 0.f)); } });
	Schedule(11.5f, [this]() { LookAtPoint(FVector(0.f, -760.f, 120.f)); });
	Schedule(12.0f, [this]() { Shot(TEXT("04_Werkstatt_Erweiterung")); });
	// Into the hall: open its door, set up a furnace, a packing machine, a high rack and a big material store.
	Schedule(12.5f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(880.f, -200.f, 100.f), FRotator::ZeroRotator); } });
	Schedule(13.0f, [this, Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(FindDoorNear(FVector(990.f, -200.f, 0.f))); } });
	Schedule(15.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(1200.f, -200.f, 100.f), FRotator::ZeroRotator); } });
	Schedule(15.5f, [this]()
	{
		if (UWEBuildComponent* B = GetBuild())
		{
			B->SetBuildMode(true);
			PlaceFurnitureAt(TEXT("Furnace"), 1450.f, -500.f, 0.f);
			PlaceFurnitureAt(TEXT("Packer"), 1450.f, 120.f, 0.f);
			PlaceFurnitureAt(TEXT("BigMaterialStore"), 1850.f, -750.f, 0.f);
			PlaceFurnitureAt(TEXT("HighRack"), 2100.f, -200.f, 90.f);
			LogFurniture(TEXT("S2 hall furnished"));
			B->SetBuildMode(false);
		}
	});
	Schedule(16.5f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(1250.f, -320.f, 100.f), FRotator(0.f, -90.f, 0.f)); PC->ServerDevGiveItem(TEXT("AluminiumScrap"), 10, EWEItemCondition::Used); } });
	Schedule(17.5f, [this, Interaction]()
	{
		TransferToMachine(TEXT("AluminiumScrap"), FindMachine(TEXT("Furnace")));
		if (UWEInteractionComponent* I = Interaction()) { I->RequestAlternativeInteractWith(FindMachine(TEXT("Furnace"))); }
	});
	Schedule(19.0f, [this, PC]() { LogExpansion(TEXT("S3 furnace running")); if (PC) { PC->ServerDevTeleport(FVector(1150.f, 300.f, 100.f), FRotator(0.f, -45.f, 0.f)); } });
	Schedule(19.5f, [this]() { LookAtPoint(FVector(1800.f, -400.f, 120.f)); });
	Schedule(20.0f, [this]() { Shot(TEXT("05_Halle_innen")); });
	Schedule(20.5f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(1250.f, -320.f, 100.f), FRotator(0.f, -90.f, 0.f)); } });
	Schedule(21.0f, [this, Interaction]() { if (AWEMachine* M = FindMachine(TEXT("Furnace"))) { LookAt(M); } if (UWEInteractionComponent* I = Interaction()) { I->UpdateFocus(); I->ToggleExamine(); } });
	Schedule(22.0f, [this]() { Shot(TEXT("06_Schmelzofen")); });
	Schedule(22.5f, [Interaction]() { if (UWEInteractionComponent* I = Interaction()) { I->CloseExamine(); } });
	// Industrial orders: the terminal's order board after the hall.
	Schedule(23.0f, [this]() { TeleportToTerminal(0.f); });
	Schedule(24.0f, [this, Interaction, Terminal]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(Terminal()); } });
	Schedule(24.5f, [this, PC, Terminal]()
	{
		const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
		for (const FWEContract& C : Contracts ? Contracts->GetContracts() : TArray<FWEContract>())
		{
			if (C.bIndustrial && C.State == EWEContractState::Offered && PC)
			{
				PC->RequestAcceptContract(Terminal(), C.Id);
				break;
			}
		}
		if (PC && PC->GetTradeWidget()) { PC->GetTradeWidget()->ShowPage(0); }
	});
	Schedule(25.5f, [this]() { Shot(TEXT("07_Industrieauftraege")); LogExpansion(TEXT("S4 industrial order accepted")); });
	// Wait for the furnace (20 s), then the factory.
	Schedule(26.0f, [PC]() { if (PC) { PC->CloseTrade(); } });
	Schedule(40.0f, [this]() { LogExpansion(TEXT("S5 ingots")); BuyUpgradeAtTerminal(TEXT("FactoryHall")); });
	Schedule(41.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(-250.f, 2700.f, 100.f), FRotator::ZeroRotator); } });
	Schedule(41.5f, [this]() { LookAtPoint(FVector(1500.f, 900.f, 300.f)); });
	Schedule(42.0f, [this]() { Shot(TEXT("08_Fabrikhalle")); LogExpansion(TEXT("S6 factory")); });
	Schedule(42.5f, [this]() { TeleportToTerminal(0.f); });
	Schedule(43.5f, [this, Interaction, Terminal]() { if (UWEInteractionComponent* I = Interaction()) { I->RequestInteractWith(Terminal()); } });
	Schedule(44.0f, [PC]() { if (PC && PC->GetTradeWidget()) { PC->GetTradeWidget()->ShowPage(3); } });
	Schedule(45.0f, [this]() { Shot(TEXT("09_Unternehmen_Fabrik")); });
	// Save, load: tier, buildings, books and orders come back.
	Schedule(45.5f, [PC]() { if (PC) { PC->CloseTrade(); PC->RequestSave(TEXT("DevP12")); } });
	Schedule(46.5f, [PC]() { if (PC) { PC->RequestLoad(TEXT("DevP12")); } });
	Schedule(48.5f, [this, PC]() { LogExpansion(TEXT("S7 after load")); if (PC) { PC->ServerDevTeleport(FVector(-250.f, 2700.f, 100.f), FRotator::ZeroRotator); } });
	Schedule(49.5f, [this]() { LookAtPoint(FVector(1500.f, 600.f, 300.f)); });
	Schedule(50.0f, [this]() { Shot(TEXT("10_Nach_dem_Laden")); });
	Schedule(51.0f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleW()
{
	// Buys the extension and the hall, accepts an industrial order, sets up and runs a furnace in the hall.
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Interaction = [this]() { AWECharacter* C = GetCharacter(); return C ? C->GetInteractionComponent() : nullptr; };
	Schedule(10.f, [this, PC]() { LogExpansion(TEXT("W0 start")); if (PC) { PC->ServerDevAddMoney(40000); } TeleportToTerminal(-60.f); });
	Schedule(11.f, [this]() { BuyUpgradeAtTerminal(TEXT("WorkshopExtension")); BuyUpgradeAtTerminal(TEXT("CommercialHall")); });
	Schedule(13.f, [this]() { LogExpansion(TEXT("W1 bought")); });
	Schedule(18.f, [this, PC]()
	{
		const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
		for (const FWEContract& C : Contracts ? Contracts->GetContracts() : TArray<FWEContract>())
		{
			if (C.bIndustrial && C.State == EWEContractState::Offered && PC)
			{
				PC->RequestAcceptContract(const_cast<AWETraderTerminal*>(FindTerminal()), C.Id);
				break;
			}
		}
	});
	Schedule(19.f, [this, PC]() { LogExpansion(TEXT("W2 industrial order accepted")); if (PC) { PC->ServerDevTeleport(FVector(1200.f, -200.f, 100.f), FRotator::ZeroRotator); } });
	Schedule(20.f, [this]()
	{
		if (UWEBuildComponent* B = GetBuild())
		{
			B->SetBuildMode(true);
			PlaceFurnitureAt(TEXT("Furnace"), 1450.f, -500.f, 0.f);
			B->SetBuildMode(false);
		}
	});
	Schedule(21.f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(1250.f, -320.f, 100.f), FRotator(0.f, -90.f, 0.f)); PC->ServerDevGiveItem(TEXT("AluminiumScrap"), 10, EWEItemCondition::Used); } });
	Schedule(22.f, [this, Interaction]()
	{
		TransferToMachine(TEXT("AluminiumScrap"), FindMachine(TEXT("Furnace")));
		if (UWEInteractionComponent* I = Interaction()) { I->RequestAlternativeInteractWith(FindMachine(TEXT("Furnace"))); }
	});
	Schedule(24.f, [this]() { LogExpansion(TEXT("W3 furnace running")); });
	Schedule(46.f, [this]() { LogExpansion(TEXT("W4 final")); });
	Schedule(48.f, [this]() { Quit(); });
}

void UWEDevAutomationComponent::RunSmokeRoleX()
{
	// Sees the buildings appear, the tier, the books and the industrial orders; walks into the hall.
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	Schedule(7.f, [this]() { LogExpansion(TEXT("X0 small garage")); });
	Schedule(15.f, [this]() { LogExpansion(TEXT("X1 sees the hall")); });
	Schedule(16.f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(1400.f, 200.f, 100.f), FRotator::ZeroRotator); } });
	Schedule(17.5f, [this]()
	{
		const AWECharacter* Me = GetCharacter();
		UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] X2 inside the hall at %s"), *Role, Me ? *Me->GetActorLocation().ToCompactString() : TEXT("-"));
	});
	Schedule(21.f, [this]() { LogExpansion(TEXT("X3 sees the order and the furnace")); });
	Schedule(46.f, [this]() { LogExpansion(TEXT("X4 final")); });
	Schedule(48.f, [this]() { Quit(); });
}

// --- Phase 13/14: sessions through the real menu ---------------------------------------------------

bool UWEDevAutomationComponent::IsInMenu() const
{
	return Cast<AWEMenuPlayerController>(GetOwner()) != nullptr;
}

void UWEDevAutomationComponent::LogCoop(const TCHAR* Step) const
{
	const UWESessionSubsystem* Session = UWESessionSubsystem::Get(this);
	TArray<FString> Players;
	if (const AGameStateBase* GS = GetWorld()->GetGameState())
	{
		for (const APlayerState* PS : GS->PlayerArray)
		{
			const AWEPlayerState* WEPS = Cast<AWEPlayerState>(PS);
			Players.Add(FString::Printf(TEXT("%s%s"), PS ? *PS->GetPlayerName() : TEXT("?"), WEPS && WEPS->IsHost() ? TEXT("(Host)") : TEXT("")));
		}
	}
	FString Departed = TEXT("-");
	if (const AWEGameMode* GM = GetWorld()->GetAuthGameMode<AWEGameMode>())
	{
		TArray<FString> Names;
		for (const FWESavedPlayer& P : GM->GetDepartedPlayers())
		{
			Names.Add(FString::Printf(TEXT("%s[%d slots]"), *P.PlayerName, P.Inventory.Num()));
		}
		Departed = FString::Join(Names, TEXT(" "));
	}
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	const AWEMenuPlayerController* MenuPC = Cast<AWEMenuPlayerController>(GetOwner());
	const AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] %s: Map=%s Net=%d Service=%d State=%d Status='%s' Players=%d{%s} Departed={%s} Balance=%s Mine=%s Pause=%d Menu='%s'"),
		*Role, Step, *UWorld::RemovePIEPrefix(GetWorld()->GetMapName()), static_cast<int32>(GetWorld()->GetNetMode()),
		Session ? static_cast<int32>(Session->GetService()) : -1, Session ? static_cast<int32>(Session->GetState()) : -1,
		Session ? *Session->GetStatusText().ToString() : TEXT(""), Players.Num(), *FString::Join(Players, TEXT(", ")), *Departed,
		Company ? *Company->GetBalance().ToDisplayString() : TEXT("-"), *DescribeInventory(GetMyInventory()),
		PC && PC->GetPauseMenu() ? PC->GetPauseMenu()->GetListedPlayers() : -1,
		MenuPC && MenuPC->GetMenu() ? *MenuPC->GetMenu()->BuildSignature() : TEXT(""));
}

void UWEDevAutomationComponent::FindAndJoin(int32 TriesLeft)
{
	UWESessionSubsystem* Session = UWESessionSubsystem::Get(this);
	if (!Session)
	{
		return;
	}
	if (TriesLeft <= 0)
	{
		// Several instances on one PC share the LAN discovery port, so the reply can reach another process.
		LogCoop(TEXT("LAN search found nothing, joining by IP"));
		Session->JoinAddress(TEXT("127.0.0.1"));
		return;
	}
	Session->FindGames(/*bLAN*/ true);
	Schedule(7.f, [this, TriesLeft]()
	{
		UWESessionSubsystem* S = UWESessionSubsystem::Get(this);
		if (S && S->GetFoundGames().Num() > 0)
		{
			LogCoop(TEXT("found"));
			S->JoinGame(S->GetFoundGames()[0].ResultIndex);
		}
		else
		{
			FindAndJoin(TriesLeft - 1);
		}
	});
}

void UWEDevAutomationComponent::RunCoopRole()
{
	// MH hosts from the menu (LAN); M1 leaves and rejoins (inventory comes back); M2 crashes; M3 is
	// kicked; M5 tries to join a full game by IP. All roles quit when the host ends the game.
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	UWESessionSubsystem* Session = UWESessionSubsystem::Get(this);
	const int32 Visit = Session ? Session->DevMenuVisits : 0;
	if (IsInMenu())
	{
		Schedule(1.f, [this, Visit]() { LogCoop(*FString::Printf(TEXT("menu visit %d"), Visit)); });
		if (Visit > 1 || Role == TEXT("M3") && Visit > 1)
		{
			// Back in the menu after leaving / being kicked / the host ending the game.
			if (Role == TEXT("M1") && Visit == 2)
			{
				Schedule(4.f, [this]() { FindAndJoin(3); });
			}
			else
			{
				Schedule(3.f, [this]() { LogCoop(TEXT("final menu")); Quit(); });
			}
			return;
		}
		if (Role == TEXT("MH"))
		{
			Schedule(3.f, [Session]() { if (Session) { Session->HostGame(FString(), /*bLAN*/ true); } });
		}
		else if (Role == TEXT("M5"))
		{
			Schedule(3.f, [Session]() { if (Session) { Session->JoinAddress(TEXT("127.0.0.1")); } });
			Schedule(25.f, [this]() { LogCoop(TEXT("M5 still in menu")); Quit(); });
		}
		else if (Role == TEXT("M1"))
		{
			Schedule(4.f, [this]() { FindAndJoin(3); });
		}
		else
		{
			// Several searching instances on one PC share the LAN discovery port: the others join by IP.
			Schedule(4.f, [Session]() { if (Session) { Session->JoinAddress(TEXT("127.0.0.1")); } });
		}
		return;
	}

	// In the garage.
	Schedule(3.f, [this]() { LogCoop(TEXT("in game")); });
	if (Role == TEXT("MH"))
	{
		for (float At = 10.f; At <= 120.f; At += 10.f)
		{
			Schedule(At, [this, At]() { LogCoop(*FString::Printf(TEXT("host t=%.0f"), At)); });
		}
		Schedule(32.f, [PC]() { if (PC) { PC->OpenPauseMenu(); } });
		Schedule(33.f, [this]() { LogCoop(TEXT("host pause menu")); });
		Schedule(34.f, [PC]() { if (PC) { PC->ClosePauseMenu(); } });
		Schedule(55.f, [this, PC]()
		{
			// Kick M3.
			for (const APlayerState* PS : GetWorld()->GetGameState()->PlayerArray)
			{
				if (PS && PS->GetPlayerName() == TEXT("M3") && PC)
				{
					PC->RequestKick(const_cast<APlayerState*>(PS));
				}
			}
		});
		Schedule(125.f, [PC]() { if (PC) { PC->RequestLeaveGame(); } });
		return;
	}
	if (Role == TEXT("M1"))
	{
		if (Session && Session->DevMenuVisits <= 1)
		{
			Schedule(5.f, [PC]() { if (PC) { PC->ServerDevGiveItem(TEXT("CopperCable"), 6, EWEItemCondition::Used); PC->ServerDevAddMoney(100); } });
			Schedule(8.f, [this]() { LogCoop(TEXT("M1 has the cable")); });
			Schedule(30.f, [PC]() { if (PC) { PC->RequestLeaveGame(); } });
		}
		else
		{
			Schedule(5.f, [this]() { LogCoop(TEXT("M1 rejoined")); });
			Schedule(30.f, [this]() { LogCoop(TEXT("M1 still in game")); });
		}
		return;
	}
	if (Role == TEXT("M2"))
	{
		Schedule(5.f, [PC]() { if (PC) { PC->ServerDevGiveItem(TEXT("RamModule"), 2, EWEItemCondition::Used); } });
		Schedule(8.f, [this]() { LogCoop(TEXT("M2 before crash")); });
		Schedule(32.f, []() { UE_LOG(LogWENet, Display, TEXT("SMOKE[M2] crashing now")); GLog->Flush(); FPlatformMisc::RequestExit(true); });
		return;
	}
	Schedule(8.f, [this]() { LogCoop(TEXT("client in game")); });
}

void UWEDevAutomationComponent::RunSteamRole()
{
	// SH hosts a Steam lobby, opens the invite overlay and ends; SC (same Steam account) looks for it
	// and tries to join (Steam cannot connect an account to itself: the error must be clean).
	UWESessionSubsystem* Session = UWESessionSubsystem::Get(this);
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	if (IsInMenu())
	{
		Schedule(2.f, [this]() { LogCoop(TEXT("steam menu")); });
		if (Session && Session->DevMenuVisits > 1)
		{
			Schedule(3.f, [this]() { LogCoop(TEXT("steam final menu")); Quit(); });
			return;
		}
		if (Role == TEXT("SH"))
		{
			Schedule(3.f, [this]() { Shot(TEXT("01_Hauptmenue_Steam")); });
			Schedule(4.f, [Session]() { if (Session) { Session->HostGame(FString(), /*bLAN*/ false); } });
		}
		else
		{
			Schedule(14.f, [Session]() { if (Session) { Session->FindGames(/*bLAN*/ false); } });
			Schedule(22.f, [this, Session]()
			{
				LogCoop(TEXT("steam search done"));
				if (Session && Session->GetFoundGames().Num() > 0)
				{
					Session->JoinGame(Session->GetFoundGames()[0].ResultIndex);
				}
			});
			Schedule(55.f, [this]() { LogCoop(TEXT("steam join result")); Quit(); });
		}
		return;
	}
	Schedule(3.f, [this, PC]() { LogCoop(TEXT("steam in game")); if (PC) { PC->OpenPauseMenu(); } });
	Schedule(4.f, [this]() { Shot(TEXT("02_Pausemenue_Steam")); });
	Schedule(5.f, [PC]() { if (PC) { PC->ClosePauseMenu(); } });
	Schedule(6.f, [this, Session]()
	{
		FText Error;
		const bool bShown = Session && Session->ShowInviteUI(Error);
		UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] invite overlay: %s %s"), *Role, bShown ? TEXT("shown") : TEXT("failed"), *Error.ToString());
	});
	Schedule(45.f, [PC]() { if (PC) { PC->RequestLeaveGame(); } });
}

void UWEDevAutomationComponent::RunCoopShots()
{
	// P13 = host (menu pages, hosting, pause menu with the client); P13C = client (joins, sees the host end the game).
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	AWEMenuPlayerController* MenuPC = Cast<AWEMenuPlayerController>(GetOwner());
	UWESessionSubsystem* Session = UWESessionSubsystem::Get(this);
	const bool bHost = ShotName == TEXT("P13");
	const int32 Visit = Session ? Session->DevMenuVisits : 0;
	if (MenuPC)
	{
		if (!bHost && Visit > 1)
		{
			Schedule(2.f, [this]() { Shot(TEXT("08_Host_hat_beendet")); LogCoop(TEXT("client back in menu")); });
			Schedule(3.f, [this]() { Quit(); });
			return;
		}
		if (bHost && Visit > 1)
		{
			Schedule(2.f, [this]() { LogCoop(TEXT("host back in menu")); Quit(); });
			return;
		}
		if (bHost)
		{
			Schedule(2.f, [this]() { Shot(TEXT("01_Hauptmenue")); });
			Schedule(2.5f, [MenuPC]() { if (MenuPC->GetMenu()) { MenuPC->GetMenu()->ShowPage(1); } });
			Schedule(3.5f, [this]() { Shot(TEXT("02_Spielstand_laden")); });
			Schedule(4.f, [MenuPC]() { if (MenuPC->GetMenu()) { MenuPC->GetMenu()->ShowPage(0); } });
			Schedule(5.f, [Session]() { if (Session) { Session->HostGame(FString(), true); } });
		}
		else
		{
			Schedule(12.f, [MenuPC, Session]() { if (MenuPC->GetMenu()) { MenuPC->GetMenu()->ShowPage(2); } if (Session) { Session->FindGames(true); } });
			Schedule(18.f, [this]() { Shot(TEXT("03_Spiel_beitreten")); LogCoop(TEXT("client found")); });
			Schedule(19.f, [Session]() { if (Session && Session->GetFoundGames().Num() > 0) { Session->JoinGame(Session->GetFoundGames()[0].ResultIndex); } });
		}
		return;
	}
	if (bHost)
	{
		Schedule(20.f, [this, PC]() { LogCoop(TEXT("host with client")); if (PC) { PC->ServerDevTeleport(FVector(250.f, -120.f, 100.f), FRotator(0.f, 180.f, 0.f)); } });
		Schedule(21.f, [this]()
		{
			for (TActorIterator<AWECharacter> It(GetWorld()); It; ++It)
			{
				if (*It != GetCharacter()) { LookAt(*It); }
			}
		});
		Schedule(22.f, [this]() { Shot(TEXT("05_Mitspieler")); });
		Schedule(23.f, [PC]() { if (PC) { PC->OpenPauseMenu(); } });
		Schedule(24.f, [this]() { Shot(TEXT("06_Pausemenue_Host")); LogCoop(TEXT("host pause")); });
		Schedule(25.f, [PC]() { if (PC) { PC->ClosePauseMenu(); } });
		Schedule(34.f, [PC]() { if (PC) { PC->RequestLeaveGame(); } });
	}
	else
	{
		Schedule(4.f, [this, PC]() { LogCoop(TEXT("client in game")); if (PC) { PC->ServerDevTeleport(FVector(-100.f, -120.f, 100.f), FRotator::ZeroRotator); } });
		Schedule(6.f, [this]() { Shot(TEXT("04_Beigetreten")); });
		Schedule(9.f, [PC]() { if (PC) { PC->OpenPauseMenu(); } });
		Schedule(10.f, [this]() { Shot(TEXT("07_Pausemenue_Mitspieler")); LogCoop(TEXT("client pause")); });
		Schedule(10.5f, [PC]() { if (PC) { PC->ClosePauseMenu(); } });
	}
}

// --- Phase 15: fixed views for the look -----------------------------------------------------------

void UWEDevAutomationComponent::ShotFrom(float At, const FVector& From, const FVector& Target, const TCHAR* Name)
{
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	const FString ShotLabel(Name);
	Schedule(At, [PC, From]() { if (PC) { PC->ServerDevTeleport(From, FRotator::ZeroRotator); } });
	Schedule(At + 0.6f, [this, Target]() { LookAtPoint(Target); });
	// Give exposure adaptation and Lumen a moment after the teleport; measure frame times meanwhile.
	Schedule(At + 1.5f, [this]() { PerfFrames = 0; PerfSeconds = 0.0; PerfGpuMs = 0.0; PerfStart = FPlatformTime::Seconds(); PerfFrameStart = GFrameCounter; });
	Schedule(At + 3.f, [this, ShotLabel]()
	{
		const double Seconds = FPlatformTime::Seconds() - PerfStart;
		const uint64 Frames = GFrameCounter - PerfFrameStart;
		const double GpuMs = FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
		UE_LOG(LogWorkshopEmpire, Display, TEXT("PERF %s: %.1f fps (%.2f ms/frame), GPU %.2f ms, %d x %d"), *ShotLabel,
			Frames / FMath::Max(Seconds, 0.001), Seconds * 1000.0 / FMath::Max<uint64>(Frames, 1), GpuMs,
			GEngine && GEngine->GameViewport ? (int32)GEngine->GameViewport->Viewport->GetSizeXY().X : 0, GEngine && GEngine->GameViewport ? (int32)GEngine->GameViewport->Viewport->GetSizeXY().Y : 0);
		Shot(ShotLabel);
	});
}

void UWEDevAutomationComponent::RunVisualShots()
{
	// Furnished workshop with machines, a worker and a second "player" mannequin, then fixed views.
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	Schedule(2.0f, [this, PC]()
	{
		if (PC) { PC->ServerDevAddMoney(20000); PC->ServerDevTeleport(FVector(40.f, -60.f, 100.f), FRotator(0.f, 90.f, 0.f)); }
	});
	Schedule(3.0f, [this]()
	{
		if (UWEBuildComponent* B = GetBuild())
		{
			B->SetBuildMode(true);
			PlaceFurnitureAt(TEXT("Shredder"), 15.f, 200.f, -90.f); // (beside the PC desk, which moved off the wall)
			PlaceFurnitureAt(TEXT("Press"), 310.f, 200.f, -90.f);
			PlaceFurnitureAt(TEXT("Conveyor"), 140.f, 205.f, 0.f);
			B->SetBuildMode(false);
		}
	});
	Schedule(3.5f, [this, PC]()
	{
		BuyUpgradeAtTerminal(TEXT("LedLights"));
		if (PC) { PC->RequestHire(const_cast<AWETraderTerminal*>(FindTerminal()), 0); }
		if (PC) { PC->ServerDevGiveItem(TEXT("OldComputer"), 1, EWEItemCondition::Defective); }
	});
	// Feed the shredder and the press and switch them on (effects, sounds, shaking).
	auto Interaction = [this]() { AWECharacter* C = GetCharacter(); return C ? C->GetInteractionComponent() : nullptr; };
	Schedule(4.0f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(150.f, 80.f, 100.f), FRotator::ZeroRotator); PC->ServerDevGiveItem(TEXT("SteelScrap"), 20, EWEItemCondition::Used); } });
	Schedule(4.5f, [this, PC]() { TransferToMachine(TEXT("SteelScrap"), FindMachine(TEXT("Press"))); if (PC) { PC->ServerDevGiveItem(TEXT("SteelScrap"), 20, EWEItemCondition::Used); } });
	Schedule(5.0f, [this, Interaction]()
	{
		TransferToMachine(TEXT("SteelScrap"), FindMachine(TEXT("Shredder")));
		if (UWEInteractionComponent* I = Interaction()) { I->RequestAlternativeInteractWith(FindMachine(TEXT("Shredder"))); I->RequestAlternativeInteractWith(FindMachine(TEXT("Press"))); }
	});
	ShotFrom(5.5f, FVector(330.f, -230.f, 100.f), FVector(-250.f, 120.f, 80.f), TEXT("01_Werkstatt"));
	ShotFrom(9.f, FVector(-150.f, -60.f, 100.f), FVector(-355.f, 0.f, 95.f), TEXT("02_Werkbank"));
	ShotFrom(13.f, FVector(120.f, -120.f, 100.f), FVector(150.f, 210.f, 70.f), TEXT("03_Maschinen"));
	ShotFrom(17.f, FVector(1150.f, -520.f, 120.f), FVector(0.f, 0.f, 120.f), TEXT("04_Hof"));
	Schedule(15.f, [this]() { if (AWEGarageDoor* Door = Cast<AWEGarageDoor>(FindDoor())) { Door->SetOpen(true); } }); // daylight through the open door
	ShotFrom(21.f, FVector(-100.f, -150.f, 100.f), FVector(900.f, 0.f, 120.f), TEXT("05_Blick_hinaus"));
	Schedule(25.f, [this]()
	{
		for (TActorIterator<AWEEmployee> It(GetWorld()); It; ++It) { LookAt(*It); break; }
	});
	Schedule(26.f, [this]() { Shot(TEXT("06_Mitarbeiter")); });
	// All effect kinds side by side (one-shot bursts and short loops), photographed right after they start.
	Schedule(27.f, [PC]() { if (PC) { PC->ServerDevTeleport(FVector(120.f, -230.f, 100.f), FRotator::ZeroRotator); } });
	Schedule(28.f, [this]() { LookAtPoint(FVector(120.f, 40.f, 70.f)); });
	Schedule(29.f, [this]()
	{
		const EWEFxKind Kinds[] = { EWEFxKind::Sparks, EWEFxKind::Dust, EWEFxKind::Chips, EWEFxKind::Steam, EWEFxKind::Embers, EWEFxKind::Smoke };
		for (int32 i = 0; i < 6; ++i)
		{
			AWEFx::Burst(this, Kinds[i], FVector(-80.f + i * 45.f, 40.f, 50.f), FVector(0.f, -0.4f, 1.f), FLinearColor::White, 1.f);
		}
	});
	Schedule(29.25f, [this]() { Shot(TEXT("07_Effekte")); });
	Schedule(29.7f, [this]() { Shot(TEXT("08_Effekte_spaeter")); });
	Schedule(31.f, [this]() { Quit(); });
}