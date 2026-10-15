#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WEMoney.h"
#include "WEPowerGridComponent.generated.h"

/**
 * Power grid and machine simulation (§13.5 "Kontrollierte Simulation", Phase 9). Lives on AWEGameState.
 *
 * The server steps every machine on one fixed interval (no per-machine tick): running cycles advance,
 * machines that want to start get power only while the connection has room (base 11 kW plus the
 * "Starkstromanschluss" upgrade), otherwise they wait ("Stromanschluss überlastet"). Energy is summed
 * up and billed to the company every few minutes ("Stromrechnung"). Load, capacity and the last bill
 * replicate for the HUD.
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWEPowerGridComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEPowerGridComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	static UWEPowerGridComponent* Get(const UObject* WorldContext);

	float GetCapacityKw() const;
	float GetLoadKw() const { return LoadKw; }
	float GetUnbilledKWh() const { return UnbilledKWh; }
	const FWEMoney& GetLastBill() const { return LastBill; }
	bool HasMachines() const { return MachineCount > 0; }

	/** Server: advances all machines by DeltaSeconds and hands out power. Timer; public for tests. */
	void StepMachines(float DeltaSeconds);

	/** Server: bills the energy used since the last bill. Timer; public for tests. */
	FWEMoney Bill();

	/** Loading. */
	void SetUnbilledKWh(float KWh) { UnbilledKWh = FMath::Max(0.f, KWh); }

	static constexpr float BaseCapacityKw = 11.f;
	static constexpr float StepSeconds = 0.5f;
	static constexpr float BillIntervalSeconds = 300.f;
	/** Game price per kWh (includes connection fees; balanced so machines cost noticeably). */
	static constexpr int64 CentsPerKWh = 250;

protected:
	void OnStepTimer();
	void OnBillTimer();

	UPROPERTY(Replicated)
	float LoadKw = 0.f;

	UPROPERTY(Replicated)
	float UnbilledKWh = 0.f;

	UPROPERTY(Replicated)
	FWEMoney LastBill;

	UPROPERTY(Replicated)
	int32 MachineCount = 0;

	/** Server: fraction of a cent carried over between bills. */
	double CentRemainder = 0.0;

private:
	FTimerHandle StepTimer;
	FTimerHandle BillTimer;
};
