#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WEInteractable.h"
#include "WEMirror.generated.h"

class UMaterialInstanceDynamic;
class USceneCaptureComponent2D;
class UStaticMeshComponent;
class UTextureRenderTarget2D;

/**
 * Phase 19: a wall mirror in the workshop. A scene capture at the glass looks into the room and the picture is
 * shown mirrored, so a player standing in front of it sees himself (the own body is visible to the capture
 * even in first person). [E] opens the look editor: clothes, face, hair - changes show at once for everybody.
 * Only renders while a local player is close and the mirror is on screen.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEMirror : public AActor, public IWEInteractable
{
	GENERATED_BODY()

public:
	AWEMirror();

	virtual void Tick(float DeltaSeconds) override;

	// IWEInteractable
	virtual FText GetInteractionText(const class AWECharacter* Interactor) const override;
	virtual bool CanInteract(const class AWECharacter* Interactor, FText& OutReason) const override;
	virtual void Interact(class AWECharacter* Interactor) override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;

	/** Size of the glass in cm (width, height). */
	UPROPERTY(EditAnywhere, Category = "Mirror")
	FVector2D GlassSize = FVector2D(70.f, 150.f);

	/** Tests and shots: pictures taken so far. */
	int32 GetCaptureCount() const { return CaptureCount; }
	/** Shots: render now, whoever is near. */
	void CaptureNow();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Frame;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Glass;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneCaptureComponent2D> Capture;
	UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> Target;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> GlassMaterial;

	float CaptureTimer = 0.f;
	int32 CaptureCount = 0;
};
