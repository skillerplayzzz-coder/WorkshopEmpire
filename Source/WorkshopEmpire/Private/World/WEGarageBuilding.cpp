#include "World/WEGarageBuilding.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "World/WEPlaceholderVisuals.h"

AWEGarageBuilding::AWEGarageBuilding()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlaceholderMat(WEPlaceholder::MaterialPath);
	PlaceholderMaterial = PlaceholderMat.Object;

	auto MakeISM = [this](const TCHAR* Name)
	{
		UInstancedStaticMeshComponent* ISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		ISM->SetupAttachment(Root);
		ISM->SetMobility(EComponentMobility::Static);
		ISM->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		if (CubeMesh.Succeeded())
		{
			ISM->SetStaticMesh(CubeMesh.Object);
		}
		return ISM;
	};

	FloorMeshes = MakeISM(TEXT("FloorMeshes"));
	WallMeshes = MakeISM(TEXT("WallMeshes"));
	RoofMeshes = MakeISM(TEXT("RoofMeshes"));
}

void AWEGarageBuilding::AddBox(UInstancedStaticMeshComponent* ISM, const FVector& Center, const FVector& Size) const
{
	// Basic cube is 100 cm, centred on its origin.
	ISM->AddInstance(FTransform(FRotator::ZeroRotator, Center, Size / 100.f));
}

void AWEGarageBuilding::ApplyTint(UInstancedStaticMeshComponent* ISM, const FLinearColor& Color)
{
	WEPlaceholder::ApplyTint(ISM, PlaceholderMaterial, Color);
}

void AWEGarageBuilding::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	FloorMeshes->ClearInstances();
	WallMeshes->ClearInstances();
	RoofMeshes->ClearInstances();

	const float T = WallThickness;
	const float HalfD = InnerDepth * 0.5f;
	const float HalfW = InnerWidth * 0.5f;
	const float H = InnerHeight;
	const float OuterD = InnerDepth + 2.f * T;
	const float OuterW = InnerWidth + 2.f * T;
	const float DoorW = FMath::Clamp(DoorWidth, 50.f, InnerWidth);
	const float DoorH = FMath::Clamp(DoorHeight, 50.f, InnerHeight);

	// Floor slab (top surface at Z = 0).
	AddBox(FloorMeshes, FVector(0.f, 0.f, -T * 0.5f), FVector(OuterD, OuterW, T));

	// Back wall (-X) and side walls (±Y).
	AddBox(WallMeshes, FVector(-HalfD - T * 0.5f, 0.f, H * 0.5f), FVector(T, OuterW, H));
	AddBox(WallMeshes, FVector(0.f, -HalfW - T * 0.5f, H * 0.5f), FVector(InnerDepth, T, H));
	AddBox(WallMeshes, FVector(0.f, HalfW + T * 0.5f, H * 0.5f), FVector(InnerDepth, T, H));

	// Front wall (+X) with door opening: two side pieces and a lintel.
	const float FrontX = HalfD + T * 0.5f;
	const float SideWidth = (OuterW - DoorW) * 0.5f;
	AddBox(WallMeshes, FVector(FrontX, -(DoorW * 0.5f + SideWidth * 0.5f), H * 0.5f), FVector(T, SideWidth, H));
	AddBox(WallMeshes, FVector(FrontX, DoorW * 0.5f + SideWidth * 0.5f, H * 0.5f), FVector(T, SideWidth, H));
	if (H > DoorH)
	{
		AddBox(WallMeshes, FVector(FrontX, 0.f, DoorH + (H - DoorH) * 0.5f), FVector(T, DoorW, H - DoorH));
	}

	// Flat roof.
	AddBox(RoofMeshes, FVector(0.f, 0.f, H + T * 0.5f), FVector(OuterD, OuterW, T));

	ApplyAllTints();
}

void AWEGarageBuilding::BeginPlay()
{
	Super::BeginPlay();
	// Tint instances are transient (not saved with the map), so rebuild them at runtime.
	ApplyAllTints();
}

void AWEGarageBuilding::ApplyAllTints()
{
	ApplyTint(FloorMeshes, FloorColor);
	ApplyTint(WallMeshes, WallColor);
	ApplyTint(RoofMeshes, RoofColor);
}

FTransform AWEGarageBuilding::GetDoorTransform() const
{
	return FTransform(FRotator::ZeroRotator, FVector(InnerDepth * 0.5f + WallThickness * 0.5f, 0.f, 0.f));
}
