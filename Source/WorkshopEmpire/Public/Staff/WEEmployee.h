#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interaction/WEInteractable.h"
#include "Items/WEItemInstance.h"
#include "Staff/WEStaffTypes.h"
#include "WEEmployee.generated.h"

class AWEMachine;
class UStaticMeshComponent;
class UWEInventoryComponent;

/**
 * An employee (§15, Phase 11): a simple, readable worker with one task at a time.
 *
 * - Lagerarbeiter: Transport – walk to the source (storage or machine output), take what the target
 *   takes (up to 25 kg, or one bulky item up to 30 kg), walk to the target, put it in, repeat.
 * - Maschinenbediener: OperateMachine – stays at a machine, switches it on, maintains it with a kit
 *   from the assigned storage, carries finished products there; the machine works 15 % faster.
 * - Fahrer: DriveVehicle – waits at the van; a tour needs him.
 *
 * Server only: decisions run in StepWork (called by UWEStaffComponent on a timer); walking steers the
 * character towards path points from the navigation system (straight line if there is none) and the
 * character movement replicates as usual. If he gets stuck he steps over the obstacle.
 *
 * [E] Aufgabe zuweisen (picks follow in the world), [Q] Pause, [T] Material, [X] Entlassen, [F] Details.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEEmployee : public ACharacter, public IWEInteractable
{
	GENERATED_BODY()

public:
	/** Work clothes colour of the role (replicated). */
	const FLinearColor& GetTint() const { return Tint; }
	AWEEmployee();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	// IWEInteractable
	virtual FText GetInteractionText(const class AWECharacter* Interactor) const override;
	virtual bool CanInteract(const class AWECharacter* Interactor, FText& OutReason) const override;
	virtual void Interact(class AWECharacter* Interactor) override {}
	virtual bool GetAlternativeInteraction(const class AWECharacter* Interactor, FText& OutText, FText& OutReason) const override;
	virtual void InteractAlternative(class AWECharacter* Interactor) override;
	virtual bool GetExtraInteraction(const class AWECharacter* Interactor, FText& OutText, FText& OutReason) const override;
	virtual void InteractExtra(class AWECharacter* Interactor) override;
	virtual bool GetSettingInteraction(const class AWECharacter* Interactor, FText& OutText, FText& OutReason) const override;
	virtual void InteractSetting(class AWECharacter* Interactor) override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;

	const FWEEmployeeProfile& GetProfile() const { return Profile; }
	const FWEEmployeeTask& GetTask() const { return Task; }
	EWEEmployeeState GetState() const { return State; }
	const FWEItemInstance& GetCarried() const { return Carried; }
	bool IsPaused() const { return bPaused; }

	FText GetRoleText() const;
	static FText GetRoleText(EWEStaffRole Role);
	FText GetStateText() const;
	/** E.g. "Transport: Materiallager → Fertigungsautomat (Stahlschrott)". */
	FText GetTaskText() const;
	FText GetWageText() const;

	/** Which task kinds this role can do. */
	static EWETaskKind GetTaskKindForRole(EWEStaffRole Role);
	/** Is Actor a valid source / target for this employee's role? */
	bool IsValidSource(const AActor* Actor) const;
	bool IsValidTarget(const AActor* Actor) const;

	// --- Server -----------------------------------------------------------------------------------

	void Setup(const FWEEmployeeProfile& InProfile, const FLinearColor& InTint);
	bool AssignTask(const FWEEmployeeTask& NewTask);
	/** No task any more (puts back what he carries). */
	void ClearTask();
	void SetPaused(bool bInPaused);
	void SetUnpaid(bool bInUnpaid);
	/** Transport filter: everything -> each material in the source -> everything. */
	void CycleFilter();
	/** Driver: in the van on a tour (hidden) or back. */
	void SetAway(bool bInAway, const FVector& ReturnLocation = FVector::ZeroVector);
	/** Driver at his van and able to go. */
	bool IsReadyToDrive() const;

	/** One decision step (timer of the staff component). */
	void StepWork(float DeltaSeconds);

	/** Seconds worked since the last payroll (for the wage). */
	float GetUnpaidSeconds() const { return UnpaidSeconds; }
	void ResetUnpaidSeconds() { UnpaidSeconds = 0.f; }

	/** Loading. */
	void RestoreCarried(const FWEItemInstance& Item);

	/** Tests: walking takes no time (test worlds do not tick). */
	static bool bInstantTravel;

	static constexpr float CarryKg = 25.f;
	static constexpr float CarryBulkyKg = 30.f;
	static constexpr float BaseWalkSpeed = 220.f;
	static constexpr float HandlingSeconds = 1.f;
	static constexpr float RetrySeconds = 3.f;
	static constexpr float OperatorBonus = 1.15f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_Visuals();

	/** Transport / operate phases. */
	enum class EPhase : uint8 { GoToSource, Take, GoToTarget, Put, AtMachine, FetchKit, BringKit, CarryProducts };

	void SetState(EWEEmployeeState NewState, const FText& Detail = FText::GetEmpty());
	void StepTransport(float DeltaSeconds);
	void StepOperate(float DeltaSeconds);
	void StepDrive(float DeltaSeconds);
	void ReleaseMachine();

	/** Walks towards Actor; true once next to it. */
	bool WalkTo(const AActor* Actor);
	FVector GetStandPoint(const AActor* Actor) const;
	bool IsNextTo(const AActor* Actor) const;
	void StopWalking();

	/** Inventory to take from / put into (storage, machine input or output, van cargo). */
	static UWEInventoryComponent* GetTakeInventory(AActor* Actor);
	/** How many units of Item the target takes now (machine: only its ingredients). */
	int32 GetAddableAt(const AActor* Destination, const FWEItemInstance& Item) const;
	/** Takes what Destination takes now (or only OnlyItem, e.g. a maintenance kit). */
	bool TakeFrom(AActor* Source, const AActor* Destination, const UWEItemDefinition* OnlyItem = nullptr);
	bool PutInto(AActor* Target);
	/** Puts what he carries back into the source (task changed, fired). */
	void ReturnCarried();

	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> CarriedMesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> PlaceholderMaterial;

	UPROPERTY(Replicated) FWEEmployeeProfile Profile;
	UPROPERTY(Replicated) FWEEmployeeTask Task;
	UPROPERTY(Replicated) EWEEmployeeState State = EWEEmployeeState::Idle;
	/** Short reason while waiting, e.g. "Quelle leer". */
	UPROPERTY(Replicated) FText StateDetail;
	UPROPERTY(ReplicatedUsing = OnRep_Visuals) FWEItemInstance Carried;
	UPROPERTY(ReplicatedUsing = OnRep_Visuals) FLinearColor Tint = FLinearColor(0.2f, 0.35f, 0.6f);
	UPROPERTY(ReplicatedUsing = OnRep_Visuals) bool bAway = false;
	UPROPERTY(Replicated) bool bPaused = false;
	UPROPERTY(Replicated) bool bUnpaid = false;

private:
	EPhase Phase = EPhase::GoToSource;
	float PhaseTimer = 0.f;
	float WaitTimer = 0.f;
	float UnpaidSeconds = 0.f;

	// Walking (server).
	TWeakObjectPtr<const AActor> WalkTarget;
	TArray<FVector> Path;
	int32 PathIndex = 0;
	bool bWalking = false;
	FVector LastCheckLocation = FVector::ZeroVector;
	float StuckTimer = 0.f;
	float NoPathUntil = 0.f;

	TWeakObjectPtr<AWEMachine> OperatedMachine;
};
