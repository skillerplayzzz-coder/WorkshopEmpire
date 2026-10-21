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
#include "Framework/WEPlayerController.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Fx/WEFx.h"
#include "Inventory/WEInventoryComponent.h"
#include "Player/WECharacter.h"

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
	for (UStaticMeshComponent* Mesh : BoxFlaps)
	{
		if (Mesh) { Mesh->DestroyComponent(); }
	}
	BoxFlaps.Reset();
	if (BoxMesh) { BoxMesh->DestroyComponent(); BoxMesh = nullptr; }
	if (FilmMesh) { FilmMesh->DestroyComponent(); FilmMesh = nullptr; }
	BoxTask = INDEX_NONE;
	ScrewCount = 0;
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
			// 0.19.1: the cover is made of the device's own housing material (a plain grey plate looked foreign), a little
			// smaller than the top so the housing edge stays visible around it.
			const FLinearColor LidTint = DeviceTint * 0.62f;
			UStaticMeshComponent* LidMesh = AddMesh(Piece, Cube, FVector(Half.X * 2.f * 0.8f / 100.f, Half.Y * 2.f * 0.8f / 100.f, 0.005f), Flat, FVector(0.f, 0.f, 0.3f), &LidTint);
			const UStaticMeshComponent* Housing = Item ? Item->FindComponentByClass<UStaticMeshComponent>() : nullptr;
			if (UMaterialInterface* HousingMat = Housing ? Housing->GetMaterial(0) : nullptr)
			{
				LidMesh->SetMaterial(0, HousingMat);
			}
			break;
		}
		case EWEWorkTaskKind::PullPart:
		case EWEWorkTaskKind::InsertPart:
		{
			UStaticMesh* PartMesh = Task.Part && Task.Part->WorldMesh ? Task.Part->WorldMesh.Get() : Cube.Get();
			const FBox PartBox = PartMesh->GetBoundingBox();
			const float Size = FMath::Max(PartBox.GetSize().GetMax(), 1.f);
			// 0.19.1: smaller and sunk into the device (a board as big as the whole monitor stuck out of it).
			const float Target = FMath::Clamp(FMath::Min(Half.X, Half.Y) * 0.55f, 5.f, 18.f);
			UStaticMeshComponent* C = AddMesh(Piece, PartMesh, FVector(Target / Size), Flat, FVector(0.f, 0.f, -FMath::Min(Half.Z * 0.35f, 6.f)), Task.Part && Task.Part->WorldMesh ? nullptr : &DeviceTint);
			if (Task.Part && Task.Part->WorldMesh && Task.Part->Surface != EWESurface::Placeholder)
			{
				WEPlaceholder::ApplyTint(C, BaseMaterial, Task.Part->Tint);
			}
			break;
		}
		case EWEWorkTaskKind::PowerTest:
			AddMesh(Piece, Cylinder, FVector(0.02f, 0.02f, 0.008f), Flat, FVector::ZeroVector, &WEWorkRigConst::Button);
			break;
		default:
			break;
		case EWEWorkTaskKind::Wipe:
			AddMesh(Piece, Sphere, FVector(0.05f, 0.04f, 0.003f), Flat, FVector::ZeroVector, &WEWorkRigConst::Dirt);
			break;
		case EWEWorkTaskKind::AirBlow:
			// Phase 21: a dusty patch that is blown away.
			if (UStaticMesh* Dust = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_DustLayer.SM_Gen_DustLayer"), nullptr, LOAD_Quiet | LOAD_NoWarn))
			{
				const float S = FMath::Clamp(FMath::Min(Half.X, Half.Y) * 0.06f, 0.35f, 1.2f);
				AddMesh(Piece, Dust, FVector(S, S, 1.f), Flat, FVector::ZeroVector, nullptr);
			}
			break;
		case EWEWorkTaskKind::PasteApply:
			// Phase 21: the new thermal paste grows out of the syringe.
			if (UStaticMesh* Blob = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_PasteBlob.SM_Gen_PasteBlob"), nullptr, LOAD_Quiet | LOAD_NoWarn))
			{
				AddMesh(Piece, Blob, FVector(1.f), Flat, FVector::ZeroVector, nullptr);
			}
			break;
		}
		if (Task.Kind == EWEWorkTaskKind::InsertPart && BoxTask == INDEX_NONE)
		{
			BuildPartBox(i);
		}
	}
}

