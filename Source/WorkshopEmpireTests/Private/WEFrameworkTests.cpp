#include "Misc/AutomationTest.h"
#include "Framework/WEGameMode.h"
#include "Framework/WEGameState.h"
#include "Framework/WEPlayerController.h"
#include "Framework/WEPlayerState.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEGameModeDefaultsTest, "WorkshopEmpire.Framework.GameModeDefaults",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FWEGameModeDefaultsTest::RunTest(const FString& Parameters)
{
	const AWEGameMode* GameMode = GetDefault<AWEGameMode>();
	if (!TestNotNull(TEXT("GameMode CDO"), GameMode))
	{
		return false;
	}

	TestTrue(TEXT("GameState class"), GameMode->GameStateClass == AWEGameState::StaticClass());
	TestTrue(TEXT("PlayerState class"), GameMode->PlayerStateClass == AWEPlayerState::StaticClass());
	TestTrue(TEXT("PlayerController class"), GameMode->PlayerControllerClass == AWEPlayerController::StaticClass());
	TestEqual(TEXT("Co-op limit is 4"), GameMode->GetMaxCoopPlayers(), 4);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
