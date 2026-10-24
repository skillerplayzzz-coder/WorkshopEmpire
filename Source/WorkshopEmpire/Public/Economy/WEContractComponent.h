#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "Items/WEItemTypes.h"
#include "WEMoney.h"
#include "WEContractComponent.generated.h"

class AWECharacter;
class UWEItemDefinition;
struct FWEItemInstance;

UENUM(BlueprintType)
enum class EWEContractKind : uint8
{
	/** Customer wants items: deliver them from the inventory. */
	Supply,
	/** Customer brings a broken device (on the delivery pallet): repair it and hand it back. */
	Repair,
	/** Phase 19: customer wants a working PC that reaches a benchmark score (build it from parts). */
	BuildPc,
	/** Phase 19: customer brings a working PC that is too slow: swap parts until it reaches the score. */
	UpgradePc
};

/** Blueprint for generated customer orders. */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEContractTemplate
{
	GENERATED_BODY()

	/** E.g. "Bastler sucht Arbeitsspeicher". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract")
	EWEContractKind Kind = EWEContractKind::Supply;

	/** Repair orders: condition of the customer's device when it arrives. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract", meta = (EditCondition = "Kind == EWEContractKind::Repair"))
	EWEItemCondition CustomerCondition = EWEItemCondition::Defective;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract")
	TObjectPtr<UWEItemDefinition> Item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract", meta = (ClampMin = "1"))
	int32 MinQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract", meta = (ClampMin = "1"))
	int32 MaxQuantity = 1;

	/** Lowest quality (0-100) the customer accepts (Phase 7). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract", meta = (ClampMin = "0", ClampMax = "100"))
	uint8 MinQuality = 0;

	/** Worst condition the customer still accepts (repair orders: condition after the repair). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract")
	EWEItemCondition MinCondition = EWEItemCondition::Damaged;

	/** Reward = market value of the requested items x this factor (orders pay better than traders). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract", meta = (ClampMin = "0.1"))
	float RewardFactor = 1.4f;

	/** Time to deliver after accepting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract", meta = (ClampMin = "1", Units = "Minutes"))
	float DurationMinutes = 20.f;

	/** Phase 12: only offered from this company tier on (1 = always). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract", meta = (ClampMin = "1", ClampMax = "5"))
	uint8 MinTier = 1;

	/** Phase 12: industrial order (separate board, bigger, a penalty if it fails). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract")
	bool bIndustrial = false;

	/** Phase 12: supply contract - the quantity is delivered this many times, each within DurationMinutes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract", meta = (ClampMin = "1"))
	int32 Rounds = 1;

	/** Phase 19 (PC orders): the benchmark score the customer wants, picked between these two. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract")
	int32 MinBenchmark = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract")
	int32 MaxBenchmark = 0;
};

/** All order templates (one Data Asset). */
UCLASS(BlueprintType)
class WORKSHOPEMPIRE_API UWEContractCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract")
	TArray<FWEContractTemplate> Templates;
};

UENUM(BlueprintType)
enum class EWEContractState : uint8
{
	Offered,
	Accepted
};

USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEContract
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Contract") FGuid Id;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") FText Title;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") TObjectPtr<UWEItemDefinition> Item;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") int32 Quantity = 1;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") EWEItemCondition MinCondition = EWEItemCondition::Damaged;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") uint8 MinQuality = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") FWEMoney Reward;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") EWEContractState State = EWEContractState::Offered;
	/** Offered: offer disappears then. Accepted: delivery deadline. Server world time. */
	UPROPERTY(BlueprintReadOnly, Category = "Contract") float ExpiresAt = 0.f;
	/** Accepted: duration after accepting, in seconds (copied from the template). */
	UPROPERTY(BlueprintReadOnly, Category = "Contract") float DeliverySeconds = 1200.f;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") FString AcceptedBy;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") EWEContractKind Kind = EWEContractKind::Supply;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") EWEItemCondition CustomerCondition = EWEItemCondition::Defective;
	/** Repair orders: instance id of the customer's device (spawned when the order is accepted). */
	UPROPERTY(BlueprintReadOnly, Category = "Contract") FGuid CustomerItemId;
	/** Phase 12: industrial order / supply contract with several deliveries. */
	UPROPERTY(BlueprintReadOnly, Category = "Contract") bool bIndustrial = false;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") int32 Rounds = 1;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") int32 RoundsDone = 0;
	/** Phase 19: PC orders - lowest benchmark score the delivered PC must reach. */
	UPROPERTY(BlueprintReadOnly, Category = "Contract") int32 MinBenchmark = 0;

	bool IsPcOrder() const { return Kind == EWEContractKind::BuildPc || Kind == EWEContractKind::UpgradePc; }

	bool IsSupplyContract() const { return Rounds > 1; }
	/** Industrial orders and supply contracts cost a penalty when they fail. */
	FWEMoney GetPenalty() const { return bIndustrial || Rounds > 1 ? FWEMoney::FromCents(Reward.GetCents() / 4) : FWEMoney(); }
};

