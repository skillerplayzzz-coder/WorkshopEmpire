#include "Workshop/WEWorkPlan.h"

#include "HAL/IConsoleManager.h"
#include "Items/WEItemDefinition.h"
#include "Workshop/WEDisassemblyDefinition.h"
#include "Workshop/WERepairDefinition.h"
#include "Workshop/WEAssemblyDefinition.h"
#include "Engine/StaticMesh.h"

#define LOCTEXT_NAMESPACE "WEWorkPlan"

static TAutoConsoleVariable<int32> CVarWorkView(TEXT("we.WorkView"), 1,
	TEXT("1 = [E]/[Q] on a device on the workbench opens the hands-on work view; 0 = timed steps without the view (test scripts)."));

float FWEWorkPlan::GetTotalWeight() const
{
	float Total = 0.f;
	for (const FWEWorkTask& Task : Tasks)
	{
		Total += Task.Weight;
	}
	return FMath::Max(Total, 0.01f);
}

float FWEWorkPlan::GetTaskSeconds(int32 Index, float StepSeconds) const
{
	return Tasks.IsValidIndex(Index) ? StepSeconds * Tasks[Index].Weight / GetTotalWeight() : 0.f;
}

bool FWEWorkPlan::IsTaskAvailable(int32 Index, uint32 DoneMask) const
{
	if (!Tasks.IsValidIndex(Index) || (DoneMask & (1u << Index)) != 0)
	{
		return false;
	}
	for (int32 i = 0; i < Tasks.Num(); ++i)
	{
		if (Tasks[i].Phase < Tasks[Index].Phase && (DoneMask & (1u << i)) == 0)
		{
			return false;
		}
	}
	return true;
}

namespace
{
	EWEWorkTaskKind ToolTaskKind(EWEToolType Tool)
	{
		switch (Tool)
		{
		case EWEToolType::Screwdriver:   return EWEWorkTaskKind::Screw;
		case EWEToolType::Pliers:        return EWEWorkTaskKind::Grip;
		case EWEToolType::SideCutter:    return EWEWorkTaskKind::Cut;
		case EWEToolType::AngleGrinder:  return EWEWorkTaskKind::Cut;
		case EWEToolType::AirDuster:     return EWEWorkTaskKind::AirBlow;
		case EWEToolType::Hammer:        return EWEWorkTaskKind::Knock;
		case EWEToolType::Multimeter:    return EWEWorkTaskKind::Probe;
		case EWEToolType::SolderingIron: return EWEWorkTaskKind::Solder;
		default:                         return EWEWorkTaskKind::Clip;
		}
	}

	float KindWeight(EWEWorkTaskKind Kind)
	{
		switch (Kind)
		{
		case EWEWorkTaskKind::Screw:      return 1.f;
		case EWEWorkTaskKind::Clip:       return 0.6f;
		case EWEWorkTaskKind::Unplug:     return 0.7f;
		case EWEWorkTaskKind::Lid:        return 1.2f;
		case EWEWorkTaskKind::PullPart:   return 1.6f;
		case EWEWorkTaskKind::InsertPart: return 1.6f;
		case EWEWorkTaskKind::Solder:     return 1.5f;
		case EWEWorkTaskKind::PowerTest:  return 2.f;
		default:                          return 1.f;
		}
	}

	/** Builder with a stream seeded by the device and the step: same plan everywhere, different per device. */
	struct FBuilder
	{
		FWEWorkPlan Plan;
		FRandomStream Random;

		FWEWorkTask& Add(EWEWorkTaskKind Kind, EWEToolType Tool, const FVector& Local, uint8 Phase, const UWEItemDefinition* Part = nullptr)
		{
			FWEWorkTask& T = Plan.Tasks.AddDefaulted_GetRef();
			T.Kind = Kind;
			T.Tool = Tool;
			T.Local = Local;
			T.Weight = KindWeight(Kind);
			T.Phase = Phase;
			T.Part = Part;
			return T;
		}

