#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WEInteractable.h"
#include "WEPickupItem.generated.h"

class AWECharacter;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * A physical object in the world that can be picked up, carried, dropped and examined.
 *
 * Phase 1 prototype: name, description and weight live on the actor. From Phase 2 on these come
 * from an item definition (Data Asset) plus a per-instance state; this actor then represents the
 * world presence of such an item instance.
 *
 * Replication: Holder is server-owned. While held the item is attached to the holder's hold point
 * with physics and collision off; when dropped, physics runs on the server and is replicated.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEPickupItem : public AActor, public IWEInteractable
{
	GENERATED_BODY()

public:
	AWEPickupItem();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IWEInteractable
	virtual FText GetInteractionText(const AWECharacter* Interactor) const override;
	virtual bool CanInteract(const AWECharacter* Interactor, FText& OutReason) const override;
	virtual void Interact(AWECharacter* Interactor) override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;

	bool CanBePickedUpBy(const AWECharacter* Character, FText& OutReason) const;

	/** Server only, called by AWECharacter::TryPickUpItem after validation. */
	void HandlePickedUp(AWECharacter* NewHolder, USceneComponent* AttachTo);

	/** Server only, called by AWECharacter::DropHeldItem. */
	void HandleDropped(const FTransform& DropTransform);

	AWECharacter* GetHolder() const { return Holder; }
	bool IsHeld() const { return Holder != nullptr; }
	const FText& GetDisplayName() const { return DisplayName; }
	float GetWeightKg() const { return WeightKg; }

	/** Rough radius used to keep dropped items out of walls. */
	float GetCarryRadius() const;

	static FText FormatWeight(float Kg);

	/** Editor/test helper: sets the presentation data in one call. */
	void SetItemData(const FText& InName, const FText& InDescription, float InWeightKg);

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_AttachmentReplication() override;

	UFUNCTION()
	void OnRep_Holder();

	void ApplyTint();

	/** Toggles physics/collision for the held or free state. Runs on server and clients. */
	void ApplyHeldState(bool bHeld);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, Category = "Item")
	TObjectPtr<UStaticMesh> ItemMesh;

	UPROPERTY(EditAnywhere, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, Category = "Item", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, Category = "Item", meta = (ClampMin = "0.01", Units = "kg"))
	float WeightKg = 1.f;

	/** Tintable block-out material (M_WEPlaceholder). Replaced by real materials in Phase 15. */
	UPROPERTY(EditAnywhere, Category = "Item")
	TObjectPtr<UMaterialInterface> PlaceholderMaterial;

	/** Placeholder tint (applied to the "Color" parameter of PlaceholderMaterial). */
	UPROPERTY(EditAnywhere, Category = "Item")
	FLinearColor Tint = FLinearColor(0.6f, 0.6f, 0.6f);

	UPROPERTY(ReplicatedUsing = OnRep_Holder, VisibleInstanceOnly, Category = "Item")
	TObjectPtr<AWECharacter> Holder;
};
