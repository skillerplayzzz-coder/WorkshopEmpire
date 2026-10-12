#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "WEGameState.generated.h"

/**
 * Replicated state shared by every player of the session.
 *
 * Planned home of the shared company (account balance, unlocks, progression) - see
 * TECHNICAL_ARCHITECTURE.md. Mutations will only happen on the server; clients read
 * replicated values and send requests through their PlayerController.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEGameState : public AGameStateBase
{
	GENERATED_BODY()
};
