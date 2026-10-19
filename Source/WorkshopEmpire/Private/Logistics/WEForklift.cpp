#include "Logistics/WEForklift.h"

#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Framework/WEPlayerController.h"
#include "Items/WEPickupItem.h"
#include "Net/UnrealNetwork.h"
#include "Player/WECharacter.h"
#include "WELog.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Workshop/WEBuildComponent.h"

#define LOCTEXT_NAMESPACE "WEForklift"

AWEForklift::AWEForklift()
{
	PropType = EWEPlaceholderPropType::Forklift;
	Size = FVector(210.f, 110.f, 210.f);
	Tint = FLinearColor(0.9f, 0.62f, 0.08f);
	bHasStorage = false;
	RequiredUpgrade = TEXT("Forklift");
	Forks = CreateDefaultSubobject<USceneComponent>(TEXT("Forks"));
	Forks->SetupAttachment(RootComponent);
	// Phase 19: on the fork blades of the modelled forklift (Tools/Assets/build_forklift.py).
	Forks->SetRelativeLocation(FVector(62.f, 0.f, -6.f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Model(TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_Forklift.SM_Gen_Forklift"));
	if (Model.Succeeded())
	{
		VisualMesh = Model.Object;
	}
}

void AWEForklift::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWEForklift, Driver);
}

void AWEForklift::OnRep_Driver()
{
	ApplyDriven();
}

void AWEForklift::ApplyDriven()
{
	// While driven the character's capsule collides; the forklift only follows (attached under the
	// driver's seat - done on every machine, so it follows smoothly on clients too).
	SetActorEnableCollision(Driver == nullptr && bUnlocked);
	if (Driver && GetAttachParentActor() != Driver)
	{
		AttachToComponent(Driver->GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SetActorRelativeLocation(FVector(-GetSeatOffset().X, 0.f, -Driver->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
		SetActorRelativeRotation(FRotator::ZeroRotator);
	}
	if (Driver)
	{
		Driver->OnForkliftChanged();
	}
}

void AWEForklift::SetUnlocked(bool bInUnlocked)
{
	Super::SetUnlocked(bInUnlocked);
	ApplyDriven();
}

bool AWEForklift::Mount(AWECharacter* Character)
{
	FText Reason;
	if (!HasAuthority() || !CanInteract(Character, Reason))
	{
		return false;
	}
	// The driver keeps his position; the forklift moves under him and follows from now on.
	Driver = Character;
	Character->SetForklift(this);
	ApplyDriven();
	ForceNetUpdate();
	UE_LOG(LogWENet, Log, TEXT("%s drives %s"), *Character->GetName(), *GetName());
	return true;
}

bool AWEForklift::Dismount()
{
	AWECharacter* Character = Driver;
	if (!HasAuthority() || !Character)
	{
		return false;
	}
	// A load heavier than the hands can take goes down in front of the forks.
	if (Character->GetHeldItem())
	{
		Character->DropHeldItem();
	}
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	FVector Location = GetActorLocation();
	Location.Z = 0.f;
	SetActorLocationAndRotation(Location, FRotator(0.f, GetActorRotation().Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	Driver = nullptr;
	Character->SetForklift(nullptr);
	// Step out to the left of the seat so the driver does not stand inside the forklift.
	const FVector Out = GetActorTransform().TransformPosition(GetSeatOffset() + FVector(0.f, -Size.Y * 0.5f - 45.f, 0.f));
	Character->SetActorLocation(Out + FVector(0.f, 0.f, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
	ApplyDriven();
	ForceNetUpdate();
	UE_LOG(LogWENet, Log, TEXT("%s left %s"), *Character->GetName(), *GetName());
	return true;
}

FText AWEForklift::GetInteractionText(const AWECharacter* Interactor) const
{
	return LOCTEXT("Drive", "Gabelstapler fahren");
}

bool AWEForklift::CanInteract(const AWECharacter* Interactor, FText& OutReason) const
{
	if (!Interactor || !bUnlocked || Driver)
	{
		return false;
	}
	if (Interactor->GetHeldItem())
	{
		OutReason = LOCTEXT("HandsFull", "Gabelstapler: erst die Hände frei machen");
		return false;
	}
	return Interactor->GetForklift() == nullptr;
}

void AWEForklift::Interact(AWECharacter* Interactor)
{
	Mount(Interactor);
}

bool AWEForklift::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	OutInfo.Title = LOCTEXT("Title", "Gabelstapler");
	OutInfo.Description = LOCTEXT("Desc", "Hebt schwere Lasten bis 1.500 kg: Industriemotoren, Ballen, Paletten. [E] aufsteigen, dann [E] aufnehmen/absetzen wie mit den Händen, [X] absteigen.");
	OutInfo.Details = { Driver ? FText::Format(LOCTEXT("Driven", "Wird gefahren von {0}"), FText::FromString(Driver->GetName())) : LOCTEXT("Free", "Frei") };
	return true;
}

#undef LOCTEXT_NAMESPACE
