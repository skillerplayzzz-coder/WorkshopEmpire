#include "World/WEDayNightComponent.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Light.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "Framework/WEGameState.h"
#include "Net/UnrealNetwork.h"
#include "Dev/WEDevAutomationComponent.h"
#include "World/WECustomer.h"
#include "Components/AudioComponent.h"
#include "Fx/WEFx.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundBase.h"

#define LOCTEXT_NAMESPACE "WEDayNight"

static TAutoConsoleVariable<float> CVarWeatherCoverage(TEXT("we.WeatherCloudCoverage"), 0.75f, TEXT("Cloud_GlobalCoverage of the sky clouds at full overcast (rain)."));

UWEDayNightComponent::UWEDayNightComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.f;
	SetIsReplicatedByDefault(true);
}

void UWEDayNightComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWEDayNightComponent, Day);
	DOREPLIFETIME(UWEDayNightComponent, Minute);
	DOREPLIFETIME(UWEDayNightComponent, Weather);
}

UWEDayNightComponent* UWEDayNightComponent::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AWEGameState* GameState = World ? World->GetGameState<AWEGameState>() : nullptr;
	return GameState ? GameState->GetDayNight() : nullptr;
}

void UWEDayNightComponent::BeginPlay()
{
	Super::BeginPlay();
	LocalMinute = Minute;
	// Dev runs and screenshots: -WETime=21.5 starts at 21:30.
	float StartHour = -1.f;
	if (GetOwner()->HasAuthority() && FParse::Value(FCommandLine::Get(), TEXT("WETime="), StartHour) && StartHour >= 0.f)
	{
		SetTime(Day, StartHour * 60.f);
	}
	FString WeatherArg;
	if (GetOwner()->HasAuthority() && FParse::Value(FCommandLine::Get(), TEXT("WEWeather="), WeatherArg))
	{
		const EWEWeather Forced = WeatherArg == TEXT("Storm") ? EWEWeather::Storm : WeatherArg == TEXT("Rain") ? EWEWeather::Rain
			: WeatherArg == TEXT("Cloudy") ? EWEWeather::Cloudy : EWEWeather::Clear;
		SetWeather(Forced, true);
	}
	// First roll a few minutes in.
	WeatherTimer = 240.f;
	ApplyLighting();
}

void UWEDayNightComponent::OnRep_Clock()
{
	// Small drift is smoothed away, a jump (loading, dev) is taken over.
	const float Diff = FMath::Abs(Minute - LocalMinute);
	if (Diff > 3.f && Diff < 1437.f)
	{
		LocalMinute = Minute;
		ApplyLighting();
	}
}

void UWEDayNightComponent::SetTime(int32 InDay, float InMinute)
{
	Day = FMath::Max(1, InDay);
	Minute = FMath::Fmod(FMath::Max(0.f, InMinute), 1440.f);
	LocalMinute = Minute;
	GetOwner()->ForceNetUpdate();
	ApplyLighting();
}

void UWEDayNightComponent::Advance(float GameMinutes)
{
	LocalMinute += GameMinutes;
	while (LocalMinute >= 1440.f)
	{
		LocalMinute -= 1440.f;
		if (GetOwner()->HasAuthority())
		{
			++Day;
		}
	}
	if (GetOwner()->HasAuthority())
	{
		Minute = LocalMinute;
	}
}

void UWEDayNightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const float Rate = 1440.f / (DayLengthMinutes * 60.f); // game minutes per real second
	Advance(DeltaTime * Rate);
	if (GetOwner()->HasAuthority())
	{
		// Replicate the clock now and then (clients run it on their own in between).
		SyncTimer += DeltaTime;
		if (SyncTimer >= 5.f)
		{
			SyncTimer = 0.f;
			GetOwner()->ForceNetUpdate();
		}
		// Walk-in customers now and then during opening hours (not in scripted test runs unless asked for).
		CustomerTimer += DeltaTime;
		if (CustomerTimer >= 20.f)
		{
			CustomerTimer = 0.f;
			static const bool bScripted = UWEDevAutomationComponent::IsRequested() && !FParse::Param(FCommandLine::Get(), TEXT("WECustomers"));
			if (!bScripted && !FParse::Param(FCommandLine::Get(), TEXT("NoCustomers")) && FMath::FRand() < 0.35f)
			{
				WEWalkIn::TrySpawn(GetWorld());
			}
		}
		// Weather changes every 5-10 real minutes (a few game hours); forced weather (dev runs) stays.
		// Scripted test runs keep the weather fixed (clear unless -WEWeather= asks for one), so their pictures compare.
		static const bool bForcedWeather = FString(FCommandLine::Get()).Contains(TEXT("WEWeather=")) || UWEDevAutomationComponent::IsRequested();
		WeatherTimer -= DeltaTime;
		if (WeatherTimer <= 0.f)
		{
			WeatherTimer = FMath::FRandRange(300.f, 600.f);
			if (!bForcedWeather)
			{
				const float Roll = FMath::FRand();
				SetWeather(Roll < 0.48f ? EWEWeather::Clear : Roll < 0.73f ? EWEWeather::Cloudy : Roll < 0.92f ? EWEWeather::Rain : EWEWeather::Storm);
			}
		}
	}
	TickWeather(DeltaTime);
	LightTimer += DeltaTime;
	if (LightTimer >= 0.1f)
	{
		LightTimer = 0.f;
		ApplyLighting();
	}
}

