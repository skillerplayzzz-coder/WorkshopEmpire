#include "World/WEMirror.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Framework/WEPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Player/WECharacter.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "WEMirror"

static TAutoConsoleVariable<int32> CVarMirrorProjection(TEXT("we.MirrorProjection"), 1, TEXT("Mirror: 1 = off-centre projection through the glass, 0 = plain view (debug)."));

namespace WEMirrorData
{
	const TCHAR* GlassMaterialPath = TEXT("/Game/WorkshopEmpire/Materials/M_WEMirror.M_WEMirror");
	constexpr float FrameDepth = 4.f;
	constexpr float FrameBorder = 5.f;
	constexpr float ViewDistance = 700.f;
}

AWEMirror::AWEMirror()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.f;
	bReplicates = false; // map actor, the same on every machine

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Frame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame"));
	Frame->SetupAttachment(Root);
	Frame->SetCollisionProfileName(TEXT("BlockAll"));
	Glass = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Glass"));
	Glass->SetupAttachment(Root);
	Glass->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Glass->SetCollisionResponseToAllChannels(ECR_Block);
	Glass->CastShadow = false;
	if (CubeMesh.Succeeded())
	{
		Frame->SetStaticMesh(CubeMesh.Object);
		Glass->SetStaticMesh(CubeMesh.Object);
	}

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	Capture->SetupAttachment(Root);
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->bAlwaysPersistRenderingState = true;
	// The finished picture (colour grading of the room included); the material undoes the viewer's exposure.
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->FOVAngle = 52.f;
	// A small picture: no motion blur, bloom or lens flares needed.
	Capture->ShowFlags.SetMotionBlur(false);
	Capture->ShowFlags.SetBloom(false);
	Capture->ShowFlags.SetLensFlares(false);
}

void AWEMirror::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	using namespace WEMirrorData;
	// Actor origin = bottom centre at the wall; the glass faces the actor's +X.
	const float W = GlassSize.X + 2.f * FrameBorder;
	const float H = GlassSize.Y + 2.f * FrameBorder;
	Frame->SetRelativeLocation(FVector(FrameDepth * 0.5f, 0.f, H * 0.5f));
	Frame->SetRelativeScale3D(FVector(FrameDepth, W, H) / 100.f);
	Glass->SetRelativeLocation(FVector(FrameDepth + 0.2f, 0.f, H * 0.5f));
	Glass->SetRelativeScale3D(FVector(0.4f, GlassSize.X, GlassSize.Y) / 100.f);
	Capture->SetRelativeLocation(FVector(FrameDepth + 3.f, 0.f, H * 0.5f));
	Capture->SetRelativeRotation(FRotator::ZeroRotator);
}

void AWEMirror::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
		return;
	}
	Target = NewObject<UTextureRenderTarget2D>(this);
	Target->RenderTargetFormat = RTF_RGBA8_SRGB;
	Target->ClearColor = FLinearColor::Black;
	const int32 Width = 384;
	Target->InitAutoFormat(Width, FMath::RoundToInt(Width * GlassSize.Y / FMath::Max(1.f, GlassSize.X)));
	Target->UpdateResourceImmediate(true);
	Capture->TextureTarget = Target;
	// The capture looks through its own glass from behind: never draw the mirror into its own picture.
	Capture->HiddenComponents.Add(Glass);
	Capture->HiddenComponents.Add(Frame);
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, WEMirrorData::GlassMaterialPath))
	{
		GlassMaterial = UMaterialInstanceDynamic::Create(Base, this);
		GlassMaterial->SetTextureParameterValue(TEXT("Capture"), Target);
		Glass->SetMaterial(0, GlassMaterial);
	}
	if (UMaterialInterface* FrameMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/WorkshopEmpire/Materials/Props/MI_Prop_TrimBlack.MI_Prop_TrimBlack")))
	{
		Frame->SetMaterial(0, FrameMat);
	}
}

