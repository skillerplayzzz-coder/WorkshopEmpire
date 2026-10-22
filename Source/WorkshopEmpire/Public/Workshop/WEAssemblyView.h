#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WEAssemblyView.generated.h"

class AWEPickupItem;
class AWEWorkbench;
class UMaterialInterface;
class UStaticMeshComponent;
class UWEAssemblyDefinition;

/**
 * What a device with a part layout looks like on the workbench (Phase 18a), on every machine: the case with all
 * parts that are still in it, their screws, plugs and clips. Built locally from the replicated item state
 * (RemovedParts) and the bench's work state (which part is being worked on, which fasteners are done), so it always
 * matches the server. Plays the visible actions: screws turn out and drop into the tray, plugs come off, clips open,
 * a part slides out along its mounting direction, is lifted and goes down into the bench storage; the side panel
 * comes off and lies beside the case.
 */
UCLASS(NotPlaceable, Transient)
class WORKSHOPEMPIRE_API AWEAssemblyView : public AActor
{
	GENERATED_BODY()

public:
	AWEAssemblyView();

	void Init(AWEWorkbench* InBench, AWEPickupItem* InItem);
	AWEPickupItem* GetItem() const { return Item.Get(); }
	virtual void Tick(float DeltaSeconds) override;
	virtual void Destroyed() override;

	const UWEAssemblyDefinition* GetAssembly() const { return Assembly; }
	/** Part hit by the ray (world), only parts that are still in the device; INDEX_NONE if none. */
	int32 FindPartUnderRay(const FVector& Origin, const FVector& Direction) const;
	/** Part is in the device (not out, not taken out for a repair). */
	bool IsPartInside(int32 Part) const;
	FVector GetPartCenter(int32 Part) const;
	FVector GetFastenerLocation(int32 Part, int32 Fastener) const;
	/** Phase 21: direction out of the device along the fastener (a screwdriver comes from there). */
	FVector GetFastenerAxis(int32 Part, int32 Fastener) const;
	/** Phase 21: a loosened screw on its way to the bowl (on the magnetic tip). */
	bool GetCarryPoint(FVector& OutPoint) const;
	int32 GetScrewsInBowl() const;
	/** Tests: where the parts that came out lie (visible ones on the bench). */
	TArray<FVector> GetPartsOnBench() const;
	FVector DeviceToWorld(const FVector& DevicePoint) const;
	/** Device bounds in world space (parts that are inside). */
	FBox GetDeviceBounds() const;

	void SetHover(int32 Part) { HoverPart = Part; }
	void SetSelected(int32 Part) { SelectedPart = Part; }
	/** Local feedback while the mouse is held: Fastener = INDEX_NONE means the part itself (pulled out a little). */
	void SetActiveTask(int32 Part, int32 Fastener, float Progress);
	/** Reassembly: a removed part the player picked hovers in front of its place (shown while it goes back in). */
	void SetInstallPreview(int32 Part) { InstallPreview = Part; }

protected:
	struct FFastenerVis
	{
		TObjectPtr<UStaticMeshComponent> Mesh;
		FTransform Rest;
		float DoneAt = -1.f;
		/** Phase 21: lies in the screw bowl (until it goes back in). */
		int32 BowlIndex = INDEX_NONE;
		bool bInBowl = false;
		bool bLanded = false;
		FVector CarryFrom = FVector::ZeroVector;
		FVector CarryAxis = FVector::UpVector;
	};
	int32 BowlCount = 0;
	struct FPartVis
	{
		TObjectPtr<UStaticMeshComponent> Mesh;
		TArray<FFastenerVis> Fasteners;
		bool bOut = false;
		float OutAt = -1.f;
		bool bGone = false;
		/** Phase 21: where it lies on the bench beside the device once out (order of removal). */
		int32 BenchSlot = INDEX_NONE;
		/** Reassembly: the part is set in (its own task done) and when that happened. */
		bool bInserted = false;
		float InsertedAt = -1.f;
	};

	void Build();
	void UpdateState(float Now);
	void AnimatePart(int32 Index, FPartVis& Vis, float Now);
	int32 RepairPartIndex() const;
	FTransform DeviceTransform() const;

	TWeakObjectPtr<AWEWorkbench> Bench;
	TWeakObjectPtr<AWEPickupItem> Item;
	UPROPERTY(Transient) TObjectPtr<const UWEAssemblyDefinition> Assembly;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> HighlightMaterial;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> SelectMaterial;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> AllMeshes;
	TArray<FPartVis> Parts;
	int32 HoverPart = INDEX_NONE;
	int32 SelectedPart = INDEX_NONE;
	int32 InstallPreview = INDEX_NONE;
	/** Part going back in this frame (running install job, else the preview). */
	int32 InstallingPart = INDEX_NONE;
	int32 ActivePart = INDEX_NONE;
	int32 ActiveFastener = INDEX_NONE;
	float ActiveProgress = 0.f;
	int32 TrayCount = 0;
	int32 NextBenchSlot = 0;
	bool bBuilt = false;
};
