#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Items/WEItemTypes.h"
#include "Workshop/WEBrandComponent.h"
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

	// --- Work view at the workbench (Phase 17.5, local) ------------------------------------------

	/** Close-up of the device on Bench; tools and hands-on tasks. bRepair: start in repair mode. */
	void OpenWorkView(class AWEWorkbench* Bench, bool bRepair);
	void CloseWorkView();
	bool IsWorkViewOpen() const;
	class UWEWorkViewWidget* GetWorkView() const { return WorkViewWidget; }

	/** Work view requests (server checks reach, tool, order and time). */
	void RequestWorkStart(class AWEWorkbench* Bench, bool bRepair, int32 Part = INDEX_NONE, bool bInstall = false);
	void RequestWorkTask(class AWEWorkbench* Bench, int32 TaskIndex, EWEToolType Tool);
	void RequestWorkAuto(class AWEWorkbench* Bench, bool bRepair, int32 Part = INDEX_NONE, bool bInstall = false);
	void RequestWorkCancel(class AWEWorkbench* Bench);

	/** Any full-screen menu (inventory, trade, pause, work view) open. */
	bool IsMenuOpen() const { return IsInventoryOpen() || IsTradeOpen() || IsPauseMenuOpen() || IsWorkViewOpen() || IsMirrorOpen(); }

	// --- Phase 19: the mirror (look editor in game) ---------------------------------------------------
	void OpenMirror();
	void CloseMirror();
	bool IsMirrorOpen() const;
	class UWEMirrorWidget* GetMirrorWidget() const { return MirrorWidget; }
	/** Server -> owning client: open the look editor at the mirror. */
	UFUNCTION(Client, Reliable)
	void ClientOpenMirror();

	// --- Pause menu and session (Phase 13) ----------------------------------------------------------

	void TogglePauseMenu();
	void OpenPauseMenu();
	void ClosePauseMenu();
	bool IsPauseMenuOpen() const;
	class UWEPauseMenuWidget* GetPauseMenu() const { return PauseMenuWidget; }

	/** Host: saves ("Autosave"), sends every client to the main menu, then leaves. Client: leaves. */
	void RequestLeaveGame(bool bQuitAfterwards = false);
	/** Host only: removes a player from the session. */
	void RequestKick(APlayerState* KickedPlayer);
	/** Host only: saves to the slot the game was loaded from (or "Quicksave"). */
	void RequestSaveCurrent();

	/** Server -> client: the host ended the game or removed this player; back to the main menu with Reason. */
	UFUNCTION(Client, Reliable)
	void ClientSessionEnded(const FText& Reason);

	virtual void ClientWasKicked_Implementation(const FText& KickReason) override;
	/** Server: a remote player leaves (quit, kicked, connection lost): remember position and inventory first. */
	virtual void PawnLeavingGame() override;

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
	/** Phase 17.5: sell an old building / move its loose items and employees out first. */
	void RequestSellBuilding(class AWETraderTerminal* Terminal, FName UpgradeId);
	void RequestClearBuilding(class AWETraderTerminal* Terminal, FName UpgradeId);
	/** Phase 17.5: personalise a building (paint, floor, lighting, door) from the workshop PC. */
	void RequestApplyStyle(class AWETraderTerminal* Terminal, FName Zone, uint8 Category, int32 Option);
	/** Phase 20: new company logo / work clothes colour (free). */
	void RequestSetBrand(const FWECompanyBrand& Brand);
	UFUNCTION(Server, Reliable)
	void ServerSetBrand(const FWECompanyBrand& Brand);
	/** Phase 17.5 tutorial: 0 = start / restart, 1 = skip. */
	void RequestTutorial(uint8 Action);
	/** Phase 17.5 tutorial: things only the client sees (inventory opened, work view, PC). */
	void NotifyTutorial(uint8 Event);

	// Staff (Phase 11).
	void RequestHire(class AWETraderTerminal* Terminal, int32 ApplicantIndex);
	/** Fire / pause / resume, from the terminal's staff page or standing next to the employee. */
	void RequestStaffAction(class AWETraderTerminal* Terminal, class AWEEmployee* Employee, uint8 Action);
	void RequestAssignTask(class AWEEmployee* Employee, AActor* Source, AActor* Target);

	enum EStaffAction : uint8 { StaffFire = 0, StaffPause = 1, StaffResume = 2 };

	/** Moves every stack from a container into the player inventory (as much as fits). */
	void RequestTakeAll(UWEInventoryComponent* From);

	/** Phase 19: Ferrit OS classifieds (the server checks money, access and the seller's answer). */
	void RequestClassifiedOffer(const FGuid& AdId, int64 Cents);
	void RequestClassifiedAcceptCounter(const FGuid& AdId);
	void RequestClassifiedList(UWEInventoryComponent* From, int32 Slot, int64 Cents);
	void RequestClassifiedAcceptBuyer(const FGuid& ListingId);
	void RequestClassifiedWithdraw(const FGuid& ListingId);

	/** Server-side execution of RequestTakeAll (public for tests). */
	bool ExecuteTakeAll(UWEInventoryComponent* From);
	void RequestDeliverContract(class AWETraderTerminal* Terminal, const FGuid& ContractId);
	/** Quality overhaul: give an accepted order back. */
	void RequestCancelContract(const FGuid& ContractId);
	UFUNCTION(Server, Reliable)
	void ServerCancelContract(FGuid ContractId);

	/** Mike: skip the night to 07:30 (only while closed) and fast-forward (1x / 2x / 4x). Host (or solo) only. */
	void RequestSkipNight();
	void RequestTimeSpeed(float Speed);
	UFUNCTION(Server, Reliable)
	void ServerSkipNight();
	UFUNCTION(Server, Reliable)
	void ServerTimeSpeed(float Speed);
	/** True if this player may change the time (solo or the host). */
	bool CanControlTime() const;

	/** Test hook: sends the same request id twice (must be booked once). Not in Shipping. */
	void DevRequestSellTwice(class AWETraderTerminal* Terminal, UWEInventoryComponent* From, int32 Slot, int32 Quantity);

	/** Development builds only: moves the pawn on the server (test automation). */
	UFUNCTION(Server, Reliable)
	void ServerDevTeleport(FVector Location, FRotator Rotation);

	/** Development builds only: ends every auction the company currently leads (test automation). */
	UFUNCTION(Server, Reliable)
	void ServerDevEndAuctions();

	class UWETradeWidget* GetTradeWidget() const { return TradeWidget; }
	UWEInventoryWidget* GetInventoryWidget() const { return InventoryWidget; }

	/** Development builds only: adds a customer order offer from a fixed template (test automation). */
	UFUNCTION(Server, Reliable)
	void ServerDevAddContractOffer(int32 TemplateIndex);

	/** Development builds only: gives an item (inventory, or in front of the pawn if bulky) (test automation). */
	UFUNCTION(Server, Reliable)
	void ServerDevGiveItem(FName ItemId, int32 Quantity, EWEItemCondition Condition);

	/** Development builds only: adds money to the company account (test automation). */
	UFUNCTION(Server, Reliable)
	void ServerDevAddMoney(int32 Euros);

	/** Development builds only (Phase 21 net test): 1 = rules Realistisch + two orders + courier now. */
	UFUNCTION(Server, Reliable)
	void ServerDevPhase21(uint8 Action);

	/** Server -> owning client: short on-screen message ("Inventar voll", "Gespeichert"). */
	UFUNCTION(Client, Reliable)
	void ClientShowMessage(const FText& Message);

	// --- Inventory requests (local wrappers around the Server RPCs) ----------------------------

	void RequestMoveItem(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To, int32 ToSlot, int32 Quantity);
	void RequestQuickTransfer(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To);
	void RequestDropFromInventory(UWEInventoryComponent* From, int32 Slot, int32 Quantity);
	/** Quality update: one item of the stack into the hands (to carry it to the workbench). */
	void RequestTakeIntoHands(UWEInventoryComponent* From, int32 Slot);
	/** Mode: 0 category, 1 name, 2 value, 3 weight (Phase 19). */
	void RequestSort(UWEInventoryComponent* Inventory, uint8 Mode = 0);
	/** Phase 17.5: favourite / lock / release-for-sale marks on a stack (FWEItemInstance::EMarks). */
	void RequestSetItemMarks(UWEInventoryComponent* Inventory, int32 Slot, uint8 Mask, bool bSet);
	bool ExecuteSetItemMarks(UWEInventoryComponent* Inventory, int32 Slot, uint8 Mask, bool bSet);

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
	bool ExecuteTakeIntoHands(UWEInventoryComponent* From, int32 Slot);

