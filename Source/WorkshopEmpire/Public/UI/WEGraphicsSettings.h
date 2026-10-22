#pragma once

#include "CoreMinimal.h"

/**
 * Graphics quality (Phase 15): one switch over the engine scalability groups (Niedrig, Mittel, Hoch, Episch),
 * stored in GameUserSettings. Used by the main menu and the pause menu.
 */
namespace WEGraphics
{
	constexpr int32 NumLevels = 4;

	/** 0 = Niedrig … 3 = Episch; mixed group settings count as the lowest group's level. */
	WORKSHOPEMPIRE_API int32 GetQuality();
	/** Applies the level (clamped) and, unless bSave is false (tests), stores it in GameUserSettings. */
	WORKSHOPEMPIRE_API void SetQuality(int32 Level, bool bSave = true);
	/** Phase 21: once per settings version (first start, or an older install that came up on "Episch" at full resolution):
	 *  "Hoch" with automatic render resolution. The player's own choice afterwards stays. */
	WORKSHOPEMPIRE_API void ApplyFirstStartDefaults();
	/** Next level, wrapping from Episch to Niedrig. */
	WORKSHOPEMPIRE_API int32 CycleQuality(bool bSave = true);
	WORKSHOPEMPIRE_API FText GetQualityName(int32 Level);
	/** "Grafik: Hoch" */
	WORKSHOPEMPIRE_API FText GetButtonLabel();
}
