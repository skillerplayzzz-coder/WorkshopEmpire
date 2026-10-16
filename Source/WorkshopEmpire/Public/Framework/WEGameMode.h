#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Save/WESaveGame.h"
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
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) override;

	// --- Co-op session (Phase 13) -----------------------------------------------------------------------

	/** Host only: removes a player from the session (they see the reason in the main menu). */
	bool KickPlayer(APlayerController* KickedPlayer);
	/** Host leaves: every client is sent back to the main menu with "Der Host hat das Spiel beendet". */
	void EndSessionForClients();
	/** Players who left: their position and inventory come back when they rejoin and are saved. */
	const TArray<FWESavedPlayer>& GetDepartedPlayers() const { return DepartedPlayers; }
	/** Server-side reason for refusing a login (full, other version); empty = allowed. Public for tests. */
	static FString CheckLogin(int32 NumPlayers, int32 MaxPlayers, const FString& Options);
	/** Remembers a leaving player (replaces an older entry with the same key). Public for tests. */
	void RememberDepartedPlayer(const FWESavedPlayer& Player);
	/** A rejoining player's stored data, removed from the list; false if none. Public for tests. */
	bool TakeDepartedPlayer(const FString& Key, const FString& Name, FWESavedPlayer& OutPlayer);

	int32 GetMaxCoopPlayers() const { return MaxCoopPlayers; }

	/** Saves the whole session to Slot and tells all players. */
	bool SaveToSlot(const FString& Slot);

	/** The slot this game was loaded from / last saved to ("Quicksave" for a new game). */
	const FString& GetCurrentSlot() const { return CurrentSlot; }

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

	UPROPERTY(Transient)
	TArray<FWESavedPlayer> DepartedPlayers;

	/** Save slot to load when the host starts the session (?WELoad=Slot). */
	FString StartSlot;

	/** Last loaded save, applied to players who join afterwards. */
	UPROPERTY(Transient)
	TObjectPtr<UWESaveGame> LastLoadedSave;

	FTimerHandle AutosaveTimer;
	bool bEndingSession = false;
	FString CurrentSlot = TEXT("Quicksave");
};
