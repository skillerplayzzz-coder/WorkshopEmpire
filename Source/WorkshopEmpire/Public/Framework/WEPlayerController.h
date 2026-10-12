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