		/** Screws in the corners of the top (2: diagonal, 3-4: corners, 5-6: corners and edge middles). */
		void Corners(EWEWorkTaskKind Kind, EWEToolType Tool, int32 Count, uint8 Phase)
		{
			static const FVector Spots[] = { FVector(0.78f, 0.8f, 1.f), FVector(-0.78f, -0.8f, 1.f), FVector(0.78f, -0.8f, 1.f), FVector(-0.78f, 0.8f, 1.f),
				FVector(0.f, 0.86f, 1.f), FVector(0.f, -0.86f, 1.f) };
			for (int32 i = 0; i < FMath::Min(Count, 6); ++i)
			{
				Add(Kind, Tool, Spots[i], Phase);
			}
		}

		/** Points along the front edge (cuts, solder joints, measuring points). */
		void Row(EWEWorkTaskKind Kind, EWEToolType Tool, int32 Count, uint8 Phase, float X = 0.55f)
		{
			for (int32 i = 0; i < Count; ++i)
			{
				const float Y = Count > 1 ? FMath::Lerp(-0.55f, 0.55f, i / float(Count - 1)) : 0.f;
				Add(Kind, Tool, FVector(X + Random.FRandRange(-0.08f, 0.08f), Y, 1.f), Phase);
			}
		}

		FVector Inner()
		{
			return FVector(Random.FRandRange(-0.35f, 0.35f), Random.FRandRange(-0.4f, 0.4f), 1.f);
		}
	};

	bool Has(const FString& Text, std::initializer_list<const TCHAR*> Words)
	{
		for (const TCHAR* W : Words)
		{
			if (Text.Contains(W))
			{
				return true;
			}
		}
		return false;
	}
}

