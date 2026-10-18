#include "Workshop/WEWorkRig.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "World/WEPlaceholderVisuals.h"
#include "Workshop/WEWorkbench.h"
#include "Workshop/WEAssemblyView.h"

namespace WEWorkRigConst
{
	constexpr float FlySeconds = 0.75f;
	const FLinearColor Steel(0.32f, 0.33f, 0.35f);
	const FLinearColor Plastic(0.08f, 0.08f, 0.09f);
	const FLinearColor Copper(0.75f, 0.38f, 0.16f);
	const FLinearColor Solder(0.72f, 0.74f, 0.76f);
	const FLinearColor ProbeRed(0.8f, 0.08f, 0.06f);
	const FLinearColor Dirt(0.24f, 0.19f, 0.12f);
	const FLinearColor Button(0.12f, 0.55f, 0.2f);
}

AWEWorkRig::AWEWorkRig()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AWEWorkRig::Init(AWEWorkbench* InBench, bool bInRepair)
{
	Bench = InBench;
	bRepair = bInRepair;
	BaseMaterial = LoadObject<UMaterialInterface>(nullptr, WEPlaceholder::MaterialPath);
	Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	RebuildPlan();
}

void AWEWorkRig::SetRepairMode(bool bInRepair)
{
	if (bRepair != bInRepair)
	{
		bRepair = bInRepair;
		RebuildPlan();
	}
}

uint32 AWEWorkRig::ComputeSignature() const
{
	const AWEWorkbench* B = Bench.Get();
	const AWEPickupItem* Item = B ? B->GetPlacedItem() : nullptr;
	if (!Item)
	{
		return 0;
	}
	uint32 Hash = HashCombine(PointerHash(Item), GetTypeHash(B->GetNextStepIndex()));
	Hash = HashCombine(Hash, HashCombine(GetTypeHash(SelectedPart), GetTypeHash(B->GetActivePart())));
	Hash = HashCombine(Hash, GetTypeHash(Item->GetItem().RemovedParts));
	Hash = HashCombine(Hash, HashCombine(GetTypeHash(bRepair), GetTypeHash(bInstall)));
	if (bRepair)
	{
		FText Reason;
		const FWERepairStepInfo Info = B->GetNextRepairStep(Reason);
		Hash = HashCombine(Hash, HashCombine(GetTypeHash(static_cast<uint8>(Info.Kind)), GetTypeHash(Info.FaultIndex)));
	}
	return Hash == 0 ? 1 : Hash;
}

bool AWEWorkRig::IsTaskDone(int32 Index) const
{
	const AWEWorkbench* B = Bench.Get();
	return B && B->IsManualWork() && B->IsRepairJob() == bRepair && B->IsInstallJob() == bInstall && (B->GetTaskMask() & (1u << Index)) != 0;
}

FVector AWEWorkRig::GetTaskLocation(int32 Index) const
{
	if (!Plan.Tasks.IsValidIndex(Index))
	{
		return Center;
	}
	const FWEWorkTask& Task = Plan.Tasks[Index];
	if (const AWEAssemblyView* View = Task.AssemblyPart != INDEX_NONE ? GetView() : nullptr)
	{
		return Task.Fastener != INDEX_NONE ? View->GetFastenerLocation(Task.AssemblyPart, Task.Fastener) : View->GetPartCenter(Task.AssemblyPart);
	}
	const FVector L = Task.Local;
	return Center + AxisX * Half.X * L.X + AxisY * Half.Y * L.Y + FVector(0.f, 0.f, Half.Z * (L.Z - 1.f) + 0.4f);
}

void AWEWorkRig::ComputeFrame()
{
	const AWEWorkbench* B = Bench.Get();
	const AWEPickupItem* Item = B ? B->GetPlacedItem() : nullptr;
	if (!B)
	{
		return;
	}
	AxisX = B->GetActorForwardVector();
	AxisY = B->GetActorRightVector();
	FVector Origin = B->GetWorkSpotLocation();
	FVector Extent(12.f, 12.f, 6.f);
	if (const AWEAssemblyView* View = GetView())
	{
		// Open device with a part layout: the frame is what is still inside.
		View->GetDeviceBounds().GetCenterAndExtents(Origin, Extent);
	}
	else if (Item)
	{
		Item->GetActorBounds(true, Origin, Extent);
	}
	// Axis-aligned bounds projected on the bench axes.
	Half.X = FMath::Max(3.f, FMath::Abs(AxisX.X) * Extent.X + FMath::Abs(AxisX.Y) * Extent.Y);
	Half.Y = FMath::Max(3.f, FMath::Abs(AxisY.X) * Extent.X + FMath::Abs(AxisY.Y) * Extent.Y);
	Half.Z = FMath::Max(1.f, Extent.Z);
	Center = Origin + FVector(0.f, 0.f, Extent.Z);
	// Parts tray: on the bench in front of the device (towards the player and the camera), so removed parts stay in view.
	const FVector Spot = B->GetWorkSpotLocation();
	Tray = FVector(Spot.X, Spot.Y, Spot.Z) + AxisX * FMath::Min(Half.X + 12.f, 34.f) - AxisY * FMath::Min(Half.Y, 25.f) + FVector(0.f, 0.f, 1.f);
}

