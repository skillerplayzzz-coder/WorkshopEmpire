#include "Interaction/WEInteractionComponent.h"

#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "Player/WEInputConfig.h"
#include "TimerManager.h"
#include "WELog.h"

#define LOCTEXT_NAMESPACE "WEInteraction"

UWEInteractionComponent::UWEInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true); // required for the Server RPCs; the component has no replicated properties
}

void UWEInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	// The focus trace is purely cosmetic/local. A dedicated server never needs it; on other machines
	// UpdateFocus returns early unless this pawn is locally controlled.
	if (GetNetMode() != NM_DedicatedServer)
	{
		GetWorld()->GetTimerManager().SetTimer(FocusTimer, this, &UWEInteractionComponent::UpdateFocus, FocusUpdateInterval, true);
	}
}

void UWEInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FocusTimer);
	}
	Super::EndPlay(EndPlayReason);
}

AWECharacter* UWEInteractionComponent::GetCharacter() const
{
	return Cast<AWECharacter>(GetOwner());
}

void UWEInteractionComponent::UpdateFocus()
{
	AWECharacter* Character = GetCharacter();
	if (!Character || !Character->IsLocallyControlled())
	{
		FocusedActor.Reset();
		return;
	}

	FVector EyeLocation;
	FRotator EyeRotation;
	Character->GetActorEyesViewPoint(EyeLocation, EyeRotation);
	if (const UCameraComponent* Camera = Character->GetFirstPersonCamera())
	{
		EyeLocation = Camera->GetComponentLocation(); // follows the smoothed crouch height
	}

	const FVector End = EyeLocation + EyeRotation.Vector() * InteractionRange;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WEInteractionTrace), false, Character);
	if (AWEPickupItem* Held = Character->GetHeldItem())
	{
		Params.AddIgnoredActor(Held);
	}

	AActor* NewFocus = nullptr;
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, End, ECC_Visibility, Params))
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor && HitActor->Implements<UWEInteractable>())
		{
			NewFocus = HitActor;
		}
	}
	FocusedActor = NewFocus;

	// Close the examine panel once the player neither looks at nor holds the examined object.
	if (ExaminedActor.IsValid() && ExaminedActor.Get() != NewFocus && ExaminedActor.Get() != Character->GetHeldItem())
	{
		CloseExamine();
	}
}

// --- Requests (owning client) ------------------------------------------------------------------

void UWEInteractionComponent::RequestPrimaryInteraction()
{
	AWECharacter* Character = GetCharacter();
	if (!Character)
	{
		return;
	}

	if (Character->GetHeldItem())
	{
		if (Character->HasAuthority())
		{
			Character->DropHeldItem();
		}
		else
		{
			ServerDropHeldItem();
		}
		return;
	}

	if (AActor* Target = FocusedActor.Get())
	{
		RequestInteractWith(Target);
	}
}

void UWEInteractionComponent::RequestInteractWith(AActor* Target)
{
	if (!Target)
	{
		return;
	}
	if (GetOwner()->HasAuthority())
	{
		ExecuteInteract(Target);
	}
	else
	{
		ServerInteract(Target);
	}
}

void UWEInteractionComponent::ServerInteract_Implementation(AActor* Target)
{
	ExecuteInteract(Target);
}

void UWEInteractionComponent::ServerDropHeldItem_Implementation()
{
	if (AWECharacter* Character = GetCharacter())
	{
		Character->DropHeldItem();
	}
}

// --- Server ------------------------------------------------------------------------------------

bool UWEInteractionComponent::IsWithinReach(const AWECharacter* Character, const AActor* Target, float MaxDistance)
{
	if (!Character || !Target)
	{
		return false;
	}
	FVector EyeLocation;
	FRotator EyeRotation;
	Character->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	const FBox Bounds = Target->GetComponentsBoundingBox(/*bNonColliding*/ false);
	const float DistSq = Bounds.IsValid ? Bounds.ComputeSquaredDistanceToPoint(EyeLocation) : FVector::DistSquared(EyeLocation, Target->GetActorLocation());
	return DistSq <= FMath::Square(MaxDistance);
}

