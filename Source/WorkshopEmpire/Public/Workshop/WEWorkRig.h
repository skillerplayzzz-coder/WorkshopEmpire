#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Workshop/WEWorkPlan.h"
#include "WEWorkRig.generated.h"

class AWEWorkbench;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Local-only props of the work view (Phase 17.5): the screws, clips, plugs, cover, parts and measuring
 * points of the current step, placed on the device on the bench. Done tasks fly to the tray beside it
 * (screws spin out, the cover lifts off, parts are lifted out, a spare part drops in). Never replicated;
 * what is done comes from the bench's replicated task mask, so it always matches the server.
 */
UCLASS(NotPlaceable, Transient)
class WORKSHOPEMPIRE_API AWEWorkRig : public AActor
{
	GENERATED_BODY()

public:
	AWEWorkRig();

	void Init(AWEWorkbench* InBench, bool bInRepair);
	void SetRepairMode(bool bInRepair);
	bool IsRepairMode() const { return bRepair; }
	/** Reassembly: the picked part's plan is the one for putting it back in. */
	void SetInstallMode(bool bInInstall) { if (bInstall != bInInstall) { bInstall = bInInstall; RebuildPlan(); } }

	virtual void Tick(float DeltaSeconds) override;

	const FWEWorkPlan& GetPlan() const { return Plan; }
	/** Done according to the bench (only while the bench runs this hands-on step). */
	bool IsTaskDone(int32 Index) const;
	FVector GetTaskLocation(int32 Index) const;

	/** UI feedback: task under the cursor and the task being worked on (0..1). */
	void SetHover(int32 Index) { HoverTask = Index; }
	void SetActive(int32 Index, float Progress);
	/** Devices with a part layout: the part the player picked (its fasteners become the tasks). */
	void SetSelectedPart(int32 Part) { SelectedPart = Part; }
	int32 GetSelectedPart() const { return SelectedPart; }

	/** Plan changed (new step, other mode): the UI rebuilds its markers. */
	uint32 GetPlanSignature() const { return PlanSignature; }

protected:
	void RebuildPlan();
	uint32 ComputeSignature() const;
	void ComputeFrame();
	void PlayTaskSound(EWEWorkTaskKind Kind) const;

	struct FPiece
	{
		TArray<TObjectPtr<UStaticMeshComponent>> Meshes;
		FVector Base = FVector::ZeroVector;
		float DoneAt = -1.f;
	};

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> AllMeshes;

	TArray<FPiece> Pieces;
	TWeakObjectPtr<AWEWorkbench> Bench;
	FWEWorkPlan Plan;
	uint32 PlanSignature = 0;
	bool bRepair = false;
	bool bInstall = false;
	int32 HoverTask = INDEX_NONE;
	int32 ActiveTask = INDEX_NONE;
	int32 SelectedPart = INDEX_NONE;
	class AWEAssemblyView* GetView() const;
	float ActiveProgress = 0.f;
	uint32 LastMask = 0;

	// Frame of the device: centre of the top, half sizes along the bench axes.
	FVector Center = FVector::ZeroVector;
	FVector AxisX = FVector::ForwardVector;
	FVector AxisY = FVector::RightVector;
	FVector Half = FVector(10.f);
	FVector Tray = FVector::ZeroVector;

	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> BaseMaterial;
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> Cube;
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> Cylinder;
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> Sphere;
};
