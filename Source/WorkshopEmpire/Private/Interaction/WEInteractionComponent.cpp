#include "Interaction/WEInteractionComponent.h"

#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "Player/WEInputConfig.h"
#include "TimerManager.h"
#include "WELog.h"
#include "Workshop/WEWorkbench.h"
#include "Machines/WEMachine.h"
#include "Machines/WEMachineDefinition.h"

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

void UWEInteractionComponent::RequestSecondaryInteraction()
{
	const AWECharacter* Character = GetCharacter();
	if (Character && !Character->GetHeldItem())
	{
		RequestSecondaryInteractWith(FocusedActor.Get());
	}
}

void UWEInteractionComponent::RequestSecondaryInteractWith(AActor* Target)
{
	if (!Target)
	{
		return;
	}
	if (GetOwner()->HasAuthority())
	{
		ExecuteInteractSecondary(Target);
	}
	else
	{
		ServerInteractSecondary(Target);
	}
}

void UWEInteractionComponent::ServerInteractSecondary_Implementation(AActor* Target)
{
	ExecuteInteractSecondary(Target);
}

bool UWEInteractionComponent::ExecuteInteractSecondary(AActor* Target)
{
	AWECharacter* Character = GetCharacter();
	IWEInteractable* Interactable = Cast<IWEInteractable>(Target);
	if (!Character || !Character->HasAuthority() || !IsValid(Target) || !Interactable)
	{
		return false;
	}
	if (!IsWithinReach(Character, Target, InteractionRange + ServerRangeTolerance))
	{
		UE_LOG(LogWENet, Warning, TEXT("%s secondary interaction with %s rejected: out of reach"), *Character->GetName(), *Target->GetName());
		return false;
	}
	FText Text;
	if (!Interactable->GetSecondaryInteraction(Character, Text))
	{
		UE_LOG(LogWENet, Log, TEXT("%s secondary interaction with %s rejected: not available"), *Character->GetName(), *Target->GetName());
		return false;
	}
	Interactable->InteractSecondary(Character);
	return true;
}

void UWEInteractionComponent::RequestAlternativeInteraction()
{
	const AWECharacter* Character = GetCharacter();
	if (Character && !Character->GetHeldItem())
	{
		RequestAlternativeInteractWith(FocusedActor.Get());
	}
}

void UWEInteractionComponent::RequestAlternativeInteractWith(AActor* Target)
{
	if (!Target)
	{
		return;
	}
	if (GetOwner()->HasAuthority())
	{
		ExecuteInteractAlternative(Target);
	}
	else
	{
		ServerInteractAlternative(Target);
	}
}

void UWEInteractionComponent::ServerInteractAlternative_Implementation(AActor* Target)
{
	ExecuteInteractAlternative(Target);
}

bool UWEInteractionComponent::ExecuteInteractAlternative(AActor* Target)
{
	AWECharacter* Character = GetCharacter();
	IWEInteractable* Interactable = Cast<IWEInteractable>(Target);
	if (!Character || !Character->HasAuthority() || !IsValid(Target) || !Interactable)
	{
		return false;
	}
	if (!IsWithinReach(Character, Target, InteractionRange + ServerRangeTolerance))
	{
		UE_LOG(LogWENet, Warning, TEXT("%s alternative interaction with %s rejected: out of reach"), *Character->GetName(), *Target->GetName());
		return false;
	}
	FText Text;
	FText Reason;
	if (!Interactable->GetAlternativeInteraction(Character, Text, Reason))
	{
		UE_LOG(LogWENet, Log, TEXT("%s alternative interaction with %s rejected: %s"), *Character->GetName(), *Target->GetName(), *Reason.ToString());
		return false;
	}
	Interactable->InteractAlternative(Character);
	return true;
}

void UWEInteractionComponent::RequestExtraInteraction()
{
	const AWECharacter* Character = GetCharacter();
	if (Character && !Character->GetHeldItem())
	{
		RequestExtraInteractWith(FocusedActor.Get());
	}
}

