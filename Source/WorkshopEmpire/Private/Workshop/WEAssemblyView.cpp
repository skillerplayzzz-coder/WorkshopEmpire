#include "Workshop/WEAssemblyView.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "Workshop/WEAssemblyDefinition.h"
#include "Workshop/WEDisassemblyDefinition.h"
#include "Workshop/WERepairDefinition.h"
#include "Workshop/WEWorkbench.h"
#include "Workshop/WEWorkPlan.h"

namespace WEAssemblyViewConst
{
	constexpr float SlideSeconds = 0.35f;
	constexpr float LiftSeconds = 0.3f;
	constexpr float AwaySeconds = 0.6f;
	constexpr float LiftHeight = 14.f;

	const TCHAR* FastenerMesh(EWEFastenerKind Kind)
	{
		switch (Kind)
		{
		case EWEFastenerKind::Screw:      return TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_FastenerScrew.SM_Gen_FastenerScrew");
		case EWEFastenerKind::Thumbscrew: return TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_FastenerThumbscrew.SM_Gen_FastenerThumbscrew");
		case EWEFastenerKind::Plug:       return TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_FastenerPlug.SM_Gen_FastenerPlug");
		case EWEFastenerKind::Clip:       return TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_FastenerClip.SM_Gen_FastenerClip");
		case EWEFastenerKind::Lever:      return TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_FastenerLever.SM_Gen_FastenerLever");
		}
		return nullptr;
	}

	const TCHAR* DoneSound(EWEFastenerKind Kind)
	{
		switch (Kind)
		{
		case EWEFastenerKind::Plug:  return TEXT("/Game/WorkshopEmpire/Audio/Kenney/click_003.click_003");
		case EWEFastenerKind::Clip:
		case EWEFastenerKind::Lever: return TEXT("/Game/WorkshopEmpire/Audio/Kenney/click_004.click_004");
		default:                     return TEXT("/Game/WorkshopEmpire/Audio/Kenney/impactMetal_light_001.impactMetal_light_001");
		}
	}

	float Ease(float A)
	{
		A = FMath::Clamp(A, 0.f, 1.f);
		return A * A * (3.f - 2.f * A);
	}
}

AWEAssemblyView::AWEAssemblyView()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AWEAssemblyView::Init(AWEWorkbench* InBench, AWEPickupItem* InItem)
{
	Bench = InBench;
	Item = InItem;
	const UWEItemDefinition* Def = InItem ? InItem->GetItem().Definition.Get() : nullptr;
	Assembly = Def && Def->HasAssembly() ? Def->Assembly.Get() : nullptr;
	HighlightMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/WorkshopEmpire/Materials/FX/M_WEHighlightHover.M_WEHighlightHover"), nullptr, LOAD_Quiet | LOAD_NoWarn);
	SelectMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/WorkshopEmpire/Materials/FX/M_WEHighlightSelect.M_WEHighlightSelect"), nullptr, LOAD_Quiet | LOAD_NoWarn);
	Build();
}

void AWEAssemblyView::Destroyed()
{
	// The device's own (closed) model comes back when it leaves the bench.
	if (AWEPickupItem* Device = Item.Get(); Device && Device->GetMesh())
	{
		Device->GetMesh()->SetVisibility(true);
	}
	Super::Destroyed();
}

FTransform AWEAssemblyView::DeviceTransform() const
{
	const AWEPickupItem* Device = Item.Get();
	if (!Device || !Device->GetMesh())
	{
		return GetActorTransform();
	}
	FTransform T = Device->GetMesh()->GetComponentTransform();
	T.SetScale3D(FVector::OneVector);
	return T;
}

