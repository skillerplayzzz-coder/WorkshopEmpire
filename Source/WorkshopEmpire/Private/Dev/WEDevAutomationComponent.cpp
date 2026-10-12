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
