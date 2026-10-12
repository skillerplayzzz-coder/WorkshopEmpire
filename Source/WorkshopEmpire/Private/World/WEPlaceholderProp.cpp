#include "World/WEPlaceholderProp.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "World/WEPlaceholderVisuals.h"

AWEPlaceholderProp::AWEPlaceholderProp()
{
	PrimaryActorTick.bCanEverTick = false;

	Boxes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Boxes"));
	Boxes->SetMobility(EComponentMobility::Static);
	Boxes->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RootComponent = Boxes;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Boxes->SetStaticMesh(CubeMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlaceholderMat(WEPlaceholder::MaterialPath);
	PlaceholderMaterial = PlaceholderMat.Object;
}

void AWEPlaceholderProp::AddBox(const FVector& Center, const FVector& BoxSize)
{
	Boxes->AddInstance(FTransform(FRotator::ZeroRotator, Center, BoxSize / 100.f));
}

float AWEPlaceholderProp::GetSurfaceHeight() const
{
	return PropType == EWEPlaceholderPropType::Workbench ? Size.Z : Size.Z * 0.2f;
}

void AWEPlaceholderProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Boxes->ClearInstances();

	const float D = Size.X;
	const float W = Size.Y;
	const float H = Size.Z;

	switch (PropType)
	{
	case EWEPlaceholderPropType::Workbench:
	{
		const float Top = 5.f;
		const float Leg = 7.f;
		AddBox(FVector(0.f, 0.f, H - Top * 0.5f), FVector(D, W, Top));
		for (const float SX : { -1.f, 1.f })
		{
			for (const float SY : { -1.f, 1.f })
			{
				AddBox(FVector(SX * (D * 0.5f - Leg), SY * (W * 0.5f - Leg), (H - Top) * 0.5f), FVector(Leg, Leg, H - Top));
			}
		}
		// Lower storage board.
		AddBox(FVector(0.f, 0.f, 18.f), FVector(D - 2.f * Leg, W - 2.f * Leg, 3.f));
		break;
	}
	case EWEPlaceholderPropType::Shelf:
	{
		const float Post = 5.f;
		for (const float SX : { -1.f, 1.f })
		{
			for (const float SY : { -1.f, 1.f })
			{
				AddBox(FVector(SX * (D * 0.5f - Post * 0.5f), SY * (W * 0.5f - Post * 0.5f), H * 0.5f), FVector(Post, Post, H));
			}
		}
		for (const float Level : { 0.2f, 0.55f, 0.9f })
		{
			AddBox(FVector(0.f, 0.f, H * Level), FVector(D, W, 3.f));
		}
		break;
	}
	}

	ApplyTint();
}

void AWEPlaceholderProp::BeginPlay()
{
	Super::BeginPlay();
	ApplyTint();
}

void AWEPlaceholderProp::ApplyTint()
{
	WEPlaceholder::ApplyTint(Boxes, PlaceholderMaterial, Tint);
}
