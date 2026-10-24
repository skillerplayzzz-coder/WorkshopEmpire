#include "Framework/WEPlayerController.h"
#include "Framework/WEGameRulesComponent.h"
#include "Economy/WECourierComponent.h"
#include "Economy/WEClassifiedsComponent.h"
#include "GameFramework/PlayerState.h"
#include "UI/WEMenuWidgets.h"
#include "Online/WESessionSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "Save/WESaveSystem.h"

#include "Economy/WEAuctionComponent.h"
#include "Workshop/WEWorkshopLayoutComponent.h"
#include "World/WEPlaceholderProp.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WECompanyComponent.h"
#include "Staff/WEEmployee.h"
#include "Staff/WEStaffComponent.h"
#include "Progression/WETutorialComponent.h"
#include "Workshop/WEWorkshopStyleComponent.h"
#include "UI/WESettings.h"
#include "UI/WEUIKit.h"
#include "UI/WEWorkViewWidget.h"
#include "Interaction/WEInteractionComponent.h"
#include "Workshop/WEWorkbench.h"

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
#include "UI/WEMirrorWidget.h"
#include "Workshop/WEBrandComponent.h"
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
		WESettings::ApplyGameValues(WESettings::Load()); // UI scale, mouse, field of view, volume

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
	NotifyTutorial(static_cast<uint8>(EWETutorialEvent::InventoryOpened));
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
	// Phase 17.5: sit down at the PC - the view moves to the monitor, the desktop fades in on it.
	if (ComputerTerminal != Terminal)
	{
		ComputerTerminal = Terminal;
		if (Terminal->GetScreenCamera())
		{
			SetViewTargetWithBlend(Terminal, 0.45f, VTBlend_Cubic);
		}
		Terminal->SetLocalUser(GetPawn<AWECharacter>(), true);
		if (HUDWidget)
		{
			HUDWidget->SetFocusMode(true);
		}
		NotifyTutorial(static_cast<uint8>(EWETutorialEvent::ComputerOpened));
	}
}

