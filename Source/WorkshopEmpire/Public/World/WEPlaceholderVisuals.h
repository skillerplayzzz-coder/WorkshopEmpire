#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
class UMeshComponent;

/** Shared helpers for block-out geometry (Phase 1 placeholders until real assets arrive). */
namespace WEPlaceholder
{
	/** Asset path of the tintable placeholder material (vector parameter "Color"). */
	inline const TCHAR* MaterialPath = TEXT("/Game/WorkshopEmpire/Materials/M_WEPlaceholder.M_WEPlaceholder");

	/**
	 * Gives Mesh a transient dynamic instance of BaseMaterial tinted with Color.
	 * Falls back to the mesh's own material when BaseMaterial is null. Never saved into maps.
	 */
	WORKSHOPEMPIRE_API void ApplyTint(UMeshComponent* Mesh, UMaterialInterface* BaseMaterial, const FLinearColor& Color);
}
