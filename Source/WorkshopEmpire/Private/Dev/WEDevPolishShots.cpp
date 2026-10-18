// Phase 17.5 screenshot run (-WEShots=P175...): main menu pages, then a new company with the tutorial, the inventory,
// the workshop PC (Ferrit OS) apps, personalising the garage, the pause menu and build mode.
#include "Dev/WEDevAutomationComponent.h"

#include "Economy/WETraderTerminal.h"
#include "Framework/WEMenuGameMode.h"
#include "Framework/WEPlayerController.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Online/WESessionSubsystem.h"
#include "Player/WECharacter.h"
#include "Progression/WETutorialComponent.h"
#include "UI/WEInventoryWidget.h"
#include "UI/WEMenuWidgets.h"
#include "UI/WETradeWidget.h"
#include "UI/WEWorkViewWidget.h"
#include "Items/WEPickupItem.h"
#include "Interaction/WEInteractionComponent.h"
#include "TimerManager.h"
#include "Workshop/WEBuildComponent.h"
#include "Workshop/WEWorkbench.h"
#include "Workshop/WEWorkshopStyleComponent.h"
#include "Logistics/WEVehicle.h"
#include "EngineUtils.h"
#include "WELog.h"

int32 UWEDevAutomationComponent::FindMySlot(const TCHAR* ItemId) const
{
	const UWEInventoryComponent* Mine = GetMyInventory();
	for (const FWEInventorySlotData& Slot : Mine ? Mine->GetAllSlots() : TArray<FWEInventorySlotData>())
	{
		if (Slot.Item.Definition && Slot.Item.Definition->ItemId == FName(ItemId))
		{
			return Slot.SlotIndex;
		}
	}
	return INDEX_NONE;
}

void UWEDevAutomationComponent::RunPolishShots()
{
#if !UE_BUILD_SHIPPING
	if (!IsInMenu())
	{
		if (ShotName.StartsWith(TEXT("P175V")))
		{
			RunVanShots();
			return;
		}
		if (ShotName.StartsWith(TEXT("P175W")))
		{
			RunWorkViewShots();
			return;
		}
		RunPolishGameShots();
		return;
	}
	AWEMenuPlayerController* MenuPC = Cast<AWEMenuPlayerController>(GetOwner());
	auto Menu = [MenuPC]() { return MenuPC ? MenuPC->GetMenu() : nullptr; };
	Schedule(2.5f, [this]() { Shot(TEXT("00_Hauptmenue")); });
	Schedule(3.5f, [Menu]() { if (Menu()) { Menu()->ShowPage(3); } });
	Schedule(4.5f, [this]() { Shot(TEXT("01_Neues_Spiel")); });
	Schedule(5.0f, [Menu]() { if (Menu()) { Menu()->ShowPage(4); } });
	Schedule(6.0f, [this]() { Shot(TEXT("02_Einstellungen")); });
	Schedule(6.5f, [Menu]() { if (Menu()) { Menu()->ShowPage(1); } });
	Schedule(7.5f, [this]() { Shot(TEXT("03_Fortsetzen")); });
	Schedule(8.0f, [Menu]() { if (Menu()) { Menu()->ShowPage(2); } });
	Schedule(9.0f, [this]() { Shot(TEXT("04_Multiplayer")); });
	Schedule(9.5f, [this]()
	{
		if (UWESessionSubsystem* Session = UWESessionSubsystem::Get(this))
		{
			UE_LOG(LogWENet, Display, TEXT("P175 starting a new company with tutorial"));
			Session->HostNewGame(TEXT("QA_Werkstatt"), true, true);
		}
	});
#endif
}