void AWEAssemblyView::Build()
{
	if (!Assembly || !Item.IsValid())
	{
		return;
	}
	if (Item->GetMesh())
	{
		Item->GetMesh()->SetVisibility(false);
	}
	SetActorTransform(DeviceTransform());
	auto Make = [this](UStaticMesh* Mesh, const FTransform& Relative)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(Mesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetMobility(EComponentMobility::Movable);
		C->SetupAttachment(GetRootComponent());
		C->SetRelativeTransform(Relative);
		C->RegisterComponent();
		AllMeshes.Add(C);
		return C;
	};
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	for (const FWEAssemblyPart& Part : Assembly->Parts)
	{
		FPartVis& Vis = Parts.AddDefaulted_GetRef();
		Vis.Mesh = Part.Mesh ? Make(Part.Mesh, FTransform::Identity) : nullptr;
		for (const FWEFastener& F : Part.Fasteners)
		{
			FFastenerVis& FV = Vis.Fasteners.AddDefaulted_GetRef();
			UStaticMesh* FM = LoadObject<UStaticMesh>(nullptr, WEAssemblyViewConst::FastenerMesh(F.Kind), nullptr, LOAD_Quiet | LOAD_NoWarn);
			const FVector Dir = F.Direction.GetSafeNormal();
			const FMatrix Basis = F.Side.IsNearlyZero() ? FRotationMatrix::MakeFromZ(Dir) : FRotationMatrix::MakeFromZX(Dir, F.Side);
			FV.Rest = FTransform(Basis.Rotator(), F.Location);
			FV.Mesh = Make(FM ? FM : Cylinder, FV.Rest);
			if (!FM)
			{
				FV.Mesh->SetRelativeScale3D(FVector(0.008f, 0.008f, 0.004f));
			}
		}
	}
	bBuilt = true;
	// Parts that were already out before we came (joined later, loaded game): no animation.
	UpdateState(-100.f);
	for (FPartVis& Vis : Parts)
	{
		if (Vis.bOut)
		{
			Vis.OutAt = -100.f;
		}
	}
}

int32 AWEAssemblyView::RepairPartIndex() const
{
	// The part of the current repair fault that is taken out (Removed and not yet Replaced) is shown as out.
	const AWEPickupItem* Device = Item.Get();
	const UWEItemDefinition* Def = Device ? Device->GetItem().Definition.Get() : nullptr;
	const UWERepairDefinition* Repair = Def && Def->CanBeRepaired() ? Def->Repair.Get() : nullptr;
	if (!Repair || !Def->Disassembly)
	{
		return INDEX_NONE;
	}
	const FWERepairState& State = Device->GetItem().Repair;
	for (int32 f = 0; f < Repair->Faults.Num() && f < UWERepairDefinition::MaxFaults; ++f)
	{
		if (!FWERepairState::HasBit(State.Removed, f) || FWERepairState::HasBit(State.Replaced, f))
		{
			continue;
		}
		for (int32 i = 0; i < Def->Disassembly->NumSteps(); ++i)
		{
			for (const FWEDisassemblyOutput& O : Def->Disassembly->GetStep(i)->Outputs)
			{
				if (O.Item == Repair->Faults[f].Part && O.bInheritCondition)
				{
					return i;
				}
			}
		}
	}
	return INDEX_NONE;
}

bool AWEAssemblyView::IsPartInside(int32 Part) const
{
	return Parts.IsValidIndex(Part) && !Parts[Part].bOut;
}

void AWEAssemblyView::SetActiveTask(int32 Part, int32 Fastener, float Progress)
{
	ActivePart = Part;
	ActiveFastener = Fastener;
	ActiveProgress = Progress;
}

