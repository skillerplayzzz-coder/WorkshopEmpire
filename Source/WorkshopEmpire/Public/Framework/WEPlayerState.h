#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "WEPlayerState.generated.h"

/**
 * Replicated per-player data that every client may see (name, role in the company,
 * personal statistics). Company-wide data belongs in AWEGameState instead.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEPlayerState : public APlayerState
{
	GENERATED_BODY()
};
