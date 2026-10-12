#include "Framework/WEGameMode.h"

#include "Framework/WEGameState.h"
#include "Framework/WEPlayerController.h"
#include "Framework/WEPlayerState.h"
#include "Player/WECharacter.h"
#include "Save/WESaveGame.h"
#include "Save/WESaveSystem.h"
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

void AWEGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (AutosaveIntervalSeconds > 0.f)
	{
		GetWorldTimerManager().SetTimer(AutosaveTimer, this, &AWEGameMode::Autosave, AutosaveIntervalSeconds, true);
	}
}

void AWEGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (ErrorMessage.IsEmpty() && GetNumPlayers() >= MaxCoopPlayers)
	{
		ErrorMessage = FString::Printf(TEXT("Session is full (%d/%d players)."), GetNumPlayers(), MaxCoopPlayers);
		UE_LOG(LogWENet, Warning, TEXT("Rejected login from %s: %s"), *Address, *ErrorMessage);
	}
}

void AWEGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	UE_LOG(LogWENet, Log, TEXT("Player joined: %s (%d/%d)"), *GetNameSafe(NewPlayer), GetNumPlayers(), MaxCoopPlayers);

	// A player rejoining after a load gets their saved position and inventory back.
	if (LastLoadedSave)
	{
		FWESaveSystem::ApplyPlayer(NewPlayer, LastLoadedSave);
	}
}

void AWEGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	UE_LOG(LogWENet, Log, TEXT("Player left: %s"), *GetNameSafe(Exiting));
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
	FText Error;
	if (FWESaveSystem::SaveWorld(GetWorld(), Slot, Error))
	{
		BroadcastMessage(FText::Format(LOCTEXT("Saved", "Gespeichert: {0}"), FText::FromString(Slot)));
		return true;
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