void AWEWorkRig::BuildPartBox(int32 TaskIndex)
{
	// Phase 21: the spare part waits in its open cardboard box beside the device (a new one under a protective film).
	const FWEWorkTask& Task = Plan.Tasks[TaskIndex];
	if (!Pieces.IsValidIndex(TaskIndex) || Pieces[TaskIndex].Meshes.Num() == 0)
	{
		return;
	}
	UStaticMesh* Box = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_PartBox.SM_Gen_PartBox"), nullptr, LOAD_Quiet | LOAD_NoWarn);
	UStaticMesh* Flap = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_BoxFlap.SM_Gen_BoxFlap"), nullptr, LOAD_Quiet | LOAD_NoWarn);
	if (!Box || !Flap)
	{
		return;
	}
	BoxTask = TaskIndex;
	BoxBuiltAt = GetWorld()->GetTimeSeconds();
	BoxStage = 0;
	// New spare part available (inventory or bench storage)? Then it comes with a film.
	bBoxNew = false;
	const AWEWorkbench* B = Bench.Get();
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const AWECharacter* Me = PC ? Cast<AWECharacter>(PC->GetPawn()) : nullptr;
	for (const UWEInventoryComponent* Inv : { Me ? Me->GetPlayerInventory() : nullptr, B ? B->GetStorage() : nullptr })
	{
		for (const FWEInventorySlotData& Slot : Inv ? Inv->GetAllSlots() : TArray<FWEInventorySlotData>())
		{
			bBoxNew |= Slot.Item.Definition == Task.Part && Slot.Item.Condition == EWEItemCondition::New;
		}
	}
	const FBox PartBounds = Pieces[TaskIndex].Meshes[0]->Bounds.GetBox();
	const float PartSize = FMath::Max(PartBounds.GetSize().X, PartBounds.GetSize().Y);
	PartLift = PartBounds.GetExtent().Z;
	const float Scale = FMath::Clamp(PartSize * 1.3f / 14.f, 0.6f, 2.5f);
	// Beside the device on the free side of the bench, where the camera sees it.
	BoxSpot = FVector(Center.X, Center.Y, Tray.Z) + AxisY * (Half.Y + 9.f * Scale + 4.f) + AxisX * 4.f;
	auto Make = [this](UStaticMesh* Mesh)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(Mesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetMobility(EComponentMobility::Movable);
		C->SetupAttachment(GetRootComponent());
		C->RegisterComponent();
		return C;
	};
	BoxMesh = Make(Box);
	BoxMesh->SetWorldScale3D(FVector(Scale));
	BoxMesh->SetWorldLocationAndRotation(BoxSpot, AxisX.Rotation());
	for (int32 Side = 0; Side < 2; ++Side)
	{
		UStaticMeshComponent* C = Make(Flap);
		C->SetWorldScale3D(FVector(Scale));
		BoxFlaps.Add(C);
	}
	if (bBoxNew)
	{
		FilmMesh = Make(Cube);
		if (UMaterialInterface* Film = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/WorkshopEmpire/Materials/Props/MI_Prop_GlassClear.MI_Prop_GlassClear"), nullptr, LOAD_Quiet | LOAD_NoWarn))
		{
			FilmMesh->SetMaterial(0, Film);
		}
		FilmMesh->SetCastShadow(false);
	}
	if (USoundBase* Open = LoadObject<USoundBase>(nullptr, TEXT("/Game/WorkshopEmpire/Audio/P21/SFX_K_bookOpen.SFX_K_bookOpen"), nullptr, LOAD_Quiet | LOAD_NoWarn))
	{
		UGameplayStatics::PlaySoundAtLocation(this, Open, BoxSpot, 0.5f);
	}
}

void AWEWorkRig::AnimatePartBox(float Now, float Progress, bool bDone)
{
	if (!BoxMesh)
	{
		return;
	}
	const float Scale = BoxMesh->GetComponentScale().X;
	const FQuat BoxRot = AxisX.Rotation().Quaternion();
	// Flaps fold open when the box is put down.
	const float Open = FMath::SmoothStep(0.f, 1.f, (Now - BoxBuiltAt) / 0.6f) * 115.f;
	for (int32 Side = 0; Side < BoxFlaps.Num(); ++Side)
	{
		const float Sign = Side == 0 ? 1.f : -1.f;
		const FVector Hinge = BoxSpot + BoxRot.RotateVector(FVector(0.f, Sign * 5.f * Scale, 6.f * Scale));
		const FQuat Rot = BoxRot * FQuat(FVector::UpVector, Side == 0 ? PI : 0.f) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Open));
		BoxFlaps[Side]->SetWorldLocationAndRotation(Hinge, Rot);
	}
	// Film: on the part until it is peeled off (0.3 - 0.5 of the task), then gone.
	if (FilmMesh && Pieces.IsValidIndex(BoxTask) && Pieces[BoxTask].Meshes.Num() > 0)
	{
		const FBox Part = Pieces[BoxTask].Meshes[0]->Bounds.GetBox();
		const float Peel = bDone ? 1.f : FMath::Clamp((Progress - 0.3f) / 0.2f, 0.f, 1.f);
		if (Peel >= 1.f)
		{
			FilmMesh->SetVisibility(false);
		}
		else
		{
			if (Peel > 0.f && BoxStage < 2)
			{
				BoxStage = 2;
				if (USoundBase* Peeling = LoadObject<USoundBase>(nullptr, TEXT("/Game/WorkshopEmpire/Audio/P21/SFX_K_cloth2.SFX_K_cloth2"), nullptr, LOAD_Quiet | LOAD_NoWarn))
				{
					UGameplayStatics::PlaySoundAtLocation(this, Peeling, Part.GetCenter(), 0.6f);
				}
			}
			const FVector Ext = Part.GetExtent();
			const FVector Pivot = FVector(Part.GetCenter().X, Part.GetCenter().Y, Part.Max.Z + 0.08f) - AxisX * Ext.X;
			const FQuat Lift(AxisY, FMath::DegreesToRadians(-170.f * Peel));
			const FVector FilmCenter = Pivot + Lift.RotateVector(AxisX * Ext.X);
			FilmMesh->SetVisibility(true);
			FilmMesh->SetWorldScale3D(FVector(Ext.X * 2.1f / 100.f, Ext.Y * 2.1f / 100.f, 0.0006f));
			FilmMesh->SetWorldLocationAndRotation(FilmCenter, Lift * BoxRot);
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
			// Phase 21: lifted out of its box, film peeled off (new part), then set into the device; done = in place.
			const float A = bDone ? 1.f : P;
			const FVector InBox = BoxMesh ? BoxSpot + FVector(0.f, 0.f, 1.4f * BoxMesh->GetComponentScale().X + PartLift + FMath::Min(Half.Z * 0.35f, 6.f)) : Tray + AxisX * -18.f + FVector(0.f, 0.f, 4.f);
			const FVector Lifted = InBox + FVector(0.f, 0.f, 10.f);
			if (A < 0.3f)
			{
				Location = FMath::Lerp(InBox, Lifted, FMath::SmoothStep(0.f, 1.f, A / 0.3f));
			}
			else if (A < 0.5f)
			{
				Location = Lifted;
			}
			else
			{
				Location = FMath::Lerp(Lifted, Piece.Base + FVector(0.f, 0.f, 2.f), FMath::InterpEaseInOut(0.f, 1.f, (A - 0.5f) / 0.5f, 2.f));
			}
			if (i == BoxTask)
			{
				AnimatePartBox(Now, A, bDone);
			}
		}
		else if (Task.Kind == EWEWorkTaskKind::Screw && bDone && Task.AssemblyPart == INDEX_NONE)
		{
			// Phase 21: the screw sticks to the magnetic tip and is dropped into the screw bowl, where it stays.
			if (Piece.BowlIndex == INDEX_NONE)
			{
				Piece.BowlIndex = ScrewCount++;
			}
			const FVector Spot = WEWorkPlan::BowlSpot(B->GetScrewBowlLocation(), Piece.BowlIndex);
			const float A = FMath::Clamp((Now - Piece.DoneAt) / WEWorkPlan::CarrySeconds, 0.f, 1.f);
			Location = WEWorkPlan::CarryPath(Piece.Base, FVector::UpVector, Spot, A);
			Spin = A < 0.25f ? 1080.f + A * 800.f : 0.f;
			if (A >= 1.f && !Piece.bLanded)
			{
				Piece.bLanded = true;
				if (USoundBase* Clink = LoadObject<USoundBase>(nullptr, TEXT("/Game/WorkshopEmpire/Audio/P21/SFX_K_handleCoins.SFX_K_handleCoins"), nullptr, LOAD_Quiet | LOAD_NoWarn))
				{
					UGameplayStatics::PlaySoundAtLocation(this, Clink, Spot, 0.45f);
				}
			}
		}
		else if (Task.Kind == EWEWorkTaskKind::Wipe)
		{
			Scale = bDone ? 0.f : 1.f - 0.8f * P;
			bVisible = Scale > 0.01f;
		}
		else if (Task.Kind == EWEWorkTaskKind::AirBlow)
		{
			// Blown away while the air is on; gone when done.
			Scale = bDone ? 0.f : 1.f - 0.9f * P;
			bVisible = Scale > 0.02f;
		}
		else if (Task.Kind == EWEWorkTaskKind::PasteApply)
		{
			// A pea-sized blob that grows while the plunger is pressed.
			Scale = bDone ? 1.f : P;
			bVisible = Scale > 0.02f;
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
			if (bDone && Task.Kind == EWEWorkTaskKind::PullPart)
			{
				Offset = FVector::ZeroVector; // out of the device: lies on the tray
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
			else if (Task.Kind == EWEWorkTaskKind::AirBlow)
			{
				const float S = FMath::Clamp(FMath::Min(Half.X, Half.Y) * 0.06f, 0.35f, 1.2f);
				C->SetWorldScale3D(FVector(S, S, 1.f) * FMath::Max(Scale, 0.01f));
			}
			else if (Task.Kind == EWEWorkTaskKind::PasteApply)
			{
				C->SetWorldScale3D(FVector(1.2f, 1.2f, 1.f) * FMath::Max(Scale, 0.01f));
			}
		}
	}
	LastMask = Mask;
	TickCleaning(Now);
	TickSuccess(Now);
}

