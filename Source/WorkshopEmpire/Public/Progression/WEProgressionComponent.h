#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WEMoney.h"
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
	int32 GetInventoryExtraSlots() const;
	bool AreToolsWeightless() const;
	float GetWorkSpeedMultiplier() const;
	float GetLightMultiplier() const;
	float GetPowerCapacityBonusKw() const;
	int32 GetStaffSlotBonus() const;

	// --- Company (Phase 12) -----------------------------------------------------------------------

	/** 1 Kleine Garage, 2 Große Werkstatt, 3 Gewerbehalle, 4 Fabrik, 5 Industrieunternehmen. */
	int32 GetCompanyTier() const;
	static FText GetTierName(int32 Tier);
	static constexpr int32 MaxTier = 5;
	/** Firmenwert: account + half of what furniture, machines, buildings and vehicles cost. */
	FWEMoney GetCompanyValue() const;
	/** The fifth tier needs a factory and this company value. */
	static constexpr int64 IndustrialCompanyValueCents = 25000000;
	/** What the next tier needs (empty at the top). */
	FText GetNextTierText() const;

	/** Actors tagged "WEUpgrade.<Id>" (buildings, doors, build areas, lights) appear with that upgrade. */
	static FName MakeUpgradeTag(FName UpgradeId);

	/** Re-applies all effects to the world (furniture, storages, inventories, lights). Any machine. */
	void ApplyEffects();

	// --- Selling old buildings (Phase 17.5) -----------------------------------------------------------

	/** Share of the purchase price paid back when a building is sold (a clear loss). */
	static constexpr float BuildingResaleFactor = 0.35f;

	/** An owned building upgrade while a bigger building is owned too (the garage itself is the company's seat). */
	bool CanOfferBuildingSale(FName Id) const;
	FWEMoney GetBuildingSalePrice(FName Id) const;

	/** What stops the sale right now: items, furniture/machines or employees inside, power or staff limits. Empty = can be sold. */
	TArray<FText> GetBuildingSaleBlockers(FName Id) const;

	/** What changes: power capacity, staff places, storage places, company tier. */
	TArray<FText> GetBuildingSaleEffects(FName Id) const;

	/** True if Location lies inside the (bought) building of the upgrade Id. */
	bool IsInsideBuilding(FName Id, const FVector& Location) const;

	/**
	 * Server only: clears what can be moved automatically - loose items go to the delivery pallet, employees
	 * working there lose that task and wait in the yard. Furniture and machines must be moved by the players.
	 * Returns the number of things moved.
	 */
	int32 ClearBuildingForSale(FName Id);

	/** Server only: gives a building upgrade back (payment by the caller). Fails while blockers exist. */
	bool RevokeBuilding(FName Id);

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
