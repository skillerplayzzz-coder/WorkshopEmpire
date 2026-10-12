#include "World/WEGarageDoor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "World/WEPlaceholderVisuals.h"
#include "Player/WECharacter.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "WELog.h"

#define LOCTEXT_NAMESPACE "WEGarageDoor"

namespace WEGarageDoor
{
	constexpr float StepInterval = 1.f / 60.f;
	/** Panel stays this far below the lintel when open so it remains visible as a "rolled up" door. */
	constexpr float OpenVisibleHeight = 20.f;
}

AWEGarageDoor::AWEGarageDoor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(10.f);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Panel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Panel"));
	Panel->SetupAttachment(Root);
	Panel->SetMobility(EComponentMobility::Movable);
	Panel->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Panel->SetStaticMesh(CubeMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlaceholderMat(WEPlaceholder::MaterialPath);
	PlaceholderMaterial = PlaceholderMat.Object;
}

void AWEGarageDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Basic cube is 100 cm and centred. Local X = door thickness, Y = width, Z = up; origin at floor centre of the opening.
	Panel->SetRelativeScale3D(FVector(PanelThickness / 100.f, DoorWidth / 100.f, DoorHeight / 100.f));
	Panel->SetRelativeLocation(FVector(0.f, 0.f, GetTargetHeight() + DoorHeight * 0.5f));
	ApplyTint();
}

void AWEGarageDoor::BeginPlay()
{
	Super::BeginPlay();
	Panel->SetRelativeLocation(FVector(0.f, 0.f, GetTargetHeight() + DoorHeight * 0.5f));
	ApplyTint(); // transient tint is not saved with the map
}

void AWEGarageDoor::ApplyTint()
{
	WEPlaceholder::ApplyTint(Panel, PlaceholderMaterial, Tint);
}

void AWEGarageDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWEGarageDoor, bIsOpen);
}

float AWEGarageDoor::GetTargetHeight() const
{
	// Height of the panel's bottom edge above the floor.
	return bIsOpen ? DoorHeight - WEGarageDoor::OpenVisibleHeight : 0.f;
}

FText AWEGarageDoor::GetInteractionText(const AWECharacter* Interactor) const
{
	return bIsOpen ? LOCTEXT("Close", "Tor schließen") : LOCTEXT("Open", "Tor öffnen");
}

bool AWEGarageDoor::CanInteract(const AWECharacter* Interactor, FText& OutReason) const
{
	return Interactor != nullptr;
}

void AWEGarageDoor::Interact(AWECharacter* Interactor)
{
	SetOpen(!bIsOpen);
}

bool AWEGarageDoor::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	OutInfo.Title = LOCTEXT("Title", "Garagentor");
	OutInfo.Description = LOCTEXT("Desc", "Ein altes Sektionaltor. Es quietscht, aber es funktioniert.");
	OutInfo.Details = { bIsOpen ? LOCTEXT("StateOpen", "Zustand: Offen") : LOCTEXT("StateClosed", "Zustand: Geschlossen") };
	return true;
}

void AWEGarageDoor::SetOpen(bool bOpen)
{
	if (!HasAuthority() || bIsOpen == bOpen)
	{
		return;
	}
	bIsOpen = bOpen;
	UE_LOG(LogWENet, Log, TEXT("%s %s"), *GetName(), bIsOpen ? TEXT("opening") : TEXT("closing"));
	StartMoving(); // OnRep does not fire on the server
	ForceNetUpdate();
}

void AWEGarageDoor::OnRep_IsOpen()
{
	StartMoving();
}

void AWEGarageDoor::StartMoving()
{
	GetWorldTimerManager().SetTimer(MoveTimer, this, &AWEGarageDoor::StepMovement, WEGarageDoor::StepInterval, true);
}

void AWEGarageDoor::StepMovement()
{
	FVector Location = Panel->GetRelativeLocation();
	const float TargetZ = GetTargetHeight() + DoorHeight * 0.5f;
	Location.Z = FMath::FInterpConstantTo(Location.Z, TargetZ, WEGarageDoor::StepInterval, OpenSpeed);
	Panel->SetRelativeLocation(Location);

	if (FMath::IsNearlyEqual(Location.Z, TargetZ, 0.1f))
	{
		GetWorldTimerManager().ClearTimer(MoveTimer);
	}
}

#undef LOCTEXT_NAMESPACE