namespace WEWorkPlan
{
	FWEWorkPlan ForDisassembly(const UWEItemDefinition* Def, int32 StepIndex)
	{
		const FWEDisassemblyStep* Step = Def && Def->Disassembly ? Def->Disassembly->GetStep(StepIndex) : nullptr;
		FBuilder B;
		if (!Step)
		{
			return B.Plan;
		}
		B.Random.Initialize(static_cast<int32>(GetTypeHash(Def->ItemId)) ^ (StepIndex * 7919));
		B.Plan.StepTool = Step->RequiredTool;
		const FString Text = Step->ActionText.ToString().ToLower();
		const EWEToolType Tool = Step->RequiredTool;

		// The part this step yields (first output that keeps the device's condition, else the first output).
		const UWEItemDefinition* Part = nullptr;
		for (const FWEDisassemblyOutput& Out : Step->Outputs)
		{
			if (Out.Item && (Out.bInheritCondition || !Part))
			{
				Part = Out.Item;
				if (Out.bInheritCondition)
				{
					break;
				}
			}
		}

		const bool bScrews = Has(Text, { TEXT("schraub") });
		const bool bCover = Has(Text, { TEXT("abdeckung"), TEXT("deckel"), TEXT("rückwand"), TEXT("gehäuse öffnen"), TEXT("abnehmen"), TEXT("klappe") });
		const bool bTakeOut = Has(Text, { TEXT("ausbau"), TEXT("entfern"), TEXT("ziehen"), TEXT("entnehm"), TEXT("herausnehm") });
		const bool bBreakUp = Has(Text, { TEXT("zerleg"), TEXT("sortier"), TEXT("verwert"), TEXT("ausschlacht") });
		const bool bPlugged = Has(Text, { TEXT("netzteil"), TEXT("mainboard"), TEXT("platine"), TEXT("grafik"), TEXT("laufwerk"), TEXT("motor") });

		// 1) The tool work first.
		if (Tool != EWEToolType::None)
		{
			const EWEWorkTaskKind Kind = ToolTaskKind(Tool);
			if (Kind == EWEWorkTaskKind::Screw)
			{
				B.Corners(Kind, Tool, bScrews && !bTakeOut ? B.Random.RandRange(4, 6) : B.Random.RandRange(2, 3), 0);
			}
			else
			{
				B.Row(Kind, Tool, B.Random.RandRange(2, 4), 0);
			}
		}
		else if (!bCover && !bTakeOut && !bBreakUp)
		{
			B.Row(EWEWorkTaskKind::Clip, EWEToolType::None, 2, 0, 0.9f); // housing clips
		}
		// 2) Plugs, 3) the cover, 4) the parts.
		if (bTakeOut && bPlugged)
		{
			B.Add(EWEWorkTaskKind::Unplug, EWEToolType::None, FVector(-0.55f, 0.55f, 1.f), 1);
		}
		if (bCover)
		{
			if (Tool == EWEToolType::None)
			{
				B.Row(EWEWorkTaskKind::Clip, EWEToolType::None, 2, 1, 0.9f);
			}
			B.Add(EWEWorkTaskKind::Lid, EWEToolType::None, FVector(0.f, 0.f, 1.f), 2, Part);
		}
		if (bBreakUp)
		{
			// Several pieces come out: one per output kind (at least two).
			int32 Pieces = 0;
			for (const FWEDisassemblyOutput& Out : Step->Outputs)
			{
				if (Out.Item && Pieces < 4)
				{
					B.Add(EWEWorkTaskKind::PullPart, EWEToolType::None, B.Inner(), 3, Out.Item);
					++Pieces;
				}
			}
			if (Pieces < 2)
			{
				B.Add(EWEWorkTaskKind::PullPart, EWEToolType::None, B.Inner(), 3, Part);
			}
		}
		else if (bTakeOut || (!bCover && !bScrews && Part))
		{
			B.Add(EWEWorkTaskKind::PullPart, EWEToolType::None, B.Inner(), 3, Part);
		}
		if (B.Plan.Tasks.Num() == 0)
		{
			B.Add(EWEWorkTaskKind::PullPart, EWEToolType::None, B.Inner(), 0, Part);
		}
		B.Plan.Tasks.SetNum(FMath::Min(B.Plan.Tasks.Num(), MaxTasks));
		return B.Plan;
	}