void AWEAssemblyView::UpdateState(float Now)
{
	const AWEPickupItem* Device = Item.Get();
	AWEWorkbench* B = Bench.Get();
	if (!Device || !B || !Assembly)
	{
		return;
	}
	const FWEItemInstance& State = Device->GetItem();
	const int32 RepairOut = RepairPartIndex();
	// A repair works with the case open: the side panel (service access) is shown off while one is under way.
	const bool bServiceOpen = State.Repair.IsInProgress() || (B->IsBusy() && B->IsRepairJob());
	for (int32 i = 0; i < Parts.Num(); ++i)
	{
		FPartVis& Vis = Parts[i];
		const bool bOut = State.IsPartRemoved(i) || i == RepairOut || (bServiceOpen && Assembly->Parts[i].bServiceAccess);
		if (bOut && !Vis.bOut)
		{
			Vis.bOut = true;
			Vis.OutAt = Now;
			Vis.bGone = false;
			if (Now > 0.f)
			{
				if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, TEXT("/Game/WorkshopEmpire/Audio/SFX_PartRemoved.SFX_PartRemoved")))
				{
					UGameplayStatics::PlaySoundAtLocation(this, Sound, GetPartCenter(i), 0.8f);
				}
			}
		}
		else if (!bOut && Vis.bOut)
		{
			// Back in (spare part installed, cancelled repair): at its place again.
			Vis.bOut = false;
			Vis.OutAt = -1.f;
			Vis.bGone = false;
			if (Vis.Mesh)
			{
				Vis.Mesh->SetVisibility(true);
				Vis.Mesh->SetRelativeTransform(FTransform::Identity);
			}
		}
	}

	// Fasteners done in the running hands-on job.
	const bool bManual = B->IsManualWork();
	const uint32 Mask = bManual ? B->GetTaskMask() : 0;
	const FWEWorkPlan Plan = bManual ? B->GetWorkPlan(B->IsRepairJob()) : FWEWorkPlan();
	for (int32 p = 0; p < Parts.Num(); ++p)
	{
		for (int32 f = 0; f < Parts[p].Fasteners.Num(); ++f)
		{
			bool bDone = false;
			for (int32 t = 0; t < Plan.Tasks.Num(); ++t)
			{
				bDone |= Plan.Tasks[t].AssemblyPart == p && Plan.Tasks[t].Fastener == f && (Mask & (1u << t)) != 0;
			}
			FFastenerVis& FV = Parts[p].Fasteners[f];
			if (bDone && FV.DoneAt < 0.f)
			{
				FV.DoneAt = Now;
				if (Now > 0.f)
				{
					if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, WEAssemblyViewConst::DoneSound(Assembly->Parts[p].Fasteners[f].Kind)))
					{
						UGameplayStatics::PlaySoundAtLocation(this, Sound, FV.Mesh ? FV.Mesh->GetComponentLocation() : GetActorLocation(), 0.7f);
					}
				}
			}
			else if (!bDone)
			{
				FV.DoneAt = -1.f;
			}
		}
	}
}

