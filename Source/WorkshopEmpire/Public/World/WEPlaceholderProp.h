#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WEInteractable.h"
#include "WEPlaceholderProp.generated.h"

class UInstancedStaticMeshComponent;
class UWEInventoryComponent;
struct FWEFurnitureDefinition;

UENUM(BlueprintType)
enum class EWEPlaceholderPropType : uint8
{
	Workbench,
	Shelf,
	/** Open crate (Phase 8). */
	Crate,
	/** Industrial machine: body, hopper, control box (Phase 9). */
	Machine,
	/** Conveyor belt segment along +X (Phase 10). */
	Conveyor
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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Builds the block-out geometry for a furniture type (also used by the build-mode preview). */
	static void BuildBoxes(UInstancedStaticMeshComponent* ISM, EWEPlaceholderPropType Type, const FVector& InSize);

	// --- Furniture (Phase 8) ------------------------------------------------------------------------

	/** Server: turns this actor into the given catalog furniture (spawned by the build system). */
	void ApplyFurniture(const FWEFurnitureDefinition& Definition);

	/** Catalog id of bought furniture; None = part of the map (can be moved, not sold). */
	FName GetFurnitureId() const { return FurnitureId; }
	bool IsPlayerPlaced() const { return !FurnitureId.IsNone(); }

	FVector GetSize() const { return Size; }
	EWEPlaceholderPropType GetPropType() const { return PropType; }
	FText GetFurnitureName() const;

	/** Height of the main surface (bench top / lowest shelf board) in cm. */
	float GetSurfaceHeight() const;

	bool HasStorage() const { return bHasStorage && bUnlocked; }

	/** Upgrade id this furniture needs (None = always there). */
	FName GetRequiredUpgrade() const { return RequiredUpgrade; }

	/** Shows/hides the furniture and its collision (bought upgrade). Any machine. */
	void SetUnlocked(bool bInUnlocked);
	bool IsUnlocked() const { return bUnlocked; }

	/** Extra storage slots from upgrades (applied on every machine). */
	void SetStorageBonusSlots(int32 Bonus);
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

	/** Rebuilds geometry and storage configuration from the current properties. */
	virtual void RebuildVisuals();

	/** Subclasses copy extra furniture settings (e.g. bench speed). */
	virtual void ApplyFurnitureExtras(const FWEFurnitureDefinition& Definition) {}

	UFUNCTION()
	void OnRep_FurnitureId();

	/** Bought furniture: catalog id (replicated so clients build the same geometry and storage). */
	UPROPERTY(ReplicatedUsing = OnRep_FurnitureId, VisibleInstanceOnly, Category = "Prop")
	FName FurnitureId;

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

	/** If set, the furniture is hidden until the company owns this upgrade. */
	UPROPERTY(EditAnywhere, Category = "Prop")
	FName RequiredUpgrade;

	bool bUnlocked = true;
	int32 StorageBonusSlots = 0;
};
