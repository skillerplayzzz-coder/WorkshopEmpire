#include "Framework/WEPlayerController.h"

#include "Economy/WEAuctionComponent.h"
#include "Workshop/WEWorkshopLayoutComponent.h"
#include "World/WEPlaceholderProp.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WECompanyComponent.h"

#include "Blueprint/UserWidget.h"
#include "Dev/WEDevAutomationComponent.h"
#include "Framework/WEGameMode.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "UI/WEHUDWidget.h"
#include "UI/WEInventoryWidget.h"
#include "UI/WETradeWidget.h"
#include "Economy/WETraderTerminal.h"
#include "WELog.h"

#define LOCTEXT_NAMESPACE "WEPlayerController"

AWEPlayerController::AWEPlayerController()
{
	HUDWidgetClass = UWEHUDWidget::StaticClass();
	InventoryWidgetClass = UWEInventoryWidget::StaticClass();
	TradeWidgetClass = UWETradeWidget::StaticClass();
}

void AWEPlayerController::BeginPlay()
{
	Super::BeginPlay();

	const TCHAR* NetModeName = TEXT("Unknown");
	switch (GetNetMode())
	{
	case NM_Standalone:      NetModeName = TEXT("Standalone"); break;
	case NM_DedicatedServer: NetModeName = TEXT("DedicatedServer"); break;
	case NM_ListenServer:    NetModeName = TEXT("ListenServer"); break;
	case NM_Client:          NetModeName = TEXT("Client"); break;
	default: break;
	}

	UE_LOG(LogWENet, Log, TEXT("%s BeginPlay (NetMode=%s, Local=%s)"),
		*GetName(), NetModeName, IsLocalController() ? TEXT("yes") : TEXT("no"));

	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);

		if (HUDWidgetClass)
		{
			HUDWidget = CreateWidget<UWEHUDWidget>(this, HUDWidgetClass);
			if (HUDWidget)
			{
				HUDWidget->AddToViewport(0);
			}
		}

		if (UWEDevAutomationComponent::IsRequested())
		{
			UWEDevAutomationComponent* Automation = NewObject<UWEDevAutomationComponent>(this, TEXT("DevAutomation"));
			Automation->RegisterComponent();
		}
	}
}

// --- Inventory UI --------------------------------------------------------------------------------

bool AWEPlayerController::IsInventoryOpen() const
{
	return InventoryWidget && InventoryWidget->IsInViewport();
}

void AWEPlayerController::ToggleInventory()
{
	if (IsInventoryOpen())
	{
		CloseInventory();
	}
	else
	{
		OpenInventory(nullptr);
	}
}

void AWEPlayerController::OpenInventory(UWEInventoryComponent* Container)
{
	const AWECharacter* WECharacter = GetPawn<AWECharacter>();
	UWEInventoryComponent* PlayerInventory = WECharacter ? WECharacter->GetPlayerInventory() : nullptr;
	if (!IsLocalController() || !PlayerInventory || !InventoryWidgetClass)
	{
		return;
	}
	CloseTrade();
	if (!InventoryWidget)
	{
		InventoryWidget = CreateWidget<UWEInventoryWidget>(this, InventoryWidgetClass);
	}
	if (!InventoryWidget)
	{
		return;
	}
	InventoryWidget->Setup(PlayerInventory, Container);
	if (!InventoryWidget->IsInViewport())
	{
		InventoryWidget->AddToViewport(10);
	}

	EnterMenuInputMode(InventoryWidget);
}

void AWEPlayerController::CloseInventory()
{
	if (InventoryWidget && InventoryWidget->IsInViewport())
	{
		InventoryWidget->RemoveFromParent();
	}
	if (!IsTradeOpen())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		ResetIgnoreLookInput();
		ResetIgnoreMoveInput();
	}
}

void AWEPlayerController::EnterMenuInputMode(UUserWidget* Widget)
{
	FInputModeGameAndUI Mode;
	Mode.SetWidgetToFocus(Widget->TakeWidget());
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	SetShowMouseCursor(true);
	ResetIgnoreLookInput();
	ResetIgnoreMoveInput();
	SetIgnoreLookInput(true);
	SetIgnoreMoveInput(true);
}

// --- Trade UI ------------------------------------------------------------------------------------

bool AWEPlayerController::IsTradeOpen() const
{
	return TradeWidget && TradeWidget->IsInViewport();
}

