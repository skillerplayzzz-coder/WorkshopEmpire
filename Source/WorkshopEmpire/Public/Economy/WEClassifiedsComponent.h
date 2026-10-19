#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/WEItemInstance.h"
#include "WEMoney.h"
#include "WEClassifiedsComponent.generated.h"

class AWECharacter;
class UWEInventoryComponent;

/** One classified ad by a private seller (Phase 19, Ferrit OS "Kleinanzeigen"). */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEClassified
{
	GENERATED_BODY()

	UPROPERTY() FGuid Id;
	UPROPERTY() FWEItemInstance Item;
	UPROPERTY() FString Seller;
	UPROPERTY() FString Text;
	UPROPERTY() FWEMoney Asking;
	/** The seller's last counter offer (0 = none yet). */
	UPROPERTY() FWEMoney Counter;
	/** Offers the company made so far; after too many low ones the seller stops answering. */
	UPROPERTY() int32 Offers = 0;
	UPROPERTY() float ExpiresAt = 0.f;
	/** Lowest price the seller takes - only the server knows it. */
	UPROPERTY(NotReplicated) FWEMoney Lowest;
};

/** Something the company bought, on its way to the delivery pallet. */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEClassifiedDelivery
{
	GENERATED_BODY()

	UPROPERTY() FWEItemInstance Item;
	UPROPERTY() float ArrivesAt = 0.f;
};

/** An ad the company placed: the item waits here until someone buys it (or the ad is taken down). */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEOwnListing
{
	GENERATED_BODY()

	UPROPERTY() FGuid Id;
	UPROPERTY() FWEItemInstance Item;
	UPROPERTY() FWEMoney Price;
	/** Best offer a buyer made (0 = none yet) and who. */
	UPROPERTY() FWEMoney Offer;
	UPROPERTY() FString Buyer;
	UPROPERTY() float NextBuyerAt = 0.f;
};

UENUM()
enum class EWEClassifiedResult : uint8
{
	Accepted,
	Countered,
	Declined,
	/** The seller no longer answers (too many low offers). */
	Ignored,
	NotFound,
	NoMoney,
	NotAllowed
};

DECLARE_MULTICAST_DELEGATE(FWEOnClassifiedsChanged);

/**
 * Phase 19: used-goods classifieds in Ferrit OS. Private sellers list parts and devices with an asking price; the
 * company can offer less (the seller accepts, counters or declines - each has a lowest price only the server knows),
 * and bought goods arrive on the delivery pallet a little later. The company can also list its own goods with a
 * price: buyers turn up over time and make offers, which the players accept or reject.
 * Lives on AWEGameState; the server decides everything, the lists replicate.
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWEClassifiedsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEClassifiedsComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	static UWEClassifiedsComponent* Get(const UObject* WorldContext);

	const TArray<FWEClassified>& GetAds() const { return Ads; }
	const TArray<FWEClassifiedDelivery>& GetDeliveries() const { return Deliveries; }
	const TArray<FWEOwnListing>& GetListings() const { return Listings; }
	float GetServerTime() const;

	// --- Server -------------------------------------------------------------------------------------------
	/** Removes old ads, adds new ones, delivers what arrived, lets buyers look at own ads. Timer; public for tests. */
	void Update();
	/** New ad from a random seller (tests may pass an item). */
	bool AddAd(const FWEItemInstance* Fixed = nullptr, int64 AskingCents = 0, int64 LowestCents = 0);

	/** Offer Cents for an ad (Cents >= asking price buys right away). */
	EWEClassifiedResult MakeOffer(const FGuid& Id, int64 Cents, const FString& PlayerName);
	/** Takes the seller's counter offer. */
	EWEClassifiedResult AcceptCounter(const FGuid& Id, const FString& PlayerName);

	/** Lists a stack (all of it) from Inventory for PriceCents. */
	EWEClassifiedResult ListItem(UWEInventoryComponent* Inventory, int32 Slot, int64 PriceCents, const FString& PlayerName);
	/** Sells to the buyer who made the current offer. */
	EWEClassifiedResult AcceptBuyer(const FGuid& ListingId, const FString& PlayerName);
	/** Takes the ad down; the item goes into Inventory (or onto the pallet if it does not fit). */
	EWEClassifiedResult Withdraw(const FGuid& ListingId, UWEInventoryComponent* Inventory);

	/** Loading / tests. */
	void SetState(const TArray<FWEClassifiedDelivery>& InDeliveries, const TArray<FWEOwnListing>& InListings);
	/** Tests: all deliveries arrive now. */
	void DeliverAllNow();

	FWEOnClassifiedsChanged OnChanged;

	static constexpr int32 MaxAds = 7;
	static constexpr int32 MaxListings = 6;
	static constexpr int32 MaxOffersPerAd = 3;

protected:
	UFUNCTION() void OnRep_Lists();
	void Changed();
	void Deliver(const FWEItemInstance& Item);
	void BuyerTurn(FWEOwnListing& Listing, float Now);

	UPROPERTY(ReplicatedUsing = OnRep_Lists) TArray<FWEClassified> Ads;
	UPROPERTY(ReplicatedUsing = OnRep_Lists) TArray<FWEClassifiedDelivery> Deliveries;
	UPROPERTY(ReplicatedUsing = OnRep_Lists) TArray<FWEOwnListing> Listings;

private:
	FTimerHandle UpdateTimer;
	FRandomStream Random;
};