void AWEWorkRig::RebuildPlan()
{
	for (UStaticMeshComponent* Mesh : AllMeshes)
	{
		if (Mesh)
		{
			Mesh->DestroyComponent();
		}
	}
	AllMeshes.Reset();
	Pieces.Reset();
	LastMask = 0;
	PlanSignature = ComputeSignature();
	AWEWorkbench* B = Bench.Get();
	if (B && PlanSignature != 0 && B->GetAssembly() && !bRepair)
	{
		// Part layout: the plan belongs to the part being worked on, else to the part the player picked (none: pick one).
		Plan = B->GetActivePart() != INDEX_NONE ? B->GetWorkPlan(false) : (SelectedPart != INDEX_NONE ? B->GetPartPlan(SelectedPart, bInstall) : FWEWorkPlan());
	}
	else
	{
		Plan = B && PlanSignature != 0 ? B->GetWorkPlan(bRepair) : FWEWorkPlan();
	}
	ComputeFrame();
	const AWEPickupItem* Item = B ? B->GetPlacedItem() : nullptr;
	const FLinearColor DeviceTint = Item && Item->GetItem().Definition ? Item->GetItem().Definition->Tint : FLinearColor(0.5f, 0.5f, 0.5f);

	auto AddMesh = [this](FPiece& Piece, UStaticMesh* Mesh, const FVector& Scale, const FRotator& Rot, const FVector& Offset, const FLinearColor* Tint)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(Mesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetMobility(EComponentMobility::Movable);
		C->SetupAttachment(GetRootComponent());
		C->RegisterComponent();
		C->SetWorldScale3D(Scale);
		C->SetWorldRotation(Rot);
		C->ComponentTags.Add(*Offset.ToString());
		if (Tint)
		{
			WEPlaceholder::ApplyTint(C, BaseMaterial, *Tint);
		}
		Piece.Meshes.Add(C);
		AllMeshes.Add(C);
		return C;
	};

	const FRotator Flat = AxisX.Rotation();
	for (int32 i = 0; i < Plan.Tasks.Num(); ++i)
	{
		const FWEWorkTask& Task = Plan.Tasks[i];
		FPiece& Piece = Pieces.AddDefaulted_GetRef();
		Piece.Base = GetTaskLocation(i);
		if (Task.AssemblyPart != INDEX_NONE)
		{
			continue; // the real screw / plug / part is shown and moved by the device view
		}
		switch (Task.Kind)
		{
		case EWEWorkTaskKind::Screw:
			AddMesh(Piece, Cylinder, FVector(0.013f, 0.013f, 0.006f), Flat, FVector::ZeroVector, &WEWorkRigConst::Steel);
			AddMesh(Piece, Cube, FVector(0.011f, 0.002f, 0.003f), Flat, FVector::ZeroVector, &WEWorkRigConst::Plastic); // slot
			break;
		case EWEWorkTaskKind::Clip:
		case EWEWorkTaskKind::Grip:
			AddMesh(Piece, Cube, FVector(0.022f, 0.01f, 0.012f), Flat, FVector::ZeroVector, &WEWorkRigConst::Plastic);
			break;
		case EWEWorkTaskKind::Cut:
			AddMesh(Piece, Cylinder, FVector(0.007f, 0.007f, 0.09f), FRotator(90.f, Flat.Yaw + 90.f, 0.f), FVector::ZeroVector, &WEWorkRigConst::Copper);
			break;
		case EWEWorkTaskKind::Solder:
			AddMesh(Piece, Sphere, FVector(0.012f), Flat, FVector::ZeroVector, &WEWorkRigConst::Solder);
			break;
		case EWEWorkTaskKind::Probe:
		case EWEWorkTaskKind::Knock:
			AddMesh(Piece, Sphere, FVector(0.011f), Flat, FVector::ZeroVector, &WEWorkRigConst::ProbeRed);
			break;
		case EWEWorkTaskKind::Unplug:
			AddMesh(Piece, Cube, FVector(0.03f, 0.02f, 0.016f), Flat, FVector::ZeroVector, &WEWorkRigConst::Plastic);
			AddMesh(Piece, Cylinder, FVector(0.006f, 0.006f, 0.08f), FRotator(90.f, Flat.Yaw, 0.f), -AxisX * 4.5f, &WEWorkRigConst::Plastic);
			break;
		case EWEWorkTaskKind::Lid:
		{
			const FLinearColor LidTint = DeviceTint * 0.62f; // a little darker than the housing, so the cover reads as a separate panel
			AddMesh(Piece, Cube, FVector(Half.X * 2.f * 0.88f / 100.f, Half.Y * 2.f * 0.88f / 100.f, 0.008f), Flat, FVector(0.f, 0.f, 0.4f), &LidTint);
			break;
		}
		case EWEWorkTaskKind::PullPart:
		case EWEWorkTaskKind::InsertPart:
		{
			UStaticMesh* PartMesh = Task.Part && Task.Part->WorldMesh ? Task.Part->WorldMesh.Get() : Cube.Get();
			const FBox PartBox = PartMesh->GetBoundingBox();
			const float Size = FMath::Max(PartBox.GetSize().GetMax(), 1.f);
			const float Target = FMath::Clamp(FMath::Min(Half.X, Half.Y) * 0.9f, 6.f, 30.f);
			UStaticMeshComponent* C = AddMesh(Piece, PartMesh, FVector(Target / Size), Flat, FVector::ZeroVector, Task.Part && Task.Part->WorldMesh ? nullptr : &DeviceTint);
			if (Task.Part && Task.Part->WorldMesh && Task.Part->Surface != EWESurface::Placeholder)
			{
				WEPlaceholder::ApplyTint(C, BaseMaterial, Task.Part->Tint);
			}
			break;
		}
		case EWEWorkTaskKind::PowerTest:
			AddMesh(Piece, Cylinder, FVector(0.02f, 0.02f, 0.008f), Flat, FVector::ZeroVector, &WEWorkRigConst::Button);
			break;
		case EWEWorkTaskKind::Wipe:
			AddMesh(Piece, Sphere, FVector(0.05f, 0.04f, 0.003f), Flat, FVector::ZeroVector, &WEWorkRigConst::Dirt);
			break;
		}
	}
}

