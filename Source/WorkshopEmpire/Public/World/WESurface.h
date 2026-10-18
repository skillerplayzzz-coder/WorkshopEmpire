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
	Osb,
	// Phase 17.5 (append only: values are stored in data assets)
	CrackedAsphalt,
	WornAsphalt,
	Sidewalk,
	RedBrick,
	PaintedBrick,
	DarkBrick,
	RustyCorrugated,
	WornCorrugated,
	RustedShutter,
	PaintedShutter,
	RubberTiles,
	PaintedConcrete,
	AntiSlip,
	WoodFloor,
	Linoleum,
	FloorTiles,
	Plaster,
	WhitewashedBrick,
	SteelPlate,
	GreenRust,
	Gravel,
	BoxProfile,
	ConcretePanels
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

	/** Phase 17.5 (paint, epoxy floors): like Apply, plus desaturation of the texture (1 = only the tint colour) and roughness scale (< 0 = keep). */
	WORKSHOPEMPIRE_API void ApplyEx(UMeshComponent* Mesh, EWESurface Kind, const FLinearColor& Tint, float Grime, float Desaturation, float Roughness, float Metallic = -1.f);
}
