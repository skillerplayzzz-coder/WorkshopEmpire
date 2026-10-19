#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WEInteractable.h"
#include "Items/WEItemInstance.h"
#include "WEMoney.h"
#include "WETraderTerminal.generated.h"

class AWECharacter;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
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
	ContractFailed,
	/** Phase 17.5: locked/favourite item or personal equipment not released for sale. */
	Protected
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

	/** Phase 19: furniture storage in the workshop (bench, shelf, crate) - sellable from the PC. */
	static bool IsWorkshopStorage(const UWEInventoryComponent* Inventory);
	TArray<UWEInventoryComponent*> GetWorkshopStorages() const;
	EWETradeResult SellFromInventory(AWECharacter* Character, UWEInventoryComponent* Inventory, int32 Slot, int32 Quantity, const FGuid& RequestId);
	EWETradeResult SellHeldItem(AWECharacter* Character, const FGuid& RequestId);
	EWETradeResult Buy(AWECharacter* Character, int32 OfferIndex, int32 Quantity, const FGuid& RequestId);
	/** Buys a workshop upgrade for the company (spec §5.5). */
	EWETradeResult BuyUpgrade(AWECharacter* Character, FName UpgradeId, const FGuid& RequestId);
	/** Phase 17.5: sells an old building (35 % of its price) once nothing is left inside. */
	EWETradeResult SellBuilding(AWECharacter* Character, FName UpgradeId, const FGuid& RequestId);
	/** Phase 17.5: moves loose items and employees out of a building that is to be sold. */
	EWETradeResult ClearBuilding(AWECharacter* Character, FName UpgradeId);

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
	/** Why a protected item cannot be sold (locked, favourite, equipment). */
	static FText ProtectedText(const FWEItemInstance& Item);

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

	/** Phase 17.5: meshes of the workshop PC set, screen widget and camera. */
	void SetupModel();

public:
	/** Phase 17.5: the camera in front of the monitor (view while using the PC). */
	class UCameraComponent* GetScreenCamera() const { return ScreenCamera; }

protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Screen;

	// --- Phase 17.5: the workshop PC (desk, tower, monitor with Ferrit OS, keyboard, mouse) ----------------
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> DeskMesh;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> TowerMesh;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> MonitorMesh;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> KeyboardMesh;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> MouseMesh;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> LampMesh;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> ChairMesh;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> ClipboardMesh;
	/** The monitor shows the Ferrit OS desktop to everybody in the room. */
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<class UWidgetComponent> ScreenWidget;
	/** The view while somebody uses the PC: straight at the monitor. */
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<class UCameraComponent> ScreenCamera;

	/** True when the modelled PC set is there (otherwise the old box desk is shown). */
	bool bModelled = false;

public:
	/** Phase 17.5: who is sitting at the PC (shown on the monitor for everybody). */
	UPROPERTY(Replicated)
	FString UserName;

	/** Local player at the PC: the monitor in the world shows "in use". Also tells the server (name for others). */
	void SetLocalUser(AWECharacter* Character, bool bUsing);

protected:

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
