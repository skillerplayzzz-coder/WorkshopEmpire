#include "World/WEBuildArea.h"

#include "Components/BoxComponent.h"
#include "EngineUtils.h"

AWEBuildArea::AWEBuildArea()
{
	PrimaryActorTick.bCanEverTick = false;
	Area = CreateDefaultSubobject<UBoxComponent>(TEXT("Area"));
	Area->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Area->SetBoxExtent(FVector(395.f, 295.f, 150.f));
	Area->SetHiddenInGame(true);
	RootComponent = Area;
}

void AWEBuildArea::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Area->SetBoxExtent(FVector(HalfSize.X, HalfSize.Y, 150.f));
}

bool AWEBuildArea::ContainsPoint2D(const FVector& Point) const
{
	const FVector Local = Area->GetComponentTransform().InverseTransformPosition(Point);
	const FVector Extent = Area->GetUnscaledBoxExtent();
	return FMath::Abs(Local.X) <= Extent.X + KINDA_SMALL_NUMBER && FMath::Abs(Local.Y) <= Extent.Y + KINDA_SMALL_NUMBER;
}

bool AWEBuildArea::ContainsFootprint(const UWorld* World, const FTransform& Transform, const FVector2D& HalfSize)
{
	if (!World)
	{
		return false;
	}
	const FVector Corners[] = {
		Transform.TransformPosition(FVector(HalfSize.X, HalfSize.Y, 0.f)),
		Transform.TransformPosition(FVector(HalfSize.X, -HalfSize.Y, 0.f)),
		Transform.TransformPosition(FVector(-HalfSize.X, HalfSize.Y, 0.f)),
		Transform.TransformPosition(FVector(-HalfSize.X, -HalfSize.Y, 0.f)),
	};
	for (TActorIterator<AWEBuildArea> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (It->IsHidden())
		{
			continue; // e.g. the hall is not bought yet
		}
		bool bAll = true;
		for (const FVector& Corner : Corners)
		{
			bAll &= It->ContainsPoint2D(Corner);
		}
		if (bAll)
		{
			return true;
		}
	}
	return false;
}
