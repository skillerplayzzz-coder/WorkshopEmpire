#pragma once

#include "CoreMinimal.h"

class USkeletalMeshComponent;
class USkeletalMesh;

/**
 * Animated bodies for players and employees (Phase 15): the UE mannequins Manny/Quinn from the engine
 * templates (Content/Characters/Mannequins, Unreal Engine EULA) with the unarmed locomotion AnimBP
 * (idle/walk/jog/jump driven by the character movement, foot IK).
 */
namespace WEMannequin
{
	inline constexpr const TCHAR* MannyPath = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple");
	inline constexpr const TCHAR* QuinnPath = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple");
	inline constexpr const TCHAR* AnimBlueprintPath = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed");

	/** Constructor setup of ACharacter::GetMesh(): placement in the capsule, no collision, no pose on servers. */
	WORKSHOPEMPIRE_API void Configure(USkeletalMeshComponent* Mesh, float CapsuleHalfHeight);

	/** Runtime: Manny or Quinn plus the AnimBP (loaded on first use). */
	WORKSHOPEMPIRE_API void ApplyBody(USkeletalMeshComponent* Mesh, bool bQuinn);

	/** Clothes colour ("Paint Tint" of the torso), the rest a darker shade of it. */
	WORKSHOPEMPIRE_API void Tint(USkeletalMeshComponent* Mesh, const FLinearColor& Clothes);

	/** Colour of player number Index (0-3: orange, blue, green, violet). */
	WORKSHOPEMPIRE_API FLinearColor PlayerColor(int32 Index);
}