void AWEPlayerController::CloseTrade()
{
	if (TradeWidget && TradeWidget->IsInViewport())
	{
		TradeWidget->RemoveFromParent();
	}
	if (AWETraderTerminal* Terminal = ComputerTerminal.Get())
	{
		// Stand up again: the view goes back to the eyes.
		ComputerTerminal.Reset();
		if (GetPawn())
		{
			SetViewTargetWithBlend(GetPawn(), 0.35f, VTBlend_Cubic);
		}
		Terminal->SetLocalUser(GetPawn<AWECharacter>(), false);
		ServerLeaveComputer(Terminal);
		if (HUDWidget)
		{
			HUDWidget->SetFocusMode(false);
		}
	}
	if (!IsInventoryOpen() && !IsPauseMenuOpen() && !IsWorkViewOpen())
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

void AWEPlayerController::RequestSellBuilding(AWETraderTerminal* Terminal, FName UpgradeId)
{
	ServerSellBuilding(Terminal, UpgradeId, FGuid::NewGuid());
}

void AWEPlayerController::RequestClearBuilding(AWETraderTerminal* Terminal, FName UpgradeId)
{
	ServerClearBuilding(Terminal, UpgradeId);
}

void AWEPlayerController::ServerSellBuilding_Implementation(AWETraderTerminal* Terminal, FName UpgradeId, FGuid RequestId)
{
	if (Terminal)
	{
		Terminal->SellBuilding(GetPawn<AWECharacter>(), UpgradeId, RequestId);
	}
}

void AWEPlayerController::ServerClearBuilding_Implementation(AWETraderTerminal* Terminal, FName UpgradeId)
{
	if (Terminal)
	{
		Terminal->ClearBuilding(GetPawn<AWECharacter>(), UpgradeId);
	}
}

void AWEPlayerController::RequestApplyStyle(AWETraderTerminal* Terminal, FName Zone, uint8 Category, int32 Option)
{
	ServerApplyStyle(Terminal, Zone, Category, Option, FGuid::NewGuid());
}

void AWEPlayerController::ServerApplyStyle_Implementation(AWETraderTerminal* Terminal, FName Zone, uint8 Category, int32 Option, FGuid RequestId)
{
	AWECharacter* Me = GetPawn<AWECharacter>();
	UWEWorkshopStyleComponent* Style = UWEWorkshopStyleComponent::Get(this);
	if (Style && Terminal && Terminal->IsWithinUseRange(Me) && Category <= static_cast<uint8>(EWEStyleCategory::Door))
	{
		Style->Apply(Me, Zone, static_cast<EWEStyleCategory>(Category), Option, RequestId);
	}
}

void AWEPlayerController::ServerLeaveComputer_Implementation(AWETraderTerminal* Terminal)
{
	const APlayerState* PS = PlayerState;
	if (Terminal && PS && Terminal->UserName == PS->GetPlayerName())
	{
		Terminal->UserName.Reset();
		Terminal->ForceNetUpdate();
	}
}

void AWEPlayerController::RequestTutorial(uint8 Action)
{
	ServerTutorial(Action);
}

void AWEPlayerController::ServerTutorial_Implementation(uint8 Action)
{
	if (UWETutorialComponent* Tutorial = UWETutorialComponent::Get(this))
	{
		if (Action == 0)
		{
			Tutorial->Start();
		}
		else
		{
			Tutorial->Skip();
		}
	}
}

void AWEPlayerController::NotifyTutorial(uint8 Event)
{
	// Only bother the server while the tutorial waits for exactly this.
	const UWETutorialComponent* Tutorial = UWETutorialComponent::Get(this);
	const FWETutorialStep* Step = Tutorial ? Tutorial->GetCurrentStep() : nullptr;
	if (Step && static_cast<uint8>(Step->Event) == Event)
	{
		ServerTutorialEvent(Event);
	}
}

void AWEPlayerController::ServerTutorialEvent_Implementation(uint8 Event)
{
	// Clients may only report what only they can see.
	const EWETutorialEvent Kind = static_cast<EWETutorialEvent>(Event);
	if (Kind != EWETutorialEvent::InventoryOpened && Kind != EWETutorialEvent::WorkViewOpened && Kind != EWETutorialEvent::ComputerOpened)
	{
		return;
	}
	if (UWETutorialComponent* Tutorial = UWETutorialComponent::Get(this))
	{
		Tutorial->NotifyEvent(Kind);
	}
}

// --- Work view (Phase 17.5) ------------------------------------------------------------------------

void AWEPlayerController::OpenWorkView(AWEWorkbench* Bench, bool bRepair)
{
	if (!IsLocalController() || !Bench || !Bench->GetPlacedItem())
	{
		return;
	}
	CloseInventory();
	if (!WorkViewWidget)
	{
		WorkViewWidget = CreateWidget<UWEWorkViewWidget>(this, UWEWorkViewWidget::StaticClass());
	}
	if (!WorkViewWidget)
	{
		return;
	}
	if (!WorkViewWidget->IsInViewport())
	{
		WorkViewWidget->AddToViewport(9);
	}
	WorkViewWidget->Setup(Bench, bRepair);
	EnterMenuInputMode(WorkViewWidget);
	Bench->AimWorkCamera();
	SetViewTargetWithBlend(Bench, 0.4f, VTBlend_Cubic);
	if (HUDWidget)
	{
		HUDWidget->SetFocusMode(true);
	}
	NotifyTutorial(static_cast<uint8>(EWETutorialEvent::WorkViewOpened));
}

void AWEPlayerController::CloseWorkView()
{
	if (!WorkViewWidget || !WorkViewWidget->IsInViewport())
	{
		return;
	}
	AWEWorkbench* Bench = WorkViewWidget->GetBench();
	if (Bench && Bench->IsManualWork() && Bench->GetWorker() == GetPawn())
	{
		RequestWorkCancel(Bench);
	}
	WorkViewWidget->Shutdown();
	WorkViewWidget->RemoveFromParent();
	if (GetPawn())
	{
		SetViewTargetWithBlend(GetPawn(), 0.35f, VTBlend_Cubic);
	}
	if (HUDWidget)
	{
		HUDWidget->SetFocusMode(false);
	}
	WEUI::Play(this, WEUI::ESound::Close, 0.4f);
	if (!IsInventoryOpen() && !IsPauseMenuOpen() && !IsTradeOpen())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		ResetIgnoreLookInput();
		ResetIgnoreMoveInput();
	}
}

