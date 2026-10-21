#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/WEItemTypes.h"
#include "Workshop/WEWorkPlan.h"
#include "WEToolHand.generated.h"

class AWEWorkbench;
class UStaticMeshComponent;

/** What the tool in the hand does this frame (filled by the work view). */
struct FWEToolHandState
{
	/** Index into the tool list (0 = hands: no tool in the hand). */
	int32 Selected = 0;
	/** Cursor over the device: the tool hovers there, ready. */
	bool bHover = false;
	FVector HoverPoint = FVector::ZeroVector;
	/** Working on a task: the tip sits on it (along Axis, pointing into the device). */
	bool bWorking = false;
	FVector WorkPoint = FVector::ZeroVector;
	FVector WorkAxis = FVector::UpVector;
	EWEWorkTaskKind WorkKind = EWEWorkTaskKind::Screw;
	float Progress = 0.f;
	/** A loosened screw hangs on the magnetic tip on its way to the bowl. */
	bool bCarrying = false;
	FVector CarryPoint = FVector::ZeroVector;
	FVector ViewLocation = FVector::ZeroVector;
};

/**
 * Phase 21: the tools of the work view as real objects (local only). The tools the player carries lie side by side on
 * the bench; the chosen one is picked up and follows the cursor, sits on the screw while the button is held and turns
 * (the cordless drill much faster, with its motor sound), the loosened screw sticks to the magnetic tip and is dropped
 * into the screw bowl; pliers grip, the multimeter probe touches the measuring point, the soldering iron smokes. A
 * different tool puts the old one back down. Purely visual, follows the work view's state.
 */
UCLASS(NotPlaceable, Transient)
class WORKSHOPEMPIRE_API AWEToolHand : public AActor
{
	GENERATED_BODY()

public:
	AWEToolHand();

	void Init(AWEWorkbench* InBench);
	/** The tools the player has (index 0 = hands); bDrill: the screwdriver slot is the cordless drill. */
	void SetTools(const TArray<EWEToolType>& InTools, bool bInDrill);
	void SetState(const FWEToolHandState& InState) { State = InState; }
	/** Tests: location of the tool's working tip (in the hand or lying on the bench). */
	FVector GetTipLocation() const;
	bool IsHolding() const { return HeldIndex > 0; }

	virtual void Tick(float DeltaSeconds) override;
	virtual void Destroyed() override;

	static const TCHAR* MeshFor(EWEToolType Tool, bool bDrill);

protected:
	struct FToolVis
	{
		EWEToolType Tool = EWEToolType::None;
		TObjectPtr<UStaticMeshComponent> Mesh;
		FVector Location = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
		bool bPlaced = false;
	};
	void Rebuild();
	FTransform RestTransform(int32 Slot, const FToolVis& Vis) const;
	FTransform HandTransform(const FToolVis& Vis, float Time) const;
	void PlaySound(const TCHAR* Name, const FVector& At, float Volume) const;

	TWeakObjectPtr<AWEWorkbench> Bench;
	TArray<EWEToolType> Tools;
	bool bDrill = false;
	TArray<FToolVis> Visuals;
	FWEToolHandState State;
	int32 HeldIndex = 0;
	float SpinAngle = 0.f;
	float SmokeTimer = 0.f;
	float LastTap = 0.f;
	UPROPERTY(Transient) TObjectPtr<class UAudioComponent> DrillAudio;
	/** Phase 21: things the bare hands hold for some tasks (thermal paste syringe, brush for the old paste). */
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Syringe;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Brush;
};
