#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/WEInteractable.h"
#include "WEInteractionComponent.generated.h"

class AWECharacter;

/** One line of the on-screen interaction hint, e.g. key "E" + text "Aufheben". */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEInteractionPrompt
{
	GENERATED_BODY()

	/** Empty if the line is only information (e.g. "Zu schwer zum Tragen"). */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FText KeyText;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FText ActionText;
};

/**
 * Finds what the local player is looking at and forwards interaction requests to the server.
 *
 * Client side: a 20 Hz line trace (no Tick) from the camera keeps FocusedActor up to date.
 * Server side: ServerInteract / ServerDropHeldItem re-validate range and rules before acting,
 * so a modified client can neither reach across the map nor duplicate items.
 */
UCLASS(ClassGroup = (WorkshopEmpire), meta = (BlueprintSpawnableComponent))
class WORKSHOPEMPIRE_API UWEInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEInteractionComponent();

	/** [E]: drop the held item, otherwise interact with the focused object. */
	void RequestPrimaryInteraction();

	/** Sends an interaction request for a specific target (used by input and by test automation). */
	void RequestInteractWith(AActor* Target);

	/** [R]: secondary action on the focused object (e.g. take an item off the workbench). */
	void RequestSecondaryInteraction();
	void RequestSecondaryInteractWith(AActor* Target);

	/** Server-side validation and execution of a secondary interaction (public for tests). */
	bool ExecuteInteractSecondary(AActor* Target);

	/** [Q]: alternative action on the focused object (e.g. next repair step on the workbench). */
	void RequestAlternativeInteraction();
	void RequestAlternativeInteractWith(AActor* Target);
	bool ExecuteInteractAlternative(AActor* Target);

	/** [X] outside build mode: extra action on the focused object (e.g. maintain a machine). */
	void RequestExtraInteraction();
	void RequestExtraInteractWith(AActor* Target);
	bool ExecuteInteractExtra(AActor* Target);

	/** [T]: setting on the focused object (e.g. recipe of a machine). */
	void RequestSettingInteraction();
	void RequestSettingInteractWith(AActor* Target);
	bool ExecuteInteractSetting(AActor* Target);

	/** [F]: open/close the examine panel for the held or focused object. Local only. */
	void ToggleExamine();
	void CloseExamine();

	/** Re-runs the focus trace immediately (normally runs on a timer). */
	void UpdateFocus();

	AActor* GetFocusedActor() const { return FocusedActor.Get(); }
	bool IsExamining() const { return ExaminedActor.IsValid(); }
	bool GetExamineInfo(FWEExamineInfo& OutInfo) const;

	/** Builds the hint lines for the HUD from the current focus / held item. */
	void BuildPrompts(TArray<FWEInteractionPrompt>& OutPrompts) const;

	/**
	 * Server-side validation and execution of an interaction. Public so tests can drive it
	 * directly in a standalone (authority) world. Returns true if the interaction was performed.
	 */
	bool ExecuteInteract(AActor* Target);

	/** True if Target's bounds are within reach (InteractionRange + tolerance) of Character's eyes. */
	static bool IsWithinReach(const AWECharacter* Character, const AActor* Target, float MaxDistance);

	float GetInteractionRange() const { return InteractionRange; }

	// --- Assigning a task to an employee (Phase 11, local until the last pick) ---------------------

	/** [E] on an employee: the following [E] presses pick source / target in the world. */
	void StartAssignment(class AWEEmployee* Employee);
	void CancelAssignment();
	bool IsAssigning() const { return AssignEmployee.IsValid(); }
	/** Prompt for the current pick, e.g. "Quelle wählen: Lager oder Maschine". */
	FText GetAssignmentStepText() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* Target);

	UFUNCTION(Server, Reliable)
	void ServerDropHeldItem();

	UFUNCTION(Server, Reliable)
	void ServerInteractSecondary(AActor* Target);

	UFUNCTION(Server, Reliable)
	void ServerInteractAlternative(AActor* Target);

	UFUNCTION(Server, Reliable)
	void ServerInteractExtra(AActor* Target);

	UFUNCTION(Server, Reliable)
	void ServerInteractSetting(AActor* Target);

	AWECharacter* GetCharacter() const;
	FText GetKeyText(const class UInputAction* Action) const;

	/** Max distance (cm) from the eyes for focusing objects. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionRange = 250.f;

	/** Extra distance the server accepts to absorb latency and movement. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float ServerRangeTolerance = 75.f;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float FocusUpdateInterval = 0.05f;

private:
	TWeakObjectPtr<AActor> FocusedActor;
	TWeakObjectPtr<AActor> ExaminedActor;

	bool HandleAssignmentPick(AActor* Picked);
	void FinishAssignment(AActor* Source, AActor* Target);
	TWeakObjectPtr<class AWEEmployee> AssignEmployee;
	TWeakObjectPtr<AActor> AssignFirstPick;
	/** [X] on an employee must be pressed twice. */
	TWeakObjectPtr<AActor> FireArmed;
	double FireArmedUntil = 0.0;
	FTimerHandle FocusTimer;
};
