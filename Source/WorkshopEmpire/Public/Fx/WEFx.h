#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WEFx.generated.h"

class UInstancedStaticMeshComponent;

/** Particle looks (Phase 15). Data assets pick them for machines; code for workbench, items, doors. */
UENUM(BlueprintType)
enum class EWEFxKind : uint8
{
	None,
	/** Glowing metal sparks with gravity (shredder, press, welding, repair). */
	Sparks,
	/** A puff of fine grey-brown dust (disassembly, heavy item set down, press). */
	Dust,
	/** Dark smoke rising and growing (furnace chimney). */
	Smoke,
	/** Small pieces of material thrown out and bouncing (shredder). */
	Chips,
	/** Slow glowing specks floating in the light (ambient, looping). */
	Motes,
	/** Orange embers rising (furnace mouth). */
	Embers,
	/** White steam / heat haze rising and fading (furnace, press hydraulics). */
	Steam,
	/** Phase 20: rain streaks falling around the camera (weather). */
	Rain
};

/**
 * Lightweight cosmetic particle effect: a few dozen instances of a basic mesh simulated on the CPU
 * (velocity, gravity, drag, floor bounce, size and fade over life) and drawn with one instanced mesh
 * component and the FX materials (per-instance fade). Purely local - never replicated, never spawned
 * on a dedicated server; machines trigger it from their replicated state on every client.
 *
 * Burst: spawns once and destroys itself. Loop: emits while active (SetEmitting) until destroyed.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEFx : public AActor
{
	GENERATED_BODY()

public:
	AWEFx();

	/** One-shot effect at Location, thrown along Direction. Null on a dedicated server. */
	static AWEFx* Burst(const UObject* WorldContext, EWEFxKind Kind, const FVector& Location, const FVector& Direction = FVector::UpVector,
		const FLinearColor& Color = FLinearColor::White, float Scale = 1.f);

	/** Continuous effect attached to Parent (follows it). Null on a dedicated server. */
	static AWEFx* Loop(USceneComponent* Parent, EWEFxKind Kind, const FVector& RelativeLocation, const FLinearColor& Color = FLinearColor::White, float Scale = 1.f);

	void SetEmitting(bool bEmit) { bEmitting = bEmit; }
	bool IsEmitting() const { return bEmitting; }
	/** Loop only: box (half size) in which motes spawn instead of a point. */
	void SetSpawnBox(const FVector& HalfSize) { SpawnBox = HalfSize; }
	int32 GetNumParticles() const { return Particles.Num(); }

	virtual void Tick(float DeltaSeconds) override;

	/** Settings of a kind (public for tests). */
	struct FSettings
	{
		int32 BurstCount = 20;
		float SpawnRate = 0.f;        // per second while looping
		float Speed = 300.f;
		float SpeedJitter = 0.5f;     // fraction
		float Spread = 0.6f;          // 0 = exactly along the direction, 1 = hemisphere
		float Gravity = -980.f;
		float Drag = 0.5f;            // per second
		float Life = 0.8f;
		float LifeJitter = 0.3f;
		float SizeStart = 0.02f;      // mesh scale (basic shapes are 1 m)
		float SizeEnd = 0.02f;
		float Stretch = 0.f;          // sparks: length along velocity per cm/s
		float Bounce = 0.f;           // > 0: bounces off the floor height
		float FadeIn = 0.05f;         // fraction of life
		bool bGlow = false;           // additive glow material, else translucent smoke material (or opaque chips)
		bool bOpaque = false;         // chips: opaque placeholder material
		bool bSphere = true;
		FLinearColor Color = FLinearColor::White;
		float Intensity = 1.f;        // glow
		float Opacity = 0.4f;         // smoke
		int32 MaxParticles = 160;
		bool bFallDown = false;       // loop: emits downwards instead of up
		bool bKillAtGround = false;   // dies where it hits the world below its spawn point (ground, roofs, vans)
	};
	/** Rain: the world-space effect that follows the view; Rate scales the spawn rate (0 = dry). */
	static AWEFx* Spawn(const UObject* WorldContext, EWEFxKind Kind, const FVector& Location);
	void SetRateScale(float InRate) { RateScale = FMath::Max(0.f, InRate); }
	static FSettings GetSettings(EWEFxKind Kind);

protected:
	void Init(EWEFxKind InKind, const FVector& Direction, const FLinearColor& Color, float InScale, bool bInLoop);
	void SpawnParticle();
	void UpdateInstances();

	struct FParticle
	{
		FVector Position = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		float Age = 0.f;
		float Life = 1.f;
		float SizeFactor = 1.f;
		float KillZ = -UE_BIG_NUMBER;
	};

	UPROPERTY(VisibleAnywhere, Category = "Fx")
	TObjectPtr<UInstancedStaticMeshComponent> Instances;

	EWEFxKind Kind = EWEFxKind::None;
	FSettings Settings;
	FVector EmitDirection = FVector::UpVector;
	FVector SpawnBox = FVector::ZeroVector;
	float Scale = 1.f;
	float FloorZ = 0.f;
	float SpawnAccumulator = 0.f;
	bool bLoop = false;
	bool bEmitting = true;
	TArray<FParticle> Particles;
	float RateScale = 1.f;
	FRandomStream Random;
};

/**
 * Placeable emitter for level dressing (Phase 15): starts a looping effect on every client, e.g. dust
 * motes floating in the workshop light (Kind = Motes, BoxHalfSize = room). Not replicated.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEFxEmitter : public AActor
{
	GENERATED_BODY()

public:
	AWEFxEmitter();

	UPROPERTY(EditAnywhere, Category = "Fx") EWEFxKind Kind = EWEFxKind::Motes;
	/** Particles start anywhere in this box (half size, cm); zero = at the emitter. */
	UPROPERTY(EditAnywhere, Category = "Fx") FVector BoxHalfSize = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, Category = "Fx") FLinearColor Color = FLinearColor::White;
	UPROPERTY(EditAnywhere, Category = "Fx", meta = (ClampMin = "0.1")) float Scale = 1.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(Transient) TObjectPtr<AWEFx> Effect;
};
