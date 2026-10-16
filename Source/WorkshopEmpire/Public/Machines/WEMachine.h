#pragma once

#include "CoreMinimal.h"
#include "Items/WEItemInstance.h"
#include "World/WEPlaceholderProp.h"
#include "WEMachine.generated.h"

class AWECharacter;
class AWEPickupItem;
class UWEInventoryComponent;
class UWEMachineDefinition;
struct FWEMachineRecipe;

UENUM(BlueprintType)
enum class EWEMachineState : uint8
{
	Off,
	/** On, waiting for input. */
	Idle,
	Running,
	/** Output storage full: take the products out. */
	OutputFull,
	/** Not enough free power on the grid to start a cycle. */
	NoPower,
	/** Maintenance at 0 %: needs a maintenance kit. */
	NeedsMaintenance
};

/** Replicated machine state (§13.5 "Replizierbare Maschinenzustände"). Progress is computed locally. */
USTRUCT()
struct WORKSHOPEMPIRE_API FWEMachineStatus
{
	GENERATED_BODY()

	UPROPERTY() EWEMachineState State = EWEMachineState::Off;
	UPROPERTY() bool bOn = false;
	UPROPERTY() int8 RecipeIndex = INDEX_NONE;
	/** Progress (0-1) when the current run segment started (cycles can pause when switched off). */
	UPROPERTY() float ProgressAtStart = 0.f;
	/** Seconds for a whole cycle at the current efficiency. */
	UPROPERTY() float CycleSeconds = 0.f;
	UPROPERTY() uint8 Maintenance = 100;
	UPROPERTY() uint16 EventCounter = 0;
	/** Recipe chosen with [T] (Phase 10); INDEX_NONE = automatic (first recipe that has its material). */
	UPROPERTY() int8 SelectedRecipe = INDEX_NONE;
};

