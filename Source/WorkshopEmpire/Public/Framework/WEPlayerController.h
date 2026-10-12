#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WEPlayerController.generated.h"

class UWEHUDWidget;

/**
 * Owning client's connection to the server.
 *
 * Player requests that change shared state (pick up, buy, sell, operate machine) are sent as
 * Server RPCs from components owned by this controller or its pawn, then validated by the server.
 * Also owns the local HUD.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AWEPlayerController();

	UWEHUDWidget* GetHUDWidget() const { return HUDWidget; }

protected:
	virtual void BeginPlay() override;

	/** HUD class; can be swapped for a UMG subclass. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UWEHUDWidget> HUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UWEHUDWidget> HUDWidget;
};
