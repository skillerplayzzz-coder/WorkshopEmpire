#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WEGarageBuilding.generated.h"

class UInstancedStaticMeshComponent;

/**
 * Parametric placeholder garage built from cube instances (floor, walls, roof, door opening).
 *
 * Origin = centre of the interior floor. The door opening is in the +X wall. Replace with modelled
 * assets in Phase 15; gameplay code must not depend on this class.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEGarageBuilding : public AActor
{
	GENERATED_BODY()

public:
	AWEGarageBuilding();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Local transform of the door opening (floor centre, facing +X). */
	FTransform GetDoorTransform() const;

protected:
	virtual void BeginPlay() override;
	void ApplyAllTints();
	void AddBox(UInstancedStaticMeshComponent* ISM, const FVector& Center, const FVector& Size) const;
	void ApplyTint(UInstancedStaticMeshComponent* ISM, const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> FloorMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> WallMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> RoofMeshes;

	UPROPERTY(EditAnywhere, Category = "Garage", meta = (Units = "cm"))
	float InnerDepth = 800.f;

	UPROPERTY(EditAnywhere, Category = "Garage", meta = (Units = "cm"))
	float InnerWidth = 600.f;

	UPROPERTY(EditAnywhere, Category = "Garage", meta = (Units = "cm"))
	float InnerHeight = 300.f;

	UPROPERTY(EditAnywhere, Category = "Garage", meta = (Units = "cm"))
	float WallThickness = 20.f;

	UPROPERTY(EditAnywhere, Category = "Garage", meta = (Units = "cm"))
	float DoorWidth = 400.f;

	UPROPERTY(EditAnywhere, Category = "Garage", meta = (Units = "cm"))
	float DoorHeight = 250.f;

	UPROPERTY(EditAnywhere, Category = "Garage")
	TObjectPtr<UMaterialInterface> PlaceholderMaterial;

	UPROPERTY(EditAnywhere, Category = "Garage")
	FLinearColor FloorColor = FLinearColor(0.16f, 0.16f, 0.15f);

	UPROPERTY(EditAnywhere, Category = "Garage")
	FLinearColor WallColor = FLinearColor(0.36f, 0.33f, 0.28f);

	UPROPERTY(EditAnywhere, Category = "Garage")
	FLinearColor RoofColor = FLinearColor(0.12f, 0.12f, 0.13f);
};