void AWEWorkRig::TickCleaning(float Now)
{
	// Phase 21: compressed air - dust puffs and the hiss while a dusty spot is being blown out.
	const bool bBlowing = Plan.Tasks.IsValidIndex(ActiveTask) && Plan.Tasks[ActiveTask].Kind == EWEWorkTaskKind::AirBlow && ActiveProgress > 0.f && ActiveProgress < 1.f;
	if (bBlowing)
	{
		DustTimer -= GetWorld()->GetDeltaSeconds();
		if (DustTimer <= 0.f)
		{
			DustTimer = 0.12f;
			AWEFx::Burst(this, EWEFxKind::Dust, GetTaskLocation(ActiveTask) + FVector(0.f, 0.f, 1.f), FVector::UpVector, FLinearColor(0.45f, 0.42f, 0.38f), 0.5f);
		}
		if (!AirAudio)
		{
			if (USoundBase* Hiss = LoadObject<USoundBase>(nullptr, TEXT("/Game/WorkshopEmpire/Audio/P20/SFX_RainLoop.SFX_RainLoop"), nullptr, LOAD_Quiet | LOAD_NoWarn))
			{
				AirAudio = UGameplayStatics::SpawnSoundAtLocation(this, Hiss, GetTaskLocation(ActiveTask), FRotator::ZeroRotator, 0.45f, 2.2f, 0.f, nullptr, nullptr, false);
			}
		}
	}
	else if (AirAudio)
	{
		AirAudio->FadeOut(0.12f, 0.f);
		AirAudio = nullptr;
	}
}

