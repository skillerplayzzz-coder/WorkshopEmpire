#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/WEItemTypes.h"
#include "WETraderDefinition.generated.h"

class UWEItemDefinition;

/** Which categories a trader buys from the player and how keen they are (1 = normal). */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWETraderCategoryRate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader")
	EWEItemCategory Category = EWEItemCategory::RawMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader", meta = (ClampMin = "0"))
	float Multiplier = 1.f;
};

/** One offer in a trader's stock. Restocks to Quantity periodically. */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWETraderStockEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader")
	TObjectPtr<UWEItemDefinition> Item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader")
	EWEItemCondition Condition = EWEItemCondition::Used;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader", meta = (ClampMin = "0", ClampMax = "100"))
	uint8 Quality = 50;

	/** Extra price factor for this offer (bargain < 1 < expensive). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader", meta = (ClampMin = "0.01"))
	float PriceMultiplier = 1.f;

	/** Chance (0-1) that this offer is in stock after a restock: rotating offers (Phase 7). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader", meta = (ClampMin = "0", ClampMax = "1"))
	float Chance = 1.f;

	/** The condition may be up to this many levels worse than Condition (rolled per restock). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader", meta = (ClampMin = "0", ClampMax = "5"))
	uint8 ConditionSpread = 0;

	/** Quality varies by up to +/- this much (rolled per restock). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader", meta = (ClampMin = "0", ClampMax = "50"))
	uint8 QualitySpread = 0;
};

/**
 * A trader (spec §11): what they buy, what they sell. Data-driven; the market terminal offers
 * the stock of all its traders and sells each item to the trader paying the most.
 */
UCLASS(BlueprintType)
class WORKSHOPEMPIRE_API UWETraderDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 0 if the trader does not buy this category. */
	float GetBuyMultiplier(EWEItemCategory Category) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader")
	TArray<FWETraderCategoryRate> Buys;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader")
	TArray<FWETraderStockEntry> Stock;

	/** Minutes between restocks (0 = never). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader", meta = (ClampMin = "0"))
	float RestockMinutes = 10.f;
};