float UWEDayNightComponent::GetDaylight() const
{
	// Sun up 6:00-20:00, highest at 13:00.
	const float Hour = LocalMinute / 60.f;
	const float Phase = (Hour - 6.f) / 14.f;
	return Phase <= 0.f || Phase >= 1.f ? 0.f : FMath::Sin(Phase * PI);
}

bool UWEDayNightComponent::IsOpen() const
{
	const float Hour = LocalMinute / 60.f;
	return Hour >= OpenHour && Hour < CloseHour;
}

FText UWEDayNightComponent::GetClockText() const
{
	const int32 Total = FMath::FloorToInt(LocalMinute);
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), (Total / 60) % 24, Total % 60));
}

FText UWEDayNightComponent::GetOpeningText() const
{
	return IsOpen() ? FText::Format(LOCTEXT("OpenUntil", "Geöffnet bis {0}:00"), FText::AsNumber(FMath::RoundToInt(CloseHour)))
		: FText::Format(LOCTEXT("ClosedUntil", "Geschlossen · öffnet {0}:00"), FText::AsNumber(FMath::RoundToInt(OpenHour)));
}

void UWEDayNightComponent::SetWeather(EWEWeather InWeather, bool bInstant)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	Weather = InWeather;
	bSnapWeather |= bInstant;
	GetOwner()->ForceNetUpdate();
	UE_LOG(LogTemp, Log, TEXT("WE_WEATHER %s"), *GetWeatherText().ToString());
}

int32 UWEDayNightComponent::GetRainParticleCount() const
{
	return RainFx ? RainFx->GetNumParticles() : 0;
}

void UWEDayNightComponent::OnRep_Weather()
{
	// Clients blend over like the host (nothing to snap: a client joining mid-rain starts with it).
}

FText UWEDayNightComponent::GetWeatherText() const
{
	switch (Weather)
	{
	case EWEWeather::Cloudy: return LOCTEXT("WeatherCloudy", "Bewölkt");
	case EWEWeather::Rain:   return LOCTEXT("WeatherRain", "Regen");
	case EWEWeather::Storm:  return LOCTEXT("WeatherStorm", "Gewitter");
	default:                 return LOCTEXT("WeatherClear", "Klar");
	}
}

namespace WEWeatherFx
{
	USoundBase* Sound(const TCHAR* Name)
	{
		return LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/WorkshopEmpire/Audio/P20/%s.%s"), Name, Name), nullptr, LOAD_Quiet | LOAD_NoWarn);
	}
}

