#pragma once

#include "CoreMinimal.h"
#include "WESurface.generated.h"

class UMeshComponent;

/**
 * Realistic surfaces (Phase 15): each kind is a material instance of M_WESurface with CC0 textures
 * (/Game/WorkshopEmpire/Materials/Surfaces/MI_WE<Kind>, built by build_surfaces.py). Until a kind's
 * instance exists the old tinted placeholder material is used, so nothing breaks.
 */
UENUM(BlueprintType)
enum class EWESurface : uint8
{
	/** Flat tinted placeholder (Phase 1 look). */
	Placeholder,
	ConcreteFloor,
	Asphalt,
	PaintedWall,
	ConcreteWall,
	Brick,
	BlockWall,
	Corrugated,
	FactoryWall,
	RoofMetal,
	PaintedMetal,
	RustyMetal,
	TreadPlate,
	Plywood,
	Osb
};

namespace WESurface
{
	/** Asset path of the material instance of Kind (may not exist yet). */
	WORKSHOPEMPIRE_API FString GetInstancePath(EWESurface Kind);

	/**
	 * Gives Mesh a transient dynamic instance of the surface, tinted with Tint (multiplied with the
	 * texture; white = texture colours). Falls back to the placeholder material with Tint as colour.
	 */
	WORKSHOPEMPIRE_API void Apply(UMeshComponent* Mesh, EWESurface Kind, const FLinearColor& Tint, float Grime = -1.f);
}
