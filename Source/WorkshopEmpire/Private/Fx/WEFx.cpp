#include "Fx/WEFx.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "World/WEPlaceholderVisuals.h"

namespace WEFxConst
{
	const TCHAR* GlowMaterial = TEXT("/Game/WorkshopEmpire/Materials/FX/M_WEFxGlow.M_WEFxGlow");
	const TCHAR* SmokeMaterial = TEXT("/Game/WorkshopEmpire/Materials/FX/M_WEFxSmoke.M_WEFxSmoke");
	const TCHAR* Sphere = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* Cube = TEXT("/Engine/BasicShapes/Cube.Cube");
	constexpr int32 MaxParticles = 160;
}

AWEFx::AWEFx()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetCanBeDamaged(false);
	Instances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Instances"));
	RootComponent = Instances;
	Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Instances->SetCastShadow(false);
	Instances->NumCustomDataFloats = 1;
	Instances->SetMobility(EComponentMobility::Movable);
	Random.GenerateNewSeed();
}

AWEFx::FSettings AWEFx::GetSettings(EWEFxKind Kind)
{
	FSettings S;
	switch (Kind)
	{
	case EWEFxKind::Sparks:
		S.BurstCount = 28; S.SpawnRate = 30.f; S.Speed = 420.f; S.Spread = 0.55f; S.Gravity = -900.f; S.Drag = 0.6f;
		S.Life = 0.6f; S.SizeStart = 0.012f; S.SizeEnd = 0.006f; S.Stretch = 0.00022f; S.Bounce = 0.35f; S.FadeIn = 0.f;
		S.bGlow = true; S.bSphere = false; S.Color = FLinearColor(1.f, 0.45f, 0.08f); S.Intensity = 40.f;
		break;
	case EWEFxKind::Embers:
		S.BurstCount = 10; S.SpawnRate = 7.f; S.Speed = 70.f; S.Spread = 0.4f; S.Gravity = 120.f; S.Drag = 0.4f;
		S.Life = 1.8f; S.SizeStart = 0.012f; S.SizeEnd = 0.004f; S.FadeIn = 0.1f;
		S.bGlow = true; S.Color = FLinearColor(1.f, 0.35f, 0.05f); S.Intensity = 30.f;
		break;
	case EWEFxKind::Dust:
		S.BurstCount = 14; S.SpawnRate = 10.f; S.Speed = 90.f; S.Spread = 0.9f; S.Gravity = 15.f; S.Drag = 2.5f;
		S.Life = 1.6f; S.SizeStart = 0.08f; S.SizeEnd = 0.35f; S.FadeIn = 0.15f;
		S.Color = FLinearColor(0.3f, 0.27f, 0.23f); S.Opacity = 0.4f;
		break;
	case EWEFxKind::Smoke:
		S.BurstCount = 6; S.SpawnRate = 3.5f; S.Speed = 60.f; S.Spread = 0.25f; S.Gravity = 45.f; S.Drag = 0.3f;
		S.Life = 5.f; S.SizeStart = 0.25f; S.SizeEnd = 1.6f; S.FadeIn = 0.15f;
		S.Color = FLinearColor(0.16f, 0.155f, 0.15f); S.Opacity = 0.7f;
		break;
	case EWEFxKind::Steam:
		S.BurstCount = 8; S.SpawnRate = 5.f; S.Speed = 80.f; S.Spread = 0.3f; S.Gravity = 60.f; S.Drag = 0.8f;
		S.Life = 2.2f; S.SizeStart = 0.12f; S.SizeEnd = 0.7f; S.FadeIn = 0.1f;
		S.Color = FLinearColor(0.85f, 0.85f, 0.85f); S.Opacity = 0.38f;
		break;
	case EWEFxKind::Chips:
		S.BurstCount = 16; S.SpawnRate = 12.f; S.Speed = 330.f; S.Spread = 0.6f; S.Gravity = -980.f; S.Drag = 0.2f;
		S.Life = 1.4f; S.SizeStart = 0.035f; S.SizeEnd = 0.03f; S.Bounce = 0.3f; S.FadeIn = 0.f;
		S.bOpaque = true; S.bSphere = false; S.Color = FLinearColor(0.5f, 0.5f, 0.5f);
		break;
	case EWEFxKind::Motes:
		S.BurstCount = 0; S.SpawnRate = 6.f; S.Speed = 6.f; S.Spread = 1.f; S.Gravity = -1.f; S.Drag = 0.f;
		S.Life = 9.f; S.LifeJitter = 0.4f; S.SizeStart = 0.004f; S.SizeEnd = 0.004f; S.FadeIn = 0.3f;
		S.bGlow = true; S.Color = FLinearColor(1.f, 0.9f, 0.75f); S.Intensity = 3.f;
		break;
	default:
		break;
	}
	return S;
}

