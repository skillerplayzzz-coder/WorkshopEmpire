#pragma once

#include "CoreMinimal.h"
#include "World/WEPlaceholderProp.h"
#include "WEForklift.generated.h"

class AWECharacter;

/**
 * Forklift (§16, Phase 11). Appears with the "Gabelstapler" upgrade.
 *
 * [E] climbs on: the forklift is attached to the player character, who now moves slower but can lift
 * one item of up to LiftKg on the forks (e.g. the 180-kg industrial motor, bales, pallets) with the usual
 * [E] pick up / put down. [X] gets off; the forklift stays where it was driven. Driving uses the
 * character's movement (so it replicates and predicts like walking); the forklift itself only follows.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEForklift : public AWEPlaceholderProp
{
	GENERATED_BODY()

public:
	AWEForklift();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IWEInteractable
	virtual FText GetInteractionText(const AWECharacter* Interactor) const override;
	virtual bool CanInteract(const AWECharacter* Interactor, FText& OutReason) const override;
	virtual void Interact(AWECharacter* Interactor) override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;
	/** Upgrades re-apply visibility: while driven the forklift keeps its collision off. */
	virtual void SetUnlocked(bool bInUnlocked) override;

	AWECharacter* GetDriver() const { return Driver; }
	USceneComponent* GetForks() const { return Forks; }

	// --- Server ---------------------------------------------------------------------------------------

	bool Mount(AWECharacter* Character);
	/** Gets the driver off: the forklift stays here, the driver steps out to the side. */
	bool Dismount();

	static constexpr float LiftKg = 1500.f;
	static constexpr float DriveSpeed = 380.f;
	/** Where the driver sits (forklift space). */
	static FVector GetSeatOffset() { return FVector(-45.f, 0.f, 0.f); }

protected:
	UFUNCTION()
	void OnRep_Driver();

	void ApplyDriven();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Forks;

	UPROPERTY(ReplicatedUsing = OnRep_Driver)
	TObjectPtr<AWECharacter> Driver;
};
