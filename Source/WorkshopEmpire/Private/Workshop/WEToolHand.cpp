#include "Workshop/WEToolHand.h"

#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Fx/WEFx.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Workshop/WEWorkbench.h"

namespace WEToolHandConst
{
	constexpr float MoveSpeed = 16.f;
	constexpr float TurnSpeed = 14.f;
	constexpr float HoverHeight = 5.f;
	const TCHAR* SoundPath(const TCHAR* Name)
	{
		static FString Path;
		Path = FString::Printf(TEXT("/Game/WorkshopEmpire/Audio/P21/%s.%s"), Name, Name);
		return *Path;
	}
}

AWEToolHand::AWEToolHand()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	bReplicates = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

const TCHAR* AWEToolHand::MeshFor(EWEToolType Tool, bool bDrill)
{
	switch (Tool)
	{
	case EWEToolType::Screwdriver:   return bDrill ? TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_Tool_CordlessDrill.SM_Gen_Tool_CordlessDrill")
		: TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_Tool_Screwdriver.SM_Gen_Tool_Screwdriver");
	case EWEToolType::Pliers:        return TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_Tool_Pliers.SM_Gen_Tool_Pliers");
	case EWEToolType::SideCutter:    return TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_Tool_SideCutter.SM_Gen_Tool_SideCutter");
	case EWEToolType::Hammer:        return TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_Tool_Hammer.SM_Gen_Tool_Hammer");
	case EWEToolType::Multimeter:    return TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_Tool_Multimeter.SM_Gen_Tool_Multimeter");
	case EWEToolType::SolderingIron: return TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_Tool_SolderingIron.SM_Gen_Tool_SolderingIron");
	case EWEToolType::AngleGrinder:  return TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_Tool_AngleGrinder.SM_Gen_Tool_AngleGrinder");
	case EWEToolType::AirDuster:     return TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_Tool_AirDuster.SM_Gen_Tool_AirDuster");
	default:                         return nullptr;
	}
}