bool AWEPlayerController::IsWorkViewOpen() const
{
	return WorkViewWidget && WorkViewWidget->IsInViewport();
}

void AWEPlayerController::RequestWorkStart(AWEWorkbench* Bench, bool bRepair, int32 Part, bool bInstall) { ServerWorkStart(Bench, bRepair, Part, bInstall); }
void AWEPlayerController::RequestWorkTask(AWEWorkbench* Bench, int32 TaskIndex, EWEToolType Tool) { ServerWorkTask(Bench, static_cast<uint8>(FMath::Clamp(TaskIndex, 0, 255)), Tool); }
void AWEPlayerController::RequestWorkAuto(AWEWorkbench* Bench, bool bRepair, int32 Part, bool bInstall) { ServerWorkAuto(Bench, bRepair, Part, bInstall); }
void AWEPlayerController::RequestWorkCancel(AWEWorkbench* Bench) { ServerWorkCancel(Bench); }

namespace
{
	/** The pawn may work at Bench (alive, close enough). */
	AWECharacter* WorkerAt(AWEPlayerController* PC, AWEWorkbench* Bench)
	{
		AWECharacter* Character = PC ? PC->GetPawn<AWECharacter>() : nullptr;
		if (!Character || !IsValid(Bench) || !UWEInteractionComponent::IsWithinReach(Character, Bench, Bench->GetWorkReach()))
		{
			return nullptr;
		}
		return Character;
	}
}

void AWEPlayerController::ServerWorkStart_Implementation(AWEWorkbench* Bench, bool bRepair, int32 Part, bool bInstall)
{
	if (AWECharacter* Worker = WorkerAt(this, Bench))
	{
		Bench->TryStartManualStep(Worker, bRepair, bRepair ? INDEX_NONE : Part, bInstall);
	}
}

void AWEPlayerController::ServerWorkTask_Implementation(AWEWorkbench* Bench, uint8 TaskIndex, EWEToolType Tool)
{
	if (AWECharacter* Worker = WorkerAt(this, Bench))
	{
		Bench->DoManualTask(Worker, TaskIndex, Tool);
	}
}

void AWEPlayerController::ServerWorkAuto_Implementation(AWEWorkbench* Bench, bool bRepair, int32 Part, bool bInstall)
{
	if (AWECharacter* Worker = WorkerAt(this, Bench))
	{
		Bench->CancelManualStep(Worker);
		if (bRepair)
		{
			Bench->TryStartRepairStep(Worker);
		}
		else if (bInstall)
		{
			Bench->TryStartInstall(Worker, Part);
		}
		else
		{
			// The picked part (none picked: the next free one).
			Bench->TryStartStep(Worker, Part);
		}
	}
}

void AWEPlayerController::ServerWorkCancel_Implementation(AWEWorkbench* Bench)
{
	if (IsValid(Bench))
	{
		Bench->CancelManualStep(GetPawn<AWECharacter>());
	}
}

void AWEPlayerController::RequestHire(AWETraderTerminal* Terminal, int32 ApplicantIndex)
{
	ServerHire(Terminal, ApplicantIndex, FGuid::NewGuid());
}

void AWEPlayerController::RequestStaffAction(AWETraderTerminal* Terminal, AWEEmployee* Employee, uint8 Action)
{
	ServerStaffAction(Terminal, Employee, Action);
}

void AWEPlayerController::RequestAssignTask(AWEEmployee* Employee, AActor* Source, AActor* Target)
{
	ServerAssignTask(Employee, Source, Target);
}

