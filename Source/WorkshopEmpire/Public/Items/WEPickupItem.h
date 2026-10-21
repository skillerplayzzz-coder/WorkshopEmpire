#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WEInteractable.h"
#include "Items/WEItemInstance.h"
#include "WEPickupItem.generated.h"

class AWECharacter;
class UMaterialInterface;
class UStaticMeshComponent;

/**
 * World presence of an item instance: a physical object that can be picked up, examined,
 * put into the inventory (small items) or carried in the hands (bulky items).
 *
 * Replication: Item and Holder are server-owned. While held the actor is attached to the holder's
 * hold point with physics and collision off; when free, physics runs on the server and is replicated.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEPickupItem : public AActor, public IWEInteractable
{
	GENERATED_BODY()

public:
	AWEPickupItem();

	/** Server only. Spawns a world item for Item at Transform. */
	static AWEPickupItem* SpawnWorldItem(UWorld* World, const FWEItemInstance& Item, const FTransform& Transform);

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IWEInteractable
	virtual FText GetInteractionText(const AWECharacter* Interactor) const override;
	virtual bool CanInteract(const AWECharacter* Interactor, FText& OutReason) const override;
	virtual void Interact(AWECharacter* Interactor) override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;
	virtual bool GetSecondaryInteraction(const AWECharacter* Interactor, FText& OutText) const override;
	virtual void InteractSecondary(AWECharacter* Interactor) override;
	virtual bool GetAlternativeInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const override;
	virtual void InteractAlternative(AWECharacter* Interactor) override;

	/** Server only, called by AWEWorkbench. Locks the item onto the bench's work spot. */
	void HandlePlacedOnWorkbench(class AWEWorkbench* Bench, USceneComponent* AttachTo);

	/** Server only, called by AWEWorkbench. Frees the item (physics back on). */
	void HandleRemovedFromWorkbench();

	class AWEWorkbench* GetPlacedOn() const { return PlacedOn; }
	UStaticMeshComponent* GetMesh() const { return Mesh; }

	/** Server only. Opens a sealed container (return pallet): spawns its contents around it and removes it. Returns the finds. */
	TArray<FWEItemInstance> OpenContainer(AWECharacter* By);

	/** True if [E] puts this into the inventory (instead of carrying it). */
	bool GoesIntoInventory() const;

	bool CanBePickedUpBy(const AWECharacter* Character, FText& OutReason) const;

	/** Server only, called by AWECharacter::TryPickUpItem after validation. */
	void HandlePickedUp(AWECharacter* NewHolder, USceneComponent* AttachTo);

	/** Server only, called by AWECharacter::DropHeldItem. */
	void HandleDropped(const FTransform& DropTransform);

	/** Server only. Sets the item data (quantity changes after partial pickup, loading, tests). */
	void SetItem(const FWEItemInstance& NewItem);
	/** Phase 17: back from net dormancy before a replicated property changes (server). */
	void WakeForReplication();
	virtual void FellOutOfWorld(const class UDamageType& DamageType) override;
	float FallVelocityAtKill = 0.f;

	const FWEItemInstance& GetItem() const { return Item; }
	AWECharacter* GetHolder() const { return Holder; }
	bool IsHeld() const { return Holder != nullptr; }
	FText GetDisplayName() const;
	float GetWeightKg() const { return Item.GetTotalWeightKg(); }

	/** Rough radius used to keep dropped items out of walls. */
	float GetCarryRadius() const;

	static FText FormatWeight(float Kg);

protected:
	UFUNCTION()
	void HandleBodySleep(UPrimitiveComponent* SleepingComponent, FName BoneName);
	UFUNCTION()
	void HandleBodyWake(UPrimitiveComponent* WakingComponent, FName BoneName);

	virtual void BeginPlay() override;
	virtual void OnRep_AttachmentReplication() override;

	UFUNCTION()
	void OnRep_Holder();

	UFUNCTION()
	void OnRep_Item();

	/** Mesh, scale, tint and mass from the item definition. */
	void ApplyDefinitionVisuals();

	/** Toggles physics/collision for the held or free state. Runs on server and clients. */
	void ApplyHeldState(bool bHeld);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Item data. Set Definition (and condition/quantity) on placed actors; the id is generated at runtime. */
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Item, Category = "Item")
	FWEItemInstance Item;

	/** Tintable block-out material (M_WEPlaceholder). Replaced by real materials in Phase 15. */
	UPROPERTY(EditAnywhere, Category = "Item")
	TObjectPtr<UMaterialInterface> PlaceholderMaterial;

	/** Phase 21: wear overlay (dirt, scratches ...). */
	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> WearMaterial;

	UPROPERTY(ReplicatedUsing = OnRep_Holder, VisibleInstanceOnly, Category = "Item")
	TObjectPtr<AWECharacter> Holder;

	/** Workbench this item currently lies on for disassembly (server-owned). */
	UPROPERTY(ReplicatedUsing = OnRep_Holder, VisibleInstanceOnly, Category = "Item")
	TObjectPtr<class AWEWorkbench> PlacedOn;
};