AWEFx* AWEFx::Burst(const UObject* WorldContext, EWEFxKind Kind, const FVector& Location, const FVector& Direction, const FLinearColor& Color, float InScale)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || Kind == EWEFxKind::None || World->GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWEFx* Fx = World->SpawnActor<AWEFx>(AWEFx::StaticClass(), FTransform(Location), Params);
	if (Fx)
	{
		Fx->Init(Kind, Direction, Color, InScale, false);
	}
	return Fx;
}

AWEFx* AWEFx::Loop(USceneComponent* Parent, EWEFxKind Kind, const FVector& RelativeLocation, const FLinearColor& Color, float InScale)
{
	UWorld* World = Parent ? Parent->GetWorld() : nullptr;
	if (!World || Kind == EWEFxKind::None || World->GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWEFx* Fx = World->SpawnActor<AWEFx>(AWEFx::StaticClass(), Parent->GetComponentTransform(), Params);
	if (Fx)
	{
		Fx->AttachToComponent(Parent, FAttachmentTransformRules::KeepWorldTransform);
		Fx->SetActorRelativeLocation(RelativeLocation);
		Fx->SetActorRelativeRotation(FRotator::ZeroRotator);
		Fx->Init(Kind, FVector::UpVector, Color, InScale, true);
	}
	return Fx;
}

void AWEFx::Init(EWEFxKind InKind, const FVector& Direction, const FLinearColor& Color, float InScale, bool bInLoop)
{
	Kind = InKind;
	Settings = GetSettings(InKind);
	if (Color != FLinearColor::White)
	{
		Settings.Color = Color;
	}
	Scale = FMath::Max(0.1f, InScale);
	bLoop = bInLoop;
	EmitDirection = Direction.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	// Particles bounce on the floor under the effect (the workshop floor is at z = 0).
	FloorZ = FMath::Max(0.f, GetActorLocation().Z - 400.f);

	Instances->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Settings.bSphere ? WEFxConst::Sphere : WEFxConst::Cube));
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
		Settings.bOpaque ? WEPlaceholder::MaterialPath : Settings.bGlow ? WEFxConst::GlowMaterial : WEFxConst::SmokeMaterial);
	if (Base)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this);
		MID->SetVectorParameterValue(TEXT("Color"), Settings.Color);
		MID->SetScalarParameterValue(TEXT("Intensity"), Settings.Intensity);
		MID->SetScalarParameterValue(TEXT("Opacity"), Settings.Opacity);
		MID->SetScalarParameterValue(TEXT("Roughness"), 0.5f);
		Instances->SetMaterial(0, MID);
	}
	if (!bLoop)
	{
		const int32 Count = FMath::RoundToInt(Settings.BurstCount * FMath::Clamp(Scale, 0.3f, 3.f));
		for (int32 i = 0; i < Count; ++i)
		{
			SpawnParticle();
		}
	}
	UpdateInstances();
}

void AWEFx::SpawnParticle()
{
	if (Particles.Num() >= WEFxConst::MaxParticles)
	{
		return;
	}
	FParticle& P = Particles.AddDefaulted_GetRef();
	const FVector Base = GetActorLocation();
	P.Position = SpawnBox.IsNearlyZero() ? Base
		: Base + FVector(Random.FRandRange(-SpawnBox.X, SpawnBox.X), Random.FRandRange(-SpawnBox.Y, SpawnBox.Y), Random.FRandRange(-SpawnBox.Z, SpawnBox.Z));
	// Direction inside a cone (Spread 1 = any direction of the hemisphere around the emit direction).
	const FVector Dir = Settings.Spread >= 1.f ? Random.GetUnitVector()
		: FMath::VRandCone(bLoop ? GetActorUpVector() : EmitDirection, FMath::DegreesToRadians(90.f * Settings.Spread));
	const float Speed = Settings.Speed * Scale * (1.f + Random.FRandRange(-Settings.SpeedJitter, Settings.SpeedJitter));
	P.Velocity = Dir * Speed;
	P.Life = Settings.Life * (1.f + Random.FRandRange(-Settings.LifeJitter, Settings.LifeJitter));
	P.SizeFactor = Random.FRandRange(0.7f, 1.3f);
}

