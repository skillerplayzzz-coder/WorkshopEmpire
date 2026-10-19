#include "Online/WESessionSubsystem.h"
#include "Internationalization/Internationalization.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GeneralProjectSettings.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "WELog.h"

#define LOCTEXT_NAMESPACE "WESession"

const FName UWESessionSubsystem::GameKey(TEXT("WEGAME"));
const FName UWESessionSubsystem::VersionKey(TEXT("WEVERSION"));
const FName UWESessionSubsystem::HostKey(TEXT("WEHOST"));

namespace WESession
{
	const TCHAR* GameId = TEXT("WorkshopEmpire");
	const TCHAR* GarageMap = TEXT("/Game/WorkshopEmpire/Maps/L_Garage");
	const TCHAR* MenuMap = TEXT("/Game/WorkshopEmpire/Maps/L_MainMenu");
	const TCHAR* ConfigSection = TEXT("/Script/WorkshopEmpire.WESession");

	FString SanitizeName(const FString& In)
	{
		FString Out;
		for (const TCHAR C : In.TrimStartAndEnd())
		{
			if (FChar::IsAlnum(C) || C == TEXT('-') || C == TEXT('_') || C == TEXT('.'))
			{
				Out.AppendChar(C);
			}
			else if (C == TEXT(' '))
			{
				Out.AppendChar(TEXT('_'));
			}
		}
		return Out.Left(20);
	}
}

void UWESessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Phase 19: numbers, dates and times the German way ("1.550", "2,5 kg"), like the texts. Not in the editor (tests).
	if (!GIsEditor)
	{
		FInternationalization::Get().SetCurrentLocale(TEXT("de-DE"));
	}

	GConfig->GetString(WESession::ConfigSection, TEXT("PlayerName"), LANPlayerName, GGameUserSettingsIni);
	if (LANPlayerName.IsEmpty())
	{
		LANPlayerName = TEXT("Spieler");
	}
	// -WEPlayerName=Anna: a name for this run only (several instances on one PC, tests); not saved.
	FString CommandLineName;
	if (FParse::Value(FCommandLine::Get(), TEXT("WEPlayerName="), CommandLineName) && !WESession::SanitizeName(CommandLineName).IsEmpty())
	{
		LANPlayerName = WESession::SanitizeName(CommandLineName);
	}

	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UWESessionSubsystem::HandleNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UWESessionSubsystem::HandleTravelFailure);
	}
	if (const IOnlineSessionPtr Sessions = GetSessions())
	{
		CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &UWESessionSubsystem::OnCreateSessionComplete));
		FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &UWESessionSubsystem::OnFindSessionsComplete));
		JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &UWESessionSubsystem::OnJoinSessionComplete));
		DestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &UWESessionSubsystem::OnDestroySessionComplete));
		InviteHandle = Sessions->AddOnSessionUserInviteAcceptedDelegate_Handle(FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &UWESessionSubsystem::OnInviteAccepted));
	}
	UE_LOG(LogWENet, Log, TEXT("Session service: %s (version %s)"), *GetServiceText().ToString(), *GetGameVersion());
}

void UWESessionSubsystem::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}
	if (const IOnlineSessionPtr Sessions = GetSessions())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
		Sessions->ClearOnSessionUserInviteAcceptedDelegate_Handle(InviteHandle);
	}
	Super::Deinitialize();
}

UWESessionSubsystem* UWESessionSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UWESessionSubsystem>() : nullptr;
}

IOnlineSessionPtr UWESessionSubsystem::GetSessions() const
{
	const IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld());
	return OSS ? OSS->GetSessionInterface() : nullptr;
}

// --- Service ----------------------------------------------------------------------------------------

EWEOnlineService UWESessionSubsystem::GetService() const
{
	const IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld());
	if (!OSS)
	{
		return EWEOnlineService::None;
	}
	return OSS->GetSubsystemName() == STEAM_SUBSYSTEM ? EWEOnlineService::Steam : EWEOnlineService::LAN;
}