void AWEAssemblyView::AnimatePart(int32 Index, FPartVis& Vis, float Now)
{
	using namespace WEAssemblyViewConst;
	const FWEAssemblyPart& Part = Assembly->Parts[Index];
	AWEWorkbench* B = Bench.Get();
	const FVector Pull = Part.PullDirection.GetSafeNormal() * Part.PullDistance;
	if (Vis.Mesh)
	{
		if (!Vis.bOut)
		{
			// Held on the part itself: it starts to move along its way out.
			const float P = ActivePart == Index && ActiveFastener == INDEX_NONE ? ActiveProgress : 0.f;
			Vis.Mesh->SetRelativeLocation(Pull * 0.35f * Ease(P));
			Vis.Mesh->SetVisibility(true);
			// Highlight: selected part, part under the cursor.
			Vis.Mesh->SetOverlayMaterial(Index == SelectedPart ? SelectMaterial.Get() : Index == HoverPart ? HighlightMaterial.Get() : nullptr);
		}
		else if (!Vis.bGone)
		{
			Vis.Mesh->SetOverlayMaterial(nullptr);
			const float T = Now - Vis.OutAt;
			const FTransform Device = GetActorTransform();
			const FVector Center = Part.Mesh ? Part.Mesh->GetBoundingBox().GetCenter() : FVector::ZeroVector;
			const FVector Slid = Device.TransformVector(Pull);
			const FVector Up(0.f, 0.f, LiftHeight);
			FVector WorldOffset = FVector::ZeroVector; // added to the part's world placement
			bool bDone = false;
			if (T < SlideSeconds)
			{
				WorldOffset = Slid * (0.35f + 0.65f * Ease(T / SlideSeconds));
			}
			else if (T < SlideSeconds + LiftSeconds)
			{
				WorldOffset = Slid + Up * Ease((T - SlideSeconds) / LiftSeconds);
			}
			else
			{
				const float A = Ease((T - SlideSeconds - LiftSeconds) / AwaySeconds);
				FVector Target;
				if (Part.bLaysBeside && B)
				{
					// Laid flat beside the device on the bench top.
					const FBox Bounds = GetDeviceBounds();
					Target = FVector(0.f, 0.f, 0.f) + B->GetActorRightVector() * (Bounds.GetExtent().Y + 30.f) + FVector(0.f, 0.f, -(Device.TransformPosition(Center).Z - B->GetWorkSpotLocation().Z) + 1.5f);
				}
				else
				{
					// Down into the bench storage (lower shelf).
					Target = (B ? B->GetWorkSpotLocation() - FVector(0.f, 0.f, 45.f) : Device.GetLocation()) - Device.TransformPosition(Center);
				}
				WorldOffset = FMath::Lerp(Slid + Up, Target, A);
				bDone = T >= SlideSeconds + LiftSeconds + AwaySeconds;
			}
			Vis.Mesh->SetWorldLocation(Device.GetLocation() + WorldOffset);
			if (bDone && !Part.bLaysBeside)
			{
				Vis.Mesh->SetVisibility(false);
				Vis.bGone = true;
			}
		}
	}
	// Fasteners: done ones are out of the way; the one being worked on moves a little.
	TrayCount = 0;
	for (int32 f = 0; f < Vis.Fasteners.Num(); ++f)
	{
		FFastenerVis& FV = Vis.Fasteners[f];
		if (!FV.Mesh)
		{
			continue;
		}
		if (Vis.bOut)
		{
			FV.Mesh->SetVisibility(false);
			continue;
		}
		FV.Mesh->SetVisibility(true);
		const FWEFastener& F = Part.Fasteners[f];
		const FVector Axis = F.Direction.GetSafeNormal();
		const bool bActive = ActivePart == Index && ActiveFastener == f && FV.DoneAt < 0.f;
		const float P = bActive ? ActiveProgress : 0.f;
		FTransform T = FV.Rest;
		auto Spin = [&T](float Degrees) { T.SetRotation(T.GetRotation() * FQuat(FVector::UpVector, FMath::DegreesToRadians(Degrees))); };
		switch (F.Kind)
		{
		case EWEFastenerKind::Screw:
		case EWEFastenerKind::Thumbscrew:
			if (FV.DoneAt >= 0.f)
			{
				const float A = (Now - FV.DoneAt) / 0.5f;
				if (A < 1.f)
				{
					T.SetLocation(FV.Rest.GetLocation() + Axis * (1.2f + 6.f * Ease(A)));
					Spin(1080.f + A * 360.f);
				}
				else
				{
					// Lies in the tray in front of the device.
					FV.Mesh->SetVisibility(false);
				}
			}
			else
			{
				T.SetLocation(FV.Rest.GetLocation() + Axis * 1.2f * P);
				Spin(1080.f * P);
			}
			break;
		case EWEFastenerKind::Plug:
			T.SetLocation(FV.Rest.GetLocation() + Axis * (FV.DoneAt >= 0.f ? 3.5f : 1.2f * P));
			break;
		case EWEFastenerKind::Clip:
		case EWEFastenerKind::Lever:
		{
			const float Open = FV.DoneAt >= 0.f ? 1.f : 0.5f * P;
			const float Max = F.Kind == EWEFastenerKind::Lever ? 80.f : 35.f;
			// Negative about Y: a lever (arm along X) lifts towards Z, a clip (standing along Z) tips away from Side.
			T.SetRotation(T.GetRotation() * FQuat(FVector::RightVector, FMath::DegreesToRadians(-Max * Open)));
			break;
		}
		}
		FV.Mesh->SetRelativeTransform(FTransform(T.GetRotation(), T.GetLocation(), FV.Mesh->GetRelativeScale3D()));
	}
}

