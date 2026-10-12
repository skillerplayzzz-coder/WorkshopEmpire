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
#include "UObject/UObjectIterator.h"

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