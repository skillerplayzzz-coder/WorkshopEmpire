#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WEInteractable.h"
#include "WEGarageDoor.generated.h"

class UStaticMeshComponent;

/**
 * Garage door (roller shutter; the collision panel slides up, the model rolls up). Interact with [E] to open/close.
 *
 * Only bIsOpen is replicated; every machine animates the panel locally with a short timer,
 * so there is no per-frame movement replication and no Tick.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEGarageDoor : public AActor, public IWEInteractable
{
	GENERATED_BODY()

public:
	AWEGarageDoor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IWEInteractable
	virtual FText GetInteractionText(const AWECharacter* Interactor) const override;
	virtual bool CanInteract(const AWECharacter* Interactor, FText& OutReason) const override;
	virtual void Interact(AWECharacter* Interactor) override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;

	bool IsOpen() const { return bIsOpen; }

	/** Server only. */
	void SetOpen(bool bOpen);

	/** Phase 17.5: painted door (nullptr = original galvanised look). */
	void ApplyStyle(const struct FWEStyleOption* Paint);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_IsOpen();

	void ApplyTint();
	/** Phase 15: roller shutter model riding on the (then invisible) collision panel. */
	void ApplyShutterModel();
	/** Rolls the shutter model up with the panel (visual only). */
	void UpdateShutterVisual();
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> ShutterVisual;
	/** Phase 17.5: the same shutter turned round, so the door is solid from inside too (the model is one-sided). */
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> ShutterBack;
	void StartMoving();
	void StepMovement();
	float GetTargetHeight() const;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Panel;

	UPROPERTY(EditAnywhere, Category = "Door", meta = (Units = "cm"))
	float DoorWidth = 400.f;

	UPROPERTY(EditAnywhere, Category = "Door", meta = (Units = "cm"))
	float DoorHeight = 250.f;

	UPROPERTY(EditAnywhere, Category = "Door", meta = (Units = "cm"))
	float PanelThickness = 8.f;

	UPROPERTY(EditAnywhere, Category = "Door", meta = (Units = "CentimetersPerSecond"))
	float OpenSpeed = 180.f;

	UPROPERTY(EditAnywhere, Category = "Door")
	TObjectPtr<UMaterialInterface> PlaceholderMaterial;

	UPROPERTY(EditAnywhere, Category = "Door")
	FLinearColor Tint = FLinearColor(0.16f, 0.20f, 0.26f);

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_IsOpen, Category = "Door")
	bool bIsOpen = false;

private:
	FTimerHandle MoveTimer;
};