void AWEPlayerController::ServerHire_Implementation(AWETraderTerminal* Terminal, int32 ApplicantIndex, FGuid RequestId)
{
	AWECharacter* Me = GetPawn<AWECharacter>();
	UWEStaffComponent* Staff = UWEStaffComponent::Get(this);
	if (Terminal && Staff && Terminal->IsWithinUseRange(Me))
	{
		Staff->Hire(Me, ApplicantIndex, RequestId);
	}
}

void AWEPlayerController::ServerStaffAction_Implementation(AWETraderTerminal* Terminal, AWEEmployee* Employee, uint8 Action)
{
	AWECharacter* Me = GetPawn<AWECharacter>();
	UWEStaffComponent* Staff = UWEStaffComponent::Get(this);
	if (!Me || !Staff || !IsValid(Employee))
	{
		return;
	}
	// From the terminal (staff page) or standing next to the employee.
	const bool bAllowed = (Terminal && Terminal->IsWithinUseRange(Me)) || FVector::Dist(Me->GetActorLocation(), Employee->GetActorLocation()) <= 400.f;
	if (!bAllowed)
	{
		UE_LOG(LogWENet, Warning, TEXT("%s staff action on %s rejected: out of reach"), *GetName(), *Employee->GetName());
		return;
	}
	switch (Action)
	{
	case StaffFire:   Staff->Fire(Employee, Me); break;
	case StaffPause:  Employee->SetPaused(true); break;
	case StaffResume: Employee->SetPaused(false); break;
	default: break;
	}
}