FText UWESessionSubsystem::GetServiceText() const
{
	switch (GetService())
	{
	case EWEOnlineService::Steam:
		return FText::Format(LOCTEXT("ServiceSteam", "Steam – angemeldet als {0}"), FText::FromString(GetLocalPlayerName()));
	case EWEOnlineService::LAN:
		return LOCTEXT("ServiceLAN", "LAN / direkte IP (Steam nicht verfügbar)");
	default:
		return LOCTEXT("ServiceNone", "Nur direkte IP");
	}
}

FString UWESessionSubsystem::GetLocalPlayerName() const
{
	if (GetService() == EWEOnlineService::Steam)
	{
		if (const IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld()); OSS && OSS->GetIdentityInterface())
		{
			const FString Nick = OSS->GetIdentityInterface()->GetPlayerNickname(0);
			if (!Nick.IsEmpty())
			{
				return Nick;
			}
		}
	}
	return LANPlayerName;
}

void UWESessionSubsystem::SetLocalPlayerName(const FString& Name)
{
	const FString Clean = WESession::SanitizeName(Name);
	if (!Clean.IsEmpty())
	{
		LANPlayerName = Clean;
		GConfig->SetString(WESession::ConfigSection, TEXT("PlayerName"), *LANPlayerName, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

// --- State ------------------------------------------------------------------------------------------

void UWESessionSubsystem::SetState(EWESessionState NewState, const FText& Status)
{
	State = NewState;
	StatusText = Status;
	UE_LOG(LogWENet, Log, TEXT("Session state %d: %s"), static_cast<int32>(NewState), *Status.ToString());
	OnStateChanged.Broadcast();
}

void UWESessionSubsystem::NotifyMenuOpened()
{
	PendingInvite.Reset();
	FoundGames.Reset();
	SetState(EWESessionState::Idle, FText::GetEmpty());
}

FText UWESessionSubsystem::ConsumePendingMessage()
{
	FText Message = PendingMessage;
	PendingMessage = FText::GetEmpty();
	return Message;
}

// --- Host -------------------------------------------------------------------------------------------

void UWESessionSubsystem::HostNewGame(const FString& CompanyName, bool bTutorial, bool bLAN)
{
	// The company name becomes the save slot (letters, digits, - and _ only).
	FString SlotName;
	for (const TCHAR C : CompanyName.Left(32))
	{
		SlotName.AppendChar(FChar::IsAlnum(C) || C == TEXT('-') || C == TEXT('_') ? C : TEXT('_'));
	}
	PendingNewGameOptions = FString::Printf(TEXT("?WETutorial=%d"), bTutorial ? 1 : 0);
	if (!SlotName.IsEmpty())
	{
		PendingNewGameOptions += FString::Printf(TEXT("?WESlot=%s"), *SlotName);
	}
	HostGame(FString(), bLAN);
}

void UWESessionSubsystem::HostGame(const FString& Slot, bool bLAN)
{
	PendingSlot = Slot;
	bPendingLAN = bLAN || GetService() != EWEOnlineService::Steam;
	bHostedLAN = bPendingLAN;
	SetState(EWESessionState::Hosting, LOCTEXT("Hosting", "Spiel wird erstellt …"));

	const IOnlineSessionPtr Sessions = GetSessions();
	if (!Sessions)
	{
		OpenGarage(PendingSlot, bPendingLAN);
		return;
	}
	if (Sessions->GetNamedSession(NAME_GameSession))
	{
		// A stale session (e.g. after a crash of the last game): create the new one when it is gone.
		Sessions->DestroySession(NAME_GameSession);
		return;
	}

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = MaxPlayers;
	Settings.NumPrivateConnections = 0;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bIsLANMatch = bPendingLAN;
	Settings.bIsDedicated = false;
	Settings.bAllowInvites = true;
	Settings.bUsesPresence = !bPendingLAN;
	Settings.bAllowJoinViaPresence = !bPendingLAN;
	Settings.bUseLobbiesIfAvailable = !bPendingLAN;
	Settings.Set(GameKey, FString(WESession::GameId), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(VersionKey, GetGameVersion(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(HostKey, GetLocalPlayerName(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(SETTING_MAPNAME, FString(TEXT("L_Garage")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	if (!Sessions->CreateSession(0, NAME_GameSession, Settings))
	{
		// No session (e.g. no network): host anyway, friends can still join by IP.
		PendingMessage = LOCTEXT("CreateFailedNow", "Sitzung konnte nicht erstellt werden – Mitspieler können nur per IP beitreten.");
		OpenGarage(PendingSlot, bPendingLAN);
	}
}

void UWESessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (State != EWESessionState::Hosting)
	{
		return;
	}
	UE_LOG(LogWENet, Log, TEXT("Create session %s: %s (%s)"), *SessionName.ToString(), bWasSuccessful ? TEXT("ok") : TEXT("failed"), bPendingLAN ? TEXT("LAN") : TEXT("Steam lobby"));
	if (!bWasSuccessful)
	{
		PendingMessage = LOCTEXT("CreateFailed", "Sitzung konnte nicht erstellt werden – Mitspieler können nur per IP beitreten.");
	}
	OpenGarage(PendingSlot, bPendingLAN);
}

void UWESessionSubsystem::OpenGarage(const FString& Slot, bool bLAN)
{
	FString Options = TEXT("listen");
	if (bLAN && GetService() == EWEOnlineService::Steam)
	{
		Options += TEXT("?bIsLanMatch"); // Steam Sockets listen on IP instead of the Steam network
	}
	if (!Slot.IsEmpty())
	{
		Options += FString::Printf(TEXT("?WELoad=%s"), *Slot);
	}
	else
	{
		Options += PendingNewGameOptions; // Phase 17.5: company name (save slot) and tutorial
	}
	PendingNewGameOptions.Reset();
	if (GetService() != EWEOnlineService::Steam)
	{
		// With Steam the engine already uses the Steam name.
		Options += FString::Printf(TEXT("?WEName=%s"), *WESession::SanitizeName(GetLocalPlayerName()));
	}
	SetState(EWESessionState::InGame, LOCTEXT("Hosted", "Spiel läuft"));
	UGameplayStatics::OpenLevel(this, FName(WESession::GarageMap), true, Options);
}

// --- Find / join ------------------------------------------------------------------------------------

void UWESessionSubsystem::FindGames(bool bLAN)
{
	const IOnlineSessionPtr Sessions = GetSessions();
	FoundGames.Reset();
	if (!Sessions)
	{
		SetState(EWESessionState::Idle, LOCTEXT("NoService", "Kein Online-Dienst – bitte per IP beitreten."));
		return;
	}
	Search = MakeShared<FOnlineSessionSearch>();
	Search->bIsLanQuery = bLAN || GetService() != EWEOnlineService::Steam;
	Search->MaxSearchResults = Search->bIsLanQuery ? 50 : 200;
	Search->TimeoutInSeconds = 4.f;
	if (!Search->bIsLanQuery)
	{
		Search->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	}
	Search->QuerySettings.Set(GameKey, FString(WESession::GameId), EOnlineComparisonOp::Equals);
	SetState(EWESessionState::Searching, Search->bIsLanQuery ? LOCTEXT("SearchingLAN", "Suche Spiele im LAN …") : LOCTEXT("SearchingSteam", "Suche Spiele über Steam …"));
	if (!Sessions->FindSessions(0, Search.ToSharedRef()))
	{
		SetState(EWESessionState::Idle, LOCTEXT("SearchFailedNow", "Suche fehlgeschlagen."));
	}
}

void UWESessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (State != EWESessionState::Searching || !Search.IsValid())
	{
		return;
	}
	FoundGames.Reset();
	const FString MyVersion = GetGameVersion();
	for (int32 i = 0; i < Search->SearchResults.Num(); ++i)
	{
		const FOnlineSessionSearchResult& Result = Search->SearchResults[i];
		FString Game;
		// Steam's test app is shared by many projects: only our own games.
		if (!Result.IsValid() || !Result.Session.SessionSettings.Get(GameKey, Game) || Game != WESession::GameId)
		{
			continue;
		}
		FWESessionEntry& Entry = FoundGames.AddDefaulted_GetRef();
		Entry.ResultIndex = i;
		Result.Session.SessionSettings.Get(VersionKey, Entry.Version);
		if (!Result.Session.SessionSettings.Get(HostKey, Entry.HostName) || Entry.HostName.IsEmpty())
		{
			Entry.HostName = Result.Session.OwningUserName;
		}
		Entry.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
		Entry.Players = FMath::Clamp(Entry.MaxPlayers - Result.Session.NumOpenPublicConnections, 1, Entry.MaxPlayers);
		Entry.PingMs = Result.PingInMs;
		Entry.bCompatible = IsCompatibleVersion(Entry.Version, MyVersion);
	}
	UE_LOG(LogWENet, Log, TEXT("Found %d session(s), %d of Workshop Empire"), Search->SearchResults.Num(), FoundGames.Num());
	SetState(EWESessionState::Idle, FoundGames.IsEmpty() && !Search->bIsLanQuery ? LOCTEXT("NoGamesSteam", "Keine Spiele gefunden. Tipp: Steam-Freunde können dich auch direkt einladen.")
		: FoundGames.IsEmpty() ? LOCTEXT("NoGames", "Keine Spiele gefunden.")
		: !bWasSuccessful ? LOCTEXT("SearchFailed", "Suche fehlgeschlagen.")
		: FText::Format(LOCTEXT("GamesFound", "{0} Spiel(e) gefunden."), FoundGames.Num()));
}

void UWESessionSubsystem::JoinGame(int32 ResultIndex)
{
	if (!Search.IsValid() || !Search->SearchResults.IsValidIndex(ResultIndex))
	{
		SetState(EWESessionState::Idle, LOCTEXT("JoinGone", "Das Spiel ist nicht mehr verfügbar – bitte neu suchen."));
		return;
	}
	const FWESessionEntry* Entry = FoundGames.FindByPredicate([ResultIndex](const FWESessionEntry& E) { return E.ResultIndex == ResultIndex; });
	if (Entry && !Entry->bCompatible)
	{
		SetState(EWESessionState::Idle, FText::Format(LOCTEXT("JoinVersion", "Andere Spielversion (Host {0}, du {1})."), FText::FromString(Entry->Version), FText::FromString(GetGameVersion())));
		return;
	}
	bPendingLAN = Search->bIsLanQuery;
	const IOnlineSessionPtr Sessions = GetSessions();
	if (Sessions && Sessions->GetNamedSession(NAME_GameSession))
	{
		PendingInvite = MakeShared<FOnlineSessionSearchResult>(Search->SearchResults[ResultIndex]);
		Sessions->DestroySession(NAME_GameSession);
		return;
	}
	DoJoin(Search->SearchResults[ResultIndex]);
}

void UWESessionSubsystem::DoJoin(const FOnlineSessionSearchResult& Result)
{
	const IOnlineSessionPtr Sessions = GetSessions();
	if (!Sessions)
	{
		return;
	}
	SetState(EWESessionState::Joining, LOCTEXT("Joining", "Trete bei …"));
	if (!Sessions->JoinSession(0, NAME_GameSession, Result))
	{
		SetState(EWESessionState::Idle, LOCTEXT("JoinFailedNow", "Beitritt fehlgeschlagen."));
	}
}

void UWESessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (State != EWESessionState::Joining)
	{
		return;
	}
	FString ConnectString;
	const IOnlineSessionPtr Sessions = GetSessions();
	if ((Result != EOnJoinSessionCompleteResult::Success && Result != EOnJoinSessionCompleteResult::AlreadyInSession)
		|| !Sessions || !Sessions->GetResolvedConnectString(SessionName, ConnectString))
	{
		const EOnJoinSessionCompleteResult::Type Shown = Result == EOnJoinSessionCompleteResult::Success ? EOnJoinSessionCompleteResult::CouldNotRetrieveAddress : Result;
		if (Sessions)
		{
			Sessions->DestroySession(SessionName);
		}
		SetState(EWESessionState::Idle, DescribeJoinResult(Shown));
		return;
	}
	APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController(GetWorld()) : nullptr;
	if (!PC)
	{
		return;
	}
	const FString URL = ConnectString + MakeJoinOptions(GetService() == EWEOnlineService::Steam ? FString() : GetLocalPlayerName(), bPendingLAN && GetService() == EWEOnlineService::Steam);
	UE_LOG(LogWENet, Log, TEXT("Joining %s"), *URL);
	SetState(EWESessionState::InGame, LOCTEXT("Connecting", "Verbinde …"));
	PC->ClientTravel(URL, TRAVEL_Absolute);
}

void UWESessionSubsystem::JoinAddress(const FString& Address)
{
	const FString Normalized = NormalizeAddress(Address);
	APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController(GetWorld()) : nullptr;
	if (Normalized.IsEmpty() || !PC)
	{
		SetState(EWESessionState::Idle, LOCTEXT("BadAddress", "Ungültige Adresse (Beispiel: 192.168.0.5 oder 192.168.0.5:7777)."));
		return;
	}
	// With Steam running the game would use the Steam network; an IP address needs plain IP.
	const FString URL = Normalized + MakeJoinOptions(GetService() == EWEOnlineService::Steam ? FString() : GetLocalPlayerName(), GetService() == EWEOnlineService::Steam);
	UE_LOG(LogWENet, Log, TEXT("Joining by address %s"), *URL);
	SetState(EWESessionState::InGame, FText::Format(LOCTEXT("ConnectingTo", "Verbinde mit {0} …"), FText::FromString(Normalized)));
	PC->ClientTravel(URL, TRAVEL_Absolute);
}

// --- Leave / invites / failures ---------------------------------------------------------------------

void UWESessionSubsystem::LeaveToMainMenu(const FText& Message)
{
	if (!Message.IsEmpty())
	{
		PendingMessage = Message;
	}
	SetState(EWESessionState::Leaving, LOCTEXT("Leaving", "Verlasse das Spiel …"));
	if (const IOnlineSessionPtr Sessions = GetSessions(); Sessions && Sessions->GetNamedSession(NAME_GameSession))
	{
		Sessions->DestroySession(NAME_GameSession);
	}
	UGameplayStatics::OpenLevel(this, FName(WESession::MenuMap), true);
}

void UWESessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogWENet, Log, TEXT("Session %s destroyed: %s"), *SessionName.ToString(), bWasSuccessful ? TEXT("ok") : TEXT("failed"));
	if (PendingInvite.IsValid())
	{
		const FOnlineSessionSearchResult Result = *PendingInvite;
		PendingInvite.Reset();
		DoJoin(Result);
	}
	else if (State == EWESessionState::Hosting)
	{
		HostGame(PendingSlot, bPendingLAN);
	}
	else if (State == EWESessionState::Leaving)
	{
		SetState(EWESessionState::Idle, FText::GetEmpty());
	}
}

bool UWESessionSubsystem::ShowInviteUI(FText& OutError)
{
	const IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld());
	if (GetService() != EWEOnlineService::Steam || !OSS || !OSS->GetExternalUIInterface())
	{
		OutError = LOCTEXT("InviteNoSteam", "Einladen geht nur mit Steam. Im LAN treten Mitspieler über \"Spiel beitreten\" bei.");
		return false;
	}
	if (bHostedLAN)
	{
		OutError = LOCTEXT("InviteLAN", "Dieses Spiel läuft im LAN-Modus – Steam-Freunde treten über die IP bei.");
		return false;
	}
	if (!OSS->GetExternalUIInterface()->ShowInviteUI(0, NAME_GameSession))
	{
		OutError = LOCTEXT("InviteFailed", "Das Steam-Overlay ist nicht verfügbar (in Steam unter Einstellungen → Im Spiel aktivieren).");
		return false;
	}
	return true;
}

void UWESessionSubsystem::OnInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	UE_LOG(LogWENet, Log, TEXT("Steam invite / join request accepted: %s"), bWasSuccessful ? TEXT("ok") : TEXT("failed"));
	if (!bWasSuccessful || !InviteResult.IsValid())
	{
		PendingMessage = LOCTEXT("InviteBroken", "Die Einladung ist nicht mehr gültig.");
		return;
	}
	bPendingLAN = false;
	const IOnlineSessionPtr Sessions = GetSessions();
	if (Sessions && Sessions->GetNamedSession(NAME_GameSession))
	{
		// Still in a game (own or someone else's): leave it first.
		PendingInvite = MakeShared<FOnlineSessionSearchResult>(InviteResult);
		Sessions->DestroySession(NAME_GameSession);
		return;
	}
	DoJoin(InviteResult);
}

void UWESessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogWENet, Warning, TEXT("Network failure %s: %s"), ENetworkFailure::ToString(FailureType), *ErrorString);
	if (!World || World->GetGameInstance() != GetGameInstance())
	{
		return;
	}
	// A message set on purpose (kicked, host ended the game) wins over the technical one.
	if (PendingMessage.IsEmpty() && State != EWESessionState::Leaving)
	{
		PendingMessage = DescribeNetworkFailure(FailureType, ErrorString);
	}
	if (const IOnlineSessionPtr Sessions = GetSessions(); Sessions && Sessions->GetNamedSession(NAME_GameSession))
	{
		Sessions->DestroySession(NAME_GameSession);
	}
	State = EWESessionState::Leaving; // the engine returns to the default map (main menu) itself
}

void UWESessionSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogWENet, Warning, TEXT("Travel failure %s: %s"), ETravelFailure::ToString(FailureType), *ErrorString);
	if (PendingMessage.IsEmpty())
	{
		PendingMessage = DescribeTravelFailure(FailureType, ErrorString);
	}
	State = EWESessionState::Leaving;
}

