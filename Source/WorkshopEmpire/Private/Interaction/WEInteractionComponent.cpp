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
#include "Workshop/WEWorkPlan.h"
#include "Machines/WEMachine.h"
#include "Machines/WEConveyor.h"
#include "Staff/WEEmployee.h"
#include "Logistics/WEVehicle.h"
#include "Framework/WEPlayerController.h"
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

	if (IsAssigning())
	{
		HandleAssignmentPick(FocusedActor.Get());
		return;
	}
	if (AWEEmployee* Employee = Cast<AWEEmployee>(FocusedActor.Get()))
	{
		StartAssignment(Employee);
		return;
	}
	if (AActor* Target = FocusedActor.Get())
	{
		if (TryOpenWorkView(Target, false))
		{
			return;
		}
		RequestInteractWith(Target);
	}
}

bool UWEInteractionComponent::TryOpenWorkView(AActor* Target, bool bRepair)
{
	// Phase 17.5: a device on the workbench opens the hands-on work view instead of a timed step.
	const AWEPickupItem* Item = Cast<AWEPickupItem>(Target);
	AWEWorkbench* Bench = Item ? Item->GetPlacedOn() : nullptr;
	AWECharacter* Character = GetCharacter();
	AWEPlayerController* PC = Character ? Cast<AWEPlayerController>(Character->GetController()) : nullptr;
	if (!Bench || !PC || !PC->IsLocalController() || !WEWorkPlan::IsWorkViewEnabled())
	{
		return false;
	}
	if (Bench->IsBusy() && Bench->GetWorker() != Character)
	{
		return false; // someone else works there: the prompt says so
	}
	FText Reason;
	const bool bCanRepair = Bench->GetNextRepairStep(Reason).Kind != EWERepairStepKind::None;
	if (!Bench->GetNextStep() && !bCanRepair)
	{
		return false;
	}
	PC->OpenWorkView(Bench, bRepair);
	return true;
}

// --- Assignment (local picks, one server request at the end) ---------------------------------------

void UWEInteractionComponent::StartAssignment(AWEEmployee* Employee)
{
	if (!Employee)
	{
		return;
	}
	AssignEmployee = Employee;
	AssignFirstPick.Reset();
}

void UWEInteractionComponent::CancelAssignment()
{
	AssignEmployee.Reset();
	AssignFirstPick.Reset();
}

FText UWEInteractionComponent::GetAssignmentStepText() const
{
	const AWEEmployee* Employee = AssignEmployee.Get();
	if (!Employee)
	{
		return FText::GetEmpty();
	}
	switch (Employee->GetProfile().Role)
	{
	case EWEStaffRole::WarehouseWorker:
		return AssignFirstPick.IsValid() ? LOCTEXT("PickTarget", "Ziel wählen: Lager, Maschine oder Wagen")
			: LOCTEXT("PickSource", "Quelle wählen: Lager, Maschinenausgabe oder Wagen");
	case EWEStaffRole::MachineOperator:
		return AssignFirstPick.IsValid() ? LOCTEXT("PickStore", "Lager für Produkte und Wartungssets wählen")
			: LOCTEXT("PickMachine", "Maschine wählen");
	case EWEStaffRole::Driver:
		return LOCTEXT("PickVan", "Firmenwagen wählen");
	}
	return FText::GetEmpty();
}

bool UWEInteractionComponent::HandleAssignmentPick(AActor* Picked)
{
	AWEEmployee* Employee = AssignEmployee.Get();
	if (!Employee || !Picked)
	{
		return false;
	}
	const EWEStaffRole Role = Employee->GetProfile().Role;
	if (Role == EWEStaffRole::WarehouseWorker && !AssignFirstPick.IsValid())
	{
		if (!Employee->IsValidSource(Picked))
		{
			return false;
		}
		AssignFirstPick = Picked;
		return true;
	}
	if (Role == EWEStaffRole::WarehouseWorker)
	{
		if (!Employee->IsValidTarget(Picked) || Picked == AssignFirstPick.Get())
		{
			return false;
		}
		FinishAssignment(AssignFirstPick.Get(), Picked);
		return true;
	}
	if (Role == EWEStaffRole::MachineOperator && !AssignFirstPick.IsValid())
	{
		if (!Employee->IsValidTarget(Picked))
		{
			return false;
		}
		AssignFirstPick = Picked;
		return true;
	}
	if (Role == EWEStaffRole::MachineOperator)
	{
		if (!Employee->IsValidSource(Picked))
		{
			return false;
		}
		FinishAssignment(Picked, AssignFirstPick.Get());
		return true;
	}
	if (!Employee->IsValidTarget(Picked))
	{
		return false;
	}
	FinishAssignment(nullptr, Picked);
	return true;
}

