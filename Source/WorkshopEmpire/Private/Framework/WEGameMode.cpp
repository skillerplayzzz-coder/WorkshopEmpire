#include "Framework/WEGameMode.h"

#include "Framework/WEGameState.h"
#include "Framework/WEPlayerController.h"
#include "Framework/WEPlayerState.h"
#include "Player/WECharacter.h"
#include "WELog.h"

AWEGameMode::AWEGameMode()
{
	GameStateClass = AWEGameState::StaticClass();
	PlayerStateClass = AWEPlayerState::StaticClass();
	PlayerControllerClass = AWEPlayerController::StaticClass();
	DefaultPawnClass = AWECharacter::StaticClass();
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
}

void AWEGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	UE_LOG(LogWENet, Log, TEXT("Player left: %s"), *GetNameSafe(Exiting));
}