void AWEAssemblyView::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bBuilt || !Item.IsValid() || !Bench.IsValid())
	{
		return;
	}
	SetActorTransform(DeviceTransform());
	if (Item->GetMesh() && Item->GetMesh()->IsVisible())
	{
		Item->GetMesh()->SetVisibility(false);
	}
	const float Now = GetWorld()->GetTimeSeconds();
	UpdateState(Now);
	for (int32 i = 0; i < Parts.Num(); ++i)
	{
		AnimatePart(i, Parts[i], Now);
	}
}

FVector AWEAssemblyView::DeviceToWorld(const FVector& DevicePoint) const
{
	return GetActorTransform().TransformPosition(DevicePoint);
}

FVector AWEAssemblyView::GetPartCenter(int32 Part) const
{
	if (!Parts.IsValidIndex(Part) || !Parts[Part].Mesh)
	{
		return GetActorLocation();
	}
	return Parts[Part].Mesh->Bounds.Origin;
}

FVector AWEAssemblyView::GetFastenerLocation(int32 Part, int32 Fastener) const
{
	if (!Parts.IsValidIndex(Part) || !Parts[Part].Fasteners.IsValidIndex(Fastener) || !Parts[Part].Fasteners[Fastener].Mesh)
	{
		return GetPartCenter(Part);
	}
	return Parts[Part].Fasteners[Fastener].Mesh->GetComponentLocation();
}

FBox AWEAssemblyView::GetDeviceBounds() const
{
	FBox Box(ForceInit);
	for (const FPartVis& Vis : Parts)
	{
		if (Vis.Mesh && !Vis.bOut)
		{
			Box += Vis.Mesh->Bounds.GetBox();
		}
	}
	return Box.IsValid ? Box : FBox(GetActorLocation() - FVector(10.f), GetActorLocation() + FVector(10.f));
}

int32 AWEAssemblyView::FindPartUnderRay(const FVector& Origin, const FVector& Direction) const
{
	int32 Best = INDEX_NONE;
	float BestT = BIG_NUMBER;
	for (int32 i = 0; i < Parts.Num(); ++i)
	{
		const FPartVis& Vis = Parts[i];
		if (Vis.bOut || !Vis.Mesh || !Vis.Mesh->GetStaticMesh())
		{
			continue;
		}
		// Ray against the part's own box, in the part's space (slab test).
		const FTransform& T = Vis.Mesh->GetComponentTransform();
		const FVector O = T.InverseTransformPosition(Origin);
		const FVector D = T.InverseTransformVector(Direction);
		const FBox Box = Vis.Mesh->GetStaticMesh()->GetBoundingBox().ExpandBy(0.3f);
		float TMin = 0.f, TMax = BIG_NUMBER;
		bool bHit = true;
		for (int32 Axis = 0; Axis < 3 && bHit; ++Axis)
		{
			if (FMath::Abs(D[Axis]) < KINDA_SMALL_NUMBER)
			{
				bHit = O[Axis] >= Box.Min[Axis] && O[Axis] <= Box.Max[Axis];
				continue;
			}
			float T1 = (Box.Min[Axis] - O[Axis]) / D[Axis];
			float T2 = (Box.Max[Axis] - O[Axis]) / D[Axis];
			if (T1 > T2) { Swap(T1, T2); }
			TMin = FMath::Max(TMin, T1);
			TMax = FMath::Min(TMax, T2);
			bHit = TMin <= TMax;
		}
		// Prefer small parts inside big ones (the case box encloses everything): rank by box size when both are hit.
		const float Size = Box.GetSize().Size();
		const float Score = TMin + Size * 0.02f + (Assembly->Parts[i].bShell ? 1000.f : 0.f);
		if (bHit && Score < BestT)
		{
			BestT = Score;
			Best = i;
		}
	}
	return Best;
}
