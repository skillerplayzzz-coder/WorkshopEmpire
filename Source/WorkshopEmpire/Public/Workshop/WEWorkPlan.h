#pragma once

#include "CoreMinimal.h"
#include "Items/WEItemTypes.h"
#include "Workshop/WEWorkbench.h"

class UWEItemDefinition;

/** One hands-on action in the work view (Phase 17.5): a screw, a clip, a plug, the cover, a part ... */
enum class EWEWorkTaskKind : uint8
{
	Screw,      // turn it out (screwdriver)
	Grip,       // bend open / pull with pliers
	Cut,        // cut a cable or bracket (side cutter, angle grinder)
	Knock,      // knock loose (hammer)
	Solder,     // (de)solder a joint (soldering iron)
	Probe,      // measure (multimeter) or look closely (hands)
	Clip,       // press a clip open (hands)
	Unplug,     // pull a plug (hands)
	Lid,        // lift the cover off (hands)
	PullPart,   // take a part out (hands)
	InsertPart, // put the spare part in (hands)
	PowerTest,  // switch on and watch (hands)
	Wipe,       // clean (hands)
	AirBlow,    // Phase 21: blow the dust out (air duster)
	PasteApply  // Phase 21: put new thermal paste on (hands, syringe)
};

struct WORKSHOPEMPIRE_API FWEWorkTask
{
	EWEWorkTaskKind Kind = EWEWorkTaskKind::Clip;
	/** None = hands. Otherwise always the step's own tool (the server checks exactly that). */
	EWEToolType Tool = EWEToolType::None;
	/** Position on the device, -1..1 inside its bounding box (Z = 1 is the top). */
	FVector Local = FVector::ZeroVector;
	/** Share of the step's work time. */
	float Weight = 1.f;
	/** Tasks of a later phase wait until every earlier one is done (screws before the cover). */
	uint8 Phase = 0;
	/** Part that comes out / goes in (PullPart, InsertPart) - shown as its model. */
	const UWEItemDefinition* Part = nullptr;

	/** Phase 18a, devices with a part layout: the task sits on a real fastener / part (device space). */
	int32 AssemblyPart = INDEX_NONE;
	/** Fastener index within the part, or INDEX_NONE for taking the part itself out. */
	int32 Fastener = INDEX_NONE;
	FVector DevicePosition = FVector::ZeroVector;
};

/** The hands-on breakdown of one disassembly or repair step. Built the same way on server and clients. */
struct WORKSHOPEMPIRE_API FWEWorkPlan
{
	TArray<FWEWorkTask> Tasks;
	EWEToolType StepTool = EWEToolType::None;
	bool bRepair = false;
	/** Putting a part back in (reassembly): part first, then plugs, clips and screws. */
	bool bInstall = false;

	bool IsValid() const { return Tasks.Num() > 0; }
	float GetTotalWeight() const;
	/** Seconds this task takes when the whole step takes StepSeconds. */
	float GetTaskSeconds(int32 Index, float StepSeconds) const;
	/** All tasks of earlier phases are done (DoneMask bit per task). */
	bool IsTaskAvailable(int32 Index, uint32 DoneMask) const;
};

namespace WEWorkPlan
{
	/** Max tasks per step (bit mask in the replicated work state). */
	constexpr int32 MaxTasks = 24;

	/** Plan for disassembly step StepIndex of Def (screws, cover, plugs, the part ...). */
	WORKSHOPEMPIRE_API FWEWorkPlan ForDisassembly(const UWEItemDefinition* Def, int32 StepIndex);
	/** Plan for a repair action (diagnose: measuring points, remove/install: screws + part, test: power, refurbish: wiping). */
	WORKSHOPEMPIRE_API FWEWorkPlan ForRepair(const UWEItemDefinition* Def, const FWERepairStepInfo& Info);
	/** Phase 18a: plan for one part of a device with a part layout: its real fasteners in their order, then the part itself. */
	WORKSHOPEMPIRE_API FWEWorkPlan ForAssemblyPart(const UWEItemDefinition* Def, int32 Part);
	/** Reassembly: the same part the other way round - set it in, then plugs, clips and screws (reverse order). */
	WORKSHOPEMPIRE_API FWEWorkPlan ForAssemblyInstall(const UWEItemDefinition* Def, int32 Part);

	/** "Schraube lösen", "Stecker ziehen" ... (bInstall: "Schraube eindrehen", "Stecker einstecken" ...) */
	WORKSHOPEMPIRE_API FText GetTaskText(EWEWorkTaskKind Kind, bool bInstall = false);
	/** UI icon name (WEUI::IconTexture). */
	WORKSHOPEMPIRE_API FName GetTaskIcon(EWEWorkTaskKind Kind);
	WORKSHOPEMPIRE_API FName GetToolIcon(EWEToolType Tool);

	/** Phase 21: seconds a loosened screw hangs on the magnetic screwdriver tip on its way into the screw bowl. */
	constexpr float CarrySeconds = 1.1f;
	/** Point inside the screw bowl for the Index-th screw (a spiral, they lie side by side). */
	WORKSHOPEMPIRE_API FVector BowlSpot(const FVector& BowlCenter, int32 Index);
	/** Where a screw is A (0..1) of the way from where it came out (From, along Axis) to its spot in the bowl. */
	WORKSHOPEMPIRE_API FVector CarryPath(const FVector& From, const FVector& Axis, const FVector& Spot, float A);

	/** Console variable we.WorkView: 1 = [E]/[Q] at the bench opens the hands-on work view, 0 = timed steps (old behaviour, test scripts). */
	WORKSHOPEMPIRE_API bool IsWorkViewEnabled();
}
