#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WEInteractable.h"
#include "WEScrapPile.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UWELootTable;

/**
 * Scrap heap behind the garage (§11 "Schrottplatz": cheap or free scrap). The server keeps up to
 * MaxItems free items from LootTable lying around it and tops them up every RefillMinutes. Items
 * are normal world items: take them, carry them in, part them out. Nothing to buy, but slow.
 *
 * Counting uses the items currently lying within Radius (not references), so it also works after
 * loading a save. Only the next refill time replicates (for the HUD hint).
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEScrapPile : public AActor, public IWEInteractable
{
	GENERATED_BODY()

public:
	AWEScrapPile();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnConstruction(const FTransform& Transform) override;

	// IWEInteractable (look at it: shows when new scrap comes)
	virtual FText GetInteractionText(const AWECharacter* Interactor) const override;
	virtual bool CanInteract(const AWECharacter* Interactor, FText& OutReason) const override;
	virtual void Interact(AWECharacter* Interactor) override {}
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;

	/** Server only. Spawns items until MaxItems lie around the pile. Returns how many were added. */
	int32 Refill();

	/** Free items currently lying around the pile. */
	int32 CountItemsAround() const;

	void SetLootTable(UWELootTable* InTable) { LootTable = InTable; }
	void SetRandomSeed(int32 Seed) { Random.Initialize(Seed); }
	int32 GetMaxItems() const { return MaxItems; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void OnRefillTimer();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Heap;

	UPROPERTY(EditAnywhere, Category = "Scrap")
	TObjectPtr<UWELootTable> LootTable;

	UPROPERTY(EditAnywhere, Category = "Scrap", meta = (ClampMin = "1"))
	int32 MaxItems = 4;

	UPROPERTY(EditAnywhere, Category = "Scrap", meta = (ClampMin = "0.1", Units = "Minutes"))
	float RefillMinutes = 8.f;

	UPROPERTY(EditAnywhere, Category = "Scrap", meta = (Units = "cm"))
	float Radius = 350.f;

	UPROPERTY(EditAnywhere, Category = "Scrap")
	TObjectPtr<UMaterialInterface> PlaceholderMaterial;

	/** Server world time of the next refill (for the hint). */
	UPROPERTY(Replicated)
	float NextRefillTime = 0.f;

private:
	FRandomStream Random;
	FTimerHandle RefillTimer;
};
