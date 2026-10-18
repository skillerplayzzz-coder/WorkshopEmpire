#include "Dev/WEIconBaker.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "World/WEPlaceholderProp.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "TextureResource.h"
#include "TimerManager.h"
#include "WELog.h"

namespace WEIconBake
{
	constexpr int32 RenderSize = 512;
	constexpr int32 IconSize = 256;
	const FVector Studio(0.f, 0.f, 60000.f);
	float ExposureBias = -4.5f;
	float FixedExposure = 0.0011f;
}

AWEIconBaker::AWEIconBaker()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	ColorCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("ColorCapture"));
	ColorCapture->SetupAttachment(RootComponent);
	MaskCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MaskCapture"));
	MaskCapture->SetupAttachment(RootComponent);
	KeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(RootComponent);
	FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(RootComponent);
}

bool AWEIconBaker::IsRequested()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("WEBakeIcons"));
#endif
}

void AWEIconBaker::BeginPlay()
{
	Super::BeginPlay();
	SetActorLocation(WEIconBake::Studio);
	FParse::Value(FCommandLine::Get(), TEXT("WEBakeExposure="), WEIconBake::ExposureBias);
	FParse::Value(FCommandLine::Get(), TEXT("WEBakeFixed="), WEIconBake::FixedExposure);

	auto MakeTarget = [this](ETextureRenderTargetFormat Format)
	{
		UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(this);
		RT->RenderTargetFormat = Format;
		RT->ClearColor = FLinearColor(0.f, 0.f, 0.f, 1.f);
		RT->bAutoGenerateMips = false;
		RT->InitAutoFormat(WEIconBake::RenderSize, WEIconBake::RenderSize);
		RT->UpdateResourceImmediate(true);
		return RT;
	};
	ColorTarget = MakeTarget(RTF_RGBA8_SRGB);
	MaskTarget = MakeTarget(RTF_RGBA16f);
	for (USceneCaptureComponent2D* Capture : { ColorCapture.Get(), MaskCapture.Get() })
	{
		Capture->bCaptureEveryFrame = false;
		Capture->bCaptureOnMovement = false;
		Capture->FOVAngle = 24.f;
		Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
		Capture->ShowFlags.SetTemporalAA(false);
		Capture->ShowFlags.SetMotionBlur(false);
		Capture->ShowFlags.SetFog(false);
		Capture->ShowFlags.SetVolumetricFog(false);
		Capture->ShowFlags.SetAtmosphere(false);
		Capture->ShowFlags.SetCloud(false);
		Capture->ShowFlags.SetBloom(false);
		Capture->ShowFlags.SetEyeAdaptation(true);
		Capture->ShowFlags.SetDirectionalLights(false); // studio light only: the sun at noon blows out one side
	}
	ColorCapture->TextureTarget = ColorTarget;
	ColorCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	// Fixed exposure: every icon gets the same brightness (no eye adaptation per item).
	FPostProcessSettings& PP = ColorCapture->PostProcessSettings;
	PP.bOverride_AutoExposureMethod = true;
	PP.AutoExposureMethod = AEM_Manual;
	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = WEIconBake::ExposureBias;
	PP.bOverride_AutoExposureMinBrightness = true;
	PP.bOverride_AutoExposureMaxBrightness = true;
	PP.AutoExposureMinBrightness = 13.5f;
	PP.AutoExposureMaxBrightness = 13.5f;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.f;
	PP.bOverride_FilmGrainIntensity = true;
	PP.FilmGrainIntensity = 0.f;
	ColorCapture->PostProcessBlendWeight = 1.f;
	MaskCapture->TextureTarget = MaskTarget;
	MaskCapture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;

	for (UPointLightComponent* Light : { KeyLight.Get(), FillLight.Get() })
	{
		Light->SetIntensityUnits(ELightUnits::Lumens);
		Light->SetAttenuationRadius(3000.f);
		Light->SetCastShadows(false);
		Light->SetMobility(EComponentMobility::Movable);
	}
	KeyLight->SetIntensity(30000.f);
	KeyLight->SetLightColor(FLinearColor(1.f, 0.95f, 0.88f));
	FillLight->SetIntensity(14000.f);
	FillLight->SetLightColor(FLinearColor(0.85f, 0.92f, 1.f));

	// Every item definition in the project.
	TArray<FAssetData> Assets;
	FAssetRegistryModule::GetRegistry().ScanPathsSynchronous({ TEXT("/Game/WorkshopEmpire/Data") }, true);
	FAssetRegistryModule::GetRegistry().GetAssetsByClass(UWEItemDefinition::StaticClass()->GetClassPathName(), Assets);
	for (const FAssetData& Asset : Assets)
	{
		if (UWEItemDefinition* Def = Cast<UWEItemDefinition>(Asset.GetAsset()))
		{
			Queue.Add(Def);
		}
	}
	if (const UWEFurnitureCatalog* Furniture = LoadObject<UWEFurnitureCatalog>(nullptr, TEXT("/Game/WorkshopEmpire/Data/Workshop/DA_FurnitureCatalog.DA_FurnitureCatalog")))
	{
		FurnQueue = Furniture->Items;
	}
	FString Only;
	if (FParse::Value(FCommandLine::Get(), TEXT("WEBakeOnly="), Only))
	{
		Queue.RemoveAll([&Only](const UWEItemDefinition* D) { return !Only.Contains(D->ItemId.ToString()); });
		FurnQueue.RemoveAll([&Only](const FWEFurnitureDefinition& D) { return !Only.Contains(D.Id.ToString()); });
	}
	FString Meshes;
	if (FParse::Value(FCommandLine::Get(), TEXT("WEBakeMeshes="), Meshes, false))
	{
		bMeshMode = true;
		Queue.Reset();
		TArray<FString> Paths;
		Meshes.ParseIntoArray(Paths, TEXT(","));
		for (const FString& Path : Paths)
		{
			const FString Full = Path.Contains(TEXT("/")) ? Path : FString::Printf(TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_%s.SM_Gen_%s"), *Path, *Path);
			if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Full))
			{
				MeshQueue.Add(Mesh);
			}
		}
		IFileManager::Get().MakeDirectory(*FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MeshPreviews")), true);
	}
	UE_LOG(LogWorkshopEmpire, Display, TEXT("ICONBAKE start: %d item definitions"), Queue.Num());
	IFileManager::Get().MakeDirectory(*FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ItemIcons")), true);
	GetWorldTimerManager().SetTimer(StepTimer, this, &AWEIconBaker::Next, 1.0f, false);
}

void AWEIconBaker::Next()
{
	if (Current)
	{
		Current->Destroy();
		Current = nullptr;
	}
	++Index;
	if (bMeshMode ? !MeshQueue.IsValidIndex(Index) : !Queue.IsValidIndex(Index) && !FurnQueue.IsValidIndex(Index - Queue.Num()))
	{
		UE_LOG(LogWorkshopEmpire, Display, TEXT("ICONBAKE done: %d icons written"), Written);
		UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
		return;
	}
	if (bMeshMode)
	{
		AActor* Holder = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform(WEIconBake::Studio));
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Holder, TEXT("Mesh"));
		Comp->SetStaticMesh(MeshQueue[Index]);
		Comp->SetMobility(EComponentMobility::Movable);
		Holder->SetRootComponent(Comp);
		Comp->RegisterComponent();
		Holder->SetActorLocation(WEIconBake::Studio);
		Current = Holder;
	}
	else if (Queue.IsValidIndex(Index))
	{
		Current = AWEPickupItem::SpawnWorldItem(GetWorld(), FWEItemInstance::Make(Queue[Index], 1, EWEItemCondition::Used, 60), FTransform(WEIconBake::Studio));
	}
	else
	{
		// Furniture: the same look the placed piece has (model or block-out boxes), seen from the front-left.
		AWEPlaceholderProp* Prop = GetWorld()->SpawnActor<AWEPlaceholderProp>(AWEPlaceholderProp::StaticClass(), FTransform(FRotator(0.f, 180.f, 0.f), WEIconBake::Studio));
		if (Prop)
		{
			Prop->ApplyFurniture(FurnQueue[Index - Queue.Num()]);
			Prop->SetActorEnableCollision(false);
		}
		Current = Prop;
	}
	if (!Current)
	{
		Next();
		return;
	}
	if (UStaticMeshComponent* MeshComp = Current->FindComponentByClass<UStaticMeshComponent>())
	{
		MeshComp->SetSimulatePhysics(false);
		MeshComp->SetEnableGravity(false);
	}
	// Frame the item: view from front-left and a little above, fitted to its bounds.
	FVector Origin, Extent;
	Current->GetActorBounds(false, Origin, Extent);
	const float Radius = FMath::Max(Extent.Size(), 2.f);
	const float Distance = Radius / FMath::Tan(FMath::DegreesToRadians(ColorCapture->FOVAngle * 0.5f)) * 1.08f;
	const FVector Dir = FVector(1.f, -0.8f, 0.62f).GetSafeNormal();
	const FVector Eye = Origin + Dir * Distance;
	const FRotator Look = (Origin - Eye).Rotation();
	for (USceneCaptureComponent2D* Capture : { ColorCapture.Get(), MaskCapture.Get() })
	{
		Capture->SetWorldLocationAndRotation(Eye, Look);
		Capture->ShowOnlyActors.Reset();
		Capture->ShowOnlyActors.Add(Current);
	}
	// Light intensity grows with the square of the distance: every object gets the same illuminance, so one fixed exposure fits all.
	const float LightScale = FMath::Square(Distance / 100.f);
	KeyLight->SetIntensity(30000.f * LightScale);
	FillLight->SetIntensity(14000.f * LightScale);
	KeyLight->SetAttenuationRadius(Distance * 4.f);
	FillLight->SetAttenuationRadius(Distance * 4.f);
	KeyLight->SetWorldLocation(Origin + FVector(0.6f, -1.f, 1.2f).GetSafeNormal() * Distance * 0.8f);
	FillLight->SetWorldLocation(Origin + FVector(0.9f, 1.f, 0.2f).GetSafeNormal() * Distance * 0.9f);
	// Give streaming and the mesh a moment, then capture.
	GetWorldTimerManager().SetTimer(StepTimer, this, &AWEIconBaker::Capture, 0.35f, false);
}

