#pragma once

#include "CoreMinimal.h"

/**
 * Phase 17.5: player settings of the settings page (main menu and [Esc] menu). Engine values (resolution, window
 * mode, VSync, frame limit, graphics quality) live in GameUserSettings; the game's own values (UI scale, mouse,
 * field of view, volume) in GameUserSettings.ini, section [WorkshopEmpire.Settings]. Applied at start and on change.
 */
namespace WESettings
{
	struct FValues
	{
		float UIScale = 1.f;            // 0.8 - 1.3
		float MouseSensitivity = 1.f;   // 0.2 - 3
		bool bInvertY = false;
		float FieldOfView = 90.f;       // 70 - 110
		float MasterVolume = 1.f;       // 0 - 1
		FIntPoint Resolution = FIntPoint(1920, 1080);
		int32 WindowMode = 0;           // 0 fullscreen, 1 windowed fullscreen, 2 window
		bool bVSync = true;
		float FrameLimit = 0.f;         // 0 = unlimited
	};

	WORKSHOPEMPIRE_API FValues Load();
	/** Stores and applies everything (engine settings with ApplySettings). */
	WORKSHOPEMPIRE_API void Save(const FValues& Values);
	/** Applies the game's own values (start of the game; tests may skip the engine part). */
	WORKSHOPEMPIRE_API void ApplyGameValues(const FValues& Values);

	WORKSHOPEMPIRE_API TArray<FIntPoint> GetResolutions();
	WORKSHOPEMPIRE_API FText GetWindowModeName(int32 Mode);
	WORKSHOPEMPIRE_API FText GetFrameLimitName(float Limit);
}