void AWEPlayerController::OpenTrade(AWETraderTerminal* Terminal)
{
	if (!IsLocalController() || !Terminal || !TradeWidgetClass)
	{
		return;
	}
	CloseInventory();
	if (!TradeWidget)
	{
		TradeWidget = CreateWidget<UWETradeWidget>(this, TradeWidgetClass);
	}
	if (!TradeWidget)
	{
		return;
	}
	TradeWidget->Setup(Terminal);
	if (!TradeWidget->IsInViewport())
	{
		TradeWidget->AddToViewport(10);
	}
	EnterMenuInputMode(TradeWidget);
}

void AWEPlayerController::CloseTrade()
{
	if (TradeWidget && TradeWidget->IsInViewport())
	{
		TradeWidget->RemoveFromParent();
	}
	if (!IsInventoryOpen())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		ResetIgnoreLookInput();
		ResetIgnoreMoveInput();
	}
}

void AWEPlayerController::ClientOpenTrade_Implementation(AWETraderTerminal* Terminal)
{
	OpenTrade(Terminal);
}

// --- Trade requests --------------------------------------------------------------------------------

void AWEPlayerController::RequestSell(AWETraderTerminal* Terminal, UWEInventoryComponent* From, int32 Slot, int32 Quantity)
{
	ServerSell(Terminal, From, Slot, Quantity, FGuid::NewGuid());
}

void AWEPlayerController::RequestSellHeld(AWETraderTerminal* Terminal)
{
	ServerSellHeld(Terminal, FGuid::NewGuid());
}

void AWEPlayerController::RequestBuy(AWETraderTerminal* Terminal, int32 OfferIndex, int32 Quantity)
{
	ServerBuy(Terminal, OfferIndex, Quantity, FGuid::NewGuid());
}

void AWEPlayerController::RequestAcceptContract(AWETraderTerminal* Terminal, const FGuid& ContractId)
{
	ServerAcceptContract(Terminal, ContractId, FGuid::NewGuid());
}

void AWEPlayerController::RequestBuyUpgrade(AWETraderTerminal* Terminal, FName UpgradeId)
{
	ServerBuyUpgrade(Terminal, UpgradeId, FGuid::NewGuid());
}

void AWEPlayerController::RequestBid(AWETraderTerminal* Terminal, const FGuid& LotId)
{
	ServerBid(Terminal, LotId, FGuid::NewGuid());
}

void AWEPlayerController::ServerPlaceFurniture_Implementation(FName FurnitureId, FTransform Transform, FGuid RequestId)
{
	if (UWEWorkshopLayoutComponent* Layout = UWEWorkshopLayoutComponent::Get(this))
	{
		Layout->PlaceFurniture(GetPawn<AWECharacter>(), FurnitureId, Transform, RequestId);
	}
}

void AWEPlayerController::ServerMoveFurniture_Implementation(AWEPlaceholderProp* Prop, FTransform Transform)
{
	if (UWEWorkshopLayoutComponent* Layout = UWEWorkshopLayoutComponent::Get(this))
	{
		Layout->MoveFurniture(GetPawn<AWECharacter>(), Prop, Transform);
	}
}

void AWEPlayerController::ServerRemoveFurniture_Implementation(AWEPlaceholderProp* Prop, FGuid RequestId)
{
	if (UWEWorkshopLayoutComponent* Layout = UWEWorkshopLayoutComponent::Get(this))
	{
		Layout->RemoveFurniture(GetPawn<AWECharacter>(), Prop, RequestId);
	}
}

void AWEPlayerController::ServerBid_Implementation(AWETraderTerminal* Terminal, FGuid LotId, FGuid RequestId)
{
	if (Terminal)
	{
		Terminal->BidOnAuction(GetPawn<AWECharacter>(), LotId, RequestId);
	}
}

void AWEPlayerController::ServerBuyUpgrade_Implementation(AWETraderTerminal* Terminal, FName UpgradeId, FGuid RequestId)
{
	if (Terminal)
	{
		Terminal->BuyUpgrade(GetPawn<AWECharacter>(), UpgradeId, RequestId);
	}
}

void AWEPlayerController::RequestTakeAll(UWEInventoryComponent* From)
{
	ServerTakeAll(From);
}

void AWEPlayerController::ServerTakeAll_Implementation(UWEInventoryComponent* From)
{
	ExecuteTakeAll(From);
}