void AWEWorkRig::TickSuccess(float Now)
{
	const AWEWorkbench* B = Bench.Get();
	const AWEPickupItem* Item = B ? B->GetPlacedItem() : nullptr;
	if (!Item)
	{
		bPrevKnown = false;
		return;
	}
	const FWEItemInstance& State = Item->GetItem();
	const bool bBroken = State.IsBroken() || State.NeedsFunctionTest();
	if (bPrevKnown && bPrevBroken && !bBroken)
	{
		PlaySuccess();
	}
	bPrevKnown = true;
	bPrevBroken = bBroken;
	if (SuccessAt >= 0.f)
	{
		const float T = Now - SuccessAt;
		const float Glow = T < 0.3f ? T / 0.3f : FMath::Clamp((7.f - T) / 1.5f, 0.f, 1.f);
		if (SuccessLight)
		{
			SuccessLight->SetIntensity(Glow * 900.f * (0.85f + 0.15f * FMath::Sin(T * 6.f)));
			SuccessLight->SetVisibility(Glow > 0.f);
		}
		if (ScreenGlow)
		{
			ScreenGlow->SetVisibility(Glow > 0.f);
		}
		if (T > 7.f)
		{
			SuccessAt = -1.f;
			if (SuccessAudio) { SuccessAudio->FadeOut(0.8f, 0.f); SuccessAudio = nullptr; }
		}
	}
}

