#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WEPlayerController.generated.h"

/**
 * Owning client's connection to the server.
 *
 * Player requests that change shared state (pick up, buy, sell, operate machine) will be sent
 * as Server RPCs from here or from components owned by this controller/pawn, then validated
 * by the server.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