void AWEFx::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dt = FMath::Min(DeltaSeconds, 0.05f);
	if (bLoop && bEmitting && Settings.SpawnRate > 0.f)
	{
		SpawnAccumulator += Dt * Settings.SpawnRate * FMath::Clamp(Scale, 0.3f, 3.f);
		while (SpawnAccumulator >= 1.f)
		{
			SpawnAccumulator -= 1.f;
			SpawnParticle();
		}
	}
	for (int32 i = Particles.Num() - 1; i >= 0; --i)
	{
		FParticle& P = Particles[i];
		P.Age += Dt;
		if (P.Age >= P.Life)
		{
			Particles.RemoveAtSwap(i);
			continue;
		}
		P.Velocity.Z += Settings.Gravity * Dt;
		P.Velocity *= FMath::Max(0.f, 1.f - Settings.Drag * Dt);
		P.Position += P.Velocity * Dt;
		if (Settings.Bounce > 0.f && P.Position.Z < FloorZ + 1.f && P.Velocity.Z < 0.f)
		{
			P.Position.Z = FloorZ + 1.f;
			P.Velocity.Z = -P.Velocity.Z * Settings.Bounce;
			P.Velocity.X *= 0.6f;
			P.Velocity.Y *= 0.6f;
		}
	}
	UpdateInstances();
	if (!bLoop && Particles.IsEmpty())
	{
		Destroy();
	}
}

void AWEFx::UpdateInstances()
{
	// Few dozen instances: rebuilding the list each frame is cheaper than bookkeeping.
	Instances->ClearInstances();
	if (Particles.IsEmpty())
	{
		return;
	}
	TArray<FTransform> Transforms;
	Transforms.Reserve(Particles.Num());
	const FTransform ToLocal = GetActorTransform().Inverse();
	for (const FParticle& P : Particles)
	{
		const float T = P.Age / FMath::Max(P.Life, 0.01f);
		const float Size = FMath::Lerp(Settings.SizeStart, Settings.SizeEnd, T) * P.SizeFactor * Scale;
		FVector Scale3D(Size);
		FQuat Rotation = FQuat::Identity;
		if (Settings.Stretch > 0.f && !P.Velocity.IsNearlyZero())
		{
			Rotation = P.Velocity.Rotation().Quaternion();
			Scale3D.X += P.Velocity.Size() * Settings.Stretch;
		}
		const FTransform World(Rotation, P.Position, Scale3D);
		Transforms.Add(World * ToLocal);
	}
	Instances->AddInstances(Transforms, false, false, false);
	for (int32 i = 0; i < Particles.Num(); ++i)
	{
		const FParticle& P = Particles[i];
		const float T = P.Age / FMath::Max(P.Life, 0.01f);
		const float In = Settings.FadeIn > 0.f ? FMath::Clamp(T / Settings.FadeIn, 0.f, 1.f) : 1.f;
		const float Out = FMath::Clamp((1.f - T) / 0.35f, 0.f, 1.f);
		Instances->SetCustomDataValue(i, 0, In * Out, false);
	}
	Instances->MarkRenderStateDirty();
}

// --- Emitter ----------------------------------------------------------------------------------------

AWEFxEmitter::AWEFxEmitter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AWEFxEmitter::BeginPlay()
{
	Super::BeginPlay();
	Effect = AWEFx::Loop(GetRootComponent(), Kind, FVector::ZeroVector, Color, Scale);
	if (Effect)
	{
		Effect->SetSpawnBox(BoxHalfSize);
	}
}

void AWEFxEmitter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Effect)
	{
		Effect->Destroy();
	}
	Super::EndPlay(EndPlayReason);
}
