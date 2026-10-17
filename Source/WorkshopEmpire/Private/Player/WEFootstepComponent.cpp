#include "Player/WEFootstepComponent.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

UWEFootstepComponent::UWEFootstepComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWEFootstepComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetWorld() || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// Recorded steps (Kenney, CC0) if imported, else the synthesised one.
	for (int32 i = 0; i < 5; ++i)
	{
		const FString Path = FString::Printf(TEXT("/Game/WorkshopEmpire/Audio/Kenney/footstep_concrete_00%d.footstep_concrete_00%d"), i, i);
		if (USoundBase* Sound = Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, *Path, nullptr, LOAD_Quiet | LOAD_NoWarn)))
		{
			Sounds.Add(Sound);
		}
	}
	if (Sounds.IsEmpty())
	{
		if (USoundBase* Step = Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, TEXT("/Game/WorkshopEmpire/Audio/SFX_Step.SFX_Step"), nullptr, LOAD_Quiet | LOAD_NoWarn)))
		{
			Sounds.Add(Step);
		}
	}
	LastLocation = GetOwner()->GetActorLocation();
	GetWorld()->GetTimerManager().SetTimer(Timer, this, &UWEFootstepComponent::Sample, 0.05f, true);
}

void UWEFootstepComponent::Sample()
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || Sounds.IsEmpty() || Character->IsHidden())
	{
		return;
	}
	const FVector Location = Character->GetActorLocation();
	const UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	const float Moved = FVector::Dist2D(Location, LastLocation);
	LastLocation = Location;
	// Teleports and lifts (forklift) are not steps.
	if (!Move || !Move->IsMovingOnGround() || Moved > 120.f)
	{
		return;
	}
	Accumulated += Moved;
	const float Stride = StrideCm * (Move->IsCrouching() ? 0.7f : Move->Velocity.Size2D() > 450.f ? 1.35f : 1.f);
	if (Accumulated < Stride)
	{
		return;
	}
	Accumulated = 0.f;
	++StepCount;
	int32 Index = FMath::RandRange(0, Sounds.Num() - 1);
	if (Sounds.Num() > 1 && Index == LastSound)
	{
		Index = (Index + 1) % Sounds.Num();
	}
	LastSound = Index;
	const float Half = Character->GetCapsuleComponent() ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.f;
	const float Volume = VolumeMultiplier * (Move->IsCrouching() ? 0.5f : 1.f);
	UGameplayStatics::PlaySoundAtLocation(this, Sounds[Index], Location - FVector(0.f, 0.f, Half), Volume, FMath::FRandRange(0.92f, 1.08f));
}