	FWEWorkPlan ForRepair(const UWEItemDefinition* Def, const FWERepairStepInfo& Info)
	{
		FBuilder B;
		B.Plan.bRepair = true;
		B.Plan.StepTool = Info.Tool;
		if (!Def || Info.Kind == EWERepairStepKind::None)
		{
			return B.Plan;
		}
		B.Random.Initialize(static_cast<int32>(GetTypeHash(Def->ItemId)) ^ (static_cast<int32>(Info.Kind) * 104729) ^ (Info.FaultIndex * 31));
		const UWERepairDefinition* Repair = Def->Repair;
		const FWERepairFault* Fault = Repair ? Repair->GetFault(Info.FaultIndex) : nullptr;
		const UWEItemDefinition* Part = Fault ? Fault->Part.Get() : Info.Material;
		const EWEToolType Tool = Info.Tool;
		auto ToolWork = [&B, Tool](int32 Count, uint8 Phase)
		{
			if (Tool == EWEToolType::None)
			{
				return;
			}
			const EWEWorkTaskKind Kind = ToolTaskKind(Tool);
			if (Kind == EWEWorkTaskKind::Screw)
			{
				B.Corners(Kind, Tool, Count, Phase);
			}
			else
			{
				B.Row(Kind, Tool, Count, Phase);
			}
		};
		switch (Info.Kind)
		{
		case EWERepairStepKind::Diagnose:
			// Phase 21: first switch it on and watch what it does, then measure every component that could be at fault
			// (one measuring point each, in the order of the repair data) - each shows its own result.
			B.Add(EWEWorkTaskKind::PowerTest, EWEToolType::None, FVector(0.7f, -0.7f, 1.f), 0);
			B.Row(Tool == EWEToolType::None ? EWEWorkTaskKind::Probe : ToolTaskKind(Tool), Tool, Repair ? FMath::Clamp(Repair->Faults.Num(), 1, 8) : 3, 1, 0.35f);
			break;
		case EWERepairStepKind::Remove:
			ToolWork(B.Random.RandRange(2, 3), 0);
			B.Add(EWEWorkTaskKind::Unplug, EWEToolType::None, FVector(-0.55f, 0.55f, 1.f), 1);
			B.Add(EWEWorkTaskKind::PullPart, EWEToolType::None, B.Inner(), 2, Part);
			break;
		case EWERepairStepKind::Install:
			B.Add(EWEWorkTaskKind::InsertPart, EWEToolType::None, B.Inner(), 0, Part);
			B.Add(EWEWorkTaskKind::Unplug, EWEToolType::None, FVector(-0.55f, 0.55f, 1.f), 1); // plug back in
			ToolWork(B.Random.RandRange(2, 3), 2);
			break;
		case EWERepairStepKind::Test:
			if (Tool != EWEToolType::None)
			{
				B.Row(ToolTaskKind(Tool), Tool, 1, 0, 0.35f);
			}
			B.Add(EWEWorkTaskKind::PowerTest, EWEToolType::None, FVector(0.7f, -0.7f, 1.f), 1);
			break;
		case EWERepairStepKind::Refurbish:
			for (int32 i = 0; i < 3; ++i)
			{
				B.Add(EWEWorkTaskKind::Wipe, EWEToolType::None, B.Inner(), 0, Info.Material);
			}
			break;
		case EWERepairStepKind::Clean:
			// Phase 21: four dusty spots (vents, corners) blown out one by one.
			B.Corners(EWEWorkTaskKind::AirBlow, Tool, 4, 0);
			break;
		case EWERepairStepKind::Paste:
			// Phase 21: old paste wiped off the chip, new paste put on.
			B.Add(EWEWorkTaskKind::Wipe, EWEToolType::None, FVector(0.15f, -0.1f, 1.f), 0);
			B.Add(EWEWorkTaskKind::PasteApply, EWEToolType::None, FVector(0.15f, -0.1f, 1.f), 1, Info.Material);
			break;
		default:
			break;
		}
		// Devices with a part layout: taking a broken part out / putting the spare in uses that part's real fasteners.
		if ((Info.Kind == EWERepairStepKind::Remove || Info.Kind == EWERepairStepKind::Install) && Def->HasAssembly() && Part)
		{
			int32 PartIndex = INDEX_NONE;
			for (int32 i = 0; i < Def->Disassembly->NumSteps() && PartIndex == INDEX_NONE; ++i)
			{
				for (const FWEDisassemblyOutput& O : Def->Disassembly->GetStep(i)->Outputs)
				{
					PartIndex = O.Item == Part && O.bInheritCondition ? i : PartIndex;
				}
			}
			FWEWorkPlan Real = PartIndex != INDEX_NONE ? ForAssemblyPart(Def, PartIndex) : FWEWorkPlan();
			if (Real.IsValid())
			{
				Real.bRepair = true;
				Real.StepTool = Tool;
				uint8 MaxPhase = 0;
				for (FWEWorkTask& T : Real.Tasks)
				{
					// The server accepts only the repair action's own tool (or hands).
					if (T.Tool != EWEToolType::None)
					{
						T.Tool = Tool;
					}
					MaxPhase = FMath::Max(MaxPhase, T.Phase);
				}
				if (Info.Kind == EWERepairStepKind::Install)
				{
					// The other way round: part in first, then plugs and screws.
					for (FWEWorkTask& T : Real.Tasks)
					{
						T.Phase = MaxPhase - T.Phase;
						if (T.Fastener == INDEX_NONE)
						{
							T.Kind = EWEWorkTaskKind::InsertPart;
						}
					}
				}
				return Real;
			}
		}
		return B.Plan;
	}

