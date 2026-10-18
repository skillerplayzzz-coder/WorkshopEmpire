#include "Player/WEAvatarPreview.h"

#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Player/WEAvatarComponent.h"
#include "Player/WEMannequin.h"

namespace WEAvatarPreviewConst
{
	// Whole figure, a little off centre (the menu card covers the right part of the screen).
	const FVector FarEye(560.f, -30.f, 110.f);
	const FVector FarLook(0.f, -30.f, 92.f);
	const FVector NearEye(150.f, -14.f, 162.f);
	const FVector NearLook(0.f, -14.f, 158.f);
}

AWEAvatarPreview::AWEAvatarPreview()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	Turntable = CreateDefaultSubobject<USceneComponent>(TEXT("Turntable"));
	Turntable->SetupAttachment(GetRootComponent());
	Driver = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Driver"));
	Driver->SetupAttachment(Turntable);
	Avatar = CreateDefaultSubobject<UWEAvatarComponent>(TEXT("Avatar"));
	Avatar->SetupAttachment(Turntable);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetRootComponent());
	Camera->SetFieldOfView(40.f);
	Camera->PostProcessSettings.bOverride_MotionBlurAmount = true;
	Camera->PostProcessSettings.MotionBlurAmount = 0.f;
	Camera->PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;
	Camera->PostProcessSettings.DepthOfFieldFocalDistance = 320.f;
	KeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(GetRootComponent());
	KeyLight->SetRelativeLocation(FVector(180.f, -140.f, 220.f));
	KeyLight->SetIntensityUnits(ELightUnits::Candelas);
	KeyLight->SetIntensity(60.f);
	KeyLight->SetAttenuationRadius(800.f);
	KeyLight->SetLightColor(FLinearColor(1.f, 0.95f, 0.88f));
	FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(GetRootComponent());
	FillLight->SetRelativeLocation(FVector(150.f, 200.f, 140.f));
	FillLight->SetIntensityUnits(ELightUnits::Candelas);
	FillLight->SetIntensity(22.f);
	FillLight->SetAttenuationRadius(800.f);
	FillLight->SetCastShadows(false);
	FillLight->SetLightColor(FLinearColor(0.8f, 0.88f, 1.f));
}

void AWEAvatarPreview::BeginPlay()
{
	Super::BeginPlay();
	// The mannequin with the idle animation drives the avatar; it faces the camera (+X of this actor).
	WEMannequin::Configure(Driver, 0.f);
	Driver->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	WEMannequin::ApplyBody(Driver, false);
	Avatar->SetDriver(Driver);
	Avatar->SetOwnerNoSee(false);
	SetCloseUp(false);
	CameraBlend = 0.f;
}

void AWEAvatarPreview::SetLook(const FWECharacterLook& Look)
{
	WEMannequin::ApplyBody(Driver, Look.Body == 1);
	Avatar->ApplyLook(Look);
}

void AWEAvatarPreview::Turn(float Degrees)
{
	Yaw = FRotator::NormalizeAxis(Yaw + Degrees);
}

void AWEAvatarPreview::SetYaw(float Degrees)
{
	Yaw = FRotator::NormalizeAxis(Degrees);
	ShownYaw = Yaw;
}

void AWEAvatarPreview::SetCloseUp(bool bClose)
{
	bCloseUp = bClose;
}

void AWEAvatarPreview::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	using namespace WEAvatarPreviewConst;
	// Smooth turning and a smooth camera move between the whole figure and the face.
	ShownYaw = FMath::FInterpTo(ShownYaw, ShownYaw + FRotator::NormalizeAxis(Yaw - ShownYaw), DeltaSeconds, 8.f);
	Turntable->SetRelativeRotation(FRotator(0.f, ShownYaw, 0.f));
	CameraBlend = FMath::FInterpTo(CameraBlend, bCloseUp ? 1.f : 0.f, DeltaSeconds, 5.f);
	const FVector Eye = FMath::Lerp(FarEye, NearEye, CameraBlend);
	const FVector Look = FMath::Lerp(FarLook, NearLook, CameraBlend);
	Camera->SetRelativeLocationAndRotation(Eye, (Look - Eye).Rotation());
	Camera->PostProcessSettings.DepthOfFieldFocalDistance = (Look - Eye).Size();
}