// --- Helpers ----------------------------------------------------------------------------------------

FString UWESessionSubsystem::GetGameVersion()
{
	return GetDefault<UGeneralProjectSettings>()->ProjectVersion;
}

bool UWESessionSubsystem::IsCompatibleVersion(const FString& Host, const FString& Client)
{
	TArray<FString> A, B;
	Host.ParseIntoArray(A, TEXT("."));
	Client.ParseIntoArray(B, TEXT("."));
	if (A.Num() < 2 || B.Num() < 2)
	{
		return false;
	}
	return A[0] == B[0] && A[1] == B[1];
}

FText UWESessionSubsystem::DescribeNetworkFailure(ENetworkFailure::Type Type, const FString& ErrorString)
{
	switch (Type)
	{
	case ENetworkFailure::PendingConnectionFailure:
		// The server's PreLogin reason ("Das Spiel ist voll …", "Andere Spielversion …") arrives here.
		if (ErrorString.Contains(TEXT("Server full")))
		{
			return LOCTEXT("PendingFull", "Beitritt abgelehnt: Das Spiel ist voll (4/4 Spieler).");
		}
		return ErrorString.IsEmpty() || ErrorString.Contains(TEXT("Your connection to the host has been lost"))
			? LOCTEXT("PendingFailed", "Beitritt fehlgeschlagen: Der Host hat nicht geantwortet.")
			: FText::Format(LOCTEXT("PendingFailedReason", "Beitritt abgelehnt: {0}"), FText::FromString(ErrorString));
	case ENetworkFailure::ConnectionTimeout:
		return LOCTEXT("Timeout", "Verbindung zum Host verloren (Zeitüberschreitung).");
	case ENetworkFailure::FailureReceived:
		return ErrorString.IsEmpty() ? LOCTEXT("Received", "Der Host hat die Verbindung beendet.")
			: FText::Format(LOCTEXT("ReceivedReason", "Verbindung beendet: {0}"), FText::FromString(ErrorString));
	case ENetworkFailure::OutdatedClient:
	case ENetworkFailure::OutdatedServer:
		return LOCTEXT("Outdated", "Andere Spielversion als der Host – bitte beide aktualisieren.");
	case ENetworkFailure::NetDriverAlreadyExists:
	case ENetworkFailure::NetDriverCreateFailure:
	case ENetworkFailure::NetDriverListenFailure:
		return LOCTEXT("NetDriver", "Netzwerk konnte nicht gestartet werden (Port belegt oder Firewall).");
	case ENetworkFailure::ConnectionLost:
	default:
		return LOCTEXT("Lost", "Verbindung zum Host verloren.");
	}
}