bool UWEInteractionComponent::ExecuteInteract(AActor* Target)
{
	AWECharacter* Character = GetCharacter();
	if (!Character || !Character->HasAuthority() || !IsValid(Target))
	{
		return false;
	}

	IWEInteractable* Interactable = Cast<IWEInteractable>(Target);
	if (!Interactable)
	{
		UE_LOG(LogWENet, Warning, TEXT("%s requested interaction with non-interactable %s"), *Character->GetName(), *Target->GetName());
		return false;
	}

	if (!IsWithinReach(Character, Target, InteractionRange + ServerRangeTolerance))
	{
		UE_LOG(LogWENet, Warning, TEXT("%s interaction with %s rejected: out of reach"), *Character->GetName(), *Target->GetName());
		return false;
	}

	FText Reason;
	if (!Interactable->CanInteract(Character, Reason))
	{
		UE_LOG(LogWENet, Log, TEXT("%s interaction with %s rejected: %s"), *Character->GetName(), *Target->GetName(), *Reason.ToString());
		return false;
	}

	Interactable->Interact(Character);
	return true;
}

// --- Examine / prompts (local) -----------------------------------------------------------------

void UWEInteractionComponent::ToggleExamine()
{
	if (ExaminedActor.IsValid())
	{
		CloseExamine();
		return;
	}

	AWECharacter* Character = GetCharacter();
	AActor* Candidate = Character && Character->GetHeldItem() ? Character->GetHeldItem() : FocusedActor.Get();
	FWEExamineInfo Info;
	if (IWEInteractable* Interactable = Cast<IWEInteractable>(Candidate); Interactable && Interactable->GetExamineInfo(Info))
	{
		ExaminedActor = Candidate;
	}
}

void UWEInteractionComponent::CloseExamine()
{
	ExaminedActor.Reset();
}

bool UWEInteractionComponent::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	const IWEInteractable* Interactable = Cast<IWEInteractable>(ExaminedActor.Get());
	return Interactable && Interactable->GetExamineInfo(OutInfo);
}

FText UWEInteractionComponent::GetKeyText(const UInputAction* Action) const
{
	const AWECharacter* Character = GetCharacter();
	const UWEInputConfig* Config = Character ? Character->GetInputConfig() : nullptr;
	const FKey Key = Config ? Config->GetKeyForAction(Action) : EKeys::Invalid;
	return Key.IsValid() ? Key.GetDisplayName(false) : FText::FromString(TEXT("?"));
}

void UWEInteractionComponent::BuildPrompts(TArray<FWEInteractionPrompt>& OutPrompts) const
{
	OutPrompts.Reset();

	const AWECharacter* Character = GetCharacter();
	const UWEInputConfig* Config = Character ? Character->GetInputConfig() : nullptr;
	if (!Character || !Config)
	{
		return;
	}

	const FText InteractKey = GetKeyText(Config->InteractAction);
	const FText ExamineKey = GetKeyText(Config->ExamineAction);
	const FText ExamineText = IsExamining() ? LOCTEXT("CloseExamine", "Schließen") : LOCTEXT("Examine", "Untersuchen");

	if (AWEPickupItem* Held = Character->GetHeldItem())
	{
		OutPrompts.Add({ InteractKey, LOCTEXT("Drop", "Ablegen") });
		OutPrompts.Add({ ExamineKey, ExamineText });
		return;
	}

	AActor* Focus = FocusedActor.Get();
	const IWEInteractable* Interactable = Cast<IWEInteractable>(Focus);
	if (!Interactable)
	{
		return;
	}

	FText Reason;
	if (Interactable->CanInteract(Character, Reason))
	{
		OutPrompts.Add({ InteractKey, Interactable->GetInteractionText(Character) });
	}
	else if (!Reason.IsEmpty())
	{
		OutPrompts.Add({ FText::GetEmpty(), Reason });
	}

	FWEExamineInfo Info;
	if (Interactable->GetExamineInfo(Info))
	{
		OutPrompts.Add({ ExamineKey, ExamineText });
	}
}

#undef LOCTEXT_NAMESPACE
