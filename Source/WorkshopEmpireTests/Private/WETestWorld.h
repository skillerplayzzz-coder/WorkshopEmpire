#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/WorldSettings.h"

/**
 * Minimal standalone game world for gameplay unit tests (authority = local, no players, no ticking).
 * Server-side gameplay rules can be exercised directly; replication itself is covered by the
 * multi-process network smoke test (see TEST_REPORT.md).
 */
struct FWETestWorld
{
	UWorld* World = nullptr;

	FWETestWorld()
	{
		World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld*/ false, TEXT("WETestWorld"));
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		Context.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
		// No GameMode in this world, so start play directly: actors spawned afterwards get BeginPlay/EndPlay
		// exactly like in a real session.
		if (AWorldSettings* Settings = World->GetWorldSettings())
		{
			Settings->NotifyBeginPlay();
		}
	}

	~FWETestWorld()
	{
		if (World)
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
			World = nullptr;
		}
	}

	template <typename T>
	T* Spawn(const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<T>(T::StaticClass(), Location, Rotation, Params);
	}
};
