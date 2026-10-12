#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WEPlaceholderProp.generated.h"

class UInstancedStaticMeshComponent;

UENUM(BlueprintType)
enum class EWEPlaceholderPropType : uint8
{
	Workbench,
	Shelf
};

/**
 * Static block-out furniture (workbench, shelf) built from cube instances.
 * Purely visual/collision; the interactive workbench arrives in Phase 3, real assets in Phase 15.
 * Origin = floor centre, front faces +X.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEPlaceholderProp : public AActor
{
	GENERATED_BODY()

public:
	AWEPlaceholderProp();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Height of the main surface (bench top / lowest shelf board) in cm. */
	float GetSurfaceHeight() const;

protected:
	virtual void BeginPlay() override;
	void ApplyTint();
	void AddBox(const FVector& Center, const FVector& Size);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Boxes;

	UPROPERTY(EditAnywhere, Category = "Prop")
	EWEPlaceholderPropType PropType = EWEPlaceholderPropType::Workbench;

	/** X = depth, Y = width, Z = height (cm). */
	UPROPERTY(EditAnywhere, Category = "Prop")
	FVector Size = FVector(80.f, 200.f, 90.f);

	UPROPERTY(EditAnywhere, Category = "Prop")
	TObjectPtr<UMaterialInterface> PlaceholderMaterial;

	UPROPERTY(EditAnywhere, Category = "Prop")
	FLinearColor Tint = FLinearColor(0.45f, 0.30f, 0.18f);
};
