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
#include "Progression/WEProgressionComponent.h"

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
	GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda(MoveTemp(Action)), Delay, false);
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