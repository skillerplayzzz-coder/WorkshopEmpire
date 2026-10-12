#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WEGameMode.generated.h"

/**
 * Server-only rules of a Workshop Empire session.
 *
 * Exists only on the server / listen-server host. Owns session rules such as the co-op
 * player limit; shared company state lives in AWEGameState so clients can see it.
 */
UCLASS(Config = Game)
class WORKSHOPEMPIRE_API AWEGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWEGameMode();

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	int32 GetMaxCoopPlayers() const { return MaxCoopPlayers; }

protected:
	/** Co-op design target is 2-4 players (spec section 17). */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Session", meta = (ClampMin = "1", ClampMax = "4"))
	int32 MaxCoopPlayers = 4;
};
