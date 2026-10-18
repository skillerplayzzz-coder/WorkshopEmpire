#pragma once

#include "CoreMinimal.h"
#include "Items/WEItemInstance.h"
#include "World/WEPlaceholderProp.h"
#include "WEWorkbench.generated.h"

class AWECharacter;
class AWEPickupItem;
class UCameraComponent;
class USoundBase;
struct FWEWorkPlan;
class UStaticMeshComponent;
struct FWEDisassemblyStep;

UENUM()
enum class EWEWorkbenchEvent : uint8
{
	None,
	Placed,
	Removed,
	StepStarted,
	StepCompleted,
	StepCancelled,
	Finished,
	/** Phase 17.5 work view: one hands-on task (screw, plug, part ...) done. */
	TaskDone
};

/** Kind of repair work (§9.5). None = the running job is a disassembly step. */
UENUM()
enum class EWERepairStepKind : uint8
{
	None,
	Diagnose,
	Remove,
	Install,
	Test,
	Refurbish
};

/** The next repair action for the item on the bench (computed, not replicated). */
struct WORKSHOPEMPIRE_API FWERepairStepInfo
{
	EWERepairStepKind Kind = EWERepairStepKind::None;
	int32 FaultIndex = INDEX_NONE;
	FText Text;
	EWEToolType Tool = EWEToolType::None;
	float Seconds = 0.f;
	/** Spare part (Install) or consumable (Refurbish) that the step uses up. */
	const class UWEItemDefinition* Material = nullptr;
};

/** Replicated state of the current job. Clients compute progress locally from the server time. */
USTRUCT()
struct WORKSHOPEMPIRE_API FWEWorkbenchWork
{
	GENERATED_BODY()

	/** Step being worked on, INDEX_NONE when idle. */
	UPROPERTY() int32 StepIndex = INDEX_NONE;
	UPROPERTY() float StartServerTime = 0.f;
	UPROPERTY() float Duration = 0.f;
	UPROPERTY() TObjectPtr<AWECharacter> Worker;

	/** Repair job kind (None = disassembly step) and the fault it works on. */
	UPROPERTY() EWERepairStepKind RepairKind = EWERepairStepKind::None;
	UPROPERTY() int8 RepairFault = INDEX_NONE;

	/** Phase 17.5: hands-on step in the work view - finished task by task (bit per task of FWEWorkPlan), no timer. */
	UPROPERTY() bool bManual = false;
	UPROPERTY() uint8 TaskCount = 0;
	UPROPERTY() uint32 TaskMask = 0;

	/** Incremented on every event so clients react even to repeated events (sounds, effects). */
	UPROPERTY() uint16 EventCounter = 0;
	UPROPERTY() EWEWorkbenchEvent LastEvent = EWEWorkbenchEvent::None;
};