	FWEWorkPlan ForAssemblyPart(const UWEItemDefinition* Def, int32 PartIndex)
	{
		FWEWorkPlan Plan;
		const UWEAssemblyDefinition* Assembly = Def && Def->HasAssembly() ? Def->Assembly.Get() : nullptr;
		const FWEDisassemblyStep* Step = Assembly ? Def->Disassembly->GetStep(PartIndex) : nullptr;
		if (!Step || !Assembly->Parts.IsValidIndex(PartIndex))
		{
			return Plan;
		}
		const FWEAssemblyPart& Part = Assembly->Parts[PartIndex];
		Plan.StepTool = Step->RequiredTool;
		uint8 LastOrder = 0;
		for (int32 i = 0; i < Part.Fasteners.Num() && Plan.Tasks.Num() < MaxTasks - 1; ++i)
		{
			const FWEFastener& F = Part.Fasteners[i];
			FWEWorkTask& T = Plan.Tasks.AddDefaulted_GetRef();
			T.AssemblyPart = PartIndex;
			T.Fastener = i;
			T.DevicePosition = F.Location;
			T.Phase = F.Order;
			LastOrder = FMath::Max(LastOrder, F.Order);
			switch (F.Kind)
			{
			case EWEFastenerKind::Screw:
				// Only the step's tool is accepted by the server; a step without a tool has finger screws.
				T.Kind = EWEWorkTaskKind::Screw;
				T.Tool = Step->RequiredTool;
				T.Weight = 1.f;
				break;
			case EWEFastenerKind::Thumbscrew:
				T.Kind = EWEWorkTaskKind::Screw;
				T.Weight = 0.7f;
				break;
			case EWEFastenerKind::Plug:
				T.Kind = EWEWorkTaskKind::Unplug;
				T.Weight = 0.7f;
				break;
			case EWEFastenerKind::Clip:
			case EWEFastenerKind::Lever:
				T.Kind = EWEWorkTaskKind::Clip;
				T.Weight = 0.6f;
				break;
			}
		}
		// Finally the part itself comes out (the side panel is lifted off).
		const UWEItemDefinition* Out = nullptr;
		for (const FWEDisassemblyOutput& O : Step->Outputs)
		{
			if (O.Item && (O.bInheritCondition || !Out))
			{
				Out = O.Item;
			}
		}
		FWEWorkTask& Pull = Plan.Tasks.AddDefaulted_GetRef();
		Pull.Kind = Part.bServiceAccess || Part.bShell ? EWEWorkTaskKind::Lid : EWEWorkTaskKind::PullPart;
		Pull.Weight = 1.6f;
		Pull.Phase = static_cast<uint8>(FMath::Min<int32>(LastOrder + 1, 255));
		Pull.Part = Out;
		Pull.AssemblyPart = PartIndex;
		Pull.DevicePosition = Part.Mesh ? Part.Mesh->GetBoundingBox().GetCenter() : FVector::ZeroVector;
		return Plan;
	}