protected:
	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable)
	void ServerMoveItem(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To, int32 ToSlot, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerQuickTransfer(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To);

	UFUNCTION(Server, Reliable)
	void ServerDropFromInventory(UWEInventoryComponent* From, int32 Slot, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerTakeIntoHands(UWEInventoryComponent* From, int32 Slot);

	UFUNCTION(Server, Reliable)
	void ServerSortInventory(UWEInventoryComponent* Inventory, uint8 Mode);

	UFUNCTION(Server, Reliable)
	void ServerSetItemMarks(UWEInventoryComponent* Inventory, int32 Slot, uint8 Mask, bool bSet);

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
	void ServerHire(class AWETraderTerminal* Terminal, int32 ApplicantIndex, FGuid RequestId);

	UFUNCTION(Server, Reliable)
	void ServerStaffAction(class AWETraderTerminal* Terminal, class AWEEmployee* Employee, uint8 Action);

	UFUNCTION(Server, Reliable)
	void ServerAssignTask(class AWEEmployee* Employee, AActor* Source, AActor* Target);

	UFUNCTION(Server, Reliable)
	void ServerBid(class AWETraderTerminal* Terminal, FGuid LotId, FGuid RequestId);

	UFUNCTION(Server, Reliable)
	void ServerSellBuilding(class AWETraderTerminal* Terminal, FName UpgradeId, FGuid RequestId);

	UFUNCTION(Server, Reliable)
	void ServerClearBuilding(class AWETraderTerminal* Terminal, FName UpgradeId);

	UFUNCTION(Server, Reliable)
	void ServerApplyStyle(class AWETraderTerminal* Terminal, FName Zone, uint8 Category, int32 Option, FGuid RequestId);

	UFUNCTION(Server, Reliable)
	void ServerTutorial(uint8 Action);

	/** Phase 17.5: left the workshop PC (the monitor stops showing this player's name). */
	UFUNCTION(Server, Reliable)
	void ServerLeaveComputer(class AWETraderTerminal* Terminal);

	UFUNCTION(Server, Reliable)
	void ServerTutorialEvent(uint8 Event);

	UFUNCTION(Server, Reliable)
	void ServerWorkStart(class AWEWorkbench* Bench, bool bRepair, int32 Part, bool bInstall);

	UFUNCTION(Server, Reliable)
	void ServerWorkTask(class AWEWorkbench* Bench, uint8 TaskIndex, EWEToolType Tool);

	UFUNCTION(Server, Reliable)
	void ServerWorkAuto(class AWEWorkbench* Bench, bool bRepair, int32 Part, bool bInstall);

	UFUNCTION(Server, Reliable)
	void ServerWorkCancel(class AWEWorkbench* Bench);

	UPROPERTY(Transient)
	TObjectPtr<class UWEWorkViewWidget> WorkViewWidget;

public:
	// --- Build mode (Phase 8): the client sends the snapped transform, the server checks everything again.
	UFUNCTION(Server, Reliable)
	void ServerPlaceFurniture(FName FurnitureId, FTransform Transform, FGuid RequestId);

	UFUNCTION(Server, Reliable)
	void ServerMoveFurniture(class AWEPlaceholderProp* Prop, FTransform Transform);

	UFUNCTION(Server, Reliable)
	void ServerRemoveFurniture(class AWEPlaceholderProp* Prop, FGuid RequestId);

protected:

	UFUNCTION(Server, Reliable)
	void ServerTakeAll(UWEInventoryComponent* From);

	UFUNCTION(Server, Reliable) void ServerClassifiedOffer(FGuid AdId, int64 Cents);
	UFUNCTION(Server, Reliable) void ServerClassifiedAcceptCounter(FGuid AdId);
	UFUNCTION(Server, Reliable) void ServerClassifiedList(UWEInventoryComponent* From, int32 Slot, int64 Cents);
	UFUNCTION(Server, Reliable) void ServerClassifiedAcceptBuyer(FGuid ListingId);
	UFUNCTION(Server, Reliable) void ServerClassifiedWithdraw(FGuid ListingId);

	UFUNCTION(Server, Reliable)
	void ServerDeliverContract(class AWETraderTerminal* Terminal, FGuid ContractId, FGuid RequestId);

	void EnterMenuInputMode(UUserWidget* Widget);

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UWETradeWidget> TradeWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<class UWETradeWidget> TradeWidget;

	/** Phase 17.5: the workshop PC this player is sitting at (view on its monitor). */
	TWeakObjectPtr<class AWETraderTerminal> ComputerTerminal;

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

	UPROPERTY(Transient)
	TObjectPtr<class UWEPauseMenuWidget> PauseMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<class UWEMirrorWidget> MirrorWidget;

	FTimerHandle LeaveTimer;
};
