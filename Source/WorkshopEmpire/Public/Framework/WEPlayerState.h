#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "WEPlayerState.generated.h"

class UWEInventoryComponent;

/**
 * Replicated per-player data. Holds the player inventory so it survives pawn respawns and can be
 * saved per player by the host. Company-wide data belongs in AWEGameState instead.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AWEPlayerState();

	UWEInventoryComponent* GetInventory() const { return Inventory; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TObjectPtr<UWEInventoryComponent> Inventory;
};