	FWEWorkPlan ForAssemblyInstall(const UWEItemDefinition* Def, int32 PartIndex)
	{
		FWEWorkPlan Plan = ForAssemblyPart(Def, PartIndex);
		if (!Plan.IsValid())
		{
			return Plan;
		}
		Plan.bInstall = true;
		uint8 MaxPhase = 0;
		for (const FWEWorkTask& T : Plan.Tasks)
		{
			MaxPhase = FMath::Max(MaxPhase, T.Phase);
		}
		// Reverse order: the part goes in first, the fastener that came off first goes on last.
		for (FWEWorkTask& T : Plan.Tasks)
		{
			T.Phase = MaxPhase - T.Phase;
			if (T.Fastener == INDEX_NONE)
			{
				T.Kind = T.Kind == EWEWorkTaskKind::Lid ? EWEWorkTaskKind::Lid : EWEWorkTaskKind::InsertPart;
			}
		}
		// Quality update: fresh thermal paste goes on before the cooler is set in (the syringe, from the inventory).
		const FWEAssemblyPart& Part = Def->Assembly->Parts[PartIndex];
		if (Part.InstallMaterial && Plan.Tasks.Num() < MaxTasks)
		{
			for (FWEWorkTask& T : Plan.Tasks)
			{
				T.Phase = static_cast<uint8>(FMath::Min<int32>(T.Phase + 1, 255));
			}
			FWEWorkTask& Paste = Plan.Tasks.AddDefaulted_GetRef();
			Paste.Kind = EWEWorkTaskKind::PasteApply;
			Paste.Weight = 1.2f;
			Paste.Phase = 0;
			Paste.Part = Part.InstallMaterial;
			// On the processor (it sits under the cooler); the marker shows there.
			const int32 Cpu = Def->Assembly->Parts.IndexOfByPredicate([](const FWEAssemblyPart& P) { return P.Id == TEXT("Cpu"); });
			Paste.AssemblyPart = Cpu != INDEX_NONE ? Cpu : PartIndex;
			Paste.Fastener = INDEX_NONE;
			const FWEAssemblyPart& Under = Def->Assembly->Parts[Paste.AssemblyPart];
			Paste.DevicePosition = Under.Mesh ? Under.Mesh->GetBoundingBox().GetCenter() : FVector::ZeroVector;
		}
		// The part task first in the list too (markers and hints read the list in order).
		Plan.Tasks.StableSort([](const FWEWorkTask& A, const FWEWorkTask& B) { return A.Phase < B.Phase; });
		return Plan;
	}

	FText GetTaskText(EWEWorkTaskKind Kind, bool bInstall)
	{
		switch (Kind)
		{
		case EWEWorkTaskKind::Screw:      return bInstall ? LOCTEXT("ScrewIn", "Schraube eindrehen") : LOCTEXT("Screw", "Schraube lösen");
		case EWEWorkTaskKind::Grip:       return LOCTEXT("Grip", "Mit der Zange lösen");
		case EWEWorkTaskKind::Cut:        return LOCTEXT("Cut", "Durchtrennen");
		case EWEWorkTaskKind::Knock:      return LOCTEXT("Knock", "Losklopfen");
		case EWEWorkTaskKind::Solder:     return bInstall ? LOCTEXT("SolderIn", "Einlöten") : LOCTEXT("Solder", "Lötstelle lösen");
		case EWEWorkTaskKind::Probe:      return LOCTEXT("Probe", "Messpunkt prüfen");
		case EWEWorkTaskKind::Clip:       return bInstall ? LOCTEXT("ClipIn", "Klammer einrasten") : LOCTEXT("Clip", "Klammer aufdrücken");
		case EWEWorkTaskKind::Unplug:     return bInstall ? LOCTEXT("PlugIn", "Stecker einstecken") : LOCTEXT("Unplug", "Stecker ziehen");
		case EWEWorkTaskKind::Lid:        return bInstall ? LOCTEXT("LidOn", "Abdeckung aufsetzen") : LOCTEXT("Lid", "Abdeckung abnehmen");
		case EWEWorkTaskKind::PullPart:   return LOCTEXT("PullPart", "Teil herausnehmen");
		case EWEWorkTaskKind::InsertPart: return bInstall ? LOCTEXT("InsertPartIn", "Teil einsetzen") : LOCTEXT("InsertPart", "Ersatzteil einsetzen");
		case EWEWorkTaskKind::PowerTest:  return LOCTEXT("PowerTest", "Einschalten und testen");
		case EWEWorkTaskKind::Wipe:       return LOCTEXT("Wipe", "Reinigen");
		case EWEWorkTaskKind::AirBlow:    return LOCTEXT("AirBlow", "Staub ausblasen");
		case EWEWorkTaskKind::PasteApply: return LOCTEXT("PasteApply", "Wärmeleitpaste auftragen");
		}
		return FText::GetEmpty();
	}

