#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WEInteractable.h"
#include "WEPlaceholderProp.generated.h"

class UInstancedStaticMeshComponent;
class UWEInventoryComponent;

UENUM(BlueprintType)
enum class EWEPlaceholderPropType : uint8
{
	Workbench,
	Shelf
};

/**
 * Block-out furniture (workbench, shelf) built from cube instances, optionally with storage
 * (Werkbanklager / Regallager). With storage, [E] opens the container UI next to the player inventory.
 * The interactive workbench arrives in Phase 3, real assets in Phase 15.
 * Origin = floor centre, front faces +X.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEPlaceholderProp : public AActor, public IWEInteractable
{
	GENERATED_BODY()

public:
	AWEPlaceholderProp();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Height of the main surface (bench top / lowest shelf board) in cm. */
	float GetSurfaceHeight() const;

	bool HasStorage() const { return bHasStorage; }
	UWEInventoryComponent* GetStorage() const { return Storage; }

	// IWEInteractable
	virtual FText GetInteractionText(const AWECharacter* Interactor) const override;
	virtual bool CanInteract(const AWECharacter* Interactor, FText& OutReason) const override;
	virtual void Interact(AWECharacter* Interactor) override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;

protected:
	virtual void BeginPlay() override;
	void ApplyTint();
	void AddBox(const FVector& Center, const FVector& Size);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Boxes;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UWEInventoryComponent> Storage;

	UPROPERTY(EditAnywhere, Category = "Prop")
	EWEPlaceholderPropType PropType = EWEPlaceholderPropType::Workbench;

	/** X = depth, Y = width, Z = height (cm). */
	UPROPERTY(EditAnywhere, Category = "Prop")
	FVector Size = FVector(80.f, 200.f, 90.f);

	UPROPERTY(EditAnywhere, Category = "Prop")
	TObjectPtr<UMaterialInterface> PlaceholderMaterial;

	UPROPERTY(EditAnywhere, Category = "Prop")
	FLinearColor Tint = FLinearColor(0.45f, 0.30f, 0.18f);

	UPROPERTY(EditAnywhere, Category = "Storage")
	bool bHasStorage = false;

	UPROPERTY(EditAnywhere, Category = "Storage", meta = (EditCondition = "bHasStorage"))
	FText StorageName;

	UPROPERTY(EditAnywhere, Category = "Storage", meta = (EditCondition = "bHasStorage", ClampMin = "1"))
	int32 StorageSlots = 12;

	UPROPERTY(EditAnywhere, Category = "Storage", meta = (EditCondition = "bHasStorage", Units = "kg"))
	float StorageMaxWeightKg = 150.f;
};
