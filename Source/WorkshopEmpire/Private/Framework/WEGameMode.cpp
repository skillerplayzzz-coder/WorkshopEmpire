#include "Framework/WEGameMode.h"
#include "Framework/WEGameRulesComponent.h"

#include "Economy/WECompanyComponent.h"
#include "Framework/WEGameState.h"
#include "Progression/WEProgressionComponent.h"
#include "Progression/WETutorialComponent.h"
#include "Dev/WEIconBaker.h"
#include "Framework/WEPlayerController.h"
#include "Framework/WEPlayerState.h"
#include "GameFramework/GameSession.h"
#include "Kismet/GameplayStatics.h"
#include "Online/WESessionSubsystem.h"
#include "Player/WECharacter.h"
#include "Save/WESaveGame.h"
#include "Save/WESaveSystem.h"
#include "Engine/NetDriver.h"
#include "EngineUtils.h"
#include "HAL/PlatformMemory.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "TimerManager.h"
#include "WELog.h"

#define LOCTEXT_NAMESPACE "WEGameMode"

AWEGameMode::AWEGameMode()
{
	GameStateClass = AWEGameState::StaticClass();
	PlayerStateClass = AWEPlayerState::StaticClass();
	PlayerControllerClass = AWEPlayerController::StaticClass();
	DefaultPawnClass = AWECharacter::StaticClass();
}

void AWEGameMode::InitGameState()
{
	Super::InitGameState();
	if (const AWEGameState* WEGameState = GetGameState<AWEGameState>(); WEGameState && WEGameState->GetCompany())
	{
		WEGameState->GetCompany()->SetBalance(FWEMoney::FromCents(StartingCapitalCents));
	}
}

void AWEGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	StartSlot = UGameplayStatics::ParseOption(Options, TEXT("WELoad"));
	// Phase 17.5: a new company from the main menu - its save slot (company name) and whether the tutorial runs.
	const FString NewSlot = UGameplayStatics::ParseOption(Options, TEXT("WESlot"));
	if (StartSlot.IsEmpty() && FWESaveSystem::IsValidSlotName(NewSlot))
	{
		CurrentSlot = NewSlot;
	}
	bStartTutorial = StartSlot.IsEmpty() && UGameplayStatics::ParseOption(Options, TEXT("WETutorial")) == TEXT("1");
	// Phase 21: the difficulty chosen for a new company (a loaded save brings its own).
	bHasNewRules = StartSlot.IsEmpty() && FWEGameRules::FromOption(UGameplayStatics::ParseOption(Options, TEXT("WERules")), NewRules);
}

FString AWEGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	const FString Error = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
	// The name chosen in the menu (LAN) or the Steam name; the engine's "Name" is the service nickname.
	const FString Chosen = UGameplayStatics::ParseOption(Options, TEXT("WEName")).Left(20);
	if (!Chosen.IsEmpty())
	{
		ChangeName(NewPlayerController, Chosen, false);
	}
	return Error;
}

#if !UE_BUILD_SHIPPING
void AWEGameMode::LogServerMonitor()
{
	const double Now = FPlatformTime::Seconds();
	const uint64 Frames = GFrameCounter - MonitorFrame;
	const double Seconds = FMath::Max(0.001, Now - MonitorTime);
	MonitorTime = Now;
	MonitorFrame = GFrameCounter;
	const UNetDriver* Driver = GetWorld()->GetNetDriver();
	const uint32 InTotal = Driver ? Driver->InTotalBytes : 0u;
	const uint32 OutTotal = Driver ? Driver->OutTotalBytes : 0u;
	const double InRate = (InTotal - MonitorInBytes) / Seconds;
	const double OutRate = (OutTotal - MonitorOutBytes) / Seconds;
	MonitorInBytes = InTotal;
	MonitorOutBytes = OutTotal;
	int32 Items = 0;
	FString Names;
	for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It)
	{
		++Items;
		if (bMonitorListItems) { Names += FString::Printf(TEXT("%s@%s, "), *It->GetItem().GetDisplayName().ToString(), *It->GetActorLocation().ToCompactString()); }
	}
	if (bMonitorListItems && GetNumPlayers() > 0)
	{
		bMonitorListItems = false;
		UE_LOG(LogWENet, Display, TEXT("MONITOR items: %s"), *Names);
	}
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	UE_LOG(LogWENet, Display, TEXT("MONITOR players=%d frame=%.2f ms (%.0f fps) in=%.0f B/s out=%.0f B/s memory=%.0f MB items=%d balance=%s"),
		GetNumPlayers(), Seconds * 1000.0 / FMath::Max<uint64>(1, Frames), Frames / Seconds, InRate, OutRate,
		FPlatformMemory::GetStats().UsedPhysical / (1024.0 * 1024.0), Items, Company ? *Company->GetBalance().ToDisplayString() : TEXT("-"));
}
#endif

void AWEGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (bHasNewRules)
	{
		if (UWEGameRulesComponent* Rules = UWEGameRulesComponent::Get(this))
		{
			Rules->SetRules(NewRules);
		}
	}
	if (!StartSlot.IsEmpty())
	{
		// The host chose a save in the main menu: load it once the level runs.
		FTimerHandle LoadTimer;
		GetWorldTimerManager().SetTimer(LoadTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { LoadFromSlot(StartSlot); }), 0.2f, false);
	}
	else if (bStartTutorial)
	{
		if (UWETutorialComponent* Tutorial = UWETutorialComponent::Get(this))
		{
			Tutorial->Start();
		}
	}
	if (AWEIconBaker::IsRequested())
	{
		GetWorld()->SpawnActor<AWEIconBaker>();
	}
	if (AutosaveIntervalSeconds > 0.f)
	{
		GetWorldTimerManager().SetTimer(AutosaveTimer, this, &AWEGameMode::Autosave, AutosaveIntervalSeconds, true);
#if !UE_BUILD_SHIPPING
	// Phase 17 QA: -WEServerMonitor logs server frame time, traffic and memory every 10 s (stress and soak tests).
	if (FParse::Param(FCommandLine::Get(), TEXT("WEServerMonitor")))
	{
		MonitorTime = FPlatformTime::Seconds();
		MonitorFrame = GFrameCounter;
		GetWorldTimerManager().SetTimer(MonitorTimer, this, &AWEGameMode::LogServerMonitor, 10.f, true);
		// -WESpawnItems=N: a yard full of loose items for traffic measurements.
		int32 SpawnItems = 0;
		if (FParse::Value(FCommandLine::Get(), TEXT("WESpawnItems="), SpawnItems))
		{
			UWEItemDefinition* Scrap = UWEItemDefinition::FindById(TEXT("SteelScrap"));
			for (int32 i = 0; Scrap && i < SpawnItems; ++i)
			{
				AWEPickupItem::SpawnWorldItem(GetWorld(), FWEItemInstance::Make(Scrap, 1), FTransform(FVector(500.f + (i % 20) * 40.f, -400.f + (i / 20) * 40.f, 30.f)));
			}
		}
	}
#endif
	}
}

void AWEGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (ErrorMessage.IsEmpty())
	{
		ErrorMessage = CheckLogin(GetNumPlayers(), MaxCoopPlayers, Options);
		if (!ErrorMessage.IsEmpty())
		{
			UE_LOG(LogWENet, Warning, TEXT("Rejected login from %s: %s"), *Address, *ErrorMessage);
		}
	}
}

void AWEGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	const FString Name = NewPlayer && NewPlayer->PlayerState ? NewPlayer->PlayerState->GetPlayerName() : FString();
	UE_LOG(LogWENet, Log, TEXT("Player joined: %s '%s' (%d/%d)"), *GetNameSafe(NewPlayer), *Name, GetNumPlayers(), MaxCoopPlayers);
	if (AWEPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<AWEPlayerState>() : nullptr)
	{
		PS->SetIsHost(NewPlayer->IsLocalController());
	}

	// A player who left this session earlier gets their place and inventory back; otherwise the
	// last loaded save knows them.
	FWESavedPlayer Departed;
	if (TakeDepartedPlayer(FWESaveSystem::GetPlayerKey(NewPlayer), Name, Departed))
	{
		FWESaveSystem::ApplySavedPlayer(NewPlayer, Departed);
		UE_LOG(LogWENet, Log, TEXT("Restored returning player '%s' (%d inventory slots)"), *Name, Departed.Inventory.Num());
	}
	else if (LastLoadedSave)
	{
		FWESaveSystem::ApplyPlayer(NewPlayer, LastLoadedSave);
	}
	if (NewPlayer && !NewPlayer->IsLocalController())
	{
		BroadcastMessage(FText::Format(LOCTEXT("Joined", "{0} ist beigetreten ({1}/{2})"), FText::FromString(Name), GetNumPlayers(), MaxCoopPlayers));
	}
}

void AWEGameMode::Logout(AController* Exiting)
{
	APlayerController* PC = Cast<APlayerController>(Exiting);
	const FString Name = PC && PC->PlayerState ? PC->PlayerState->GetPlayerName() : FString();
	const bool bRemote = PC && !PC->IsLocalController();
	// (What the player had was remembered in AWEPlayerController::PawnLeavingGame, before the pawn went.)
	Super::Logout(Exiting);
	UE_LOG(LogWENet, Log, TEXT("Player left: %s '%s'"), *GetNameSafe(Exiting), *Name);
	if (bRemote && !bEndingSession)
	{
		BroadcastMessage(FText::Format(LOCTEXT("Left", "{0} hat das Spiel verlassen"), FText::FromString(Name)));
	}
}

