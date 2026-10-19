#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WEDayNightComponent.generated.h"

class ADirectionalLight;
class ULightComponent;

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
	/** Moves the sun, sets its strength, switches street lights (every machine that renders). */
	void ApplyLighting();

	UPROPERTY(ReplicatedUsing = OnRep_Clock) int32 Day = 1;
	UPROPERTY(ReplicatedUsing = OnRep_Clock) float Minute = 9.f * 60.f;

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
};
