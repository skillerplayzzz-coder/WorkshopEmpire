#pragma once

#include "CoreMinimal.h"
#include "Items/WEItemInstance.h"
#include "World/WEPlaceholderProp.h"
#include "WEConveyor.generated.h"

class AWEPickupItem;
class UStaticMeshComponent;

/** One stack lying on a conveyor. */
USTRUCT()
struct WORKSHOPEMPIRE_API FWEConveyorItem
{
	GENERATED_BODY()

	UPROPERTY() FWEItemInstance Item;

	/** Distance from the belt start in cm (0 .. length). */
	UPROPERTY() float Position = 0.f;
};

/**
 * Conveyor belt segment (§13.4 Automatisierung, Phase 10). Placed with the build mode like furniture;
 * it runs along its +X axis (start at the back, end at the front).
 *
 * Ports: the end hands items to whatever stands right in front of it (another conveyor, a machine input,
 * a storage); the start takes products from a machine output or a material store right behind it.
 * Segments joined end to start (also at angles) form a line. Items are data, not actors: the server moves
 * them in the flow step of UWEPowerGridComponent; clients draw them and move them on smoothly between
 * updates. A blocked end simply stops the belt behind it (items keep their spacing).
 *
 * [E] carried item: put it on the belt. [E] empty-handed: take the front item. [Q] switch on/off.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEConveyor : public AWEPlaceholderProp
{
	GENERATED_BODY()

public:
	AWEConveyor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IWEInteractable
	virtual FText GetInteractionText(const AWECharacter* Interactor) const override;
	virtual bool CanInteract(const AWECharacter* Interactor, FText& OutReason) const override;
	virtual void Interact(AWECharacter* Interactor) override;
	virtual bool GetAlternativeInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const override;
	virtual void InteractAlternative(AWECharacter* Interactor) override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;

	static constexpr float SpeedCmPerSecond = 60.f;
	static constexpr float ItemSpacing = 35.f;
	static constexpr float PowerKw = 0.15f;
	/** How far beyond its ends a conveyor looks for a neighbour (cm). */
	static constexpr float PortReach = 15.f;
	static constexpr int32 MaxVisibleItems = 8;

	float GetLength() const { return Size.X; }
	bool IsOn() const { return bOn; }
	const TArray<FWEConveyorItem>& GetItems() const { return Items; }
	int32 CountUnits(const class UWEItemDefinition* Definition = nullptr) const;

	/** World points just behind the start / just beyond the end (where the neighbours are looked up). */
	FVector GetInputPortPoint() const;
	FVector GetOutputPortPoint() const;
	/** Belt position (cm from the start) of a world point, clamped to the belt. */
	float ProjectToBelt(const FVector& WorldPoint) const;

	/** Room for an item at this belt position? */
	bool CanAcceptAt(float Position) const;

	/** Where this belt line ends: follows joined conveyors to the first machine/storage (null = open end). */
	AWEPlaceholderProp* FindLineDestination() const;

	// --- Server ---------------------------------------------------------------------------------------

	bool AddItemAt(const FWEItemInstance& Item, float Position);
	bool SetPower(bool bInOn);
	/** A carried item onto the belt (at the point closest to the player, if there is room). */
	bool PutOnBelt(AWEPickupItem* Item, const AWECharacter* By);
	bool CanPutOnBelt(const AWEPickupItem* Item, const AWECharacter* By, FText& OutReason) const;
	/** Takes the front item: small stacks into the inventory, bulky items into the hands. */
	bool TakeFrontItem(AWECharacter* Character);

	/** Flow step: moves the items, hands the front item over, takes a new one in at the start. */
	void StepFlow(float DeltaSeconds);

	/** Loading / tests. */
	void RestoreState(bool bInOn, const TArray<FWEConveyorItem>& InItems);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_Items();

	bool HandOver(const FWEItemInstance& Item);
	bool PullFromSource();
	void ItemsChanged();
	void UpdateItemVisuals();
	float GetBeltHeight() const { return Size.Z; }

	UPROPERTY(ReplicatedUsing = OnRep_Items)
	TArray<FWEConveyorItem> Items;

	UPROPERTY(ReplicatedUsing = OnRep_Items)
	bool bOn = true;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> ItemMeshes;

private:
	/** Local time of the last server step / replication: drawn positions move on from there. */
	float LastUpdateTime = 0.f;
	FTimerHandle VisualTimer;
};