bool AWEPlayerController::ExecuteTakeAll(UWEInventoryComponent* From)
{
	const AWECharacter* WECharacter = GetPawn<AWECharacter>();
	UWEInventoryComponent* Mine = WECharacter ? WECharacter->GetPlayerInventory() : nullptr;
	if (!HasAuthority() || !Mine || From == Mine || !CanUseInventory(From))
	{
		return false;
	}
	const int32 Moved = From->TransferAllTo(Mine);
	if (From->GetAllSlots().Num() > 0)
	{
		ClientShowMessage(LOCTEXT("NotAllFit", "Nicht alles passt ins Inventar"));
	}
	return Moved > 0;
}

void AWEPlayerController::RequestDeliverContract(AWETraderTerminal* Terminal, const FGuid& ContractId)
{
	ServerDeliverContract(Terminal, ContractId, FGuid::NewGuid());
}

void AWEPlayerController::DevRequestSellTwice(AWETraderTerminal* Terminal, UWEInventoryComponent* From, int32 Slot, int32 Quantity)
{
#if !UE_BUILD_SHIPPING
	const FGuid Same = FGuid::NewGuid();
	ServerSell(Terminal, From, Slot, Quantity, Same);
	ServerSell(Terminal, From, Slot, Quantity, Same);
#endif
}

void AWEPlayerController::ServerSell_Implementation(AWETraderTerminal* Terminal, UWEInventoryComponent* From, int32 Slot, int32 Quantity, FGuid RequestId)
{
	if (Terminal)
	{
		Terminal->SellFromInventory(GetPawn<AWECharacter>(), From, Slot, Quantity, RequestId);
	}
}

void AWEPlayerController::ServerSellHeld_Implementation(AWETraderTerminal* Terminal, FGuid RequestId)
{
	if (Terminal)
	{
		Terminal->SellHeldItem(GetPawn<AWECharacter>(), RequestId);
	}
}

void AWEPlayerController::ServerBuy_Implementation(AWETraderTerminal* Terminal, int32 OfferIndex, int32 Quantity, FGuid RequestId)
{
	if (Terminal)
	{
		Terminal->Buy(GetPawn<AWECharacter>(), OfferIndex, Quantity, RequestId);
	}
}

void AWEPlayerController::ServerAcceptContract_Implementation(AWETraderTerminal* Terminal, FGuid ContractId, FGuid RequestId)
{
	if (Terminal)
	{
		Terminal->AcceptContract(GetPawn<AWECharacter>(), ContractId, RequestId);
	}
}

void AWEPlayerController::ServerDeliverContract_Implementation(AWETraderTerminal* Terminal, FGuid ContractId, FGuid RequestId)
{
	if (Terminal)
	{
		Terminal->DeliverContract(GetPawn<AWECharacter>(), ContractId, RequestId);
	}
}

void AWEPlayerController::ServerDevTeleport_Implementation(FVector Location, FRotator Rotation)
{
#if !UE_BUILD_SHIPPING
	if (APawn* MyPawn = GetPawn())
	{
		MyPawn->TeleportTo(Location, Rotation, false, true);
		ClientSetRotation(Rotation);
		UE_LOG(LogWENet, Log, TEXT("%s dev-teleported to %s"), *GetName(), *Location.ToCompactString());
	}
#endif
}

void AWEPlayerController::ServerDevEndAuctions_Implementation()
{
#if !UE_BUILD_SHIPPING
	if (UWEAuctionComponent* Auctions = UWEAuctionComponent::Get(this))
	{
		TArray<FGuid> Leading;
		for (const FWEAuctionLot& Lot : Auctions->GetLots())
		{
			if (Lot.bCompanyLeads)
			{
				Leading.Add(Lot.Id);
			}
		}
		for (const FGuid& Id : Leading)
		{
			Auctions->FinishLot(Id);
		}
		UE_LOG(LogWENet, Log, TEXT("%s dev-ended %d auctions"), *GetName(), Leading.Num());
	}
#endif
}

void AWEPlayerController::ServerDevAddContractOffer_Implementation(int32 TemplateIndex)
{
#if !UE_BUILD_SHIPPING
	if (UWEContractComponent* Contracts = UWEContractComponent::Get(this))
	{
		const bool bAdded = Contracts->AddOffer(TemplateIndex);
		UE_LOG(LogWENet, Log, TEXT("%s dev-added contract offer from template %d: %s"), *GetName(), TemplateIndex, bAdded ? TEXT("ok") : TEXT("failed"));
	}
#endif
}

