#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WESessionSubsystem.generated.h"

class FOnlineSessionSearch;
class FOnlineSessionSearchResult;

/** Which online service carries the co-op session (Phase 13/14). */
UENUM()
enum class EWEOnlineService : uint8
{
	/** No online subsystem at all (should not happen; direct IP still works). */
	None,
	/** NULL subsystem: LAN sessions and direct IP (Steam not running or -nosteam). */
	LAN,
	/** Steam: lobbies, friends, invites, Steam networking (no port forwarding). */
	Steam
};

/** One game found by FindGames. */
USTRUCT()
struct WORKSHOPEMPIRE_API FWESessionEntry
{
	GENERATED_BODY()

	UPROPERTY() FString HostName;
	UPROPERTY() int32 Players = 0;
	UPROPERTY() int32 MaxPlayers = 4;
	UPROPERTY() int32 PingMs = 0;
	UPROPERTY() FString Version;
	UPROPERTY() bool bCompatible = true;
	/** Index into the last search's results. */
	UPROPERTY() int32 ResultIndex = INDEX_NONE;
};

/** What the main menu is doing right now (for the UI and tests). */
UENUM()
enum class EWESessionState : uint8
{
	Idle,
	Hosting,
	Searching,
	Joining,
	InGame,
	Leaving
};

DECLARE_MULTICAST_DELEGATE(FWEOnSessionStateChanged);

/**
 * Co-op sessions (Phase 13 Multiplayer-Integration, Phase 14 Steam-Integration).
 *
 * Lives on the game instance, so it survives map travel. Wraps the online subsystem's session
 * interface: the host creates a session (Steam lobby, or LAN beacon without Steam) and opens the
 * garage as listen server; clients find it (or get a Steam invite) and travel there. Network and
 * travel failures send the player back to the main menu with a German explanation, which the menu
 * shows (ConsumePendingMessage).
 *
 * Player count, version and save rules are enforced by the server (AWEGameMode::PreLogin), never
 * here: a client could skip this class entirely.
 */
UCLASS()
class WORKSHOPEMPIRE_API UWESessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UWESessionSubsystem* Get(const UObject* WorldContext);

	// --- Service --------------------------------------------------------------------------------------

	EWEOnlineService GetService() const;
	/** "Steam – angemeldet als Mike" / "LAN (Steam nicht verfügbar)". */
	FText GetServiceText() const;
	/** Steam persona name, otherwise the name chosen in the menu. */
	FString GetLocalPlayerName() const;
	/** LAN only: the name others see (Steam always uses the Steam name). */
	void SetLocalPlayerName(const FString& Name);
	bool CanChangePlayerName() const { return GetService() != EWEOnlineService::Steam; }

	// --- Host / join / leave ------------------------------------------------------------------------

	/** Creates the session and opens the garage as listen server. Slot empty = new game. bLAN = LAN even with Steam. */
	void HostGame(const FString& Slot, bool bLAN);
	/** Phase 17.5: a new company with a name (its save slot) and, if wanted, the tutorial. */
	void HostNewGame(const FString& CompanyName, bool bTutorial, bool bLAN);
	/** Searches LAN games (bLAN) or Steam lobbies of Workshop Empire. */
	void FindGames(bool bLAN);
	void JoinGame(int32 ResultIndex);
	/** "192.168.0.5" or "192.168.0.5:7777" (IP connection, also with Steam). */
	void JoinAddress(const FString& Address);
	/** Leaves the session and opens the main menu (Message is shown there; empty = none). */
	void LeaveToMainMenu(const FText& Message = FText::GetEmpty());
	/** Steam friends overlay with "invite"; false (with message) without Steam. */
	bool ShowInviteUI(FText& OutError);

	EWESessionState GetState() const { return State; }
	const TArray<FWESessionEntry>& GetFoundGames() const { return FoundGames; }
	FText GetStatusText() const { return StatusText; }
	bool IsLANSession() const { return bHostedLAN; }

	/** Message for the main menu after a failure / kick / host ending the game (cleared when read). */
	FText ConsumePendingMessage();
	void SetPendingMessage(const FText& Message) { PendingMessage = Message; }
	/** The main menu opened: nothing is in progress any more. */
	void NotifyMenuOpened();

	FWEOnSessionStateChanged OnStateChanged;

	// --- Pure helpers (public for tests) -------------------------------------------------------------

	static FString GetGameVersion();
	/** Same major.minor = compatible (patch differences are fine). Unparsable = incompatible. */
	static bool IsCompatibleVersion(const FString& Host, const FString& Client);
	static FText DescribeNetworkFailure(ENetworkFailure::Type Type, const FString& ErrorString);
	static FText DescribeTravelFailure(ETravelFailure::Type Type, const FString& ErrorString);
	static FText DescribeJoinResult(EOnJoinSessionCompleteResult::Type Result);
	/** Options appended to every join URL: name and version for the server's PreLogin check. */
	static FString MakeJoinOptions(const FString& PlayerName, bool bLAN);
	/** Adds the default port if missing; empty if the address is not usable. */
	static FString NormalizeAddress(const FString& Address);

	static constexpr int32 MaxPlayers = 4;
	static constexpr int32 GamePort = 7777;
	static const FName GameKey;
	static const FName VersionKey;
	static const FName HostKey;

	/** Development: counts how often the menu was entered (automation scripts with several rounds). */
	int32 DevMenuVisits = 0;

protected:
	void SetState(EWESessionState NewState, const FText& Status);
	void OpenGarage(const FString& Slot, bool bLAN);
	void DoJoin(const FOnlineSessionSearchResult& Result);

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	IOnlineSessionPtr GetSessions() const;

	EWESessionState State = EWESessionState::Idle;
	FText StatusText;
	FText PendingMessage;
	TArray<FWESessionEntry> FoundGames;
	TSharedPtr<FOnlineSessionSearch> Search;
	FString PendingSlot;
	FString PendingNewGameOptions;
	bool bPendingLAN = false;
	bool bHostedLAN = false;
	/** Invite accepted while a session was still running: join after it is destroyed. */
	TSharedPtr<FOnlineSessionSearchResult> PendingInvite;
	FString LANPlayerName;

	FDelegateHandle CreateHandle;
	FDelegateHandle FindHandle;
	FDelegateHandle JoinHandle;
	FDelegateHandle DestroyHandle;
	FDelegateHandle InviteHandle;
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;
};
