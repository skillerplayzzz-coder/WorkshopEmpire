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
	ApplyShutterModel();
}

void AWEGarageDoor::ApplyShutterModel()
{
	UStaticMesh* Shutter = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr,
		TEXT("/Game/WorkshopEmpire/Art/Models/SM_rollershutter_door.SM_rollershutter_door"), nullptr, LOAD_Quiet | LOAD_NoWarn));
	if (!Shutter || ShutterVisual)
	{
		return;
	}
	ShutterVisual = NewObject<UStaticMeshComponent>(this, TEXT("ShutterVisual"), RF_Transient);
	ShutterVisual->SetStaticMesh(Shutter);
	ShutterVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// On the root, not the panel: the panel's non-uniform scale would distort the turned model.
	ShutterVisual->SetupAttachment(Root);
	ShutterVisual->SetRelativeRotation(FRotator(0.f, 90.f, 0.f)); // model width (X) along the opening (Y)
	ShutterVisual->RegisterComponent();
	UpdateShutterVisual();
	Panel->SetVisibility(false); // stays as collision
}

void AWEGarageDoor::UpdateShutterVisual()
{
	if (!ShutterVisual || !ShutterVisual->GetStaticMesh())
	{
		return;
	}
	// A roller shutter rolls up: the visible curtain reaches from the lintel down to the panel's bottom edge.
	const FBox Bounds = ShutterVisual->GetStaticMesh()->GetBoundingBox();
	const FVector ModelSize = Bounds.GetSize();
	const float Bottom = Panel->GetRelativeLocation().Z - DoorHeight * 0.5f;
	const float Visible = FMath::Max(DoorHeight - Bottom, 45.f); // open: a squeezed "roll" under the lintel
	const FVector Scale(DoorWidth / FMath::Max(ModelSize.X, 1.f), 14.f / FMath::Max(ModelSize.Y, 1.f), Visible / FMath::Max(ModelSize.Z, 1.f));
	ShutterVisual->SetRelativeScale3D(Scale);
	ShutterVisual->SetRelativeLocation(FVector(0.f, 0.f, DoorHeight - Visible - Bounds.Min.Z * Scale.Z));
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
	OutInfo.Description = LOCTEXT("Desc", "Ein altes Rolltor. Es rattert, aber es funktioniert.");
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
	UpdateShutterVisual();

	if (FMath::IsNearlyEqual(Location.Z, TargetZ, 0.1f))
	{
		GetWorldTimerManager().ClearTimer(MoveTimer);
	}
}

#undef LOCTEXT_NAMESPACE
