#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WETutorialComponent.generated.h"

/** What players did - the tutorial moves on when the current step's event happens (server). */
UENUM()
enum class EWETutorialEvent : uint8
{
	Moved,
	ToolStored,
	InventoryOpened,
	ItemPlacedOnBench,
	WorkViewOpened,
	DisassemblyStep,
	RepairStep,
	ComputerOpened,
	ItemSold,
	ContractAccepted,
	GameSaved
};

UENUM()
enum class EWETutorialState : uint8
{
	/** Not offered (old saves, dev runs): the HUD shows the "Erste Schritte" guide as before. */
	Off,
	Active,
	Skipped,
	Completed
};

/** One step: short text, the key, what finishes it and where its marker stands in the world. */
struct FWETutorialStep
{
	FText Title;
	FText Text;
	FText Key;
	EWETutorialEvent Event = EWETutorialEvent::Moved;
	/** Marker target: class name of an actor (e.g. "WEWorkbench") or an item id ("Item:Screwdriver"); empty = none. */
	FString Target;
};

/**
 * Phase 17.5 tutorial: offered when a new company starts, ten short steps through movement, inventory,
 * disassembly (work view), repair, the workshop PC, selling and orders. Hints appear in the world (HUD markers)
 * and as one line on the objective card - no text walls. Can be skipped at any time and restarted later
 * (pause menu, Ferrit OS help). Lives on the game state (one company, one tutorial), replicated and saved.
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWETutorialComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWETutorialComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	static UWETutorialComponent* Get(const UObject* WorldContext);

	static const TArray<FWETutorialStep>& GetSteps();
	static int32 NumSteps() { return GetSteps().Num(); }

	EWETutorialState GetState() const { return State; }
	bool IsActive() const { return State == EWETutorialState::Active; }
	int32 GetStep() const { return Step; }
	const FWETutorialStep* GetCurrentStep() const;

	/** World positions for the current step's marker (any machine). */
	void GetMarkerLocations(TArray<FVector>& OutLocations) const;

	// --- Server -------------------------------------------------------------------------------------------
	void Start();
	void Skip();
	void NotifyEvent(EWETutorialEvent Event);
	/** Loading / tests. */
	void SetState(EWETutorialState InState, int32 InStep);

protected:
	UFUNCTION()
	void OnRep_Tutorial();

	void CheckMovement();
	void Announce(const FText& Message) const;

	UPROPERTY(ReplicatedUsing = OnRep_Tutorial)
	EWETutorialState State = EWETutorialState::Off;

	UPROPERTY(ReplicatedUsing = OnRep_Tutorial)
	int32 Step = 0;

private:
	FTimerHandle MoveTimer;
	TMap<TWeakObjectPtr<APawn>, FVector> StartPositions;
};