void UWEDayNightComponent::TickWeather(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	const float CloudTarget = Weather == EWEWeather::Clear ? 0.f : Weather == EWEWeather::Cloudy ? 0.55f : Weather == EWEWeather::Rain ? 0.8f : 1.f;
	const float RainTarget = Weather == EWEWeather::Rain ? 0.8f : Weather == EWEWeather::Storm ? 1.f : 0.f;
	// A local first frame (or a forced dev weather) starts with it, else ~25 s to blend over.
	const bool bSnap = bSnapWeather || !HasBegunPlay() || GetWorld()->GetTimeSeconds() < 1.f;
	bSnapWeather = false;
	CloudBlend = bSnap ? CloudTarget : FMath::FInterpConstantTo(CloudBlend, CloudTarget, DeltaTime, 0.04f);
	RainBlend = bSnap ? RainTarget : FMath::FInterpConstantTo(RainBlend, RainTarget, DeltaTime, 0.04f);
	const float StormTarget = Weather == EWEWeather::Storm ? 1.f : 0.f;
	StormBlend = bSnap ? StormTarget : FMath::FInterpConstantTo(StormBlend, StormTarget, DeltaTime, 0.04f);

	const APlayerController* PC = World->GetFirstPlayerController();
	const APlayerCameraManager* Camera = PC ? PC->PlayerCameraManager.Get() : nullptr;
	if (!Camera)
	{
		return;
	}
	const FVector View = Camera->GetCameraLocation();
	IndoorTimer -= DeltaTime;
	if (IndoorTimer <= 0.f)
	{
		// Under a roof: something static above the view.
		IndoorTimer = 0.25f;
		FHitResult Hit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(WEIndoor), false);
		if (const APawn* Pawn = PC->GetPawn())
		{
			Query.AddIgnoredActor(Pawn);
		}
		bViewIndoors = World->LineTraceSingleByChannel(Hit, View, View + FVector(0.f, 0.f, 3000.f), ECC_Visibility, Query);
	}

	// Rain streaks around the view (drops spawn high up and die where they land - not inside buildings).
	if (RainBlend > 0.01f && !RainFx)
	{
		RainFx = AWEFx::Spawn(this, EWEFxKind::Rain, View + FVector(0.f, 0.f, 1500.f));
		if (RainFx)
		{
			RainFx->SetSpawnBox(FVector(900.f, 900.f, 100.f));
		}
	}
	if (RainFx)
	{
		RainFx->SetActorLocation(View + FVector(0.f, 0.f, 1500.f));
		RainFx->SetRateScale(RainBlend);
		if (RainBlend <= 0.01f && RainFx->GetNumParticles() == 0)
		{
			RainFx->Destroy();
			RainFx = nullptr;
		}
	}

	// Rain sound, muffled under a roof.
	const float VolumeTarget = RainBlend * (bViewIndoors ? 0.32f : 0.75f);
	RainVolume = FMath::FInterpTo(RainVolume, VolumeTarget, DeltaTime, 2.f);
	if (RainVolume > 0.01f && !RainAudio)
	{
		if (USoundBase* Loop = WEWeatherFx::Sound(TEXT("SFX_RainLoop")))
		{
			RainAudio = UGameplayStatics::SpawnSound2D(this, Loop, 0.01f, 1.f, 0.f, nullptr, true, false);
		}
	}
	if (RainAudio)
	{
		RainAudio->SetVolumeMultiplier(FMath::Max(RainVolume, 0.001f));
		RainAudio->SetLowPassFilterEnabled(bViewIndoors);
		RainAudio->SetLowPassFilterFrequency(1800.f);
		if (RainVolume <= 0.01f && VolumeTarget <= 0.f)
		{
			RainAudio->Stop();
			RainAudio->DestroyComponent();
			RainAudio = nullptr;
		}
	}

	// Thunderstorm: now and then a lightning flash (two flickers), thunder follows a moment later.
	if (Weather == EWEWeather::Storm && RainBlend > 0.5f)
	{
		LightningTimer -= DeltaTime;
		if (LightningTimer <= 0.f)
		{
			LightningTimer = FMath::FRandRange(10.f, 28.f);
			FlashAge = 0.f;
			ThunderDelay = FMath::FRandRange(0.5f, 2.2f);
			++LightningCount;
			if (!Lightning.IsValid())
			{
				FActorSpawnParameters Params;
				Params.ObjectFlags |= RF_Transient;
				if (ADirectionalLight* L = World->SpawnActor<ADirectionalLight>(FVector(0.f, 0.f, 1500.f), FRotator(-62.f, FMath::FRandRange(0.f, 360.f), 0.f), Params))
				{
					if (UDirectionalLightComponent* C = Cast<UDirectionalLightComponent>(L->GetLightComponent()))
					{
						C->SetMobility(EComponentMobility::Movable);
						C->SetAtmosphereSunLight(false);
						C->SetCastShadows(false);
						// The sun stays the light for translucency and fog (no "competing lights" warning).
						C->SetForwardShadingPriority(0);
						C->SetLightColor(FLinearColor(0.78f, 0.84f, 1.f));
						C->SetIntensity(0.f);
					}
					Lightning = L;
				}
			}
		}
	}
	if (ThunderDelay >= 0.f)
	{
		ThunderDelay -= DeltaTime;
		if (ThunderDelay < 0.f)
		{
			if (USoundBase* Thunder = WEWeatherFx::Sound(TEXT("SFX_Thunder")))
			{
				UGameplayStatics::PlaySound2D(this, Thunder, bViewIndoors ? 0.55f : 0.9f, FMath::FRandRange(0.9f, 1.05f));
			}
		}
	}
	if (ADirectionalLight* L = Lightning.Get())
	{
		float Flash = 0.f;
		if (FlashAge >= 0.f)
		{
			FlashAge += DeltaTime;
			// Flicker: bright - dark - brighter - fade.
			Flash = FlashAge < 0.07f ? 0.7f : FlashAge < 0.13f ? 0.1f : FlashAge < 0.2f ? 1.f : FMath::Max(0.f, 1.f - (FlashAge - 0.2f) / 0.35f);
			if (FlashAge > 0.6f)
			{
				FlashAge = -1.f;
			}
		}
		// Bright enough to light the yard even at noon under the storm clouds.
		L->GetLightComponent()->SetIntensity(Flash * FMath::Max(CurrentSunLux * 1.8f, 90.f));
		L->GetLightComponent()->SetVisibility(Flash > 0.f);
	}
}