void AWEToolHand::Init(AWEWorkbench* InBench)
{
	Bench = InBench;
	auto Prop = [this](const TCHAR* Path)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Path, nullptr, LOAD_Quiet | LOAD_NoWarn));
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetMobility(EComponentMobility::Movable);
		C->SetupAttachment(GetRootComponent());
		C->RegisterComponent();
		C->SetVisibility(false);
		return C;
	};
	if (!Syringe)
	{
		Syringe = Prop(TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_Tool_PasteSyringe.SM_Gen_Tool_PasteSyringe"));
		Brush = Prop(TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_Tool_Brush.SM_Gen_Tool_Brush"));
	}
}

void AWEToolHand::SetTools(const TArray<EWEToolType>& InTools, bool bInDrill)
{
	if (InTools == Tools && bInDrill == bDrill)
	{
		return;
	}
	Tools = InTools;
	bDrill = bInDrill;
	Rebuild();
}

void AWEToolHand::Rebuild()
{
	for (FToolVis& Vis : Visuals)
	{
		if (Vis.Mesh)
		{
			Vis.Mesh->DestroyComponent();
		}
	}
	Visuals.Reset();
	for (const EWEToolType Tool : Tools)
	{
		FToolVis& Vis = Visuals.AddDefaulted_GetRef();
		Vis.Tool = Tool;
		const TCHAR* Path = MeshFor(Tool, bDrill);
		UStaticMesh* Mesh = Path ? LoadObject<UStaticMesh>(nullptr, Path, nullptr, LOAD_Quiet | LOAD_NoWarn) : nullptr;
		if (!Mesh)
		{
			continue; // hands: nothing to show
		}
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(Mesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetMobility(EComponentMobility::Movable);
		C->SetCastShadow(true);
		C->SetupAttachment(GetRootComponent());
		C->RegisterComponent();
		Vis.Mesh = C;
	}
	HeldIndex = 0;
}

FTransform AWEToolHand::RestTransform(int32 Slot, const FToolVis& Vis) const
{
	const AWEWorkbench* B = Bench.Get();
	if (!B || !Vis.Mesh)
	{
		return FTransform::Identity;
	}
	// Lying on the bench, tip towards the front edge, handle towards the back.
	const FVector Back = -B->GetActorForwardVector();
	const bool bHammer = Vis.Tool == EWEToolType::Hammer;
	const FMatrix Basis = bHammer ? FRotationMatrix::MakeFromXZ(Back, FVector::UpVector) : FRotationMatrix::MakeFromZX(Back, FVector::UpVector);
	const FBox Box = Vis.Mesh->GetStaticMesh()->GetBoundingBox();
	const float Lift = bHammer ? -Box.Min.Z : -Box.Min.X;
	return FTransform(Basis.ToQuat(), B->GetToolRestLocation(Slot) + FVector(0.f, 0.f, Lift + 0.1f));
}

FTransform AWEToolHand::HandTransform(const FToolVis& Vis, float Time) const
{
	const FVector Up = FVector::UpVector;
	const FVector Anchor = State.bWorking ? State.WorkPoint : State.bCarrying ? State.CarryPoint : State.HoverPoint;
	FVector ToView = State.ViewLocation - Anchor;
	ToView.Z = 0.f;
	ToView = ToView.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	// The handle leans away from the camera and to the right, so the tool never hides the screw it works on.
	const FVector Side = FVector::CrossProduct(Up, ToView).GetSafeNormal();
	const FVector Lean = (-ToView * 0.35f + Side * 0.35f);
	if (Vis.Tool == EWEToolType::Hammer)
	{
		// Face down on the spot, handle towards the player; taps while working.
		const float Tap = State.bWorking ? 3.f * FMath::Abs(FMath::Sin(Time * 16.f)) : WEToolHandConst::HoverHeight;
		return FTransform(FRotationMatrix::MakeFromXZ(ToView, Up).ToQuat(), Anchor + Up * Tap);
	}
	FVector Dir;
	FVector Tip;
	if (State.bWorking)
	{
		Dir = State.WorkAxis.GetSafeNormal(UE_SMALL_NUMBER, Up);
		if (Vis.Tool == EWEToolType::Multimeter || Vis.Tool == EWEToolType::SolderingIron || Vis.Tool == EWEToolType::Pliers || Vis.Tool == EWEToolType::SideCutter)
		{
			// Held at an angle like a pen.
			Dir = (Dir + Lean * 1.2f).GetSafeNormal();
		}
		Tip = State.WorkPoint + State.WorkAxis.GetSafeNormal(UE_SMALL_NUMBER, Up) * 0.15f;
	}
	else if (State.bCarrying)
	{
		Dir = (Up + Lean * 0.6f).GetSafeNormal();
		Tip = State.CarryPoint + Up * 0.55f;
	}
	else
	{
		Dir = (Up + Lean).GetSafeNormal();
		Tip = State.HoverPoint + Up * WEToolHandConst::HoverHeight;
	}
	FQuat Rot = FRotationMatrix::MakeFromZX(Dir, -ToView).ToQuat();
	// Turning: the screwdriver by the screw's progress, the drill fast; pliers rock a little.
	const bool bTurns = Vis.Tool == EWEToolType::Screwdriver;
	if (bTurns && State.bWorking)
	{
		Rot = FQuat(Dir, FMath::DegreesToRadians(-SpinAngle)) * Rot;
	}
	else if (State.bWorking && (Vis.Tool == EWEToolType::Pliers || Vis.Tool == EWEToolType::SideCutter))
	{
		Rot = FQuat(Dir, FMath::DegreesToRadians(10.f * FMath::Sin(Time * 9.f))) * Rot;
	}
	return FTransform(Rot, Tip);
}

void AWEToolHand::PlaySound(const TCHAR* Name, const FVector& At, float Volume) const
{
	if (USoundBase* S = LoadObject<USoundBase>(nullptr, WEToolHandConst::SoundPath(Name), nullptr, LOAD_Quiet | LOAD_NoWarn))
	{
		UGameplayStatics::PlaySoundAtLocation(this, S, At, Volume);
	}
}

void AWEToolHand::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Bench.IsValid())
	{
		return;
	}
	const float Time = GetWorld()->GetTimeSeconds();
	const int32 Wanted = Visuals.IsValidIndex(State.Selected) && Visuals[State.Selected].Mesh ? State.Selected : 0;
	if (Wanted != HeldIndex)
	{
		// Put the old one down, pick the new one up.
		if (Visuals.IsValidIndex(HeldIndex) && Visuals[HeldIndex].Mesh)
		{
			PlaySound(TEXT("SFX_K_beltHandle1"), Visuals[HeldIndex].Location, 0.5f);
		}
		if (Visuals.IsValidIndex(Wanted) && Visuals[Wanted].Mesh)
		{
			PlaySound(TEXT("SFX_K_drawKnife1"), Visuals[Wanted].Location, 0.35f);
		}
		HeldIndex = Wanted;
	}

	// Spin: the screwdriver follows the screw (three turns over the task), the drill whirs.
	const bool bDrillWorking = State.bWorking && bDrill && Visuals.IsValidIndex(HeldIndex) && Visuals[HeldIndex].Tool == EWEToolType::Screwdriver;
	if (State.bWorking)
	{
		SpinAngle = bDrillWorking ? SpinAngle + DeltaSeconds * 2600.f : State.Progress * 1080.f;
	}
	if (bDrillWorking && !DrillAudio)
	{
		if (USoundBase* Motor = LoadObject<USoundBase>(nullptr, TEXT("/Game/WorkshopEmpire/Audio/SFX_MotorLoop.SFX_MotorLoop"), nullptr, LOAD_Quiet | LOAD_NoWarn))
		{
			DrillAudio = UGameplayStatics::SpawnSoundAtLocation(this, Motor, State.WorkPoint, FRotator::ZeroRotator, 0.4f, 2.3f, 0.f, nullptr, nullptr, false);
		}
	}
	else if (!bDrillWorking && DrillAudio)
	{
		DrillAudio->FadeOut(0.15f, 0.f);
		DrillAudio = nullptr;
	}

	for (int32 i = 0; i < Visuals.Num(); ++i)
	{
		FToolVis& Vis = Visuals[i];
		if (!Vis.Mesh)
		{
			continue;
		}
		const bool bHeld = i == HeldIndex && i > 0;
		const FTransform Target = bHeld && (State.bHover || State.bWorking || State.bCarrying) ? HandTransform(Vis, Time)
			: bHeld ? FTransform(RestTransform(i - 1, Vis).GetRotation(), RestTransform(i - 1, Vis).GetLocation() + FVector(0.f, 0.f, 9.f))
			: RestTransform(i - 1, Vis);
		if (!Vis.bPlaced)
		{
			Vis.Location = Target.GetLocation();
			Vis.Rotation = Target.GetRotation();
			Vis.bPlaced = true;
		}
		else
		{
			// Working: the tip stays exactly on the screw; otherwise it glides.
			const float Move = bHeld && State.bWorking ? 30.f : WEToolHandConst::MoveSpeed;
			Vis.Location = FMath::VInterpTo(Vis.Location, Target.GetLocation(), DeltaSeconds, Move);
			Vis.Rotation = FQuat::Slerp(Vis.Rotation, Target.GetRotation(), FMath::Clamp(DeltaSeconds * WEToolHandConst::TurnSpeed, 0.f, 1.f));
		}
		Vis.Mesh->SetWorldLocationAndRotation(Vis.Location, Vis.Rotation);
	}

	// Phase 21: bare hands with the syringe (new thermal paste) or a brush (old paste, wiping).
	const bool bHands = HeldIndex == 0 && State.bWorking;
	for (UStaticMeshComponent* P : { Syringe.Get(), Brush.Get() })
	{
		if (!P) { continue; }
		const bool bShow = bHands && ((P == Syringe && State.WorkKind == EWEWorkTaskKind::PasteApply) || (P == Brush && State.WorkKind == EWEWorkTaskKind::Wipe));
		P->SetVisibility(bShow);
		if (bShow)
		{
			FVector ToView = State.ViewLocation - State.WorkPoint;
			ToView.Z = 0.f;
			ToView = ToView.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
			const FVector Dir = (FVector::UpVector - ToView * 0.35f).GetSafeNormal();
			// The brush scrubs back and forth, the syringe stays on the spot and presses.
			const FVector Scrub = P == Brush ? FVector::CrossProduct(FVector::UpVector, ToView) * 1.5f * FMath::Sin(Time * 14.f) : FVector::ZeroVector;
			P->SetWorldLocationAndRotation(State.WorkPoint + FVector(0.f, 0.f, 0.3f) + Scrub, FRotationMatrix::MakeFromZX(Dir, -ToView).Rotator());
		}
	}

	// Soldering: a wisp of smoke at the tip.
	if (State.bWorking && Visuals.IsValidIndex(HeldIndex) && Visuals[HeldIndex].Tool == EWEToolType::SolderingIron)
	{
		SmokeTimer -= DeltaSeconds;
		if (SmokeTimer <= 0.f)
		{
			SmokeTimer = 0.35f;
			AWEFx::Burst(this, EWEFxKind::Smoke, State.WorkPoint, FVector::UpVector, FLinearColor::White, 0.12f);
		}
	}
	// Hammer: a knock at every tap.
	if (State.bWorking && Visuals.IsValidIndex(HeldIndex) && Visuals[HeldIndex].Tool == EWEToolType::Hammer)
	{
		const float Phase = FMath::Fmod(Time * 16.f, PI);
		if (Phase < LastTap)
		{
			if (USoundBase* Knock = LoadObject<USoundBase>(nullptr, TEXT("/Game/WorkshopEmpire/Audio/Kenney/impactMetal_light_000.impactMetal_light_000"), nullptr, LOAD_Quiet | LOAD_NoWarn))
			{
				UGameplayStatics::PlaySoundAtLocation(this, Knock, State.WorkPoint, 0.35f);
			}
		}
		LastTap = Phase;
	}
}

FVector AWEToolHand::GetTipLocation() const
{
	return Visuals.IsValidIndex(HeldIndex) && Visuals[HeldIndex].Mesh ? Visuals[HeldIndex].Location : GetActorLocation();
}

void AWEToolHand::Destroyed()
{
	if (DrillAudio)
	{
		DrillAudio->Stop();
		DrillAudio = nullptr;
	}
	Super::Destroyed();
}
