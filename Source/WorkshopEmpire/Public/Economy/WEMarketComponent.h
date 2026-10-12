#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/WEItemTypes.h"
#include "WEMoney.h"
#include "WEMarketComponent.generated.h"

struct FWEItemInstance;
class UWETraderDefinition;

/**
 * Market conditions shared by all players (spec §10.4): a demand factor per item category that
 * drifts slowly (server random walk within 0.8-1.2, replicated). Pricing helpers combine item value,
 * condition, quality, demand and trader multipliers into buy/sell prices in whole cents.
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWEMarketComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEMarketComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	static UWEMarketComponent* Get(const UObject* WorldContext);

	float GetDemand(EWEItemCategory Category) const;

	/** Server only (loading/tests). */
	void SetDemand(EWEItemCategory Category, float Value);
	const TArray<float>& GetAllDemand() const { return Demand; }
	void SetAllDemand(const TArray<float>& Values);

	/** Server only. One random-walk step for all categories (timer; public for tests). */
	void DriftDemand();

	/** Share of the market value a trader pays when buying from the player. */
	static constexpr float SellShare = 0.6f;

	/** Markup when the player buys from a trader. */
	static constexpr float BuyMarkup = 1.3f;

	/** Market value of an item (estimated value x demand). Market may be null (demand 1). */
	static FWEMoney GetMarketValue(const FWEItemInstance& Item, const UWEMarketComponent* Market);

	/** What Trader pays for Item (all units). Zero if the trader does not buy this category. */
	static FWEMoney GetSellPrice(const FWEItemInstance& Item, const UWETraderDefinition* Trader, const UWEMarketComponent* Market);

	/** What the player pays the trader for Item (all units). */
	static FWEMoney GetBuyPrice(const FWEItemInstance& Item, float PriceMultiplier, const UWEMarketComponent* Market);

	static constexpr float MinDemand = 0.8f;
	static constexpr float MaxDemand = 1.2f;

protected:
	UPROPERTY(Replicated)
	TArray<float> Demand;

	/** Seconds between demand changes. */
	UPROPERTY(EditDefaultsOnly, Category = "Market")
	float DriftIntervalSeconds = 300.f;

private:
	FTimerHandle DriftTimer;
};