void AWEWorkRig::PlayTaskSound(EWEWorkTaskKind Kind) const
{
	const TCHAR* Path = TEXT("/Game/WorkshopEmpire/Audio/Kenney/impactMetal_light_002.impactMetal_light_002");
	switch (Kind)
	{
	case EWEWorkTaskKind::Screw:      Path = TEXT("/Game/WorkshopEmpire/Audio/Kenney/impactMetal_light_001.impactMetal_light_001"); break;
	case EWEWorkTaskKind::Clip:
	case EWEWorkTaskKind::Grip:       Path = TEXT("/Game/WorkshopEmpire/Audio/Kenney/click_004.click_004"); break;
	case EWEWorkTaskKind::Unplug:     Path = TEXT("/Game/WorkshopEmpire/Audio/Kenney/click_003.click_003"); break;
	case EWEWorkTaskKind::Cut:        Path = TEXT("/Game/WorkshopEmpire/Audio/Kenney/impactPlate_light_003.impactPlate_light_003"); break;
	case EWEWorkTaskKind::Lid:        Path = TEXT("/Game/WorkshopEmpire/Audio/Kenney/impactPlate_medium_001.impactPlate_medium_001"); break;
	case EWEWorkTaskKind::PullPart:
	case EWEWorkTaskKind::InsertPart: Path = TEXT("/Game/WorkshopEmpire/Audio/SFX_PartRemoved.SFX_PartRemoved"); break;
	case EWEWorkTaskKind::PowerTest:  Path = TEXT("/Game/WorkshopEmpire/Audio/Kenney/confirmation_001.confirmation_001"); break;
	case EWEWorkTaskKind::Wipe:       Path = TEXT("/Game/WorkshopEmpire/Audio/Kenney/impactSoft_medium_002.impactSoft_medium_002"); break;
	case EWEWorkTaskKind::Knock:      Path = TEXT("/Game/WorkshopEmpire/Audio/Kenney/impactMetal_medium_001.impactMetal_medium_001"); break;
	default: break;
	}
	if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, Path))
	{
		UGameplayStatics::PlaySound2D(this, Sound, 0.7f);
	}
}