void AWEWorkRig::PlaySuccess()
{
	// Phase 21: it works - the device shows it: a light comes on, it sounds like itself, a screen shows a picture.
	const AWEWorkbench* B = Bench.Get();
	const AWEPickupItem* Item = B ? B->GetPlacedItem() : nullptr;
	if (!Item || !Item->GetItem().Definition)
	{
		return;
	}
	SuccessAt = GetWorld()->GetTimeSeconds();
	const FBox Box = Item->GetComponentsBoundingBox(true);
	const FString Id = Item->GetItem().Definition->ItemId.ToString();
	// The side that faces the player.
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const FVector View = PC && PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraLocation() : Box.GetCenter() + AxisX * 100.f;
	FVector Face = View - Box.GetCenter();
	Face.Z = 0.f;
	Face = FMath::Abs(Face.X) > FMath::Abs(Face.Y) ? FVector(FMath::Sign(Face.X), 0.f, 0.f) : FVector(0.f, FMath::Sign(Face.Y), 0.f);
	const FVector Ext = Box.GetExtent();
	if (!SuccessLight)
	{
		SuccessLight = NewObject<UPointLightComponent>(this);
		SuccessLight->SetupAttachment(GetRootComponent());
		SuccessLight->RegisterComponent();
		SuccessLight->SetAttenuationRadius(120.f);
		SuccessLight->SetCastShadows(false);
	}
	const bool bScreen = Id.Contains(TEXT("Monitor")) || Id.Contains(TEXT("Television")) || Id.Contains(TEXT("Computer"));
	SuccessLight->SetLightColor(bScreen ? FLinearColor(0.55f, 0.75f, 1.f) : FLinearColor(0.4f, 1.f, 0.45f));
	SuccessLight->SetWorldLocation(Box.GetCenter() + Face * (FMath::Abs(Face.X) > 0.f ? Ext.X : Ext.Y) + FVector(0.f, 0.f, Ext.Z * 0.3f) + Face * 6.f);
	if (bScreen && !Id.Contains(TEXT("Computer")))
	{
		// A picture on the screen side (the desktop of the workshop OS as a test image).
		if (!ScreenGlow)
		{
			ScreenGlow = NewObject<UStaticMeshComponent>(this);
			ScreenGlow->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
			ScreenGlow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ScreenGlow->SetCastShadow(false);
			ScreenGlow->SetupAttachment(GetRootComponent());
			ScreenGlow->RegisterComponent();
			if (UMaterialInterface* Screen = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/WorkshopEmpire/Materials/Props/MI_Prop_OsDesktop.MI_Prop_OsDesktop"), nullptr, LOAD_Quiet | LOAD_NoWarn))
			{
				ScreenGlow->SetMaterial(0, Screen);
			}
		}
		const float W = (FMath::Abs(Face.X) > 0.f ? Ext.Y : Ext.X) * 1.3f;
		const float H = Ext.Z * 1.1f;
		ScreenGlow->SetWorldScale3D(FVector(H / 100.f, W / 100.f, 1.f));
		// Plane faces +Z: turn it to face outwards on that side.
		ScreenGlow->SetWorldRotation(FRotationMatrix::MakeFromZX(Face, FVector::UpVector).Rotator());
		ScreenGlow->SetWorldLocation(Box.GetCenter() + Face * ((FMath::Abs(Face.X) > 0.f ? Ext.X : Ext.Y) + 0.3f) + FVector(0.f, 0.f, Ext.Z * 0.1f));
		ScreenGlow->SetVisibility(true);
	}
	// It sounds like itself.
	const TCHAR* SoundPath = TEXT("/Game/WorkshopEmpire/Audio/Kenney/confirmation_002.confirmation_002");
	float StartAt = 0.f;
	float Pitch = 1.f;
	if (Id.Contains(TEXT("Radio")) || Id.Contains(TEXT("Television")))
	{
		SoundPath = TEXT("/Game/WorkshopEmpire/Audio/P20/SFX_Radio3.SFX_Radio3");
		StartAt = 42.f;
	}
	else if (Id.Contains(TEXT("Drill")))
	{
		SoundPath = TEXT("/Game/WorkshopEmpire/Audio/SFX_MotorLoop.SFX_MotorLoop");
		Pitch = 2.0f;
	}
	if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, SoundPath, nullptr, LOAD_Quiet | LOAD_NoWarn))
	{
		SuccessAudio = UGameplayStatics::SpawnSoundAtLocation(this, Sound, Box.GetCenter(), FRotator::ZeroRotator, 0.55f, Pitch, StartAt, nullptr, nullptr, false);
	}
	UE_LOG(LogTemp, Log, TEXT("WE_REPAIR_SUCCESS %s"), *Id);
}