void UWEDevAutomationComponent::RunPolishGameShots()
{
#if !UE_BUILD_SHIPPING
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Log = [this](const TCHAR* Step)
	{
		const UWETutorialComponent* Tutorial = UWETutorialComponent::Get(this);
		UE_LOG(LogWENet, Display, TEXT("P175 %s: tutorial state=%d step=%d"), Step, Tutorial ? static_cast<int32>(Tutorial->GetState()) : -1, Tutorial ? Tutorial->GetStep() : -1);
	};
	Schedule(2.0f, [this, Log]() { Log(TEXT("start")); LookAt(FindBench()); });
	Schedule(3.0f, [this]() { Shot(TEXT("10_Tutorial_HUD")); });
	Schedule(3.5f, [PC]()
	{
		if (!PC)
		{
			return;
		}
		PC->ServerDevAddMoney(6000);
		for (const TCHAR* Id : { TEXT("Screwdriver"), TEXT("Multimeter"), TEXT("RamModule"), TEXT("GraphicsCard"), TEXT("RareGpu"), TEXT("PowerSupply") })
		{
			PC->ServerDevGiveItem(Id, 1, EWEItemCondition::Used);
		}
		PC->ServerDevGiveItem(TEXT("CopperCable"), 6, EWEItemCondition::Used);
		PC->ServerDevGiveItem(TEXT("SheetMetalScrews"), 30, EWEItemCondition::VeryGood);
	});
	Schedule(5.0f, [this, PC]()
	{
		UWEInventoryComponent* Mine = GetMyInventory();
		if (PC && Mine)
		{
			PC->RequestSetItemMarks(Mine, FindMySlot(TEXT("RareGpu")), FWEItemInstance::Favorite, true);
			PC->RequestSetItemMarks(Mine, FindMySlot(TEXT("CopperCable")), FWEItemInstance::Locked, true);
		}
	});
	Schedule(6.0f, [PC]() { if (PC) { PC->OpenInventory(); } });
	Schedule(6.6f, [this, PC]() { if (PC && PC->GetInventoryWidget()) { PC->GetInventoryWidget()->DevShowPlayerSlot(FindMySlot(TEXT("RareGpu")), false); } });
	Schedule(7.3f, [this, Log]() { Shot(TEXT("11_Inventar")); Log(TEXT("inventory")); });
	Schedule(7.8f, [this, PC]() { if (PC && PC->GetInventoryWidget()) { PC->GetInventoryWidget()->DevShowPlayerSlot(FindMySlot(TEXT("Screwdriver")), true); } });
	Schedule(8.5f, [this]() { Shot(TEXT("12_Kontextmenue")); });
	Schedule(9.0f, [this, PC]()
	{
		if (PC)
		{
			PC->CloseInventory();
		}
		TeleportToTerminal(0.f);
	});
	Schedule(10.0f, [this, PC]() { LookAt(FindTerminal()); });
	Schedule(10.6f, [this]() { Shot(TEXT("13_Werkstatt_PC")); });
	Schedule(11.0f, [this, PC]() { if (PC) { PC->OpenTrade(FindTerminal()); } });
	Schedule(12.5f, [this, Log]() { Shot(TEXT("14_OS_Verkaufen")); Log(TEXT("computer")); });
	auto Trade = [PC]() { return PC ? PC->GetTradeWidget() : nullptr; };
	Schedule(13.0f, [Trade]() { if (Trade()) { Trade()->ShowMarketTab(1); } });
	Schedule(13.8f, [this]() { Shot(TEXT("15_OS_Kaufen")); });
	Schedule(14.2f, [Trade]() { if (Trade()) { Trade()->ShowMarketTab(2); } });
	Schedule(15.0f, [this]() { Shot(TEXT("16_OS_Shop")); });
	const TPair<int32, const TCHAR*> Apps[] = { { 4, TEXT("17_OS_Auftraege") }, { 1, TEXT("18_OS_Auktionen") }, { 2, TEXT("19_OS_Personal") }, { 5, TEXT("20_OS_Finanzen") },
		{ 3, TEXT("21_OS_Unternehmen") }, { 6, TEXT("22_OS_Gestaltung") }, { 7, TEXT("23_OS_Hilfe") } };
	float At = 15.4f;
	for (const TPair<int32, const TCHAR*>& App : Apps)
	{
		const int32 Page = App.Key;
		const FString Name = App.Value;
		Schedule(At, [Trade, Page]() { if (Trade()) { Trade()->ShowPage(Page); } });
		Schedule(At + 0.9f, [this, Name]() { Shot(Name); });
		At += 1.4f;
	}
	Schedule(At, [Trade]()
	{
		if (Trade())
		{
			Trade()->ShowPage(0);
			Trade()->ShowConfirm(NSLOCTEXT("P175", "T", "Wertvollen Gegenstand verkaufen?"), NSLOCTEXT("P175", "M", "Grafikkarte (Sammlerstück) ist selten oder wertvoll. Wirklich verkaufen?"),
				NSLOCTEXT("P175", "B", "Verkaufen"), nullptr);
		}
	});
	Schedule(At + 0.9f, [this]() { Shot(TEXT("24_OS_Rueckfrage")); });
	// Personalise the garage: blue walls, grey epoxy floor, cold LED light, red door.
	Schedule(At + 1.4f, [this, PC]()
	{
		if (!PC)
		{
			return;
		}
		PC->RequestApplyStyle(FindTerminal(), TEXT("Garage"), static_cast<uint8>(EWEStyleCategory::WallPaint), 4);
		PC->RequestApplyStyle(FindTerminal(), TEXT("Garage"), static_cast<uint8>(EWEStyleCategory::Floor), 1);
		PC->RequestApplyStyle(FindTerminal(), TEXT("Garage"), static_cast<uint8>(EWEStyleCategory::Lighting), 2);
		PC->RequestApplyStyle(FindTerminal(), TEXT("Garage"), static_cast<uint8>(EWEStyleCategory::Door), 2);
	});
	Schedule(At + 2.6f, [PC]() { if (PC) { PC->CloseTrade(); } });
	Schedule(At + 3.6f, [this]() { LookYawPitch(10.f, -12.f); });
	Schedule(At + 4.4f, [this]() { Shot(TEXT("25_Gestaltete_Garage")); });
	Schedule(At + 4.8f, [this]() { LookAt(FindDoor()); });
	Schedule(At + 5.6f, [this]() { Shot(TEXT("26_Tor_rot")); });
	Schedule(At + 6.0f, [PC]() { if (PC) { PC->OpenPauseMenu(); } });
	Schedule(At + 6.8f, [this]() { Shot(TEXT("27_Pause")); });
	Schedule(At + 7.2f, [PC]() { if (PC && PC->GetPauseMenu()) { PC->GetPauseMenu()->ShowSettings(true); } });
	Schedule(At + 8.0f, [this]() { Shot(TEXT("28_Pause_Einstellungen")); });
	Schedule(At + 8.4f, [PC]() { if (PC) { PC->ClosePauseMenu(); } });
	Schedule(At + 9.0f, [this]() { if (GetBuild()) { GetBuild()->SetBuildMode(true); } LookYawPitch(200.f, -20.f); });
	Schedule(At + 10.0f, [this]() { Shot(TEXT("29_Baumodus")); });
	Schedule(At + 10.4f, [this, Log]() { if (GetBuild()) { GetBuild()->SetBuildMode(false); } Log(TEXT("end")); });
	Schedule(At + 11.5f, [this]() { Quit(); });
#endif
}

