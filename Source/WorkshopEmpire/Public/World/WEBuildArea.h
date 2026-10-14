#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WEBuildArea.generated.h"

class UBoxComponent;

/**
 * Region where furniture may be placed (§14 "innerhalb sinnvoller Grenzen"), e.g. the garage
 * interior. Several areas can exist (new rooms later). Map data only; same on every machine.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEBuildArea : public AActor
{
	GENERATED_BODY()

public:
	AWEBuildArea();

	/** True if the whole footprint (rotated rectangle around Transform, HalfSize in X/Y) lies inside one area. */
	static bool ContainsFootprint(const UWorld* World, const FTransform& Transform, const FVector2D& HalfSize);

	bool ContainsPoint2D(const FVector& Point) const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Build")
	TObjectPtr<UBoxComponent> Area;
};