void AWEPlayerController::ServerAssignTask_Implementation(AWEEmployee* Employee, AActor* Source, AActor* Target)
{
	AWECharacter* Me = GetPawn<AWECharacter>();
	if (!Me || !IsValid(Employee))
	{
		return;
	}
	FWEEmployeeTask Task;
	Task.Kind = AWEEmployee::GetTaskKindForRole(Employee->GetProfile().Role);
	Task.Source = Source;
	Task.Target = Target;
	if (!Employee->AssignTask(Task))
	{
		ClientShowMessage(NSLOCTEXT("WEStaff", "AssignFailed", "Diese Aufgabe passt nicht zu diesem Mitarbeiter"));
		return;
	}
	ClientShowMessage(FText::Format(NSLOCTEXT("WEStaff", "Assigned", "{0}: {1}"), FText::FromString(Employee->GetProfile().Name), Employee->GetTaskText()));
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

void AWEPlayerController::RequestCancelContract(const FGuid& ContractId)
{
	ServerCancelContract(ContractId);
}

void AWEPlayerController::ServerCancelContract_Implementation(FGuid ContractId)
{
	UWEContractComponent* Contracts = UWEContractComponent::Get(this);
	const FString Name = PlayerState ? PlayerState->GetPlayerName() : GetName();
	if (Contracts && Contracts->Cancel(ContractId, Name) == EWEContractResult::Success)
	{
		ClientShowMessage(LOCTEXT("ContractCancelled", "Auftrag abgegeben – der Kunde holt sein Gerät ab."));
	}
}

void AWEPlayerController::ServerDevTeleport_Implementation(FVector Location, FRotator Rotation)
{
#if !UE_BUILD_SHIPPING
	if (APawn* MyPawn = GetPawn())
	{
		// Phase 17: check for room first (a capsule teleported into loose items shoved them through the floor);
		// only if there is none, place it anyway as before.
		if (!MyPawn->TeleportTo(Location, Rotation, false, false))
		{
			MyPawn->TeleportTo(Location, Rotation, false, true);
		}
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

void AWEPlayerController::ServerDevPhase21_Implementation(uint8 Action)
{
#if !UE_BUILD_SHIPPING
	if (Action == 1)
	{
		if (UWEGameRulesComponent* Rules = UWEGameRulesComponent::Get(this))
		{
			Rules->SetRules(FWEGameRules::FromPreset(EWEDifficultyPreset::Realistic));
		}
		if (UWECourierComponent* Courier = UWECourierComponent::Get(this))
		{
			Courier->Order(FWEItemInstance::Make(UWEItemDefinition::FindById(TEXT("Multimeter")), 1, EWEItemCondition::New, 100), TEXT("Werkzeug-Versand Hammerl"));
			Courier->Order(FWEItemInstance::Make(UWEItemDefinition::FindById(TEXT("ThermalPaste")), 2, EWEItemCondition::New, 100), TEXT("Ersatzteil-Express"));
			Courier->DevSendCourierNow();
		}
	}
#endif
}

void AWEPlayerController::ClientOpenContainer_Implementation(UWEInventoryComponent* Container){
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

void AWEPlayerController::RequestTakeIntoHands(UWEInventoryComponent* From, int32 Slot)
{
	ServerTakeIntoHands(From, Slot);
}

void AWEPlayerController::ServerTakeIntoHands_Implementation(UWEInventoryComponent* From, int32 Slot)
{
	ExecuteTakeIntoHands(From, Slot);
}

bool AWEPlayerController::ExecuteTakeIntoHands(UWEInventoryComponent* From, int32 Slot)
{
	AWECharacter* WECharacter = GetPawn<AWECharacter>();
	const FWEItemInstance* Source = From ? From->GetItemInSlot(Slot) : nullptr;
	if (!HasAuthority() || !WECharacter || !CanUseInventory(From) || !Source)
	{
		return false;
	}
	if (WECharacter->GetHeldItem() || WECharacter->GetForklift())
	{
		ClientShowMessage(LOCTEXT("HandsFullTake", "Hände sind voll – erst ablegen"));
		return false;
	}
	if (Source->GetUnitWeightKg() > WECharacter->GetMaxCarryWeightKg())
	{
		ClientShowMessage(LOCTEXT("TooHeavyTake", "Zu schwer zum Tragen"));
		return false;
	}
	FWEItemInstance Removed;
	if (From->RemoveFromSlot(Slot, 1, Removed) != EWEInventoryResult::Success)
	{
		return false;
	}
	// Appears in the hands: spawned at the eyes and picked up in the same frame.
	FVector Eye;
	FRotator EyeRotation;
	WECharacter->GetActorEyesViewPoint(Eye, EyeRotation);
	AWEPickupItem* Spawned = AWEPickupItem::SpawnWorldItem(GetWorld(), Removed, FTransform(FRotator(0.f, EyeRotation.Yaw, 0.f), Eye + EyeRotation.Vector() * 60.f));
	if (!Spawned || !WECharacter->TryPickUpItem(Spawned))
	{
		// Never lose the item: back into the slot it came from (or anywhere in the inventory).
		if (Spawned)
		{
			Spawned->Destroy();
		}
		int32 Back = 0;
		From->TryAddItem(Removed, Back);
		return false;
	}
	UE_LOG(LogWENet, Log, TEXT("%s took %s from the inventory into the hands"), *GetName(), *Removed.GetDisplayName().ToString());
	return true;
}

void AWEPlayerController::RequestSort(UWEInventoryComponent* Inventory, uint8 Mode)
{
	ServerSortInventory(Inventory, Mode);
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

void AWEPlayerController::ServerSortInventory_Implementation(UWEInventoryComponent* Inventory, uint8 Mode)
{
	if (CanUseInventory(Inventory))
	{
		Inventory->SortItems(Mode);
	}
}

void AWEPlayerController::RequestSetItemMarks(UWEInventoryComponent* Inventory, int32 Slot, uint8 Mask, bool bSet)
{
	ServerSetItemMarks(Inventory, Slot, Mask, bSet);
}

void AWEPlayerController::ServerSetItemMarks_Implementation(UWEInventoryComponent* Inventory, int32 Slot, uint8 Mask, bool bSet)
{
	ExecuteSetItemMarks(Inventory, Slot, Mask, bSet);
}

bool AWEPlayerController::ExecuteSetItemMarks(UWEInventoryComponent* Inventory, int32 Slot, uint8 Mask, bool bSet)
{
	constexpr uint8 Allowed = FWEItemInstance::Favorite | FWEItemInstance::Locked | FWEItemInstance::ReleasedForSale;
	if (!HasAuthority() || (Mask & ~Allowed) != 0 || !CanUseInventory(Inventory))
	{
		return false;
	}
	const FWEItemInstance* Item = Inventory->GetItemInSlot(Slot);
	if (!Item)
	{
		return false;
	}
	const FText Name = Item->GetDisplayName();
	if (Inventory->SetSlotMarks(Slot, Mask, bSet) != EWEInventoryResult::Success)
	{
		return false;
	}
	FText Message;
	if (Mask & FWEItemInstance::Locked)
	{
		Message = bSet ? FText::Format(LOCTEXT("MarkLocked", "{0} gesperrt – wird nicht verkauft"), Name) : FText::Format(LOCTEXT("MarkUnlocked", "{0} entsperrt"), Name);
	}
	else if (Mask & FWEItemInstance::Favorite)
	{
		Message = bSet ? FText::Format(LOCTEXT("MarkFav", "{0} als Favorit markiert"), Name) : FText::Format(LOCTEXT("MarkUnfav", "{0}: Favorit entfernt"), Name);
	}
	else
	{
		Message = bSet ? FText::Format(LOCTEXT("MarkRelease", "{0} zum Verkauf freigegeben"), Name) : FText::Format(LOCTEXT("MarkKeep", "{0} wieder als Ausrüstung geschützt"), Name);
	}
	ClientShowMessage(Message);
	return true;
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

// --- Pause menu and session (Phase 13) ----------------------------------------------------------------

bool AWEPlayerController::IsPauseMenuOpen() const
{
	return PauseMenuWidget && PauseMenuWidget->IsInViewport();
}

void AWEPlayerController::TogglePauseMenu()
{
	if (IsPauseMenuOpen())
	{
		ClosePauseMenu();
	}
	else
	{
		OpenPauseMenu();
	}
}

void AWEPlayerController::OpenPauseMenu()
{
	if (!IsLocalController())
	{
		return;
	}
	CloseInventory();
	CloseTrade();
	CloseWorkView();
	if (!PauseMenuWidget)
	{
		PauseMenuWidget = CreateWidget<UWEPauseMenuWidget>(this, UWEPauseMenuWidget::StaticClass());
	}
	if (PauseMenuWidget && !PauseMenuWidget->IsInViewport())
	{
		PauseMenuWidget->AddToViewport(20);
		EnterMenuInputMode(PauseMenuWidget);
	}
}

void AWEPlayerController::ClosePauseMenu()
{
	if (PauseMenuWidget && PauseMenuWidget->IsInViewport())
	{
		PauseMenuWidget->RemoveFromParent();
	}
	if (!IsInventoryOpen() && !IsTradeOpen())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		ResetIgnoreLookInput();
		ResetIgnoreMoveInput();
	}
}

// --- Brand (Phase 20) ------------------------------------------------------------------------------------

void AWEPlayerController::RequestSetBrand(const FWECompanyBrand& Brand)
{
	ServerSetBrand(Brand);
}

void AWEPlayerController::ServerSetBrand_Implementation(const FWECompanyBrand& Brand)
{
	if (UWEBrandComponent* Component = UWEBrandComponent::Get(this); Component && Component->SetBrand(Brand))
	{
		UE_LOG(LogWENet, Log, TEXT("Brand changed by %s: symbol %d, name %s"), PlayerState ? *PlayerState->GetPlayerName() : TEXT("?"), Component->GetBrand().Symbol, *Component->GetBrand().Name);
	}
}

// --- Mirror (Phase 19) -----------------------------------------------------------------------------------

bool AWEPlayerController::IsMirrorOpen() const
{
	return MirrorWidget && MirrorWidget->IsInViewport();
}

void AWEPlayerController::ClientOpenMirror_Implementation()
{
	OpenMirror();
}

void AWEPlayerController::OpenMirror()
{
	if (!IsLocalController())
	{
		return;
	}
	CloseInventory();
	CloseTrade();
	CloseWorkView();
	ClosePauseMenu();
	if (!MirrorWidget)
	{
		MirrorWidget = CreateWidget<UWEMirrorWidget>(this, UWEMirrorWidget::StaticClass());
	}
	if (MirrorWidget && !MirrorWidget->IsInViewport())
	{
		MirrorWidget->AddToViewport(15);
		EnterMenuInputMode(MirrorWidget);
	}
}

void AWEPlayerController::CloseMirror()
{
	if (!IsMirrorOpen())
	{
		return;
	}
	MirrorWidget->RemoveFromParent();
	if (!IsInventoryOpen() && !IsTradeOpen() && !IsPauseMenuOpen())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		ResetIgnoreLookInput();
		ResetIgnoreMoveInput();
	}
}

void AWEPlayerController::RequestSaveCurrent()
{
	AWEGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AWEGameMode>() : nullptr;
	RequestSave(GameMode ? GameMode->GetCurrentSlot() : FString(TEXT("Quicksave")));
}

void AWEPlayerController::RequestLeaveGame(bool bQuitAfterwards)
{
	UWESessionSubsystem* Session = UWESessionSubsystem::Get(this);
	AWEGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AWEGameMode>() : nullptr;
	auto Leave = [this, bQuitAfterwards]()
	{
		if (bQuitAfterwards)
		{
			UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
		}
		else if (UWESessionSubsystem* S = UWESessionSubsystem::Get(this))
		{
			S->LeaveToMainMenu();
		}
	};
	if (GameMode && IsLocalController())
	{
		// Host: nothing gets lost, and every friend is told why the game ends.
		GameMode->SaveToSlot(TEXT("Autosave"));
		GameMode->EndSessionForClients();
		GetWorldTimerManager().SetTimer(LeaveTimer, FTimerDelegate::CreateWeakLambda(this, Leave), 0.5f, false);
		return;
	}
	if (Session)
	{
		Session->SetPendingMessage(FText::GetEmpty());
	}
	Leave();
}

void AWEPlayerController::RequestKick(APlayerState* KickedPlayer)
{
	AWEGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AWEGameMode>() : nullptr;
	APlayerController* Target = KickedPlayer ? KickedPlayer->GetPlayerController() : nullptr;
	if (!GameMode || !IsLocalController() || !Target || Target == this)
	{
		ShowLocalMessage(LOCTEXT("HostOnlyKick", "Nur der Host kann Mitspieler entfernen."));
		return;
	}
	GameMode->KickPlayer(Target);
}

void AWEPlayerController::ClientSessionEnded_Implementation(const FText& Reason)
{
	UE_LOG(LogWENet, Log, TEXT("%s: session ended by the host: %s"), *GetName(), *Reason.ToString());
	if (UWESessionSubsystem* Session = UWESessionSubsystem::Get(this))
	{
		Session->LeaveToMainMenu(Reason);
	}
}

void AWEPlayerController::PawnLeavingGame()
{
	if (HasAuthority() && !IsLocalController() && GetPawn())
	{
		if (AWEGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AWEGameMode>() : nullptr)
		{
			// Keep what the player had: it is theirs when they come back, and it is in the next save.
			GameMode->RememberDepartedPlayer(FWESaveSystem::CapturePlayer(this));
		}
	}
	Super::PawnLeavingGame();
}

void AWEPlayerController::ClientWasKicked_Implementation(const FText& KickReason)
{
	Super::ClientWasKicked_Implementation(KickReason);
	if (UWESessionSubsystem* Session = UWESessionSubsystem::Get(this))
	{
		Session->SetPendingMessage(LOCTEXT("WasKicked", "Der Host hat dich aus dem Spiel entfernt."));
	}
}


// --- Phase 19: classifieds -----------------------------------------------------------------------------------------

namespace
{
	FString ClassifiedResultText(EWEClassifiedResult Result)
	{
		switch (Result)
		{
		case EWEClassifiedResult::Accepted:  return TEXT("Abgemacht!");
		case EWEClassifiedResult::Countered: return TEXT("Der Verkäufer macht ein Gegenangebot.");
		case EWEClassifiedResult::Declined:  return TEXT("Viel zu wenig - der Verkäufer lehnt ab.");
		case EWEClassifiedResult::Ignored:   return TEXT("Der Verkäufer antwortet nicht mehr.");
		case EWEClassifiedResult::NoMoney:   return TEXT("Nicht genug Geld auf dem Firmenkonto.");
		case EWEClassifiedResult::NotAllowed:return TEXT("Das lässt sich nicht inserieren.");
		default:                             return TEXT("Die Anzeige gibt es nicht mehr.");
		}
	}
}

void AWEPlayerController::RequestClassifiedOffer(const FGuid& AdId, int64 Cents) { ServerClassifiedOffer(AdId, Cents); }
void AWEPlayerController::RequestClassifiedAcceptCounter(const FGuid& AdId) { ServerClassifiedAcceptCounter(AdId); }
void AWEPlayerController::RequestClassifiedList(UWEInventoryComponent* From, int32 Slot, int64 Cents) { ServerClassifiedList(From, Slot, Cents); }
void AWEPlayerController::RequestClassifiedAcceptBuyer(const FGuid& ListingId) { ServerClassifiedAcceptBuyer(ListingId); }
void AWEPlayerController::RequestClassifiedWithdraw(const FGuid& ListingId) { ServerClassifiedWithdraw(ListingId); }

void AWEPlayerController::ServerClassifiedOffer_Implementation(FGuid AdId, int64 Cents)
{
	if (UWEClassifiedsComponent* Classifieds = UWEClassifiedsComponent::Get(this))
	{
		ClientShowMessage(FText::FromString(ClassifiedResultText(Classifieds->MakeOffer(AdId, Cents, PlayerState ? PlayerState->GetPlayerName() : FString()))));
	}
}

void AWEPlayerController::ServerClassifiedAcceptCounter_Implementation(FGuid AdId)
{
	if (UWEClassifiedsComponent* Classifieds = UWEClassifiedsComponent::Get(this))
	{
		ClientShowMessage(FText::FromString(ClassifiedResultText(Classifieds->AcceptCounter(AdId, PlayerState ? PlayerState->GetPlayerName() : FString()))));
	}
}

void AWEPlayerController::ServerClassifiedList_Implementation(UWEInventoryComponent* From, int32 Slot, int64 Cents)
{
	UWEClassifiedsComponent* Classifieds = UWEClassifiedsComponent::Get(this);
	// Own inventory or the workshop's storages (the PC is in the workshop).
	if (Classifieds && From && (CanUseInventory(From) || AWETraderTerminal::IsWorkshopStorage(From)))
	{
		const EWEClassifiedResult Result = Classifieds->ListItem(From, Slot, Cents, PlayerState ? PlayerState->GetPlayerName() : FString());
		ClientShowMessage(Result == EWEClassifiedResult::Accepted ? NSLOCTEXT("WEClassifieds", "Listed", "Anzeige aufgegeben – Interessenten melden sich hier.")
			: FText::FromString(ClassifiedResultText(Result)));
	}
}

void AWEPlayerController::ServerClassifiedAcceptBuyer_Implementation(FGuid ListingId)
{
	if (UWEClassifiedsComponent* Classifieds = UWEClassifiedsComponent::Get(this))
	{
		Classifieds->AcceptBuyer(ListingId, PlayerState ? PlayerState->GetPlayerName() : FString());
	}
}

void AWEPlayerController::ServerClassifiedWithdraw_Implementation(FGuid ListingId)
{
	UWEClassifiedsComponent* Classifieds = UWEClassifiedsComponent::Get(this);
	const AWECharacter* Me = GetPawn<AWECharacter>();
	if (Classifieds)
	{
		Classifieds->Withdraw(ListingId, Me ? Me->GetPlayerInventory() : nullptr);
	}
}
#undef LOCTEXT_NAMESPACE