void AWEMirror::CaptureNow()
{
	if (!Capture || !Target)
	{
		return;
	}
	// 0.19.1: a real mirror. The capture sits at the viewer's eye mirrored behind the glass and looks through the glass
	// rectangle (off-centre projection); its near plane lies just in front of the glass, so the wall behind is cut away.
	// What it sees changes with where the player stands, like in a real mirror.
	const FVector N = GetActorForwardVector();
	const FVector Up = GetActorUpVector();
	const FVector Center = Glass->GetComponentLocation() + N * 0.2f;
	FVector Eye = Center + N * 150.f;
	FRotator ViewRot;
	if (const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->GetPlayerViewPoint(Eye, ViewRot);
	}
	const float Dist = FMath::Max(FVector::DotProduct(Eye - Center, N), 10.f);
	const FVector Mirrored = Eye - N * (FVector::DotProduct(Eye - Center, N) + Dist);
	Capture->SetWorldLocationAndRotation(Mirrored, FRotationMatrix::MakeFromXZ(N, Up).Rotator());
	const FVector Right = Capture->GetRightVector();
	const FVector Rel = Center - Mirrored;
	const float Cx = FVector::DotProduct(Rel, Right);
	const float Cy = FVector::DotProduct(Rel, Up);
	const float Near = FMath::Max(Dist - 2.f, 1.f);
	const float S = Near / Dist;
	const float L = (Cx - GlassSize.X * 0.5f) * S, R = (Cx + GlassSize.X * 0.5f) * S;
	const float B = (Cy - GlassSize.Y * 0.5f) * S, T = (Cy + GlassSize.Y * 0.5f) * S;
	// Reversed-Z projection with an infinite far plane (like the engine's), shifted onto the glass.
	Capture->bUseCustomProjectionMatrix = CVarMirrorProjection.GetValueOnGameThread() != 0;
	Capture->CustomProjectionMatrix = FMatrix(
		FPlane(2.f * Near / (R - L), 0.f, 0.f, 0.f),
		FPlane(0.f, 2.f * Near / (T - B), 0.f, 0.f),
		FPlane(-(R + L) / (R - L), -(T + B) / (T - B), 0.f, 1.f),
		FPlane(0.f, 0.f, Near, 0.f));
	// Where the glass is in the world: the material lays the picture onto it (it flips left/right for the old
	// fixed view - the opposite "right" undoes that, the reflection itself already is mirrored).
	if (GlassMaterial)
	{
		GlassMaterial->SetVectorParameterValue(TEXT("Center"), FLinearColor(Center));
		GlassMaterial->SetVectorParameterValue(TEXT("Right"), FLinearColor(-Right));
		GlassMaterial->SetVectorParameterValue(TEXT("Up"), FLinearColor(Up));
		GlassMaterial->SetVectorParameterValue(TEXT("Size"), FLinearColor(GlassSize.X, GlassSize.Y, 0.f));
	}
	Capture->CaptureScene();
	++CaptureCount;
}

void AWEMirror::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	CaptureTimer += DeltaSeconds;
	if (CaptureTimer < 1.f / 20.f)
	{
		return;
	}
	CaptureTimer = 0.f;
	// Only while the local player is close, in front of the glass and looking roughly at it.
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->IsLocalController())
	{
		return;
	}
	FVector Eye;
	FRotator View;
	PC->GetPlayerViewPoint(Eye, View);
	const FVector Center = Glass->GetComponentLocation();
	const FVector ToGlass = Center - Eye;
	const bool bNear = ToGlass.SizeSquared() < FMath::Square(WEMirrorData::ViewDistance);
	const bool bInFront = FVector::DotProduct(-ToGlass, GetActorForwardVector()) > 0.f;
	const bool bLooking = FVector::DotProduct(ToGlass.GetSafeNormal(), View.Vector()) > 0.35f;
	if (bNear && bInFront && bLooking)
	{
		CaptureNow();
	}
}

FText AWEMirror::GetInteractionText(const AWECharacter* Interactor) const
{
	return LOCTEXT("Use", "Aussehen ändern");
}

bool AWEMirror::CanInteract(const AWECharacter* Interactor, FText& OutReason) const
{
	return Interactor != nullptr;
}

void AWEMirror::Interact(AWECharacter* Interactor)
{
	if (AWEPlayerController* PC = Interactor ? Cast<AWEPlayerController>(Interactor->GetController()) : nullptr)
	{
		PC->ClientOpenMirror();
	}
}

bool AWEMirror::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	OutInfo.Title = LOCTEXT("Title", "Spiegel");
	OutInfo.Description = LOCTEXT("Desc", "Hier siehst du dich selbst. Mit [E] änderst du Kleidung, Gesicht und Frisur – deine Mitspieler sehen es sofort.");
	return true;
}

#undef LOCTEXT_NAMESPACE