FText UWESessionSubsystem::DescribeTravelFailure(ETravelFailure::Type Type, const FString& ErrorString)
{
	switch (Type)
	{
	case ETravelFailure::PackageMissing:
	case ETravelFailure::NoLevel:
	case ETravelFailure::LoadMapFailure:
		return LOCTEXT("MapMissing", "Die Karte des Hosts konnte nicht geladen werden (andere Spielversion?).");
	case ETravelFailure::PendingNetGameCreateFailure:
		return LOCTEXT("NoConnect", "Verbindung zum Host konnte nicht aufgebaut werden – Adresse prüfen.");
	default:
		return FText::Format(LOCTEXT("TravelOther", "Spiel konnte nicht geladen werden: {0}"), FText::FromString(ErrorString));
	}
}

FText UWESessionSubsystem::DescribeJoinResult(EOnJoinSessionCompleteResult::Type Result)
{
	switch (Result)
	{
	case EOnJoinSessionCompleteResult::Success:
		return LOCTEXT("JoinOk", "Beitritt erfolgreich.");
	case EOnJoinSessionCompleteResult::SessionIsFull:
		return LOCTEXT("JoinFull", "Das Spiel ist voll (4 Spieler).");
	case EOnJoinSessionCompleteResult::SessionDoesNotExist:
		return LOCTEXT("JoinGone2", "Das Spiel gibt es nicht mehr – bitte neu suchen.");
	case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
		return LOCTEXT("JoinAddress", "Die Adresse des Hosts ist nicht erreichbar.");
	case EOnJoinSessionCompleteResult::AlreadyInSession:
		return LOCTEXT("JoinAlready", "Du bist schon in einem Spiel.");
	default:
		return LOCTEXT("JoinUnknown", "Beitritt fehlgeschlagen (unbekannter Fehler).");
	}
}