void UWEDayNightComponent::ApplyLighting()
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (!bSunCached)
	{
		bSunCached = true;
		for (TActorIterator<ADirectionalLight> It(World); It; ++It)
		{
			if (UDirectionalLightComponent* C = Cast<UDirectionalLightComponent>(It->GetLightComponent()); C && C->IsUsedAsAtmosphereSunLight())
			{
				Sun = *It;
				SunDaylightLux = C->Intensity;
				// The sun leads translucency and fog lighting; moon (1) and lightning (0) come after it.
				C->SetForwardShadingPriority(2);
				DaySunAngle = C->LightSourceAngle;
				break;
			}
		}
		for (TActorIterator<APostProcessVolume> It(World); It; ++It)
		{
			if (It->bUnbound)
			{
				PostVolume = *It;
				DayMinBrightness = It->Settings.bOverride_AutoExposureMinBrightness ? It->Settings.AutoExposureMinBrightness : DayMinBrightness;
				DayExposureBias = It->Settings.bOverride_AutoExposureBias ? It->Settings.AutoExposureBias : 0.f;
				break;
			}
		}
		for (TActorIterator<AVolumetricCloud> It(World); It; ++It)
		{
			// The sky's clouds: more cover in bad weather, dark storm clouds in a thunderstorm.
			if (UVolumetricCloudComponent* C = It->FindComponentByClass<UVolumetricCloudComponent>(); C && C->Material.LoadSynchronous())
			{
				Clouds = C;
				CloudMaterial = UMaterialInstanceDynamic::Create(C->Material.Get(), this);
				float Coverage = DayCloudCoverage;
				if (CloudMaterial->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Cloud_GlobalCoverage")), Coverage))
				{
					DayCloudCoverage = Coverage;
				}
				C->SetMaterial(CloudMaterial);
			}
			break;
		}
		for (TActorIterator<ASkyAtmosphere> It(World); It; ++It)
		{
			if (USkyAtmosphereComponent* A = It->GetComponent())
			{
				Atmosphere = A;
				DayRayleigh = A->RayleighScatteringScale;
				DayMie = A->MieScatteringScale;
			}
			break;
		}
		for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
		{
			HeightFog = It->GetComponent();
			DayFogDensity = HeightFog->FogDensity;
			break;
		}
		for (TActorIterator<ALight> It(World); It; ++It)
		{
			if (It->ActorHasTag(TEXT("StreetLight")) && It->GetLightComponent())
			{
				StreetLights.Add(It->GetLightComponent());
				StreetIntensity.Add(It->GetLightComponent()->Intensity);
			}
		}
	}
	const float Hour = LocalMinute / 60.f;
	const float Daylight = GetDaylight();
	// From dusk to dawn: a weak cool moon (its own light, so the sky still goes dark), street lights on,
	// the eye adapts to the dark - but it stays a night.
	const float Night = 1.f - FMath::SmoothStep(0.02f, 0.18f, Daylight);
	if (ADirectionalLight* SunActor = Sun.Get())
	{
		// East (sunrise) over the south to the west; below the horizon at night.
		const float Phase = (Hour - 6.f) / 14.f;
		const float Elevation = FMath::Sin(Phase * PI) * 58.f;
		const float Yaw = 90.f + FMath::Clamp(Phase, -0.3f, 1.3f) * 180.f;
		SunActor->SetActorRotation(FRotator(-Elevation, Yaw, 0.f));
		if (ULightComponent* C = SunActor->GetLightComponent())
		{
			// Warm at sunrise and sunset, white at noon.
			const float Strength = FMath::SmoothStep(0.f, 0.25f, Daylight);
			// Clouds and rain swallow most of the direct sun and wash its colour out.
			const float Overcast = 1.f - 0.78f * CloudBlend;
			CurrentSunLux = FMath::Max(SunDaylightLux * Strength * Overcast, 0.f);
			C->SetIntensity(CurrentSunLux);
			const FLinearColor Clear = FMath::Lerp(FLinearColor(1.f, 0.62f, 0.38f), FLinearColor(1.f, 0.97f, 0.92f), FMath::SmoothStep(0.f, 0.5f, Daylight));
			C->SetLightColor(FMath::Lerp(Clear, FLinearColor(0.78f, 0.82f, 0.88f), CloudBlend));
			if (UDirectionalLightComponent* D = Cast<UDirectionalLightComponent>(C))
			{
				// Under clouds the light comes from a big bright sky: soft shadows.
				D->SetLightSourceAngle(FMath::Lerp(DaySunAngle, 14.f, CloudBlend));
			}
		}
	}
	if (!Moon.IsValid() && Night > 0.f)
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		if (ADirectionalLight* M = World->SpawnActor<ADirectionalLight>(FVector(0.f, 0.f, 1200.f), FRotator(-42.f, 205.f, 0.f), Params))
		{
			if (UDirectionalLightComponent* C = Cast<UDirectionalLightComponent>(M->GetLightComponent()))
			{
				C->SetMobility(EComponentMobility::Movable);
				C->SetAtmosphereSunLight(false);
				C->SetLightColor(FLinearColor(0.55f, 0.66f, 1.f));
				C->SetForwardShadingPriority(1);
				C->SetDynamicShadowCascades(2);
				C->SetIntensity(0.f);
			}
			Moon = M;
		}
	}
	if (ADirectionalLight* M = Moon.Get())
	{
		M->GetLightComponent()->SetIntensity(MoonLux * Night * (1.f - 0.6f * CloudBlend));
		M->GetLightComponent()->SetVisibility(Night > 0.01f);
	}
	if (APostProcessVolume* PPV = PostVolume.Get())
	{
		PPV->Settings.bOverride_AutoExposureMinBrightness = true;
		PPV->Settings.AutoExposureMinBrightness = FMath::Lerp(DayMinBrightness, 0.06f, Night);
		PPV->Settings.bOverride_AutoExposureBias = true;
		// Bad weather is darker (the eye would adapt the dimmer sun away otherwise).
		PPV->Settings.AutoExposureBias = FMath::Lerp(DayExposureBias, DayExposureBias - 1.f, Night) - 0.9f * CloudBlend * (1.f - Night);
	}
	if (CloudMaterial)
	{
		CloudMaterial->SetScalarParameterValue(TEXT("Cloud_GlobalCoverage"), FMath::Lerp(DayCloudCoverage, CVarWeatherCoverage.GetValueOnGameThread(), CloudBlend));
	}
	// Overcast: a grey sky instead of the blue one (less blue scattering, more haze).
	if (USkyAtmosphereComponent* A = Atmosphere.Get())
	{
		A->SetRayleighScatteringScale(FMath::Lerp(DayRayleigh, DayRayleigh * 0.3f, CloudBlend));
		A->SetMieScatteringScale(FMath::Lerp(DayMie, DayMie * 5.f, CloudBlend));
		A->SetSkyLuminanceFactor(FMath::Lerp(FLinearColor::White, FLinearColor(0.72f, 0.74f, 0.78f), CloudBlend) * (1.f - 0.4f * StormBlend));
	}
	// Rain: a hazier, greyer air.
	if (UExponentialHeightFogComponent* Fog = HeightFog.Get())
	{
		Fog->SetFogDensity(FMath::Lerp(DayFogDensity, DayFogDensity * 3.2f, CloudBlend));
	}
	// Street lights from dusk to dawn.
	for (int32 i = 0; i < StreetLights.Num(); ++i)
	{
		if (ULightComponent* L = StreetLights[i].Get())
		{
			L->SetIntensity(StreetIntensity[i] * Night);
			// (Off in daylight costs nothing to render; at zero intensity it still would.)
			L->SetVisibility(Night > 0.01f);
		}
	}
}

#undef LOCTEXT_NAMESPACE