void AWEWorkRig::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AWEWorkbench* B = Bench.Get();
	if (!B)
	{
		return;
	}
	if (ComputeSignature() != PlanSignature)
	{
		RebuildPlan();
	}
	ComputeFrame();
	const float Now = GetWorld()->GetTimeSeconds();
	const uint32 Mask = B->IsManualWork() && B->IsRepairJob() == bRepair && B->IsInstallJob() == bInstall ? B->GetTaskMask() : 0;
	int32 TrayCount = 0;
	for (int32 i = 0; i < Pieces.Num(); ++i)
	{
		FPiece& Piece = Pieces[i];
		const FWEWorkTask& Task = Plan.Tasks[i];
		const bool bDone = (Mask & (1u << i)) != 0;
		if (bDone && Piece.DoneAt < 0.f)
		{
			Piece.DoneAt = Now;
			if (Task.AssemblyPart == INDEX_NONE)
			{
				PlayTaskSound(Task.Kind);
			}
		}
		else if (!bDone)
		{
			Piece.DoneAt = -1.f;
		}
		Piece.Base = GetTaskLocation(i);
		FVector Location = Piece.Base;
		float Spin = 0.f;
		float Scale = 1.f;
		bool bVisible = true;
		const bool bActive = ActiveTask == i && !bDone;
		const float P = bActive ? ActiveProgress : 0.f;
		const float Pulse = (HoverTask == i && !bDone) ? 1.f + 0.12f * FMath::Sin(Now * 9.f) : 1.f;
		if (Task.Kind == EWEWorkTaskKind::InsertPart)
		{
			// The spare part waits beside the device and is lowered in while working; done = in place.
			const FVector Waiting = Tray + AxisX * -18.f + FVector(0.f, 0.f, 4.f);
			const float A = bDone ? 1.f : P;
			Location = FMath::Lerp(Waiting, Piece.Base + FVector(0.f, 0.f, 2.f), FMath::InterpEaseInOut(0.f, 1.f, A, 2.f));
		}
		else if (Task.Kind == EWEWorkTaskKind::Wipe)
		{
			Scale = bDone ? 0.f : 1.f - 0.8f * P;
			bVisible = Scale > 0.01f;
		}
		else if (Task.Kind == EWEWorkTaskKind::Probe || Task.Kind == EWEWorkTaskKind::PowerTest || Task.Kind == EWEWorkTaskKind::Solder)
		{
			// Stays on the device; pressed down a little while working.
			Location.Z -= 0.4f * P;
		}
		else if (bDone)
		{
			// Flies to the tray and lies there.
			const float A = FMath::Clamp((Now - Piece.DoneAt) / WEWorkRigConst::FlySeconds, 0.f, 1.f);
			const FVector Lift = Piece.Base + FVector(0.f, 0.f, Task.Kind == EWEWorkTaskKind::Lid ? 18.f : 10.f);
			const FVector Rest = Tray + AxisX * ((TrayCount % 4) * 5.f - 7.f) + AxisY * ((TrayCount / 4) * 6.f);
			Location = A < 0.4f ? FMath::Lerp(Piece.Base, Lift, A / 0.4f) : FMath::Lerp(Lift, Rest, FMath::InterpEaseInOut(0.f, 1.f, (A - 0.4f) / 0.6f, 2.f));
			++TrayCount;
		}
		else if (bActive)
		{
			// Working on it: screws turn out, plugs and parts come up a little.
			switch (Task.Kind)
			{
			case EWEWorkTaskKind::Screw:  Spin = P * 1080.f; Location.Z += P * 1.2f; break;
			case EWEWorkTaskKind::Lid:    Location.Z += P * 1.5f; break;
			case EWEWorkTaskKind::Cut:    Scale = 1.f - 0.15f * FMath::Abs(FMath::Sin(Now * 20.f)); break;
			default:                      Location.Z += P * 2.f; break;
			}
		}
		for (int32 m = 0; m < Piece.Meshes.Num(); ++m)
		{
			UStaticMeshComponent* C = Piece.Meshes[m];
			if (!C)
			{
				continue;
			}
			FVector Offset = FVector::ZeroVector;
			if (C->ComponentTags.Num() > 0)
			{
				Offset.InitFromString(C->ComponentTags[0].ToString());
			}
			C->SetVisibility(bVisible);
			C->SetWorldLocation(Location + Offset);
			if (Spin != 0.f)
			{
				C->SetWorldRotation(FRotator(0.f, AxisX.Rotation().Yaw + Spin, 0.f));
			}
			if (Task.Kind == EWEWorkTaskKind::Wipe)
			{
				C->SetWorldScale3D(FVector(0.05f, 0.04f, 0.003f) * FMath::Max(Scale, 0.01f) * Pulse);
			}
		}
	}
	LastMask = Mask;
}

AWEAssemblyView* AWEWorkRig::GetView() const
{
	const AWEWorkbench* B = Bench.Get();
	return B ? B->GetAssemblyView() : nullptr;
}

void AWEWorkRig::SetActive(int32 Index, float Progress)
{
	ActiveTask = Index;
	ActiveProgress = Progress;
	if (AWEAssemblyView* View = GetView())
	{
		const FWEWorkTask* Task = Plan.Tasks.IsValidIndex(Index) ? &Plan.Tasks[Index] : nullptr;
		if (Task && Task->AssemblyPart != INDEX_NONE)
		{
			View->SetActiveTask(Task->AssemblyPart, Task->Fastener, Progress);
		}
		else
		{
			View->SetActiveTask(INDEX_NONE, INDEX_NONE, 0.f);
		}
	}
}