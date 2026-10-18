#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "WEMenuGameMode.generated.h"

/** Main menu map (L_MainMenu, Phase 13): no pawn, only the menu player controller. */
UCLASS()
class WORKSHOPEMPIRE_API AWEMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWEMenuGameMode();
};

/**
 * Shows the main menu, looks through the first camera in the level and shows the message the session
 * service left (connection lost, kicked, host ended the game).
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	class UWEMainMenuWidget* GetMenu() const { return Menu; }

protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

	UPROPERTY(Transient)
	TObjectPtr<class UWEMainMenuWidget> Menu;

	TWeakObjectPtr<class ACameraActor> MenuCamera;
	FTransform CameraStart;
	float MenuTime = 0.f;
};