void UWEInteractionComponent::FinishAssignment(AActor* Source, AActor* Target)
{
	AWECharacter* Character = GetCharacter();
	AWEPlayerController* PC = Character ? Cast<AWEPlayerController>(Character->GetController()) : nullptr;
	if (PC && AssignEmployee.IsValid())
	{
		PC->RequestAssignTask(AssignEmployee.Get(), Source, Target);
	}
	CancelAssignment();
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
	if (IsAssigning() && AssignFirstPick.IsValid() && AssignEmployee->GetProfile().Role == EWEStaffRole::MachineOperator)
	{
		FinishAssignment(nullptr, AssignFirstPick.Get()); // operator without a store
		return;
	}
	const AWECharacter* Character = GetCharacter();
	if (Character && !Character->GetHeldItem())
	{
		if (TryOpenWorkView(FocusedActor.Get(), true))
		{
			return;
		}
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
	if (IsAssigning())
	{
		CancelAssignment();
		return;
	}
	if (Character && !Character->GetHeldItem())
	{
		AActor* Target = FocusedActor.Get();
		if (Target && Target->IsA<AWEEmployee>())
		{
			// Firing needs a second press within a few seconds.
			const double Now = FPlatformTime::Seconds();
			if (FireArmed.Get() != Target || Now > FireArmedUntil)
			{
				FireArmed = Target;
				FireArmedUntil = Now + 3.0;
				if (AWEPlayerController* PC = Cast<AWEPlayerController>(Character->GetController()))
				{
					PC->ClientShowMessage(LOCTEXT("FireConfirm", "Wirklich entlassen? Zum Bestätigen noch einmal [X]"));
				}
				return;
			}
			FireArmed.Reset();
		}
		RequestExtraInteractWith(Target);
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

void UWEInteractionComponent::RequestSettingInteraction()
{
	const AWECharacter* Character = GetCharacter();
	if (Character && !Character->GetHeldItem())
	{
		RequestSettingInteractWith(FocusedActor.Get());
	}
}

void UWEInteractionComponent::RequestSettingInteractWith(AActor* Target)
{
	if (!Target)
	{
		return;
	}
	if (GetOwner()->HasAuthority())
	{
		ExecuteInteractSetting(Target);
	}
	else
	{
		ServerInteractSetting(Target);
	}
}

void UWEInteractionComponent::ServerInteractSetting_Implementation(AActor* Target)
{
	ExecuteInteractSetting(Target);
}

bool UWEInteractionComponent::ExecuteInteractSetting(AActor* Target)
{
	AWECharacter* Character = GetCharacter();
	IWEInteractable* Interactable = Cast<IWEInteractable>(Target);
	if (!Character || !Character->HasAuthority() || !IsValid(Target) || !Interactable)
	{
		return false;
	}
	if (!IsWithinReach(Character, Target, InteractionRange + ServerRangeTolerance))
	{
		UE_LOG(LogWENet, Warning, TEXT("%s setting on %s rejected: out of reach"), *Character->GetName(), *Target->GetName());
		return false;
	}
	FText Text;
	FText Reason;
	if (!Interactable->GetSettingInteraction(Character, Text, Reason))
	{
		UE_LOG(LogWENet, Log, TEXT("%s setting on %s rejected: %s"), *Character->GetName(), *Target->GetName(), *Reason.ToString());
		return false;
	}
	Interactable->InteractSetting(Character);
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
	if (Character->GetForklift() && !IsAssigning())
	{
		OutPrompts.Add({ GetKeyText(Config->RemoveAction), LOCTEXT("ExitForklift", "Vom Gabelstapler absteigen") });
	}
	const FText ExamineKey = GetKeyText(Config->ExamineAction);
	const FText ExamineText = IsExamining() ? LOCTEXT("CloseExamine", "Schließen") : LOCTEXT("Examine", "Untersuchen");

	if (AWEPickupItem* Held = Character->GetHeldItem())
	{
		// Same check the server makes when [E] is pressed (loose items on the bench are ignored).
		const AWEWorkbench* Bench = Character->FindAimedWorkbench();
		const AWEMachine* Machine = Character->FindAimedMachine();
		const AWEConveyor* Belt = Cast<AWEConveyor>(Character->FindAimedProp());
		const AWEVehicle* Van = Cast<AWEVehicle>(Character->FindAimedProp());
		FText PlaceReason;
		if (Bench && Bench->CanPlaceItem(Held, PlaceReason))
		{
			OutPrompts.Add({ InteractKey, LOCTEXT("PlaceOnBench", "Auf die Werkbank legen") });
		}
		else if (Machine && Machine->CanInsertItem(Held, PlaceReason))
		{
			OutPrompts.Add({ InteractKey, FText::Format(LOCTEXT("PutInMachine", "In die Maschine geben ({0})"), Machine->GetMachineDefinition() ? Machine->GetMachineDefinition()->DisplayName : FText::GetEmpty()) });
		}
		else if (Belt && Belt->CanPutOnBelt(Held, Character, PlaceReason))
		{
			OutPrompts.Add({ InteractKey, LOCTEXT("PutOnBelt", "Aufs Band legen") });
		}
		else if (Van && Van->CanLoad(Held, PlaceReason))
		{
			OutPrompts.Add({ InteractKey, LOCTEXT("LoadVan", "In den Laderaum legen") });
		}
		else
		{
			OutPrompts.Add({ InteractKey, LOCTEXT("Drop", "Ablegen") });
			if ((Bench || Machine || Belt || Van) && !PlaceReason.IsEmpty())
			{
				OutPrompts.Add({ FText::GetEmpty(), PlaceReason });
			}
		}
		OutPrompts.Add({ ExamineKey, ExamineText });
		return;
	}

	AActor* Focus = FocusedActor.Get();
	if (const AWEEmployee* Assigning = AssignEmployee.Get())
	{
		OutPrompts.Add({ FText::GetEmpty(), FText::Format(LOCTEXT("AssignFor", "Aufgabe für {0}: {1}"), FText::FromString(Assigning->GetProfile().Name), GetAssignmentStepText()) });
		const bool bSecond = AssignFirstPick.IsValid();
		const EWEStaffRole Role = Assigning->GetProfile().Role;
		const bool bWantsSource = (Role == EWEStaffRole::WarehouseWorker && !bSecond) || (Role == EWEStaffRole::MachineOperator && bSecond);
		const bool bValid = Focus && Focus != AssignFirstPick.Get() && (bWantsSource ? Assigning->IsValidSource(Focus) : Assigning->IsValidTarget(Focus));
		if (bValid)
		{
			const AWEPlaceholderProp* Prop = Cast<AWEPlaceholderProp>(Focus);
			OutPrompts.Add({ InteractKey, FText::Format(LOCTEXT("PickThis", "{0} wählen"), Prop ? Prop->GetFurnitureName() : FText::FromString(Focus->GetName())) });
		}
		if (Role == EWEStaffRole::MachineOperator && bSecond)
		{
			OutPrompts.Add({ GetKeyText(Config->AlternativeAction), LOCTEXT("NoStore", "ohne Lager") });
		}
		OutPrompts.Add({ GetKeyText(Config->RemoveAction), LOCTEXT("CancelAssign", "Abbrechen") });
		return;
	}
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

	FText SettingText;
	FText SettingReason;
	if (Interactable->GetSettingInteraction(Character, SettingText, SettingReason))
	{
		OutPrompts.Add({ GetKeyText(Config->SettingAction), SettingText });
	}

	FWEExamineInfo Info;
	if (Interactable->GetExamineInfo(Info))
	{
		OutPrompts.Add({ ExamineKey, ExamineText });
	}
}

#undef LOCTEXT_NAMESPACE