UENUM()
enum class EWEContractResult : uint8
{
	Success,
	NotFound,
	WrongState,
	TooManyAccepted,
	MissingItems
};

/** Phase 21: one customer review (stars and a sentence), newest first. */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWECustomerReview
{
	GENERATED_BODY()

	UPROPERTY() int32 Stars = 0;
	UPROPERTY() FString Author;
	UPROPERTY() FString Text;
	UPROPERTY() int32 Day = 1;
};

DECLARE_MULTICAST_DELEGATE(FWEOnContractsChanged);

/**
 * Customer orders (spec §5.1, §10.2): the server keeps a few offers, players accept them for the
 * company and deliver matching items (right item, condition at least MinCondition, not partially
 * disassembled) before the deadline. Reward goes to the company account. Lives on AWEGameState.
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWEContractComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEContractComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	static UWEContractComponent* Get(const UObject* WorldContext);

	const TArray<FWEContract>& GetContracts() const { return Contracts; }
	const FWEContract* FindContract(const FGuid& Id) const;

	/** Server time (or local time in tests without game state). */
	float GetServerTime() const;

	/** Items in Character's inventory that would count for Contract (repair orders: 1 if the repaired device is in the hands). */
	static int32 CountDeliverable(const FWEContract& Contract, const AWECharacter* Character);

	/** Repair orders: the customer's device is repaired (working, not taken apart). */
	static bool IsRepairedCustomerItem(const FWEContract& Contract, const FWEItemInstance& Item);
	/** Phase 19: PC orders - Item is a PC the customer takes (built: ours, working, fast enough; upgrade: theirs, fast enough). */
	static bool IsDeliverablePc(const FWEContract& Contract, const FWEItemInstance& Item);

	/** Supply orders: Item counts (right item, condition, quality, intact, not a customer's). */
	static bool Matches(const FWEContract& Contract, const FWEItemInstance& Item);

	// --- Server only ------------------------------------------------------------------------------

	void SetCatalog(UWEContractCatalog* InCatalog) { Catalog = InCatalog; }
	UWEContractCatalog* GetCatalog() const { return Catalog; }

	/** Removes expired contracts and tops the offers up. Runs on a timer; public for tests. */
	void UpdateContracts();

	/** Creates one offer from a random template (tests may pass a fixed index). Industrial: from the industrial templates. */
	bool AddOffer(int32 TemplateIndex = INDEX_NONE, bool bIndustrial = false);

	/** Phase 12: delivers from a storage (e.g. the van's cargo on a delivery tour). */
	EWEContractResult DeliverFromStorage(const FGuid& Id, class UWEInventoryComponent* Storage, const FString& PlayerName);
	/** Items in Storage that would count for Contract. */
	static int32 CountInStorage(const FWEContract& Contract, const class UWEInventoryComponent* Storage);

	/** Phase 19: a walk-in customer's repair, accepted at the door (Device is already marked as the customer's). False if too many orders run. */
	bool AddWalkInRepair(const FText& Title, const FWEItemInstance& Device, const FWEMoney& Reward, const FString& PlayerName);
	EWEContractResult Accept(const FGuid& Id, const FString& PlayerName);
	EWEContractResult Deliver(const FGuid& Id, AWECharacter* Character);
	/** Quality overhaul: an accepted order is given back (the customer fetches the device; industrial orders pay the
	 *  penalty; with reviews on, a one-star review). Frees the slot - no order can block the game. */
	EWEContractResult Cancel(const FGuid& Id, const FString& PlayerName);

	/** Loading / tests. */
	void SetContracts(const TArray<FWEContract>& InContracts);

	/** Repair orders: removes the customer's device from the world (order expired: the customer took it back). */
	void RemoveCustomerItem(const FGuid& ItemId);

	FWEOnContractsChanged OnContractsChanged;

	// --- Phase 19: customer ratings --------------------------------------------------------------------
	/** Average stars (1-5) of delivered orders, 0 when none yet. */
	float GetAverageRating() const { return RatingCount > 0 ? RatingSum / static_cast<float>(RatingCount) : 0.f; }
	int32 GetRatingCount() const { return RatingCount; }
	int32 GetRatingSum() const { return RatingSum; }
	int32 GetLastStars() const { return LastStars; }
	/** Loading / tests. */
	void SetRatings(int32 InSum, int32 InCount) { RatingSum = FMath::Max(0, InSum); RatingCount = FMath::Max(0, InCount); }
	/** Stars for a delivered order (PC orders: benchmark against the minimum; repairs: quality; deliveries: on time). */
	static int32 RateDelivery(const FWEContract& Contract, const FWEItemInstance* Delivered, float SecondsLeft);
	/** Reward with the bonus for a good rating (4 stars +10 %, 5 stars +25 %). */
	static FWEMoney RewardWithBonus(const FWEMoney& Reward, int32 Stars);

	// --- Phase 21: reviews and reputation (rule "Kundenbewertungen") -------------------------------------
	const TArray<FWECustomerReview>& GetReviews() const { return Reviews; }
	/** Loading / tests. */
	void SetReviews(const TArray<FWECustomerReview>& InReviews);
	/** Server: a new review (kept: the last MaxReviews). Counts into the average. */
	void AddReview(int32 Stars, const FString& Author, const FString& Text);
	/** Reputation 1-5 once there are a few ratings and reviews are on; 0 otherwise (no effect). */
	float GetReputation() const;
	/** "Sehr gut", "Gut", ... for the reputation. */
	static FText GetReputationText(float Reputation);
	/** Offers on the board: more with a good name, fewer with a bad one. */
	int32 GetMaxOffers() const;
	/** New offers pay this much more (good name) or less (bad name). */
	float GetReputationRewardFactor() const;
	/** A sentence a customer would write for these stars (and kind of order). */
	static FString MakeReviewText(int32 Stars, bool bRepair, bool bLate, int32 Seed);
	/** "Frau Becker zu „Büro-PC zur Reparatur“". */
	static FString MakeReviewAuthor(int32 Seed, const FString& OrderTitle);
	static constexpr int32 MaxReviews = 12;
	static constexpr int32 MinRatingsForReputation = 3;

	static constexpr int32 MaxOffers = 3;
	static constexpr int32 MaxAccepted = 3;
	/** Phase 12: industrial orders on their own board once the company is big enough. */
	static constexpr int32 MaxIndustrialOffers = 2;
	static constexpr int32 MaxIndustrialAccepted = 2;
	/** How long an unaccepted offer stays on the board. */
	static constexpr float OfferLifetimeSeconds = 600.f;

protected:
	UFUNCTION()
	void OnRep_Contracts();

	/** Pays a delivered round / order; supply contracts continue with the next round. Stars: the customer's rating (0 = none). */
	void CompleteDelivery(int32 Index, const FString& PlayerName, int32 Stars = 0);

	UPROPERTY(Replicated) int32 RatingSum = 0;
	UPROPERTY(Replicated) int32 RatingCount = 0;
	UPROPERTY(Replicated) int32 LastStars = 0;
	UPROPERTY(Replicated) TArray<FWECustomerReview> Reviews;
	int32 GetCompanyTier() const;

	UPROPERTY(EditAnywhere, Category = "Contracts")
	TObjectPtr<UWEContractCatalog> Catalog;

	UPROPERTY(ReplicatedUsing = OnRep_Contracts)
	TArray<FWEContract> Contracts;

private:
	FTimerHandle UpdateTimer;
};
