#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "Items/WEItemInstance.h"
#include "WEMoney.h"
#include "WEAuctionComponent.generated.h"

class UWEItemDefinition;

/** One item (or stack) in an auction lot template. */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEAuctionLotItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auction")
	TObjectPtr<UWEItemDefinition> Item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auction", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auction")
	EWEItemCondition Condition = EWEItemCondition::Defective;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auction", meta = (ClampMin = "0", ClampMax = "100"))
	uint8 Quality = 40;
};

/** Blueprint of an auction lot, e.g. "Büroauflösung: 3 alte Computer". */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEAuctionTemplate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auction")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auction")
	TArray<FWEAuctionLotItem> Items;

	/** Starting bid = market value of the lot x this. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auction", meta = (ClampMin = "0.05"))
	float StartFactor = 0.35f;

	/** The rival bidder's (hidden) limit = market value x random(Min, Max). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auction", meta = (ClampMin = "0.05"))
	float BidderMinFactor = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auction", meta = (ClampMin = "0.05"))
	float BidderMaxFactor = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auction", meta = (ClampMin = "0.5", Units = "Minutes"))
	float DurationMinutes = 5.f;

	/** Relative chance to be picked for a new lot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auction", meta = (ClampMin = "0"))
	float Weight = 1.f;
};

UCLASS(BlueprintType)
class WORKSHOPEMPIRE_API UWEAuctionCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auction")
	TArray<FWEAuctionTemplate> Templates;

	/** Names of rival bidders shown in the UI. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auction")
	TArray<FString> BidderNames;
};

/** A running auction (replicated). */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEAuctionLot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Auction") FGuid Id;
	UPROPERTY(BlueprintReadOnly, Category = "Auction") FText Title;
	UPROPERTY(BlueprintReadOnly, Category = "Auction") TArray<FWEItemInstance> Items;
	/** Market value of all items (shown as "Schätzwert"). */
	UPROPERTY(BlueprintReadOnly, Category = "Auction") FWEMoney EstimatedValue;
	/** Highest bid so far; before the first bid the starting price. */
	UPROPERTY(BlueprintReadOnly, Category = "Auction") FWEMoney CurrentBid;
	UPROPERTY(BlueprintReadOnly, Category = "Auction") int32 NumBids = 0;
	/** True if our company (any player) holds the highest bid. */
	UPROPERTY(BlueprintReadOnly, Category = "Auction") bool bCompanyLeads = false;
	/** Player name (company leads) or rival bidder name; empty before the first bid. */
	UPROPERTY(BlueprintReadOnly, Category = "Auction") FString LeaderName;
	/** Server world time. */
	UPROPERTY(BlueprintReadOnly, Category = "Auction") float EndsAt = 0.f;
};

UENUM()
enum class EWEAuctionResult : uint8
{
	/** Bid placed, the company leads. */
	Leading,
	/** Bid placed, but the rival bidder immediately went higher. */
	Outbid,
	AlreadyLeading,
	NotEnoughMoney,
	NotFound
};

DECLARE_MULTICAST_DELEGATE(FWEOnAuctionsChanged);

/**
 * Auctions (§11 "Auktionen", "Industrieauktionen", "Geschäftsauflösungen"): a few lots run at the
 * same time with a deadline. The company bids against one rival bidder per lot whose limit is
 * hidden on the server. Money is only charged when the lot is won (at the deadline); if the account
 * cannot pay then, the lot is lost. Won items are delivered to the pallet. Lives on AWEGameState.
 *
 * Bids in the last 20 seconds extend the deadline (no sniping). All of it runs on the server; the
 * lots replicate to everybody.
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWEAuctionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEAuctionComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	static UWEAuctionComponent* Get(const UObject* WorldContext);

	const TArray<FWEAuctionLot>& GetLots() const { return Lots; }
	const FWEAuctionLot* FindLot(const FGuid& Id) const;
	const TArray<FString>& GetRecentResults() const { return RecentResults; }

	/** What the next bid of the company would be. */
	static FWEMoney GetNextBid(const FWEAuctionLot& Lot);
	static FWEMoney GetIncrement(const FWEMoney& Bid);

	float GetServerTime() const;

	// --- Server only --------------------------------------------------------------------------------

	void SetCatalog(UWEAuctionCatalog* InCatalog) { Catalog = InCatalog; }

	/** Places the company's next bid. OutBid receives the amount bid. */
	EWEAuctionResult PlaceBid(const FGuid& LotId, const FString& PlayerName, FWEMoney& OutBid);

	/** Ends due lots and tops the lots up. Runs on a timer; public for tests. */
	void UpdateAuctions();

	/** Ends a lot now: delivers it if the company leads and can pay. Returns true if won. */
	bool FinishLot(const FGuid& LotId);

	/** Adds a lot from a template (random if INDEX_NONE). */
	bool AddLot(int32 TemplateIndex = INDEX_NONE);

	/** Tests and loading. */
	void SetLots(const TArray<FWEAuctionLot>& InLots, const TArray<int64>& InBidderMaxCents);
	int64 GetBidderMaxCents(const FGuid& LotId) const;
	void SetBidderMaxCents(const FGuid& LotId, int64 Cents);
	void SetRandomSeed(int32 Seed) { Random.Initialize(Seed); }

	FWEOnAuctionsChanged OnAuctionsChanged;

	static constexpr int32 MaxLots = 3;
	static constexpr float AntiSnipeSeconds = 20.f;

protected:
	UFUNCTION()
	void OnRep_Lots();

	void AddResult(const FString& Text);
	FString PickBidderName();

	UPROPERTY(EditAnywhere, Category = "Auction")
	TObjectPtr<UWEAuctionCatalog> Catalog;

	UPROPERTY(ReplicatedUsing = OnRep_Lots)
	TArray<FWEAuctionLot> Lots;

	/** Last few results ("Gewonnen: … für 42,00 €"), newest first. */
	UPROPERTY(ReplicatedUsing = OnRep_Lots)
	TArray<FString> RecentResults;

private:
	/** Hidden limit of the rival bidder per lot (server only). */
	TMap<FGuid, int64> BidderMaxCents;
	/** One rival per lot: his name. */
	TMap<FGuid, FString> RivalNames;
	FRandomStream Random;
	FTimerHandle UpdateTimer;
};