void UWEInteractionComponent::RequestExtraInteractWith(AActor* Target)
{
	if (!Target)
	{
		return;
	}
	if (GetOwner()->HasAuthority())
	{
		ExecuteInteractExtra(Target);
	}
	else
	{
		ServerInteractExtra(Target);
	}
}

void UWEInteractionComponent::ServerInteractExtra_Implementation(AActor* Target)
{
	ExecuteInteractExtra(Target);
}

bool UWEInteractionComponent::ExecuteInteractExtra(AActor* Target)
{
	AWECharacter* Character = GetCharacter();
	IWEInteractable* Interactable = Cast<IWEInteractable>(Target);
	if (!Character || !Character->HasAuthority() || !IsValid(Target) || !Interactable)
	{
		return false;
	}
	if (!IsWithinReach(Character, Target, InteractionRange + ServerRangeTolerance))
	{
		UE_LOG(LogWENet, Warning, TEXT("%s extra interaction with %s rejected: out of reach"), *Character->GetName(), *Target->GetName());
		return false;
	}
	FText Text;
	FText Reason;
	if (!Interactable->GetExtraInteraction(Character, Text, Reason))
	{
		UE_LOG(LogWENet, Log, TEXT("%s extra interaction with %s rejected: %s"), *Character->GetName(), *Target->GetName(), *Reason.ToString());
		return false;
	}
	Interactable->InteractExtra(Character);
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
		// Same check the server makes when [E] is pressed (loose items on the bench are ignored).
		const AWEWorkbench* Bench = Character->FindAimedWorkbench();
		const AWEMachine* Machine = Character->FindAimedMachine();
		FText PlaceReason;
		if (Bench && Bench->CanPlaceItem(Held, PlaceReason))
		{
			OutPrompts.Add({ InteractKey, LOCTEXT("PlaceOnBench", "Auf die Werkbank legen") });
		}
		else if (Machine && Machine->CanInsertItem(Held, PlaceReason))
		{
			OutPrompts.Add({ InteractKey, FText::Format(LOCTEXT("PutInMachine", "In die Maschine geben ({0})"), Machine->GetMachineDefinition() ? Machine->GetMachineDefinition()->DisplayName : FText::GetEmpty()) });
		}
		else
		{
			OutPrompts.Add({ InteractKey, LOCTEXT("Drop", "Ablegen") });
			if ((Bench || Machine) && !PlaceReason.IsEmpty())
			{
				OutPrompts.Add({ FText::GetEmpty(), PlaceReason });
			}
		}
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

	FText SecondaryText;
	if (Interactable->GetSecondaryInteraction(Character, SecondaryText))
	{
		OutPrompts.Add({ GetKeyText(Config->SecondaryAction), SecondaryText });
	}

	FText AlternativeText;
	FText AlternativeReason;
	if (Interactable->GetAlternativeInteraction(Character, AlternativeText, AlternativeReason))
	{
		OutPrompts.Add({ GetKeyText(Config->AlternativeAction), AlternativeText });
	}
	else if (!AlternativeReason.IsEmpty())
	{
		OutPrompts.Add({ FText::GetEmpty(), AlternativeReason });
	}

	FText ExtraText;
	FText ExtraReason;
	if (Interactable->GetExtraInteraction(Character, ExtraText, ExtraReason))
	{
		OutPrompts.Add({ GetKeyText(Config->RemoveAction), ExtraText });
	}
	else if (!ExtraReason.IsEmpty())
	{
		OutPrompts.Add({ FText::GetEmpty(), ExtraReason });
	}

	FWEExamineInfo Info;
	if (Interactable->GetExamineInfo(Info))
	{
		OutPrompts.Add({ ExamineKey, ExamineText });
	}
}

#undef LOCTEXT_NAMESPACE