/**
 * Industrial machine (§13, Phase 9): shredder, sorting machine, metal press. Placed and moved with the
 * build mode like furniture; its behaviour comes from a UWEMachineDefinition (recipes, power, wear).
 *
 * - [E]: open the input storage (small items). Carrying a bulky device and looking at the machine: put it in.
 * - [R]: take all products (output storage) into the inventory.
 * - [Q]: switch on/off.  [X]: maintenance with a maintenance kit.
 *
 * There is no per-machine tick: UWEPowerGridComponent steps all machines on the server on a fixed
 * interval and decides who gets power. A cycle takes its inputs at the start (so they cannot be taken
 * out halfway) and delivers the products at the end; products that are too bulky land in front of it.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEMachine : public AWEPlaceholderProp
{
	GENERATED_BODY()

public:
	AWEMachine();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IWEInteractable
	virtual FText GetInteractionText(const AWECharacter* Interactor) const override;
	virtual bool GetSecondaryInteraction(const AWECharacter* Interactor, FText& OutText) const override;
	virtual void InteractSecondary(AWECharacter* Interactor) override;
	virtual bool GetAlternativeInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const override;
	virtual void InteractAlternative(AWECharacter* Interactor) override;
	virtual bool GetExtraInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const override;
	virtual void InteractExtra(AWECharacter* Interactor) override;
	virtual bool GetSettingInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const override;
	virtual void InteractSetting(AWECharacter* Interactor) override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;

	const UWEMachineDefinition* GetMachineDefinition() const { return MachineDefinition; }
	void SetMachineDefinition(UWEMachineDefinition* InDefinition);
	UWEInventoryComponent* GetOutputStorage() const { return OutputStorage; }
	const FWEMachineStatus& GetStatus() const { return Status; }
	bool IsOn() const { return Status.bOn; }
	bool IsRunning() const { return Status.State == EWEMachineState::Running; }
	float GetMaintenance() const { return Status.Maintenance; }
	const TArray<FWEItemInstance>& GetProcessing() const { return Processing; }

	/** 0-1 progress of the current cycle (any machine, smooth on clients). */
	float GetProgress() const;

	/** Current power draw for the grid (kW). */
	float GetPowerDrawKw() const;

	/** E.g. "Läuft – Elektronik schreddern". */
	FText GetStatusText() const;
	static FText GetStateText(EWEMachineState State);

	/** Efficiency from maintenance (1 normally, 0.7 when worn). */
	float GetEfficiency() const;

	/** Can a carried item go in? (accepted by a recipe, not a customer's, fits). */
	bool CanInsertItem(const AWEPickupItem* Item, FText& OutReason) const;

	/** Recipe chosen with [T]; INDEX_NONE = automatic. */
	int32 GetSelectedRecipe() const { return Status.SelectedRecipe; }
	/** "Automatisch" or the recipe name. */
	FText GetSelectedRecipeText() const;

	/** Material flow (Phase 10): would the input take this from a conveyor? (fits, matches the chosen recipe). */
	bool CanAcceptFromBelt(const FWEItemInstance& Item) const;

	// --- Server only --------------------------------------------------------------------------------

	bool InsertItem(AWEPickupItem* Item);
	bool SetPower(bool bOn);
	bool Maintain(AWECharacter* Character);
	/** Takes all products into the character's inventory (as much as fits). Returns units moved. */
	int32 TakeOutputs(AWECharacter* Character);

	/** Chooses a recipe (INDEX_NONE = automatic); applies from the next cycle. */
	bool SelectRecipe(int32 RecipeIndex);
	/** Next choice: automatic -> recipe 1 -> ... -> automatic. */
	void CycleRecipe();

	/** Material flow: puts a conveyor item into the input (all or nothing). */
	bool AcceptFromBelt(const FWEItemInstance& Item);
	/** Material flow: takes the first product stack for a conveyor at the output. */
	bool TakeOutputForBelt(FWEItemInstance& OutItem);
	/** Set by the flow simulation: a conveyor starts at this machine (bulky products then stay in the output). */
	void SetOutputConnected(bool bConnected) { bOutputConnected = bConnected; }
	bool IsOutputConnected() const { return bOutputConnected; }

	/** Staff (Phase 11): an operator at the machine makes it work faster (applies from the next cycle). */
	void SetOperatorBonus(float Bonus) { OperatorBonus = FMath::Max(1.f, Bonus); }
	float GetOperatorBonus() const { return OperatorBonus; }
	/** Staff: maintenance with a kit the employee brought (the kit is already used up). */
	void MaintainWithKit();

	/** Simulation (called by the power grid). */
	void AdvanceCycle(float DeltaSeconds);
	/** Recipe that could start now (input present, output room), or INDEX_NONE. Sets the waiting state. */
	int32 FindStartableRecipe();
	void StartCycle(int32 RecipeIndex);
	void SetWaitingForPower();
	void CompleteCycle();

	/** Loading / tests. */
	void RestoreState(bool bOn, uint8 Maintenance, int32 RecipeIndex, float Progress, float CycleSeconds, const TArray<FWEItemInstance>& InProcessing,
		int32 SelectedRecipe = INDEX_NONE);
	void SetRandomSeed(int32 Seed) { Random.Initialize(Seed); }

	/** Weight of the items in the running cycle (kg). */
	float GetProcessingWeightKg() const;

protected:
	virtual void BeginPlay() override;
	virtual void ApplyFurnitureExtras(const FWEFurnitureDefinition& Definition) override;
	virtual void RebuildVisuals() override;

	UFUNCTION()
	void OnRep_Status();

	void SetState(EWEMachineState NewState);
	float GetServerTime() const;
	bool MatchesRecipe(const FWEMachineRecipe& Recipe, int32& OutSlot) const;
	/** Units of an ingredient in the input (bSecond: the second ingredient). First matching slot in OutSlot. */
	int32 CountIngredient(const FWEMachineRecipe& Recipe, bool bSecond, int32& OutSlot) const;
	/** Moves Quantity units of an ingredient from the input to Processing. */
	bool TakeIngredient(const FWEMachineRecipe& Recipe, bool bSecond, int32 Quantity);
	void DeliverProducts(const TArray<FWEItemInstance>& Products);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UWEInventoryComponent> OutputStorage;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Status, Category = "Machine")
	TObjectPtr<UWEMachineDefinition> MachineDefinition;

	UPROPERTY(ReplicatedUsing = OnRep_Status)
	FWEMachineStatus Status;

	/** Inputs of the running cycle (taken from the input storage at the start). */
	UPROPERTY(Replicated)
	TArray<FWEItemInstance> Processing;

private:
	float Progress = 0.f;
	float LocalSegmentStart = 0.f;
	bool bOutputConnected = false;
	float OperatorBonus = 1.f;
	uint16 LastHandledEvent = 0;
	FRandomStream Random;
};
