#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WEPlayerController.generated.h"

class UWEHUDWidget;
class UWEInventoryComponent;
class UWEInventoryWidget;

/**
 * Owning client's connection to the server.
 *
 * Every request that changes shared state goes through a Server RPC here (inventory moves, drops,
 * sorting) or on the pawn's interaction component. The server re-validates access (own inventory
 * or a container within reach) before calling into UWEInventoryComponent.
 * Also owns the local HUD and inventory UI.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AWEPlayerController();

	UWEHUDWidget* GetHUDWidget() const { return HUDWidget; }

	// --- Inventory UI (local) ------------------------------------------------------------------

	void ToggleInventory();
	void OpenInventory(UWEInventoryComponent* Container = nullptr);
	void CloseInventory();
	bool IsInventoryOpen() const;

	/** Server -> owning client: open the container UI after a validated interaction. */
	UFUNCTION(Client, Reliable)
	void ClientOpenContainer(UWEInventoryComponent* Container);

	// --- Trade UI (local) ----------------------------------------------------------------------

	void OpenTrade(class AWETraderTerminal* Terminal);
	void CloseTrade();
	bool IsTradeOpen() const;

	/** Any full-screen menu (inventory, trade) open. */
	bool IsMenuOpen() const { return IsInventoryOpen() || IsTradeOpen(); }

	/** Server -> owning client: open the market screen after a validated interaction. */
	UFUNCTION(Client, Reliable)
	void ClientOpenTrade(class AWETraderTerminal* Terminal);

	// --- Trade requests (each call gets a fresh request id; the server ignores repeats) ----------

	void RequestSell(class AWETraderTerminal* Terminal, UWEInventoryComponent* From, int32 Slot, int32 Quantity);
	void RequestSellHeld(class AWETraderTerminal* Terminal);
	void RequestBuy(class AWETraderTerminal* Terminal, int32 OfferIndex, int32 Quantity);
	void RequestAcceptContract(class AWETraderTerminal* Terminal, const FGuid& ContractId);
	void RequestBuyUpgrade(class AWETraderTerminal* Terminal, FName UpgradeId);
	void RequestBid(class AWETraderTerminal* Terminal, const FGuid& LotId);

	/** Moves every stack from a container into the player inventory (as much as fits). */
	void RequestTakeAll(UWEInventoryComponent* From);

	/** Server-side execution of RequestTakeAll (public for tests). */
	bool ExecuteTakeAll(UWEInventoryComponent* From);
	void RequestDeliverContract(class AWETraderTerminal* Terminal, const FGuid& ContractId);

	/** Test hook: sends the same request id twice (must be booked once). Not in Shipping. */
	void DevRequestSellTwice(class AWETraderTerminal* Terminal, UWEInventoryComponent* From, int32 Slot, int32 Quantity);

	/** Development builds only: moves the pawn on the server (test automation). */
	UFUNCTION(Server, Reliable)
	void ServerDevTeleport(FVector Location, FRotator Rotation);

	/** Development builds only: ends every auction the company currently leads (test automation). */
	UFUNCTION(Server, Reliable)
	void ServerDevEndAuctions();

	class UWETradeWidget* GetTradeWidget() const { return TradeWidget; }

	/** Development builds only: adds a customer order offer from a fixed template (test automation). */
	UFUNCTION(Server, Reliable)
	void ServerDevAddContractOffer(int32 TemplateIndex);

	/** Server -> owning client: short on-screen message ("Inventar voll", "Gespeichert"). */
	UFUNCTION(Client, Reliable)
	void ClientShowMessage(const FText& Message);

	// --- Inventory requests (local wrappers around the Server RPCs) ----------------------------

	void RequestMoveItem(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To, int32 ToSlot, int32 Quantity);
	void RequestQuickTransfer(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To);
	void RequestDropFromInventory(UWEInventoryComponent* From, int32 Slot, int32 Quantity);
	void RequestSort(UWEInventoryComponent* Inventory);

	// --- Save / load (host only) ---------------------------------------------------------------

	void RequestSave(const FString& Slot);
	void RequestLoad(const FString& Slot);

	/** Console: WESave <Slot> / WELoad <Slot> for named save slots. */
	UFUNCTION(Exec)
	void WESave(const FString& Slot);

	UFUNCTION(Exec)
	void WELoad(const FString& Slot);

	// --- Server-side execution (public for tests) ----------------------------------------------

	bool CanUseInventory(const UWEInventoryComponent* Inventory) const;
	bool ExecuteMoveItem(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To, int32 ToSlot, int32 Quantity);
	bool ExecuteQuickTransfer(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To);
	bool ExecuteDropFromInventory(UWEInventoryComponent* From, int32 Slot, int32 Quantity);

protected:
	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable)
	void ServerMoveItem(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To, int32 ToSlot, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerQuickTransfer(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To);

	UFUNCTION(Server, Reliable)
	void ServerDropFromInventory(UWEInventoryComponent* From, int32 Slot, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerSortInventory(UWEInventoryComponent* Inventory);

	UFUNCTION(Server, Reliable)
	void ServerSell(class AWETraderTerminal* Terminal, UWEInventoryComponent* From, int32 Slot, int32 Quantity, FGuid RequestId);

	UFUNCTION(Server, Reliable)
	void ServerSellHeld(class AWETraderTerminal* Terminal, FGuid RequestId);

	UFUNCTION(Server, Reliable)
	void ServerBuy(class AWETraderTerminal* Terminal, int32 OfferIndex, int32 Quantity, FGuid RequestId);

	UFUNCTION(Server, Reliable)
	void ServerAcceptContract(class AWETraderTerminal* Terminal, FGuid ContractId, FGuid RequestId);

	UFUNCTION(Server, Reliable)
	void ServerBuyUpgrade(class AWETraderTerminal* Terminal, FName UpgradeId, FGuid RequestId);

	UFUNCTION(Server, Reliable)
	void ServerBid(class AWETraderTerminal* Terminal, FGuid LotId, FGuid RequestId);

	UFUNCTION(Server, Reliable)
	void ServerTakeAll(UWEInventoryComponent* From);

	UFUNCTION(Server, Reliable)
	void ServerDeliverContract(class AWETraderTerminal* Terminal, FGuid ContractId, FGuid RequestId);

	void EnterMenuInputMode(UUserWidget* Widget);

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UWETradeWidget> TradeWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<class UWETradeWidget> TradeWidget;

	void ShowLocalMessage(const FText& Message);

	/** HUD class; can be swapped for a UMG subclass. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UWEHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UWEInventoryWidget> InventoryWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UWEHUDWidget> HUDWidget;

	UPROPERTY(Transient)
	TObjectPtr<UWEInventoryWidget> InventoryWidget;
};
