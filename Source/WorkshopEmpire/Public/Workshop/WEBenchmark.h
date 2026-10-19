#pragma once

#include "CoreMinimal.h"

class UWEItemDefinition;
struct FWEItemInstance;

/**
 * Phase 19: performance of a device with a part layout (a PC), from the parts that are in it:
 * graphics card, processor, memory and drives add their BenchmarkPoints (per piece), weighted by the
 * condition and quality of the part. The same device always gets the same score, so customers can ask
 * for a minimum and the order board can check it.
 */
namespace WEBenchmark
{
	/** Item that sits in Part (the variant if one was put in), or nullptr. */
	WORKSHOPEMPIRE_API const UWEItemDefinition* GetPartItem(const FWEItemInstance& Device, int32 Part);

	/** Points of the parts as they are, ignoring their condition (what the parts are worth for speed). */
	WORKSHOPEMPIRE_API int32 RawScore(const FWEItemInstance& Device);

	/** The score a working device reaches (0 for a broken or untested one, or a device without a part layout). */
	WORKSHOPEMPIRE_API int32 Score(const FWEItemInstance& Device);

	/** Score with the device's normal parts in good order: the reference for the value of an upgraded device. */
	WORKSHOPEMPIRE_API int32 StockScore(const UWEItemDefinition* DeviceDef);

	/** Value factor of a working device with better parts (1 = normal parts). */
	WORKSHOPEMPIRE_API float ValueFactor(const FWEItemInstance& Device);

	/** 1-5 stars for a delivered PC: how far it beats the customer's minimum. */
	WORKSHOPEMPIRE_API int32 StarsFor(int32 InScore, int32 Required);

	/** "1.550" - points written the German way, whatever the system language. */
	WORKSHOPEMPIRE_API FText PointsText(int32 Points);
}