	FName GetTaskIcon(EWEWorkTaskKind Kind)
	{
		switch (Kind)
		{
		case EWEWorkTaskKind::Screw:      return TEXT("Screw");
		case EWEWorkTaskKind::Grip:       return TEXT("Pliers");
		case EWEWorkTaskKind::Cut:        return TEXT("SideCutter");
		case EWEWorkTaskKind::Knock:      return TEXT("Hammer");
		case EWEWorkTaskKind::Solder:     return TEXT("SolderingIron");
		case EWEWorkTaskKind::Probe:      return TEXT("Multimeter");
		case EWEWorkTaskKind::Clip:       return TEXT("Grip");
		case EWEWorkTaskKind::Unplug:     return TEXT("Power");
		case EWEWorkTaskKind::Lid:        return TEXT("Box");
		case EWEWorkTaskKind::PullPart:   return TEXT("Hand");
		case EWEWorkTaskKind::InsertPart: return TEXT("Download");
		case EWEWorkTaskKind::PowerTest:  return TEXT("Power");
		case EWEWorkTaskKind::Wipe:       return TEXT("Roller");
		case EWEWorkTaskKind::AirBlow:    return TEXT("Wind");
		case EWEWorkTaskKind::PasteApply: return TEXT("Syringe");
		}
		return TEXT("Hand");
	}

	FName GetToolIcon(EWEToolType Tool)
	{
		switch (Tool)
		{
		case EWEToolType::Screwdriver:   return TEXT("Screwdriver");
		case EWEToolType::Pliers:        return TEXT("Pliers");
		case EWEToolType::SideCutter:    return TEXT("SideCutter");
		case EWEToolType::Hammer:        return TEXT("Hammer");
		case EWEToolType::Multimeter:    return TEXT("Multimeter");
		case EWEToolType::SolderingIron: return TEXT("SolderingIron");
		case EWEToolType::AngleGrinder:  return TEXT("AngleGrinder");
		case EWEToolType::AirDuster:     return TEXT("Wind");
		default:                         return TEXT("Hand");
		}
	}

	bool IsWorkViewEnabled()
	{
		return CVarWorkView.GetValueOnGameThread() != 0;
	}
}

#undef LOCTEXT_NAMESPACE

FVector WEWorkPlan::BowlSpot(const FVector& BowlCenter, int32 Index)
{
	// Golden-angle spiral: up to ~30 screws lie side by side, later ones on top.
	const int32 Ring = Index % 30;
	const float Angle = Ring * 2.39996f;
	const float Radius = FMath::Min(1.0f + 0.85f * FMath::Sqrt(static_cast<float>(Ring)), 4.8f);
	return BowlCenter + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.7f + 0.35f * (Index / 30));
}

FVector WEWorkPlan::CarryPath(const FVector& From, const FVector& Axis, const FVector& Spot, float A)
{
	A = FMath::Clamp(A, 0.f, 1.f);
	const FVector Out = From + Axis.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector) * 4.f;
	if (A < 0.25f)
	{
		// Comes out of its hole, stuck to the tip.
		return FMath::Lerp(From, Out, FMath::SmoothStep(0.f, 1.f, A / 0.25f));
	}
	const FVector Above = Spot + FVector(0.f, 0.f, 7.f);
	if (A < 0.8f)
	{
		// Over to the bowl in a low arc.
		const float B = FMath::SmoothStep(0.f, 1.f, (A - 0.25f) / 0.55f);
		const FVector Line = FMath::Lerp(Out, Above, B);
		return Line + FVector(0.f, 0.f, FMath::Sin(B * PI) * 6.f);
	}
	// Dropped: falls off the tip into the bowl.
	const float C = (A - 0.8f) / 0.2f;
	return FMath::Lerp(Above, Spot, C * C);
}