#pragma once

#include "CoreMinimal.h"
#include "Items/WEItemInstance.h"

class UMaterialInstanceDynamic;
class UMeshComponent;

/** Phase 21: shows an item's wear (dirt, scratches, yellowing, visible damage) as an overlay material on its mesh. */
namespace WEWear
{
	/** Sets or clears the wear overlay (Cache keeps the material instance between calls). No-op on a dedicated server. */
	WORKSHOPEMPIRE_API void Apply(UMeshComponent* Mesh, const FWEItemInstance& Item, TObjectPtr<UMaterialInstanceDynamic>& Cache);
	/** Same for a part inside a device (e.g. dust on a PC's fan): explicit amounts. */
	WORKSHOPEMPIRE_API void ApplyAmounts(UMeshComponent* Mesh, const FWEItemWear& Wear, float Seed, TObjectPtr<UMaterialInstanceDynamic>& Cache);
}