FString UWESessionSubsystem::MakeJoinOptions(const FString& PlayerName, bool bLAN)
{
	// "Name" is replaced by the engine with the online service's nickname; WEName is ours (AWEGameMode::InitNewPlayer).
	FString Options = FString::Printf(TEXT("?WEVersion=%s"), *GetGameVersion());
	if (!WESession::SanitizeName(PlayerName).IsEmpty())
	{
		Options += FString::Printf(TEXT("?WEName=%s"), *WESession::SanitizeName(PlayerName));
	}
	if (bLAN)
	{
		Options += TEXT("?bIsLanMatch");
	}
	return Options;
}

FString UWESessionSubsystem::NormalizeAddress(const FString& Address)
{
	const FString Trimmed = Address.TrimStartAndEnd();
	if (Trimmed.IsEmpty() || Trimmed.Contains(TEXT("?")) || Trimmed.Contains(TEXT(" ")) || Trimmed.Contains(TEXT("/")))
	{
		return FString();
	}
	FString Host = Trimmed;
	FString Port;
	if (Trimmed.Split(TEXT(":"), &Host, &Port))
	{
		if (Host.IsEmpty() || !Port.IsNumeric() || FCString::Atoi(*Port) <= 0 || FCString::Atoi(*Port) > 65535)
		{
			return FString();
		}
		return Trimmed;
	}
	return FString::Printf(TEXT("%s:%d"), *Trimmed, GamePort);
}

#undef LOCTEXT_NAMESPACE
