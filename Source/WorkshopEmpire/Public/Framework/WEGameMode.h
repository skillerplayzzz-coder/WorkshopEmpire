#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WEGameMode.generated.h"

class UWESaveGame;

/**
 * Server-only rules of a Workshop Empire session.
 *
 * Exists only on the server / listen-server host. Owns session rules (co-op player limit) and the
 * company save game (manual save/load, autosave). Shared company state lives in AWEGameState.
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

	/** Saves the whole session to Slot and tells all players. */
	bool SaveToSlot(const FString& Slot);

	/** Loads Slot into the running session and tells all players. */
	bool LoadFromSlot(const FString& Slot);

	virtual void InitGameState() override;

protected:
	virtual void BeginPlay() override;

	/** Company account at the start of a new game (spec §3: 250 €). In cents. */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Economy")
	int64 StartingCapitalCents = 25000;

	void Autosave();
	void BroadcastMessage(const FText& Message);

	/** Co-op design target is 2-4 players (spec section 17). */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Session", meta = (ClampMin = "1", ClampMax = "4"))
	int32 MaxCoopPlayers = 4;

	/** Seconds between autosaves; 0 disables. */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Save", meta = (ClampMin = "0"))
	float AutosaveIntervalSeconds = 300.f;

	/** Last loaded save, applied to players who join afterwards. */
	UPROPERTY(Transient)
	TObjectPtr<UWESaveGame> LastLoadedSave;

	FTimerHandle AutosaveTimer;
};
