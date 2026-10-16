#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Staff/WEStaffTypes.h"
#include "WEMoney.h"
#include "WEStaffComponent.generated.h"

class AWECharacter;
class AWEEmployee;

UENUM()
enum class EWEStaffResult : uint8
{
	Success,
	Invalid,
	TooManyEmployees,
	Duplicate
};

/**
 * Staff management on the game state (§15, Phase 11): applicants, hiring and firing, payroll and the
 * work step of all employees and vans (one server timer, no logic per actor tick).
 *
 * Wages are paid every PayrollIntervalSeconds for the time worked. If the company cannot pay, nobody
 * works ("Lohn ausstehend") until a later payroll succeeds; the debt is kept.
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWEStaffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEStaffComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	static UWEStaffComponent* Get(const UObject* WorldContext);

	const UWEStaffCatalog* GetCatalog() const { return Catalog; }
	const UWETourCatalog* GetTourCatalog() const { return TourCatalog; }
	void SetCatalogs(UWEStaffCatalog* InCatalog, UWETourCatalog* InTours) { Catalog = InCatalog; TourCatalog = InTours; }

	const TArray<FWEEmployeeProfile>& GetApplicants() const { return Applicants; }
	TArray<AWEEmployee*> GetEmployees() const;
	const FWEMoney& GetOwedWages() const { return OwedWages; }
	/** Catalog limit plus what buildings add (Phase 12). */
	int32 GetMaxEmployees() const;
	/** Wages of all employees per minute. */
	FWEMoney GetWagesPerMinute() const;
	/** Seconds until the next payroll (from replicated server time). */
	float GetSecondsToPayroll() const;
	FText GetRoleDescription(EWEStaffRole Role) const;

	// --- Server ---------------------------------------------------------------------------------------

	EWEStaffResult Hire(AWECharacter* By, int32 ApplicantIndex, const FGuid& RequestId, AWEEmployee** OutEmployee = nullptr);
	bool Fire(AWEEmployee* Employee, AWECharacter* By);
	/** Pays the wages earned since the last payroll (timer; public for tests). */
	bool Payroll();
	/** Rolls new applicants (timer; public for tests). */
	void RefreshApplicants();
	/** Employees decide and work, vans drive (timer; public for tests). */
	void Step(float DeltaSeconds);

	/** Loading: an employee with a stable name. */
	AWEEmployee* SpawnEmployee(const FWEEmployeeProfile& Profile, const FTransform& Transform, FName ActorName = NAME_None);
	void RestoreState(const TArray<FWEEmployeeProfile>& InApplicants, const FWEMoney& InOwed);
	void SetRandomSeed(int32 Seed) { Random.Initialize(Seed); }

	static constexpr float StepSeconds = 0.25f;
	static constexpr float PayrollIntervalSeconds = 300.f;
	static constexpr float ApplicantRefreshSeconds = 600.f;
	static constexpr int32 NumApplicants = 3;

protected:
	FWEEmployeeProfile MakeApplicant();
	FVector GetSpawnLocation() const;
	void OnStepTimer();
	void OnPayrollTimer();
	void OnApplicantTimer();

	UPROPERTY(EditAnywhere, Category = "Staff")
	TObjectPtr<UWEStaffCatalog> Catalog;

	UPROPERTY(EditAnywhere, Category = "Staff")
	TObjectPtr<UWETourCatalog> TourCatalog;

	UPROPERTY(Replicated)
	TArray<FWEEmployeeProfile> Applicants;

	UPROPERTY(Replicated)
	FWEMoney OwedWages;

	/** Server time of the next payroll. */
	UPROPERTY(Replicated)
	float NextPayrollAt = 0.f;

private:
	FRandomStream Random;
	FTimerHandle StepTimer;
	FTimerHandle PayrollTimer;
	FTimerHandle ApplicantTimer;
};
