#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WEProgressionComponent.generated.h"

class UWEUpgradeCatalog;
struct FWEUpgradeDefinition;

/** Gameplay events that move the "Erste Schritte" guide forward. */
UENUM()
enum class EWEProgressEvent : uint8
{
	ToolStored,
	ItemPlacedOnBench,
	DisassemblyFinished,
	ItemSold,
	PurchaseMade,
	GameSaved
};

DECLARE_MULTICAST_DELEGATE(FWEOnProgressionChanged);

/**
 * Company progression, shared by all players (lives on AWEGameState):
 *  - owned workshop upgrades and their combined effects (bench slots, inventory capacity, work speed,
 *    lighting, unlocked furniture), applied on server and clients from the replicated list;
 *  - the "Erste Schritte" guide (spec §4.1: understand the loop within minutes, learn by doing),
 *    advanced by real gameplay events, followed by a "next goal" hint (cheapest upgrade not owned).
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWEProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEProgressionComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	static UWEProgressionComponent* Get(const UObject* WorldContext);

	// --- Upgrades ---------------------------------------------------------------------------------

	const UWEUpgradeCatalog* GetCatalog() const { return Catalog; }
	void SetCatalog(UWEUpgradeCatalog* InCatalog) { Catalog = InCatalog; ApplyEffects(); }

	bool HasUpgrade(FName Id) const { return OwnedUpgrades.Contains(Id); }
	const TArray<FName>& GetOwnedUpgrades() const { return OwnedUpgrades; }

	/** Why an upgrade cannot be bought right now (empty = can be bought, money not checked). */
	FText GetBlockReason(const FWEUpgradeDefinition& Upgrade) const;

	/** Server only: marks an upgrade as owned (payment is done by the caller) and applies effects. */
	bool GrantUpgrade(FName Id);

	/** Server only: loading. */
	void SetOwnedUpgrades(const TArray<FName>& Ids);

	// Combined effects of all owned upgrades.
	int32 GetBenchExtraSlots() const;
	float GetInventoryExtraKg() const;
	float GetWorkSpeedMultiplier() const;
	float GetLightMultiplier() const;
	float GetPowerCapacityBonusKw() const;

	/** Re-applies all effects to the world (furniture, storages, inventories, lights). Any machine. */
	void ApplyEffects();

	// --- Guide ------------------------------------------------------------------------------------

	static constexpr int32 NumGuideSteps = 6;
	int32 GetGuideStep() const { return GuideStep; }
	bool IsGuideFinished() const { return GuideStep >= NumGuideSteps; }

	/** Text of the current guide step, or the "next goal" hint once the guide is done. */
	FText GetGuideText() const;
	static FText GetGuideStepText(int32 Step);

	/** Server only. Advances the guide if Event is what the current step waits for. */
	void NotifyEvent(EWEProgressEvent Event);

	/** Server only: loading/tests. */
	void SetGuideStep(int32 Step);

	FWEOnProgressionChanged OnProgressionChanged;

protected:
	UFUNCTION()
	void OnRep_Progression();

	UPROPERTY(EditAnywhere, Category = "Progression")
	TObjectPtr<UWEUpgradeCatalog> Catalog;

	UPROPERTY(ReplicatedUsing = OnRep_Progression)
	TArray<FName> OwnedUpgrades;

	UPROPERTY(ReplicatedUsing = OnRep_Progression)
	int32 GuideStep = 0;

private:
	/** Original brightness of tagged workshop lights (local, every machine). */
	TMap<TWeakObjectPtr<class ULightComponent>, float> BaseLightIntensity;
};