void UWEDevAutomationComponent::RunWorkViewShots()
{
#if !UE_BUILD_SHIPPING
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto View = [PC]() { return PC ? PC->GetWorkView() : nullptr; };
	Schedule(3.0f, [PC]()
	{
		if (PC)
		{
			PC->ServerDevGiveItem(TEXT("Screwdriver"), 1, EWEItemCondition::Used);
			PC->ServerDevGiveItem(TEXT("SideCutter"), 1, EWEItemCondition::Used);
			PC->ServerDevGiveItem(TEXT("Multimeter"), 1, EWEItemCondition::Used);
		}
	});
	Schedule(4.5f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestInteractWith(FindItemByName(TEXT("Computer"))); } });
	Schedule(5.5f, [this]() { TeleportToBench(0.f); });
	Schedule(6.5f, [this]() { if (AWEWorkbench* B = FindBench()) { LookAtPoint(B->GetWorkSpotLocation()); } });
	Schedule(7.2f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestPrimaryInteraction(); } });
	Schedule(8.2f, [this]() { if (AWEWorkbench* B = FindBench(); B && B->GetPlacedItem()) { LookAtPoint(B->GetPlacedItem()->GetActorLocation()); } LogBenchState(TEXT("W1 placed")); });
	Schedule(9.0f, [this, PC]()
	{
		if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestPrimaryInteraction(); }
		UE_LOG(LogWENet, Display, TEXT("P175W work view open=%d"), PC && PC->IsWorkViewOpen() ? 1 : 0);
	});
	Schedule(10.5f, [this, View]() { Shot(TEXT("30_Arbeitsansicht")); if (View()) { UE_LOG(LogWENet, Display, TEXT("P175W %s"), *View()->DescribeState()); } });
	Schedule(11.0f, [this]()
	{
		GetWorld()->GetTimerManager().SetTimer(WorkViewTimer, this, &UWEDevAutomationComponent::WorkViewTick, 0.2f, true);
	});
	Schedule(12.6f, [this]() { Shot(TEXT("31_Schraube_loesen")); });
	Schedule(16.0f, [this]() { Shot(TEXT("32_Schrauben_raus")); });
	Schedule(24.0f, [this]() { Shot(TEXT("33_Weiter")); });
	Schedule(34.0f, [this, View]() { Shot(TEXT("34_Spaeter")); if (View()) { UE_LOG(LogWENet, Display, TEXT("P175W %s"), *View()->DescribeState()); } LogBenchState(TEXT("W2 later")); });
	// Wrong tool: hands on a screw gives a hint.
	Schedule(34.4f, [this, View]()
	{
		GetWorld()->GetTimerManager().ClearTimer(WorkViewTimer);
		if (View()) { View()->DevSelectTool(EWEToolType::None); const int32 T = View()->DevFindAvailableTask(); if (T != INDEX_NONE) { View()->DevHoldTask(T); } }
	});
	Schedule(35.0f, [this]() { Shot(TEXT("35_Hinweis")); });
	Schedule(35.5f, [this]() { GetWorld()->GetTimerManager().SetTimer(WorkViewTimer, this, &UWEDevAutomationComponent::WorkViewTick, 0.2f, true); });
	Schedule(70.0f, [this, PC, View]()
	{
		GetWorld()->GetTimerManager().ClearTimer(WorkViewTimer);
		LogBenchState(TEXT("W3 end"));
		UE_LOG(LogWENet, Display, TEXT("P175W tasks done by hand: %d"), WorkViewTasks);
		Shot(TEXT("36_Ende"));
	});
	Schedule(71.0f, [PC]() { if (PC) { PC->CloseWorkView(); } });
	Schedule(72.5f, [this, PC]()
	{
		Shot(TEXT("37_Zurueck"));
		UE_LOG(LogWENet, Display, TEXT("P175W work view open after close=%d"), PC && PC->IsWorkViewOpen() ? 1 : 0);
		if (PC && FindBench()) { PC->OpenInventory(FindBench()->GetStorage()); }
	});
	Schedule(73.5f, [this]() { Shot(TEXT("38_Teile")); });
	// Repair: a broken computer, opened with [Q] - the view starts in repair mode with the measuring points.
	Schedule(74.5f, [PC]() { if (PC) { PC->CloseInventory(); PC->ServerDevGiveItem(TEXT("OldComputer"), 1, EWEItemCondition::Defective); } });
	Schedule(76.0f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestInteractWith(FindNearestItem(TEXT("Computer"))); } });
	Schedule(77.0f, [this]() { if (AWEWorkbench* B = FindBench()) { LookAtPoint(B->GetWorkSpotLocation()); } });
	Schedule(77.6f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestPrimaryInteraction(); } });
	Schedule(78.6f, [this]() { if (AWEWorkbench* B = FindBench(); B && B->GetPlacedItem()) { LookAtPoint(B->GetPlacedItem()->GetActorLocation()); } LogBenchState(TEXT("W4 repair placed")); });
	Schedule(79.2f, [this, PC, View]()
	{
		if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestAlternativeInteraction(); }
		UE_LOG(LogWENet, Display, TEXT("P175W repair view open=%d %s"), PC && PC->IsWorkViewOpen() ? 1 : 0, View() ? *View()->DescribeState() : TEXT(""));
	});
	Schedule(80.6f, [this]() { Shot(TEXT("39_Reparatur")); GetWorld()->GetTimerManager().SetTimer(WorkViewTimer, this, &UWEDevAutomationComponent::WorkViewTick, 0.2f, true); });
	Schedule(82.4f, [this]() { Shot(TEXT("40_Messen")); });
	Schedule(88.0f, [this, View]()
	{
		GetWorld()->GetTimerManager().ClearTimer(WorkViewTimer);
		Shot(TEXT("41_Diagnose"));
		LogBenchState(TEXT("W5 diagnosed"));
		if (View()) { UE_LOG(LogWENet, Display, TEXT("P175W %s"), *View()->DescribeState()); }
	});
	Schedule(89.5f, [this]() { Quit(); });
