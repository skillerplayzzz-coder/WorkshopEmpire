#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "World/WEPlaceholderProp.h"
#include "WEBuildComponent.generated.h"

class AWECharacter;
class UInstancedStaticMeshComponent;
struct FWEFurnitureDefinition;

/** Local, non-replicated preview of the furniture being placed (green = fits, red = does not). */
UCLASS(NotPlaceable)
class WORKSHOPEMPIRE_API AWEBuildGhost : public AActor
{
	GENERATED_BODY()

public:
	AWEBuildGhost();
	void SetShape(EWEPlaceholderPropType Type, const FVector& Size);
	void SetValid(bool bValid);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Boxes;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> Material;

	EWEPlaceholderPropType ShapeType = EWEPlaceholderPropType::Shelf;
	FVector ShapeSize = FVector::ZeroVector;
	int32 ValidState = -1;
};

/**
 * Build mode ([B], §14 "Bau- und Platzierungssystem") of the local player:
 *  - Mouse wheel: choose furniture from the catalog, or "Bearbeiten" (edit existing furniture).
 *  - A preview follows the crosshair on the floor, snapped to a 10 cm grid; [R] turns by 90°, [Q] by 15°.
 *  - [E]: place (costs money) / in edit mode pick up the aimed furniture and place it again (move).
 *  - [X]: in edit mode sell bought furniture back for half the price.
 * The preview uses the same rule check as the server; the server re-checks before booking.
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWEBuildComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEBuildComponent();

	bool IsActive() const { return bBuildMode; }
	void Toggle();
	void SetBuildMode(bool bOn);

	/** Wheel: -1 / +1 through "Bearbeiten" (index -1) and the catalog. */
	void CycleSelection(int32 Direction);
	void SelectIndex(int32 Index);
	void Rotate(float Degrees);
	/** Absolute rotation of the preview (automation, tests). */
	void SetRotation(float Degrees);
	void Confirm();
	void Remove();

	/** Re-runs the aim trace and rule check now (normally on a timer). */
	void UpdatePreview();

	// --- For HUD and tests ----------------------------------------------------------------------
	int32 GetSelectedIndex() const { return SelectedIndex; }
	const FWEFurnitureDefinition* GetSelectedDefinition() const;
	AWEPlaceholderProp* GetMovingProp() const { return MovingProp.Get(); }
	AWEPlaceholderProp* GetHoveredProp() const { return HoveredProp.Get(); }
	bool IsPreviewValid() const { return bPreviewValid; }
	bool HasPreview() const { return bHasPreview; }
	const FTransform& GetPreviewTransform() const { return PreviewTransform; }
	FText GetStatusText() const;
	/** Lines for the HUD panel (catalog with prices, current choice marked). */
	TArray<FString> GetMenuLines() const;

	/** Tests / automation: aims the preview at a world location instead of the crosshair. */
	void SetAimOverride(const FVector& Location) { AimOverride = Location; bUseAimOverride = true; }
	void ClearAimOverride() { bUseAimOverride = false; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	AWECharacter* GetCharacter() const;
	bool TraceAim(FVector& OutLocation, AActor*& OutHitActor) const;
	void DestroyGhost();

	bool bBuildMode = false;
	int32 SelectedIndex = INDEX_NONE;
	float Yaw = 0.f;
	TWeakObjectPtr<AWEPlaceholderProp> MovingProp;
	TWeakObjectPtr<AWEPlaceholderProp> HoveredProp;
	bool bHasPreview = false;
	bool bPreviewValid = false;
	FText Reason;
	FTransform PreviewTransform;
	FVector AimOverride = FVector::ZeroVector;
	bool bUseAimOverride = false;

	UPROPERTY(Transient)
	TObjectPtr<AWEBuildGhost> Ghost;

	FTimerHandle PreviewTimer;
};