void AWEPlayerController::ServerDevGiveItem_Implementation(FName ItemId, int32 Quantity, EWEItemCondition Condition)
{
#if !UE_BUILD_SHIPPING
	UWEItemDefinition* Definition = UWEItemDefinition::FindById(ItemId);
	AWECharacter* Me = Cast<AWECharacter>(GetPawn());
	if (!Definition || !Me || Quantity <= 0)
	{
		return;
	}
	const FWEItemInstance Item = FWEItemInstance::Make(Definition, Quantity, Condition);
	int32 Added = 0;
	if (Definition->bFitsInInventory && Me->GetPlayerInventory())
	{
		Me->GetPlayerInventory()->TryAddItem(Item, Added);
	}
	// The rest in front of the pawn: one actor per stack (non-stackable items one by one, side by side).
	for (int32 Left = Quantity - Added, Index = 0; Left > 0; ++Index)
	{
		FWEItemInstance Rest = Item;
		Rest.Quantity = FMath::Min(Left, Definition->GetMaxStackSize());
		Left -= Rest.Quantity;
		const FVector Front = Me->GetActorLocation() + Me->GetActorForwardVector() * 80.f + Me->GetActorRightVector() * (Index * 60.f) + FVector(0.f, 0.f, -40.f);
		AWEPickupItem::SpawnWorldItem(GetWorld(), Rest, FTransform(Me->GetActorRotation(), Front));
	}
	UE_LOG(LogWENet, Log, TEXT("%s dev-got %d x %s"), *GetName(), Quantity, *ItemId.ToString());
#endif
}

void AWEPlayerController::ServerDevAddMoney_Implementation(int32 Euros)
{
#if !UE_BUILD_SHIPPING
	if (UWECompanyComponent* Company = UWECompanyComponent::Get(this))
	{
		Company->SetBalance(FWEMoney::FromCents(Company->GetBalance().GetCents() + Euros * 100ll));
	}
#endif
}

void AWEPlayerController::ClientOpenContainer_Implementation(UWEInventoryComponent* Container)
{
	OpenInventory(Container);
}

void AWEPlayerController::ClientShowMessage_Implementation(const FText& Message)
{
	ShowLocalMessage(Message);
}

void AWEPlayerController::ShowLocalMessage(const FText& Message)
{
	if (HUDWidget)
	{
		HUDWidget->ShowMessage(Message);
	}
	UE_LOG(LogWorkshopEmpire, Display, TEXT("Message: %s"), *Message.ToString());
}

// --- Inventory requests ------------------------------------------------------------------------------

void AWEPlayerController::RequestMoveItem(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To, int32 ToSlot, int32 Quantity)
{
	ServerMoveItem(From, FromSlot, To, ToSlot, Quantity);
}

void AWEPlayerController::RequestQuickTransfer(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To)
{
	ServerQuickTransfer(From, FromSlot, To);
}

void AWEPlayerController::RequestDropFromInventory(UWEInventoryComponent* From, int32 Slot, int32 Quantity)
{
	ServerDropFromInventory(From, Slot, Quantity);
}

void AWEPlayerController::RequestSort(UWEInventoryComponent* Inventory)
{
	ServerSortInventory(Inventory);
}

void AWEPlayerController::ServerMoveItem_Implementation(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To, int32 ToSlot, int32 Quantity)
{
	ExecuteMoveItem(From, FromSlot, To, ToSlot, Quantity);
}

void AWEPlayerController::ServerQuickTransfer_Implementation(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To)
{
	ExecuteQuickTransfer(From, FromSlot, To);
}

void AWEPlayerController::ServerDropFromInventory_Implementation(UWEInventoryComponent* From, int32 Slot, int32 Quantity)
{
	ExecuteDropFromInventory(From, Slot, Quantity);
}

void AWEPlayerController::ServerSortInventory_Implementation(UWEInventoryComponent* Inventory)
{
	if (CanUseInventory(Inventory))
	{
		Inventory->SortItems();
	}
}

bool AWEPlayerController::CanUseInventory(const UWEInventoryComponent* Inventory) const
{
	const AWECharacter* WECharacter = GetPawn<AWECharacter>();
	const bool bOk = Inventory && Inventory->CanBeAccessedBy(WECharacter);
	if (!bOk && Inventory)
	{
		UE_LOG(LogWENet, Warning, TEXT("%s denied access to inventory %s"), *GetName(), *GetPathNameSafe(Inventory));
	}
	return bOk;
}