#endif
}

void UWEDevAutomationComponent::WorkViewTick()
{
#if !UE_BUILD_SHIPPING
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	UWEWorkViewWidget* View = PC ? PC->GetWorkView() : nullptr;
	if (View && PC->IsWorkViewOpen() && View->DevIsIdle() && View->DevWorkNext())
	{
		++WorkViewTasks;
	}
#endif
}
void UWEDevAutomationComponent::RunVanShots()
{
#if !UE_BUILD_SHIPPING
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Van = [this]() -> AWEVehicle* { TActorIterator<AWEVehicle> It(GetWorld()); return It ? *It : nullptr; };
	auto ViewVan = [this, PC, Van](float Side, float Back)
	{
		if (const AWEVehicle* V = Van(); V && PC)
		{
			const FVector At = V->GetActorLocation() + V->GetActorRightVector() * Side + V->GetActorForwardVector() * Back + FVector(0.f, 0.f, 160.f);
			PC->ServerDevTeleport(At, (V->GetActorLocation() + FVector(0.f, 0.f, 110.f) - At).Rotation());
		}
	};
	Schedule(2.0f, [PC]() { if (PC) { PC->ServerDevAddMoney(60000); } });
	Schedule(2.5f, [this]() { TeleportToTerminal(0.f); });
	Schedule(3.5f, [this]() { BuyUpgradeAtTerminal(TEXT("OldVan")); });
	Schedule(5.0f, [ViewVan]() { ViewVan(-650.f, 380.f); });
	Schedule(9.0f, [this, Van]()
	{
		const AWEVehicle* V = Van();
		UE_LOG(LogWENet, Display, TEXT("P175V van=%s hidden=%d improved=%d"), V ? *V->GetStats().DisplayName.ToString() : TEXT("none"), V && V->IsHidden() ? 1 : 0, V && V->IsImproved() ? 1 : 0);
		Shot(TEXT("50_Transporter_alt"));
	});
	Schedule(9.5f, [ViewVan]() { ViewVan(520.f, -480.f); });
	Schedule(12.5f, [this]() { Shot(TEXT("51_Transporter_alt_hinten")); });
	Schedule(13.0f, [this]() { TeleportToTerminal(0.f); });
	Schedule(14.0f, [this]() { BuyUpgradeAtTerminal(TEXT("ModernVan")); });
	Schedule(15.0f, [ViewVan]() { ViewVan(-650.f, 380.f); });
	Schedule(19.0f, [this, Van]()
	{
		const AWEVehicle* V = Van();
		UE_LOG(LogWENet, Display, TEXT("P175V van=%s improved=%d"), V ? *V->GetStats().DisplayName.ToString() : TEXT("none"), V && V->IsImproved() ? 1 : 0);
		Shot(TEXT("52_Lieferwagen_modern"));
	});
	// The neighbourhood (Phase 17.5 world): fixed views from the ground.
	ShotFrom(20.0f, FVector(800.f, -2650.f, 170.f), FVector(450.f, -400.f, 220.f), TEXT("60_Strasse_zum_Tor"));
	ShotFrom(24.0f, FVector(700.f, -250.f, 170.f), FVector(600.f, -2700.f, 320.f), TEXT("61_Hof_zur_Strasse"));
	ShotFrom(28.0f, FVector(-1150.f, -1000.f, 170.f), FVector(0.f, 500.f, 150.f), TEXT("62_Hinterhof"));
	ShotFrom(32.0f, FVector(2600.f, -1650.f, 170.f), FVector(-2000.f, -1900.f, 200.f), TEXT("63_Strasse_West"));
	ShotFrom(36.0f, FVector(900.f, 100.f, 170.f), FVector(-700.f, 2600.f, 250.f), TEXT("64_Hof_Nord"));
	Schedule(40.0f, [this]() { Quit(); });
#endif
}