void AWEIconBaker::Capture()
{
	ColorCapture->CaptureScene();
	MaskCapture->CaptureScene();
	if (bMeshMode && MeshQueue.IsValidIndex(Index))
	{
		SaveIcon(TEXT("MESH:") + MeshQueue[Index]->GetName());
	}
	else if (Queue.IsValidIndex(Index))
	{
		SaveIcon(Queue[Index]->ItemId.ToString());
	}
	else if (FurnQueue.IsValidIndex(Index - Queue.Num()))
	{
		SaveIcon(TEXT("FURN:") + FurnQueue[Index - Queue.Num()].Id.ToString());
	}
	GetWorldTimerManager().SetTimer(StepTimer, this, &AWEIconBaker::Next, 0.05f, false);
}

void AWEIconBaker::SaveIcon(const FString& ItemId)
{
	FTextureRenderTargetResource* ColorRes = ColorTarget->GameThread_GetRenderTargetResource();
	FTextureRenderTargetResource* MaskRes = MaskTarget->GameThread_GetRenderTargetResource();
	TArray<FColor> Color;
	TArray<FLinearColor> Mask;
	if (!ColorRes || !MaskRes || !ColorRes->ReadPixels(Color) || !MaskRes->ReadLinearColorPixels(Mask) || Color.Num() != Mask.Num())
	{
		UE_LOG(LogWorkshopEmpire, Warning, TEXT("ICONBAKE %s: read failed"), *ItemId);
		return;
	}
	// Colour from the HDR capture with our own exposure (the LDR capture ignores exposure settings): the
	// average brightness of the object is brought to a fixed level, then a filmic curve and sRGB.
	const int32 N = WEIconBake::RenderSize;
	const int32 M = WEIconBake::IconSize;
	double LogSum = 0.0;
	int32 Count = 0;
	for (const FLinearColor& C : Mask)
	{
		if (1.f - C.A > 0.5f)
		{
			LogSum += FMath::Loge(FMath::Max(0.0001f, C.GetLuminance()));
			++Count;
		}
	}
	const float AvgLum = Count > 0 ? FMath::Exp(LogSum / Count) : 1.f;
	const float Exposure = WEIconBake::FixedExposure * FMath::Pow(2.f, WEIconBake::ExposureBias + 4.5f);
	UE_LOG(LogWorkshopEmpire, Display, TEXT("ICONBAKE %s avg luminance %.4f"), *ItemId, AvgLum);
	auto Tone = [Exposure](float V)
	{
		V *= Exposure;
		const float A = 2.51f, B = 0.03f, C = 2.43f, D = 0.59f, E = 0.14f; // ACES approximation
		const float Mapped = FMath::Clamp((V * (A * V + B)) / (V * (C * V + D) + E), 0.f, 1.f);
		return FMath::Clamp(FMath::Pow(Mapped, 1.f / 2.2f) * 255.f, 0.f, 255.f);
	};
	// Crop to the object's silhouette (a square around it with a small margin), so flat or long items fill the
	// icon as well as boxy ones; the bounding-sphere framing alone leaves them tiny.
	int32 MinX = N, MinY = N, MaxX = -1, MaxY = -1;
	for (int32 y = 0; y < N; ++y)
	{
		for (int32 x = 0; x < N; ++x)
		{
			if (1.f - Mask[y * N + x].A > 0.1f)
			{
				MinX = FMath::Min(MinX, x); MaxX = FMath::Max(MaxX, x);
				MinY = FMath::Min(MinY, y); MaxY = FMath::Max(MaxY, y);
			}
		}
	}
	float CropSize = static_cast<float>(N);
	float CropX = 0.f, CropY = 0.f;
	if (MaxX >= MinX && MaxY >= MinY)
	{
		CropSize = FMath::Clamp(FMath::Max(MaxX - MinX + 1, MaxY - MinY + 1) * 1.12f, 48.f, static_cast<float>(N));
		CropX = FMath::Clamp((MinX + MaxX + 1) * 0.5f - CropSize * 0.5f, 0.f, N - CropSize);
		CropY = FMath::Clamp((MinY + MaxY + 1) * 0.5f - CropSize * 0.5f, 0.f, N - CropSize);
	}
	const float Step = CropSize / M;
	TArray<FColor> Out;
	Out.SetNumUninitialized(M * M);
	for (int32 y = 0; y < M; ++y)
	{
		for (int32 x = 0; x < M; ++x)
		{
			FLinearColor Sum(0.f, 0.f, 0.f, 0.f);
			float AlphaSum = 0.f;
			int32 Samples = 0;
			// 3x3 sub-samples per output pixel (box filter over the cropped area).
			for (int32 dy = 0; dy < 3; ++dy)
			{
				for (int32 dx = 0; dx < 3; ++dx)
				{
					const int32 Sx = FMath::Clamp(FMath::FloorToInt(CropX + (x + (dx + 0.5f) / 3.f) * Step), 0, N - 1);
					const int32 Sy = FMath::Clamp(FMath::FloorToInt(CropY + (y + (dy + 0.5f) / 3.f) * Step), 0, N - 1);
					const FLinearColor& Hdr = Mask[Sy * N + Sx];
					Sum += FLinearColor(Hdr.R, Hdr.G, Hdr.B, 0.f);
					AlphaSum += FMath::Clamp(1.f - Hdr.A, 0.f, 1.f);
					++Samples;
				}
			}
			FColor& P = Out[y * M + x];
			if (AlphaSum > 0.001f)
			{
				// HDR scene colour is not premultiplied by coverage here: average and un-blend from the black background.
				const FLinearColor Avg = Sum / AlphaSum;
				P = FColor(static_cast<uint8>(Tone(Avg.R)), static_cast<uint8>(Tone(Avg.G)), static_cast<uint8>(Tone(Avg.B)),
					static_cast<uint8>(FMath::Clamp(AlphaSum / Samples * 255.f, 0.f, 255.f)));
			}
			else
			{
				P = FColor(0, 0, 0, 0);
			}
		}
	}
	IImageWrapperModule& Wrapper = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> Png = Wrapper.CreateImageWrapper(EImageFormat::PNG);
	if (Png && Png->SetRaw(Out.GetData(), Out.Num() * sizeof(FColor), M, M, ERGBFormat::BGRA, 8))
	{
		const FString Path = ItemId.StartsWith(TEXT("MESH:"))
			? FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MeshPreviews"), ItemId.RightChop(5) + TEXT(".png"))
			: ItemId.StartsWith(TEXT("FURN:")) ? FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ItemIcons"), FString::Printf(TEXT("T_Furn_%s.png"), *ItemId.RightChop(5)))
			: FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ItemIcons"), FString::Printf(TEXT("T_Item_%s.png"), *ItemId));
		if (FFileHelper::SaveArrayToFile(Png->GetCompressed(), *Path))
		{
			++Written;
			UE_LOG(LogWorkshopEmpire, Display, TEXT("ICONBAKE %s"), *Path);
		}
	}
}