/**
 * Workbench (§9): place an item on it, then disassemble it step by step with the right tools.
 *
 * - [E] while carrying a disassemblable item and looking at the bench: put it on the bench.
 * - [E] on the item on the bench: start the next step (needs the step's tool in the player inventory;
 *   better tools work faster). [R] on the item: take it off again (progress stays on the item).
 * - Results go into the bench storage (Werkbanklager); what does not fit is put next to the bench.
 * - [E] on the bench itself opens the bench storage (inherited from AWEPlaceholderProp).
 *
 * Server-authoritative: placement, steps, timers and outputs run on the server. Only the placed item
 * and a small work struct replicate; progress bars, sounds and the indicator are computed locally.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEWorkbench : public AWEPlaceholderProp
{
	GENERATED_BODY()

public:
	AWEWorkbench();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;

	/** Work speed of this bench (big benches are faster). */
	float GetBenchSpeedMultiplier() const { return BenchSpeedMultiplier; }

	// --- Placement --------------------------------------------------------------------------------

	bool CanPlaceItem(const AWEPickupItem* Item, FText& OutReason) const;

	/** Server only. Puts Item (usually from a player's hands) onto the work spot. */
	bool PlaceItem(AWEPickupItem* Item);

	/** Server only. Hands the placed item to By (carried). Not possible while a step runs. */
	bool TakeOffItem(AWECharacter* By);

	AWEPickupItem* GetPlacedItem() const { return PlacedItem; }
	FVector GetWorkSpotLocation() const;

	// --- Disassembly ------------------------------------------------------------------------------

	/** Next step for the placed item, or nullptr. */
	const FWEDisassemblyStep* GetNextStep() const;
	int32 GetNextStepIndex() const;
	int32 GetNumSteps() const;

	/** E.g. "Gehäuseschrauben lösen (Schraubendreher)". */
	FText GetNextStepPrompt() const;

	/** Part = INDEX_NONE: the next step (or, for devices with a part layout, the first part that can come out). */
	bool CanStartStep(const AWECharacter* Character, FText& OutReason, int32 Part = INDEX_NONE) const;

	/** Server only. Starts the next step for Character. */
	bool TryStartStep(AWECharacter* Character, int32 Part = INDEX_NONE);

	/** Server only. Finishes the running step now (timer callback; also used by tests). */
	void CompleteActiveStep();

	/** Server only. Stops the running step without result. */
	void CancelActiveStep();

	bool IsBusy() const { return Work.StepIndex != INDEX_NONE; }
	AWECharacter* GetWorker() const { return Work.Worker; }
	float GetActiveStepDuration() const { return Work.Duration; }

	/** 0..1 progress of the running step (any machine). */
	float GetProgress() const;

	// --- Repair (§9.5) -----------------------------------------------------------------------------

	/** Next repair action for the placed item. Kind None if there is none; OutReason then says why (may be empty). */
	FWERepairStepInfo GetNextRepairStep(FText& OutReason) const;

	/** E.g. "Fehler suchen (Multimeter)". */
	FText GetNextRepairPrompt() const;

	/** Text of the running job (repair or disassembly step) for the progress bar. */
	FText GetActiveJobText() const;

	bool CanStartRepairStep(const AWECharacter* Character, FText& OutReason) const;

	/** Server only. Starts the next repair action for Character. */
	bool TryStartRepairStep(AWECharacter* Character);

	/** Readable repair status of an item, e.g. "Netzteil defekt (ausgebaut)". Empty if nothing known. */
	static FText GetRepairStatusText(const FWEItemInstance& Item);

	/**
	 * Finds the best usable spare part (condition at least "Beschädigt", not a customer item) in the
	 * character's inventory, then in Storage. Returns false if there is none.
	 */
	static bool FindSparePart(const AWECharacter* Character, const class UWEInventoryComponent* InStorage, const UWEItemDefinition* Part,
		class UWEInventoryComponent*& OutInventory, int32& OutSlot);

	/** Best speed multiplier of a matching tool in the character's inventory; <= 0 if missing. Hands = 1. */
	static float FindToolSpeed(const AWECharacter* Character, EWEToolType Tool);

	/** Server only. Drops the current job and forgets the placed item (before a save is applied). */
	void ResetForLoad();

	/** Deterministic outputs for tests. */
	void SetRandomSeed(int32 Seed) { Random.Initialize(Seed); }

	// --- Work view (Phase 17.5): the same steps by hand - screws, plugs, cover, parts -------------------

	/** Hands-on plan of the disassembly step (bRepair false) or repair action (true) that runs now or comes next. */
	FWEWorkPlan GetWorkPlan(bool bRepair) const;
	/** Hands-on plan for one part of a device with a part layout. */
	FWEWorkPlan GetPartPlan(int32 Part) const;

	// --- Part layout (Phase 18a) ----------------------------------------------------------------------------
	/** The placed device has a real part layout (PC: case, mainboard, cards ...). */
	const class UWEAssemblyDefinition* GetAssembly() const;
	/** Disassembly step of Part (or the next step for INDEX_NONE). */
	const FWEDisassemblyStep* GetStepAt(int32 Part) const;
	/** Part being worked on (disassembly job), or INDEX_NONE. */
	int32 GetActivePart() const { return IsBusy() && Work.RepairKind == EWERepairStepKind::None ? Work.StepIndex : INDEX_NONE; }
	bool IsManualWork() const { return Work.bManual; }
	bool IsRepairJob() const { return Work.RepairKind != EWERepairStepKind::None; }
	uint32 GetTaskMask() const { return Work.TaskMask; }

	/** Server only. Starts the next step (or repair action) for Character without a timer: it ends when every task is done. */
	bool TryStartManualStep(AWECharacter* Character, bool bRepair, int32 Part = INDEX_NONE);
	/** Server only. One task of the running hands-on step, done with Tool. Checks order, tool and the minimum time. */
	bool DoManualTask(AWECharacter* Character, int32 TaskIndex, EWEToolType Tool);
	/** Server only. Character leaves the work view: a half-done hands-on step is dropped (the device keeps its progress). */
	void CancelManualStep(AWECharacter* Character);

	/** Close-up camera over the work spot (view target of the work view). */
	UCameraComponent* GetWorkCamera() const { return WorkCamera; }
	/** Local: puts the work camera above the device on the bench, looking down at it. */
	void AimWorkCamera();
	/** Local view of a device with a part layout lying on this bench (every machine), or nullptr. */
	class AWEAssemblyView* GetAssemblyView() const { return AssemblyView; }

	/** Max distance of the worker from the bench; further away cancels the step on completion. */
	float GetWorkReach() const { return WorkReach; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void RebuildVisuals() override;
	virtual void ApplyFurnitureExtras(const FWEFurnitureDefinition& Definition) override;

	UFUNCTION()
	void OnRep_PlacedItem();

	/** Work speed of this bench (Phase 8: bigger benches work faster). */
	UPROPERTY(EditAnywhere, Category = "Workbench", meta = (ClampMin = "0.1"))
	float BenchSpeedMultiplier = 1.f;

	UFUNCTION()
	void OnRep_Work();

	void RaiseEvent(EWEWorkbenchEvent Event);
	float GetServerTime() const;
	TArray<FWEItemInstance> GenerateOutputs(const FWEDisassemblyStep& Step, const FWEItemInstance& Source);
	void DeliverOutputs(const TArray<FWEItemInstance>& Outputs, AWECharacter* Worker, bool bAnnounce = true);
	void UpdateIndicator();
	void CompleteRepairStep(AWECharacter* Worker);
	/** Rolls the hidden faults of a broken repairable device once (server). */
	void EnsureFaultsRolled(FWEItemInstance& Item);
	void ShowMessage(AWECharacter* Character, const FText& Message) const;
	void PlayEventSound(EWEWorkbenchEvent Event) const;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> WorkPoint;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> WorkCamera;

	/** Thin bar on the front edge: shows progress (orange) and flashes green when a step is done. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> WorkIndicator;

	UPROPERTY(ReplicatedUsing = OnRep_PlacedItem, VisibleInstanceOnly, Category = "Workbench")
	TObjectPtr<AWEPickupItem> PlacedItem;

	UPROPERTY(ReplicatedUsing = OnRep_Work)
	FWEWorkbenchWork Work;

	UPROPERTY(EditAnywhere, Category = "Workbench", meta = (Units = "cm"))
	float WorkReach = 325.f;

	UPROPERTY(EditAnywhere, Category = "Workbench|Sound") TObjectPtr<USoundBase> SoundPlace;
	UPROPERTY(EditAnywhere, Category = "Workbench|Sound") TObjectPtr<USoundBase> SoundStepStart;
	UPROPERTY(EditAnywhere, Category = "Workbench|Sound") TObjectPtr<USoundBase> SoundStepDone;
	UPROPERTY(EditAnywhere, Category = "Workbench|Sound") TObjectPtr<USoundBase> SoundFinished;
	UPROPERTY(EditAnywhere, Category = "Workbench|Sound") TObjectPtr<USoundBase> SoundCancelled;

private:
	FRandomStream Random;
	FTimerHandle WorkTimer;
	FTimerHandle IndicatorTimer;
	float FlashUntil = 0.f;
	float LocalStepStartTime = 0.f;
	uint16 LastHandledEvent = 0;
	/** Server: when the last hands-on task was done (minimum time per task). */
	float LastTaskTime = 0.f;
	FTimerHandle ManualIdleTimer;
	void ClearManualState();
	void UpdateAssemblyView();
	UPROPERTY(Transient) TObjectPtr<class AWEAssemblyView> AssemblyView;
};