FString AWEGameMode::CheckLogin(int32 NumPlayers, int32 MaxPlayers, const FString& Options)
{
	if (NumPlayers >= MaxPlayers)
	{
		return FString::Printf(TEXT("Das Spiel ist voll (%d/%d Spieler)."), NumPlayers, MaxPlayers);
	}
	// Joins from the menu carry the client version; a direct console join without it is allowed.
	const FString ClientVersion = UGameplayStatics::ParseOption(Options, TEXT("WEVersion"));
	const FString HostVersion = UWESessionSubsystem::GetGameVersion();
	if (!ClientVersion.IsEmpty() && !UWESessionSubsystem::IsCompatibleVersion(HostVersion, ClientVersion))
	{
		return FString::Printf(TEXT("Andere Spielversion (Host %s, du %s)."), *HostVersion, *ClientVersion);
	}
	return FString();
}

void AWEGameMode::RememberDepartedPlayer(const FWESavedPlayer& Player)
{
	if (Player.PlayerKey.IsEmpty())
	{
		return;
	}
	DepartedPlayers.RemoveAll([&Player](const FWESavedPlayer& P) { return P.PlayerKey == Player.PlayerKey; });
	DepartedPlayers.Add(Player);
}

bool AWEGameMode::TakeDepartedPlayer(const FString& Key, const FString& Name, FWESavedPlayer& OutPlayer)
{
	const FWESavedPlayer* Found = FWESaveSystem::FindPlayer(DepartedPlayers, Key, Name);
	if (!Found)
	{
		return false;
	}
	OutPlayer = *Found;
	const FString FoundKey = Found->PlayerKey;
	DepartedPlayers.RemoveAll([&FoundKey](const FWESavedPlayer& P) { return P.PlayerKey == FoundKey; });
	return true;
}

bool AWEGameMode::KickPlayer(APlayerController* KickedPlayer)
{
	if (!KickedPlayer || KickedPlayer->IsLocalController() || !GameSession)
	{
		return false;
	}
	const FString Name = KickedPlayer->PlayerState ? KickedPlayer->PlayerState->GetPlayerName() : FString();
	if (AWEPlayerController* WEPC = Cast<AWEPlayerController>(KickedPlayer))
	{
		WEPC->ClientSessionEnded(LOCTEXT("Kicked", "Der Host hat dich aus dem Spiel entfernt."));
	}
	const bool bKicked = GameSession->KickPlayer(KickedPlayer, LOCTEXT("KickReason", "Vom Host entfernt"));
	UE_LOG(LogWENet, Log, TEXT("Kick '%s': %s"), *Name, bKicked ? TEXT("ok") : TEXT("failed"));
	return bKicked;
}

void AWEGameMode::EndSessionForClients()
{
	bEndingSession = true;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AWEPlayerController* PC = Cast<AWEPlayerController>(It->Get());
		if (PC && !PC->IsLocalController())
		{
			PC->ClientSessionEnded(LOCTEXT("HostEnded", "Der Host hat das Spiel beendet."));
		}
	}
}

void AWEGameMode::BroadcastMessage(const FText& Message)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(It->Get()))
		{
			PC->ClientShowMessage(Message);
		}
	}
}

bool AWEGameMode::SaveToSlot(const FString& Slot)
{
	// The "save" guide step counts as done *in* the save, so loading it does not ask to save again.
	UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	const int32 GuideBefore = Progression ? Progression->GetGuideStep() : 0;
	if (Progression)
	{
		Progression->NotifyEvent(EWEProgressEvent::GameSaved);
	}
	FText Error;
	if (FWESaveSystem::SaveWorld(GetWorld(), Slot, Error))
	{
		if (Slot != TEXT("Autosave"))
		{
			CurrentSlot = Slot;
		}
		BroadcastMessage(FText::Format(LOCTEXT("Saved", "Gespeichert: {0}"), FText::FromString(Slot)));
		return true;
	}
	if (Progression)
	{
		Progression->SetGuideStep(GuideBefore);
	}
	BroadcastMessage(FText::Format(LOCTEXT("SaveFailed", "Speichern fehlgeschlagen: {0}"), Error));
	return false;
}

bool AWEGameMode::LoadFromSlot(const FString& Slot)
{
	FText Error;
	UWESaveGame* Loaded = nullptr;
	if (FWESaveSystem::LoadWorld(GetWorld(), Slot, Error, &Loaded))
	{
		LastLoadedSave = Loaded;
		DepartedPlayers.Reset(); // the save knows everybody now
		if (Slot != TEXT("Autosave"))
		{
			CurrentSlot = Slot;
		}
		BroadcastMessage(FText::Format(LOCTEXT("Loaded", "Geladen: {0}"), FText::FromString(Slot)));
		return true;
	}
	BroadcastMessage(FText::Format(LOCTEXT("LoadFailed", "Laden fehlgeschlagen: {0}"), Error));
	return false;
}

void AWEGameMode::Autosave()
{
	FText Error;
	if (FWESaveSystem::SaveWorld(GetWorld(), TEXT("Autosave"), Error))
	{
		BroadcastMessage(LOCTEXT("Autosaved", "Automatisch gespeichert"));
	}
	else
	{
		UE_LOG(LogWorkshopEmpire, Warning, TEXT("Autosave failed: %s"), *Error.ToString());
	}
}

#undef LOCTEXT_NAMESPACE
