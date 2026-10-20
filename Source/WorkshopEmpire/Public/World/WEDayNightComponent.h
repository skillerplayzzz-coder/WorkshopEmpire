#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WEDayNightComponent.generated.h"

class ADirectionalLight;
class ULightComponent;
class UAudioComponent;
class AWEFx;

/** Phase 20: weather (rolled by the server now and then, every machine shows it). */
UENUM(BlueprintType)
enum class EWEWeather : uint8
{
	Clear,
	Cloudy,
	Rain,
	Storm
};

/**
 * Phase 19: time of day. A game day lasts DayLengthMinutes real minutes; the sun rises in the east, stands high at
 * noon and sets in the west, street lights come on at dusk. The workshop has opening hours (walk-in customers only
 * come then). The server owns the clock (day and minute replicate); every machine moves its own sun from it.
 * Lives on AWEGameState.
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWEDayNightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEDayNightComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	static UWEDayNightComponent* Get(const UObject* WorldContext);

	/** Minutes since midnight (0-1440), smooth on every machine. */
	float GetMinuteOfDay() const { return LocalMinute; }
	int32 GetDay() const { return Day; }
	/** "14:35". */
	FText GetClockText() const;
	/** Within opening hours (customers come in). */
	bool IsOpen() const;
	/** Sun above the horizon (0 at night, 1 at noon). */
	float GetDaylight() const;
	FText GetOpeningText() const;

	/** Phase 20: weather. Server sets it (rolls, dev runs: -WEWeather=Rain|Storm|Cloudy|Clear); clients blend over. */
	EWEWeather GetWeather() const { return Weather; }
	void SetWeather(EWEWeather InWeather, bool bInstant = false);
	FText GetWeatherText() const;
	/** 0-1, smooth: how hard it rains here right now (rain FX, sound). */
	float GetRainAmount() const { return RainBlend; }
	/** Lightning flashes so far on this machine (tests). */
	int32 GetLightningCount() const { return LightningCount; }
	/** Camera under a roof (rain sounds muffled). */
	bool IsViewIndoors() const { return bViewIndoors; }
	/** Rain drops around the view right now (tests). */
	int32 GetRainParticleCount() const;
	/** Shots / tests: the next storm frame flashes. */
	void TriggerLightning() { LightningTimer = 0.f; }

	/** Server: jump to a time (tests, dev runs, loading). Hour may have minutes as fractions. */
	void SetTime(int32 InDay, float InMinute);
	/** Server / tests: let time pass. */
	void Advance(float GameMinutes);

	static constexpr float OpenHour = 8.f;
	static constexpr float CloseHour = 19.f;
	/** A whole game day in real minutes. */
	static constexpr float DayLengthMinutes = 48.f;

protected:
	UFUNCTION() void OnRep_Clock();
	UFUNCTION() void OnRep_Weather();
	/** Every rendering machine: rain around the view, rain sound, lightning + thunder. */
	void TickWeather(float DeltaTime);
	/** Moves the sun, sets its strength, switches street lights (every machine that renders). */
	void ApplyLighting();

	UPROPERTY(ReplicatedUsing = OnRep_Clock) int32 Day = 1;
	UPROPERTY(ReplicatedUsing = OnRep_Clock) float Minute = 9.f * 60.f;
	UPROPERTY(ReplicatedUsing = OnRep_Weather) EWEWeather Weather = EWEWeather::Clear;

private:
	float LocalMinute = 9.f * 60.f;
	float SyncTimer = 0.f;
	float LightTimer = 0.f;
	float CustomerTimer = 0.f;
	float SunDaylightLux = 75000.f;
	bool bSunCached = false;
	TWeakObjectPtr<ADirectionalLight> Sun;
	TArray<TWeakObjectPtr<ULightComponent>> StreetLights;
	TArray<float> StreetIntensity;
	/** The unbound post process volume: its darkest exposure goes down at night (else the night is black). */
	TWeakObjectPtr<class APostProcessVolume> PostVolume;
	float DayMinBrightness = 3.f;
	float DayExposureBias = 0.f;
	/** Moonlight at night (spawned on demand; not the atmosphere's sun, so the sky stays dark). */
	TWeakObjectPtr<ADirectionalLight> Moon;
	static constexpr float MoonLux = 4.f;

	// Weather (server: roll timer; every machine: blends and effects).
	float WeatherTimer = 0.f;
	float CloudBlend = 0.f;
	float RainBlend = 0.f;
	bool bSnapWeather = true;
	bool bViewIndoors = false;
	float IndoorTimer = 0.f;
	float RainVolume = 0.f;
	float LightningTimer = 8.f;
	float FlashAge = -1.f;
	float ThunderDelay = -1.f;
	int32 LightningCount = 0;
	UPROPERTY(Transient) TObjectPtr<AWEFx> RainFx;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> RainAudio;
	TWeakObjectPtr<ADirectionalLight> Lightning;
	float CurrentSunLux = 0.f;
	TWeakObjectPtr<class UExponentialHeightFogComponent> HeightFog;
	float DayFogDensity = 0.012f;
	float StormBlend = 0.f;
	TWeakObjectPtr<class UVolumetricCloudComponent> Clouds;
	UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> CloudMaterial;
	float DayCloudCoverage = -0.2f;
	TWeakObjectPtr<class USkyAtmosphereComponent> Atmosphere;
	float DayRayleigh = 1.f;
	float DayMie = 1.f;
	float DaySunAngle = 0.5357f;
};
