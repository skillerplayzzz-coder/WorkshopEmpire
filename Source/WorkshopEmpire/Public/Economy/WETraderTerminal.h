#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WEInteractable.h"
#include "Items/WEItemInstance.h"
#include "WEMoney.h"
#include "WETraderTerminal.generated.h"

class AWECharacter;
class UInstancedStaticMeshComponent;
class UWEInventoryComponent;
class UWETraderDefinition;
struct FWETraderStockEntry;

UENUM()
enum class EWETradeResult : uint8
{
	Success,
	Duplicate,
	OutOfReach,
	InvalidItem,
	NoBuyer,
	NotEnoughMoney,
	OutOfStock,
	ContractFailed
};

/** One offer currently in stock (rolled at each restock from the traders' stock entries). */
USTRUCT()
struct WORKSHOPEMPIRE_API FWEStockLine
{
	GENERATED_BODY()

	UPROPERTY() int32 TraderIndex = INDEX_NONE;
	UPROPERTY() int32 EntryIndex = INDEX_NONE;
	/** What one unit looks like (condition and quality rolled at restock). */
	UPROPERTY() FWEItemInstance Item;
	UPROPERTY() int32 Remaining = 0;
};

/** One buyable line in the terminal UI (flattened over all traders). */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEMarketOffer
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Trade") int32 OfferIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly, Category = "Trade") FText TraderName;
	UPROPERTY(BlueprintReadOnly, Category = "Trade") FWEItemInstance Item;
	UPROPERTY(BlueprintReadOnly, Category = "Trade") int32 Remaining = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Trade") FWEMoney UnitPrice;
};

/**
 * Online marketplace terminal in the garage (spec §5.1 "Online-Marktplätze", §11). [E] opens the
 * trade screen: sell from the player inventory or the carried item to the trader paying most, buy
 * from the traders' stock (small items into the inventory, bulky ones to the delivery pallet),
 * accept and deliver customer orders.
 *
 * All trades are server functions called through AWEPlayerController RPCs. Each request carries a
 * GUID; a repeated GUID is ignored, so a trade can never be booked twice. The order inside a trade
 * keeps goods and money consistent: take goods/check money first, then book, roll back on failure.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWETraderTerminal : public AActor, public IWEInteractable
{
	GENERATED_BODY()

public:
	AWETraderTerminal();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnConstruction(const FTransform& Transform) override;

	// IWEInteractable
	virtual FText GetInteractionText(const AWECharacter* Interactor) const override;
	virtual bool CanInteract(const AWECharacter* Interactor, FText& OutReason) const override;
	virtual void Interact(AWECharacter* Interactor) override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;

	bool IsWithinUseRange(const AWECharacter* Character) const;

	// --- Queries (any machine) --------------------------------------------------------------------

	const TArray<TObjectPtr<UWETraderDefinition>>& GetTraders() const { return Traders; }

	/** Setup/tests: replaces the traders and restocks (server). */
	void SetTraders(const TArray<UWETraderDefinition*>& InTraders)
	{
		Traders.Reset();
		Traders.Append(InTraders);
		Restock();
	}
	TArray<FWEMarketOffer> GetOffers() const;

	/** Best price any trader of this terminal pays for Item; OutTrader receives the trader (or null). */
	FWEMoney GetBestSellPrice(const FWEItemInstance& Item, const UWETraderDefinition** OutTrader = nullptr) const;

	// --- Server -------------------------------------------------------------------------------------

	EWETradeResult SellFromInventory(AWECharacter* Character, UWEInventoryComponent* Inventory, int32 Slot, int32 Quantity, const FGuid& RequestId);
	EWETradeResult SellHeldItem(AWECharacter* Character, const FGuid& RequestId);
	EWETradeResult Buy(AWECharacter* Character, int32 OfferIndex, int32 Quantity, const FGuid& RequestId);
	/** Buys a workshop upgrade for the company (spec §5.5). */
	EWETradeResult BuyUpgrade(AWECharacter* Character, FName UpgradeId, const FGuid& RequestId);

	EWETradeResult AcceptContract(AWECharacter* Character, const FGuid& ContractId, const FGuid& RequestId);

	/** Places the company's next bid on an auction lot (Phase 7). */
	EWETradeResult BidOnAuction(AWECharacter* Character, const FGuid& LotId, const FGuid& RequestId);
	EWETradeResult DeliverContract(AWECharacter* Character, const FGuid& ContractId, const FGuid& RequestId);

	/** Refills all stock to the trader definitions (timer; public for tests and new games). */
	void Restock();

	/** Remaining units per offer (logging, change detection). */
	TArray<int32> GetStockRemaining() const;

	/** Saving/loading. */
	const TArray<FWEStockLine>& GetStockLines() const { return Lines; }
	void SetStockLines(const TArray<FWEStockLine>& InLines);

	void SetRandomSeed(int32 Seed) { StockRandom.Initialize(Seed); }

	static FText ResultText(EWETradeResult Result);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Maps a flat offer index to trader + stock entry. */
	bool ResolveOffer(int32 OfferIndex, const UWETraderDefinition*& OutTrader, const FWETraderStockEntry*& OutEntry) const;
	int32 NumOffers() const;
	void Notify(AWECharacter* Character, const FText& Message) const;
	FString PlayerNameOf(const AWECharacter* Character) const;

	/** Small items go into the buyer's inventory, the rest onto the delivery pallet. */
	void DeliverPurchase(AWECharacter* Character, const FWEItemInstance& Item);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Screen;

	UPROPERTY(EditAnywhere, Category = "Trade")
	TArray<TObjectPtr<UWETraderDefinition>> Traders;

	UPROPERTY(EditAnywhere, Category = "Trade", meta = (Units = "cm"))
	float UseRange = 325.f;

	UPROPERTY(EditAnywhere, Category = "Trade")
	TObjectPtr<UMaterialInterface> PlaceholderMaterial;

	/** Offers in stock right now (changes at every restock). */
	UPROPERTY(Replicated)
	TArray<FWEStockLine> Lines;

private:
	FTimerHandle RestockTimer;
	FRandomStream StockRandom;
};

/** Pallet near the door where bulky purchases are delivered. */
UCLASS()
class WORKSHOPEMPIRE_API AWEDeliveryZone : public AActor
{
	GENERATED_BODY()

public:
	AWEDeliveryZone();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Next free-ish drop transform on the pallet (cycles through a small grid). */
	FTransform NextDropTransform();

	static AWEDeliveryZone* Find(const UWorld* World);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Pallet;

	UPROPERTY(EditAnywhere, Category = "Delivery")
	TObjectPtr<UMaterialInterface> PlaceholderMaterial;

private:
	int32 NextIndex = 0;
};
