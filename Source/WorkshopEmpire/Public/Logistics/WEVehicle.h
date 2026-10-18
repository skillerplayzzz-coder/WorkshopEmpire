#pragma once

#include "CoreMinimal.h"
#include "World/WEPlaceholderProp.h"
#include "WEVehicle.generated.h"

class AWEEmployee;
class AWEPickupItem;
struct FWETourDefinition;

/** Stats of one van model. */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEVehicleStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle", meta = (ClampMin = "1")) int32 CargoSlots = 20;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle", meta = (Units = "kg")) float CargoMaxKg = 600.f;
	/** Tour time = tour minutes / this. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle", meta = (ClampMin = "0.1")) float TourSpeed = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle") FLinearColor Tint = FLinearColor(0.85f, 0.85f, 0.8f);
};

/**
 * The company van (§16, Phase 11). Parked outside the door; appears once the "Alter Transporter" upgrade
 * is bought, the "Moderner Lieferwagen" upgrade turns it into the bigger, faster van.
 *
 * Cargo = its storage: [E] opens it, carried items go in like into a machine (also bulky ones).
 * [T] chooses a tour, [Q] starts it. A tour needs an assigned driver (employee) standing at the van:
 * the van leaves (hidden) and comes back after the tour time. Sell tours sell the whole cargo directly
 * at the trader with a bonus; pickup tours bring a bulk purchase (paid at departure). Fuel costs money.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEVehicle : public AWEPlaceholderProp
{
	GENERATED_BODY()

public:
	AWEVehicle();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IWEInteractable
	virtual FText GetInteractionText(const AWECharacter* Interactor) const override;
	virtual bool CanInteract(const AWECharacter* Interactor, FText& OutReason) const override;
	virtual bool GetSecondaryInteraction(const AWECharacter* Interactor, FText& OutText) const override;
	virtual void InteractSecondary(AWECharacter* Interactor) override;
	virtual bool GetAlternativeInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const override;
	virtual void InteractAlternative(AWECharacter* Interactor) override;
	virtual bool GetSettingInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const override;
	virtual void InteractSetting(AWECharacter* Interactor) override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;

	virtual void SetUnlocked(bool bInUnlocked) override;
	virtual void RefreshFromUpgrades() override;

	const FWEVehicleStats& GetStats() const;
	bool IsImproved() const { return bImproved; }
	bool IsAway() const { return bAway; }
	FName GetSelectedTour() const { return SelectedTour; }
	FName GetActiveTour() const { return ActiveTour; }
	const FWETourDefinition* FindTour(FName Id) const;
	/** Seconds until the van is back (0 when parked). */
	float GetSecondsToReturn() const;
	const FString& GetLastResult() const { return LastResult; }
	/** The employee whose task is driving this van. */
	AWEEmployee* FindDriver() const;
	/** Why a tour cannot start now (empty = it can). */
	FText GetStartBlockReason() const;
	/** Tour time with this van (seconds). */
	float GetTourSeconds(const FWETourDefinition& Tour) const;

	bool CanLoad(const AWEPickupItem* Item, FText& OutReason) const;

	// --- Server ---------------------------------------------------------------------------------------

	bool LoadItem(AWEPickupItem* Item);
	/** [R]: bulky items out of the cargo, put down behind the van. Returns how many. */
	int32 UnloadBulky();
	void CycleTour();
	bool StartTour(AWECharacter* By);
	void StepTour(float DeltaSeconds);
	void FinishTour();
	/** Loading. */
	void RestoreTour(FName InSelectedTour, FName InActiveTour, float RemainingSeconds);
	float GetRemainingSeconds() const { return Remaining; }

protected:
	virtual void BeginPlay() override;
	virtual void RebuildVisuals() override;

	UFUNCTION()
	void OnRep_Away();

	void ApplyVisibility();
	/** Phase 15: the van drives off / arrives (visual only, on every client) instead of popping. */
	void StartDriveAnimation(bool bLeaving);
	void DriveTick();
	FTimerHandle DriveTimer;
	float DriveStart = 0.f;
	/** Where the body stands when parked (captured before driving; the body is the root component). */
	FVector DriveHome = FVector::ZeroVector;
	bool bDriveHomeValid = false;
	bool bDriveLeaving = false;
	bool bVisualAway = false;
	bool bVisualStateKnown = false;
	static constexpr float DriveSeconds = 3.5f;
	static constexpr float DriveDistance = 1600.f;
	void Notify(const FText& Message) const;

	/** Phase 17.5: modelled van (old / modern) with four wheels that turn while it drives; the boxes stay as collision. */
	void RebuildVanModel();
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> VanParts;
	UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> WheelPivots;
	float WheelRadius = 41.f;
	float LastDriveOffset = 0.f;
	int32 VanModelKind = -1;

	UPROPERTY(EditAnywhere, Category = "Vehicle")
	FWEVehicleStats BasicStats;

	UPROPERTY(EditAnywhere, Category = "Vehicle")
	FWEVehicleStats ImprovedStats;

	/** Owning this upgrade gives ImprovedStats. */
	UPROPERTY(EditAnywhere, Category = "Vehicle")
	FName ImprovedUpgrade = TEXT("ModernVan");

	/** Phase 16: third level for big companies (bigger lots need a truck). */
	UPROPERTY(EditAnywhere, Category = "Vehicle")
	FWEVehicleStats TruckStats;

	UPROPERTY(EditAnywhere, Category = "Vehicle")
	FName TruckUpgrade = TEXT("Truck");

	UPROPERTY(ReplicatedUsing = OnRep_Away)
	bool bAway = false;

	UPROPERTY(ReplicatedUsing = OnRep_Away)
	bool bImproved = false;

	UPROPERTY(ReplicatedUsing = OnRep_Away)
	bool bTruck = false;

	UPROPERTY(Replicated)
	FName SelectedTour;

	UPROPERTY(Replicated)
	FName ActiveTour;

	/** Server time when the van is back. */
	UPROPERTY(Replicated)
	float ReturnAt = 0.f;

	UPROPERTY(Replicated)
	FString LastResult;

private:
	float Remaining = 0.f;
};
