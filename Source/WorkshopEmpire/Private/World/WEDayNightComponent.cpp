#include "World/WEDayNightComponent.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Light.h"
#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"
#include "Framework/WEGameState.h"
#include "Net/UnrealNetwork.h"
#include "Dev/WEDevAutomationComponent.h"
#include "World/WECustomer.h"

#define LOCTEXT_NAMESPACE "WEDayNight"

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
	}
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
			C->SetIntensity(FMath::Max(SunDaylightLux * Strength, 0.f));
			C->SetLightColor(FMath::Lerp(FLinearColor(1.f, 0.62f, 0.38f), FLinearColor(1.f, 0.97f, 0.92f), FMath::SmoothStep(0.f, 0.5f, Daylight)));
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
				C->SetDynamicShadowCascades(2);
				C->SetIntensity(0.f);
			}
			Moon = M;
		}
	}
	if (ADirectionalLight* M = Moon.Get())
	{
		M->GetLightComponent()->SetIntensity(MoonLux * Night);
		M->GetLightComponent()->SetVisibility(Night > 0.01f);
	}
	if (APostProcessVolume* PPV = PostVolume.Get())
	{
		PPV->Settings.bOverride_AutoExposureMinBrightness = true;
		PPV->Settings.AutoExposureMinBrightness = FMath::Lerp(DayMinBrightness, 0.06f, Night);
		PPV->Settings.bOverride_AutoExposureBias = true;
		PPV->Settings.AutoExposureBias = FMath::Lerp(DayExposureBias, DayExposureBias - 1.f, Night);
	}	// Street lights from dusk to dawn.
	for (int32 i = 0; i < StreetLights.Num(); ++i)
	{
		if (ULightComponent* L = StreetLights[i].Get())
		{
			L->SetIntensity(StreetIntensity[i] * Night);
		}
	}
}

#undef LOCTEXT_NAMESPACE
