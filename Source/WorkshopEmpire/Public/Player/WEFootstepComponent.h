#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WEFootstepComponent.generated.h"

class USoundBase;

/**
 * Footsteps (Phase 15) for players and employees: a step sound every StrideCm of walking on the
 * ground (shorter strides when crouching, longer when sprinting), at the feet, on every client.
 * Uses the Kenney concrete footsteps when imported, else the synthesised SFX_Step. Cosmetic only.
 */
UCLASS(ClassGroup = (WorkshopEmpire), meta = (BlueprintSpawnableComponent))
class WORKSHOPEMPIRE_API UWEFootstepComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEFootstepComponent();

	UPROPERTY(EditAnywhere, Category = "Footsteps", meta = (Units = "cm")) float StrideCm = 75.f;
	UPROPERTY(EditAnywhere, Category = "Footsteps") float VolumeMultiplier = 0.6f;

	/** Steps played so far (tests). */
	int32 GetStepCount() const { return StepCount; }

protected:
	virtual void BeginPlay() override;
	void Sample();

	UPROPERTY(Transient) TArray<TObjectPtr<USoundBase>> Sounds;
	FTimerHandle Timer;
	FVector LastLocation = FVector::ZeroVector;
	float Accumulated = 0.f;
	int32 StepCount = 0;
	int32 LastSound = INDEX_NONE;
};