FVector AWEWorkRig::GetTaskAxis(int32 Index) const
{
	if (!Plan.Tasks.IsValidIndex(Index))
	{
		return FVector::UpVector;
	}
	const FWEWorkTask& Task = Plan.Tasks[Index];
	if (const AWEAssemblyView* View = Task.AssemblyPart != INDEX_NONE && Task.Fastener != INDEX_NONE ? GetView() : nullptr)
	{
		return View->GetFastenerAxis(Task.AssemblyPart, Task.Fastener);
	}
	return FVector::UpVector;
}

bool AWEWorkRig::GetCarryPoint(FVector& OutPoint) const
{
	if (const AWEAssemblyView* View = GetView(); View && View->GetCarryPoint(OutPoint))
	{
		return true;
	}
	const AWEWorkbench* B = Bench.Get();
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	float Newest = -1.f;
	for (int32 i = 0; i < Pieces.Num(); ++i)
	{
		const FPiece& Piece = Pieces[i];
		const float A = (Now - Piece.DoneAt) / WEWorkPlan::CarrySeconds;
		if (B && Piece.DoneAt > Newest && Piece.BowlIndex != INDEX_NONE && A >= 0.f && A < 0.8f)
		{
			Newest = Piece.DoneAt;
			OutPoint = WEWorkPlan::CarryPath(Piece.Base, FVector::UpVector, WEWorkPlan::BowlSpot(B->GetScrewBowlLocation(), Piece.BowlIndex), A);
		}
	}
	return Newest >= 0.f;
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