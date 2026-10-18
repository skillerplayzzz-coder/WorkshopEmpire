#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Player/WECharacterLook.h"
#include "WEAvatarPreview.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class UWEAvatarComponent;

/**
 * Character creator preview (Phase 18c): the avatar standing in the menu scene with its idle animation, its own
 * camera in front of it and a soft key light. The creator page shows it and turns it.
 */
UCLASS(NotPlaceable, Transient)
class WORKSHOPEMPIRE_API AWEAvatarPreview : public AActor
{
	GENERATED_BODY()

public:
	AWEAvatarPreview();

	void SetLook(const FWECharacterLook& Look);
	/** Turns the character (degrees, relative). */
	void Turn(float Degrees);
	void SetYaw(float Degrees);
	float GetYaw() const { return Yaw; }
	UCameraComponent* GetCamera() const { return Camera; }
	UWEAvatarComponent* GetAvatar() const { return Avatar; }
	/** Close-up of the face (head) or the whole figure. */
	void SetCloseUp(bool bClose);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Turntable;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> Driver;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UWEAvatarComponent> Avatar;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UPointLightComponent> KeyLight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UPointLightComponent> FillLight;

	float Yaw = 0.f;
	float ShownYaw = 0.f;
	bool bCloseUp = false;
	float CameraBlend = 0.f;
};