bool AWEPlayerController::ExecuteMoveItem(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To, int32 ToSlot, int32 Quantity)
{
	if (!HasAuthority() || !CanUseInventory(From) || !CanUseInventory(To))
	{
		return false;
	}
	const EWEInventoryResult Result = From->MoveItem(FromSlot, To, ToSlot, Quantity);
	if (Result == EWEInventoryResult::TooHeavy)
	{
		ClientShowMessage(LOCTEXT("TooHeavy", "Zu schwer für dieses Lager"));
	}
	else if (Result == EWEInventoryResult::NoSpace)
	{
		ClientShowMessage(LOCTEXT("NoSpace", "Kein Platz"));
	}
	return Result == EWEInventoryResult::Success || Result == EWEInventoryResult::PartiallyAdded;
}

bool AWEPlayerController::ExecuteQuickTransfer(UWEInventoryComponent* From, int32 FromSlot, UWEInventoryComponent* To)
{
	if (!HasAuthority() || From == To || !CanUseInventory(From) || !CanUseInventory(To))
	{
		return false;
	}
	const FWEItemInstance* Source = From->GetItemInSlot(FromSlot);
	if (!Source)
	{
		return false;
	}
	const int32 Movable = To->GetAddableQuantity(*Source);
	if (Movable <= 0)
	{
		ClientShowMessage(LOCTEXT("TargetFull", "Kein Platz im Ziel"));
		return false;
	}
	FWEItemInstance Removed;
	From->RemoveFromSlot(FromSlot, Movable, Removed);
	int32 Added = 0;
	To->TryAddItem(Removed, Added);
	if (Added < Removed.Quantity)
	{
		// Should not happen (we checked first); never lose items: put the rest back.
		FWEItemInstance Rest = Removed;
		Rest.Quantity -= Added;
		int32 Back = 0;
		From->TryAddItem(Rest, Back);
	}
	return Added > 0;
}

bool AWEPlayerController::ExecuteDropFromInventory(UWEInventoryComponent* From, int32 Slot, int32 Quantity)
{
	AWECharacter* WECharacter = GetPawn<AWECharacter>();
	if (!HasAuthority() || !WECharacter || !CanUseInventory(From))
	{
		return false;
	}
	const FWEItemInstance* Source = From->GetItemInSlot(Slot);
	if (!Source)
	{
		return false;
	}
	const float Radius = Source->Definition ? Source->Definition->WorldScale.GetMax() * 50.f : 20.f;
	FWEItemInstance Removed;
	if (From->RemoveFromSlot(Slot, Quantity <= 0 ? Source->Quantity : Quantity, Removed) != EWEInventoryResult::Success)
	{
		return false;
	}
	const FTransform DropTransform = WECharacter->ComputeDropTransform(Radius, nullptr);
	AWEPickupItem* Spawned = AWEPickupItem::SpawnWorldItem(GetWorld(), Removed, DropTransform);
	if (!Spawned)
	{
		// Never lose the item if spawning fails.
		int32 Back = 0;
		From->TryAddItem(Removed, Back);
		return false;
	}
	UE_LOG(LogWENet, Log, TEXT("%s dropped %d x %s from inventory"), *GetName(), Removed.Quantity, *Removed.GetDisplayName().ToString());
	return true;
}

// --- Save / load -------------------------------------------------------------------------------------

void AWEPlayerController::RequestSave(const FString& Slot)
{
	AWEGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AWEGameMode>() : nullptr;
	if (!GameMode || !IsLocalController())
	{
		ShowLocalMessage(LOCTEXT("HostOnlySave", "Nur der Host kann speichern."));
		return;
	}
	GameMode->SaveToSlot(Slot);
}

void AWEPlayerController::RequestLoad(const FString& Slot)
{
	AWEGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AWEGameMode>() : nullptr;
	if (!GameMode || !IsLocalController())
	{
		ShowLocalMessage(LOCTEXT("HostOnlyLoad", "Nur der Host kann laden."));
		return;
	}
	CloseInventory();
	GameMode->LoadFromSlot(Slot);
}

void AWEPlayerController::WESave(const FString& Slot)
{
	RequestSave(Slot.IsEmpty() ? TEXT("Quicksave") : Slot);
}

void AWEPlayerController::WELoad(const FString& Slot)
{
	RequestLoad(Slot.IsEmpty() ? TEXT("Quicksave") : Slot);
}

#undef LOCTEXT_NAMESPACE
