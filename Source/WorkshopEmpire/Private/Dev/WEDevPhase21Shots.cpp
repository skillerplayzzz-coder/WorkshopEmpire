// Phase 21 screenshot runs (-WEShots=P21 on L_Garage): repair camera, tools in hand, visible disassembly, conditions and
// diagnosis, cleaning, employees at work, tool wall, parcel courier, difficulty, customer reviews.
#include "Dev/WEDevAutomationComponent.h"
#include "Widgets/SWindow.h"
#include "Engine/GameViewportClient.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Framework/Application/SlateApplication.h"

#include "EngineUtils.h"
#include "Framework/WEPlayerController.h"
#include "Interaction/WEInteractionComponent.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "UI/WEWorkViewWidget.h"
#include "WELog.h"
#include "Workshop/WEAssemblyDefinition.h"
#include "Workshop/WEWorkbench.h"
#include "Workshop/WEAssemblyView.h"
#include "Workshop/WEToolHand.h"
#include "Workshop/WEWorkRig.h"
#include "Workshop/WERepairDefinition.h"
#include "Items/WEItemDefinition.h"
#include "TimerManager.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Economy/WETraderTerminal.h"
#include "Staff/WEEmployee.h"
#include "Staff/WEStaffComponent.h"
#include "World/WEToolWall.h"
#include "Economy/WECourierComponent.h"
#include "Interaction/WEInteractionComponent.h"
#include "Economy/WECompanyComponent.h"
#include "Framework/WEGameMode.h"
#include "Framework/WEGameRulesComponent.h"
#include "Framework/WEMenuGameMode.h"
#include "UI/WEMenuWidgets.h"
#include "PipelineStateCache.h"
#include "RHI.h"
#include "Inventory/WEInventoryComponent.h"
#include "Workshop/WEBuildComponent.h"
#include "World/WEPlaceholderProp.h"
#include "Player/WEAvatarComponent.h"

void UWEDevAutomationComponent::PlaceHeldOnBench()
{
#if !UE_BUILD_SHIPPING
	// The held device onto the bench: aim at the work spot from close by (other things on the bench can catch the aim).
	AWEWorkbench* B = FindBench();
	AWECharacter* C = GetCharacter();
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	if (!B || !C || !PC || !C->GetHeldItem())
	{
		return;
	}
	const FVector Spot = B->GetWorkSpotLocation();
	PC->ServerDevTeleport(Spot + B->GetActorForwardVector() * 95.f + FVector(0.f, 0.f, 10.f), FRotator::ZeroRotator);
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Spot]()
	{
		LookAtPoint(Spot);
		if (AWECharacter* Me = GetCharacter())
		{
			Me->GetInteractionComponent()->UpdateFocus();
			Me->GetInteractionComponent()->RequestPrimaryInteraction();
		}
	}), 0.35f, false);
#endif
}

void UWEDevAutomationComponent::RunPhase21Shots()
{
#if !UE_BUILD_SHIPPING
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto View = [PC]() { return PC ? PC->GetWorkView() : nullptr; };
	auto LogHand = [this, View](const TCHAR* Step)
	{
		const UWEWorkViewWidget* V = View();
		const AWEWorkRig* Rig = V ? V->GetRig() : nullptr;
		const AWEToolHand* Hand = V ? V->GetHand() : nullptr;
		const AWEWorkbench* B = FindBench();
		const AWEAssemblyView* Asm = B ? B->GetAssemblyView() : nullptr;
		FVector Carry;
		const bool bCarry = Rig && Rig->GetCarryPoint(Carry);
		UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 hand %s: holding %d tip %s active %d (%.2f) carrying %d bowl rig %d pc %d box %d %s"), Step, Hand && Hand->IsHolding() ? 1 : 0,
			Hand ? *Hand->GetTipLocation().ToString() : TEXT("-"), Rig ? Rig->GetActiveTask() : -2, Rig ? Rig->GetActiveProgress() : 0.f, bCarry ? 1 : 0,
			Rig ? Rig->GetScrewsInBowl() : -1, Asm ? Asm->GetScrewsInBowl() : -1, Rig && Rig->HasPartBox() ? 1 : 0, V ? *V->DescribeState() : TEXT(""));
	};
	if (IsInMenu())
	{
		// --- 9. "Neues Spiel": difficulty presets and single options -----------------------------------------------
		AWEMenuPlayerController* MenuPC = Cast<AWEMenuPlayerController>(GetOwner());
		auto Menu = [MenuPC]() { return MenuPC ? MenuPC->GetMenu() : nullptr; };
		auto LogRules = [Menu](const TCHAR* Step)
		{
			if (const UWEMainMenuWidget* M = Menu())
			{
				UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 menu %s: %s (%s)"), Step, *M->GetNewRules().Describe().ToString(), *M->GetNewRules().ToOption());
			}
		};
		Schedule(2.5f, [Menu, LogRules]() { if (Menu()) { Menu()->ShowPage(3); } LogRules(TEXT("default")); });
		Schedule(4.0f, [this]() { Shot(TEXT("60_Neues_Spiel_Realistisch")); });
		Schedule(5.0f, [Menu, LogRules]() { if (Menu()) { Menu()->DevStepRule(0, 1); } LogRules(TEXT("preset +1")); });
		Schedule(6.0f, [this]() { Shot(TEXT("61_Neues_Spiel_Experte")); });
		Schedule(7.0f, [Menu, LogRules]() { if (Menu()) { Menu()->DevStepRule(0, 1); } LogRules(TEXT("preset +2")); });
		Schedule(8.0f, [this]() { Shot(TEXT("62_Neues_Spiel_Entspannt")); });
		Schedule(9.0f, [Menu, LogRules]() { if (Menu()) { Menu()->DevStepRule(3, 1); } LogRules(TEXT("reviews toggled")); });
		Schedule(10.0f, [this]() { Shot(TEXT("63_Neues_Spiel_Individuell")); });
		Schedule(11.4f, [Menu]() { if (Menu()) { Menu()->ShowPage(4); } });
		Schedule(12.2f, [this]() { Shot(TEXT("64_Einstellungen")); });
		Schedule(11.0f, [Menu, LogRules]()
		{
			FWEGameRules Parsed;
			const bool bOk = Menu() && FWEGameRules::FromOption(Menu()->GetNewRules().ToOption(), Parsed) && Parsed == Menu()->GetNewRules();
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 menu option round trip: %s"), bOk ? TEXT("ok") : TEXT("FAIL"));
		});
		// Frame rate of the menu (Mike: 1-3 fps): every second for the first 12 s, with the pipelines still being prepared.
		TSharedRef<uint64> LastFrames = MakeShared<uint64>(GFrameCounter);
		TSharedRef<double> LastTime = MakeShared<double>(FPlatformTime::Seconds());
		for (int32 s = 1; s <= 12; ++s)
		{
			Schedule(static_cast<float>(s), [LastFrames, LastTime, s]()
			{
				const double Now = FPlatformTime::Seconds();
				UE_LOG(LogWorkshopEmpire, Display, TEXT("PERF menu t=%ds: %.1f fps, GPU %.2f ms, pipelines pending %d"), s, (GFrameCounter - *LastFrames) / FMath::Max(Now - *LastTime, 0.001),
					FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()), static_cast<int32>(PipelineStateCache::NumActivePrecacheRequests()) + PipelineStateCache::GetNumActivePipelinePrecompileTasks());
				*LastFrames = GFrameCounter;
				*LastTime = Now;
			});
		}
		// Where the GPU time goes (written to the log).
		Schedule(6.5f, [MenuPC]() { if (MenuPC && FParse::Param(FCommandLine::Get(), TEXT("WEProfileGPU"))) { MenuPC->ConsoleCommand(TEXT("ProfileGPU")); } });
		Schedule(13.f, [this]() { Quit(); });
		return;
	}
	if (ShotName == TEXT("P21L"))
	{
		// --- Old saves load (every save version still present): slots QAOld1..QAOld9 as far as they exist ------------
		float T = 2.f;
		for (int32 i = 1; i <= 9; ++i)
		{
			const FString Slot = FString::Printf(TEXT("QAOld%d"), i);
			Schedule(T, [this, Slot]()
			{
				AWEGameMode* GM = GetWorld()->GetAuthGameMode<AWEGameMode>();
				const bool bLoaded = GM && GM->LoadFromSlot(Slot);
				int32 Items = 0;
				for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It) { ++Items; }
				const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
				UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 load %s: %s items=%d balance=%s rules='%s' courier pending=%d"), *Slot, bLoaded ? TEXT("ok") : TEXT("FAILED/none"), Items,
					Company ? *Company->GetBalance().ToDisplayString() : TEXT("-"), *UWEGameRulesComponent::GetRules(this).Describe().ToString(),
					UWECourierComponent::Get(this) ? UWECourierComponent::Get(this)->GetPendingCount() : -1);
			});
			Schedule(T + 2.f, [this, Slot]() { Shot(Slot); });
			T += 4.f;
		}
		Schedule(T + 1.f, [this]() { Quit(); });
		return;
	}
	if (ShotName == TEXT("P21G"))
	{
		// --- 8. Parcel courier: order, save/load on the way, the courier walks in, parcels on the pallet, open one -------
		Schedule(1.0f, [this, PC]()
		{
			if (PC) { PC->ConsoleCommand(TEXT("DisableAllScreenMessages")); }
			if (UWEGameRulesComponent* Rules = UWEGameRulesComponent::Get(this)) { Rules->SetRules(FWEGameRules::FromPreset(EWEDifficultyPreset::Realistic)); }
		});
		auto Courier = [this]() { return UWECourierComponent::Get(this); };
		auto CountParcels = [this]()
		{
			TArray<AWEPickupItem*> Found;
			for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It)
			{
				if (UWECourierComponent::IsParcel(It->GetItem()) && !It->IsActorBeingDestroyed()) { Found.Add(*It); }
			}
			return Found;
		};
		Schedule(2.0f, [this, Courier]()
		{
			UWECourierComponent* C = Courier();
			if (!C) { return; }
			const FWEItemInstance Tool = FWEItemInstance::Make(UWEItemDefinition::FindById(TEXT("Screwdriver")), 1, EWEItemCondition::New, 100);
			const FWEItemInstance Paste = FWEItemInstance::Make(UWEItemDefinition::FindById(TEXT("ThermalPaste")), 3, EWEItemCondition::New, 100);
			const FWEItemInstance Ram = FWEItemInstance::Make(UWEItemDefinition::FindById(TEXT("RamModule")), 1, EWEItemCondition::Used, 60);
			const FWEItemInstance Heavy = FWEItemInstance::Make(UWEItemDefinition::FindById(TEXT("IndustrialMotorLarge")), 1, EWEItemCondition::Defective, 40);
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 courier ship? screwdriver %d paste %d heavy motor %d"), C->ShouldShip(Tool) ? 1 : 0, C->ShouldShip(Paste) ? 1 : 0, C->ShouldShip(Heavy) ? 1 : 0);
			C->Order(Tool, TEXT("Werkzeug-Versand Hammerl"));
			C->Order(Paste, TEXT("Ersatzteil-Express"));
			C->Order(Ram, TEXT("Gebrauchtbörse 24"));
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 courier ordered: pending %d, next visit in %.0fs"), C->GetPendingCount(), C->GetNextVisitIn());
		});
		Schedule(3.0f, [this, Courier]()
		{
			AWEGameMode* GM = GetWorld()->GetAuthGameMode<AWEGameMode>();
			const bool bSaved = GM && GM->SaveToSlot(TEXT("P21G"));
			if (UWECourierComponent* C = Courier()) { C->SetSaveState({}, {}); }
			const bool bLoaded = GM && GM->LoadFromSlot(TEXT("P21G"));
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 courier save %d load %d: pending %d (expect 3), rules '%s'"), bSaved ? 1 : 0, bLoaded ? 1 : 0,
				Courier() ? Courier()->GetPendingCount() : -1, *UWEGameRulesComponent::GetRules(this).Describe().ToString());
		});
		Schedule(5.0f, [this, Courier]() { if (UWECourierComponent* C = Courier()) { C->DevSendCourierNow(); } });
		// A camera that follows the courier from the side.
		auto Follow = [this, PC, Courier](const TCHAR* Name)
		{
			const AWECourier* Walker = Courier() ? Courier()->GetActiveCourier() : nullptr;
			if (!Walker || !PC) { UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 courier: nobody to film for %s"), Name); return; }
			if (!CatCamera.IsValid())
			{
				FActorSpawnParameters Params;
				Params.ObjectFlags |= RF_Transient;
				CatCamera = GetWorld()->SpawnActor<ACameraActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
				CatCamera->GetCameraComponent()->SetFieldOfView(60.f);
				CatCamera->GetCameraComponent()->bConstrainAspectRatio = false;
			}
			const FVector Target = Walker->GetActorLocation() + FVector(0.f, 0.f, 10.f);
			FVector From = Target + Walker->GetActorForwardVector() * 230.f + Walker->GetActorRightVector() * 160.f + FVector(0.f, 0.f, 60.f);
			for (const FVector2D Offset : { FVector2D(230.f, 160.f), FVector2D(230.f, -160.f), FVector2D(-230.f, 160.f), FVector2D(-230.f, -160.f), FVector2D(150.f, 0.f) })
			{
				const FVector Try = Target + Walker->GetActorForwardVector() * Offset.X + Walker->GetActorRightVector() * Offset.Y + FVector(0.f, 0.f, 60.f);
				FCollisionQueryParams Params(SCENE_QUERY_STAT(WEShotView), false, Walker);
				if (!GetWorld()->SweepTestByChannel(Target, Try, FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(12.f), Params))
				{
					From = Try;
					break;
				}
			}
			CatCamera->SetActorLocationAndRotation(From, (Target - From).Rotation());
			PC->SetViewTarget(CatCamera.Get());
			const FString Label(Name);
			FTimerHandle H;
			GetWorld()->GetTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [this, Label]() { Shot(Label); }), 0.2f, false);
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 courier %s: state %d carrying %d at %s"), Name, static_cast<int32>(Walker->GetCourierState()), Walker->GetParcelCount(), *Walker->GetActorLocation().ToCompactString());
		};
		Schedule(9.0f, [Follow]() { Follow(TEXT("50_Paketbote_kommt")); });
		// Watch for the moment he puts them down.
		Schedule(10.0f, [this, Courier, Follow]()
		{
			TSharedRef<bool> bShot = MakeShared<bool>(false);
			FTimerHandle Handle;
			GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [Courier, Follow, bShot]()
			{
				const AWECourier* Walker = Courier() ? Courier()->GetActiveCourier() : nullptr;
				if (!*bShot && Walker && Walker->GetCourierState() == EWECourierState::Dropping && Walker->GetParcelCount() <= 2)
				{
					*bShot = true;
					Follow(TEXT("51_Pakete_abstellen"));
				}
			}), 0.25f, true);
		});
		Schedule(40.0f, [this, PC, CountParcels, Courier]()
		{
			const TArray<AWEPickupItem*> Parcels = CountParcels();
			for (const AWEPickupItem* P : Parcels)
			{
				const FWEParcelContents* Contents = Courier() ? Courier()->FindContents(P->GetItem().InstanceId) : nullptr;
				UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 parcel at %s: %s"), *P->GetActorLocation().ToCompactString(),
					Contents && Contents->Items.Num() > 0 ? *FString::Printf(TEXT("%d x %s from %s"), Contents->Items[0].Quantity, *Contents->Items[0].GetDisplayName().ToString(), *Contents->Sender) : TEXT("unknown"));
			}
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 courier delivered: %d parcels lying, pending %d, courier %s"), Parcels.Num(), Courier() ? Courier()->GetPendingCount() : -1,
				Courier() && Courier()->GetActiveCourier() ? TEXT("still here") : TEXT("gone"));
			if (Parcels.Num() > 0 && PC)
			{
				const FVector At = Parcels[0]->GetActorLocation();
				if (!CatCamera.IsValid()) { return; }
				// (Outside in front of the door, or on the pallet inside: look from the side the parcels are on.)
				const FVector From = At + FVector(At.X > 420.f ? 200.f : -180.f, 140.f, 140.f);
				CatCamera->SetActorLocationAndRotation(From, (At - From).Rotation());
				PC->SetViewTarget(CatCamera.Get());
				FTimerHandle H;
				GetWorld()->GetTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [this]() { Shot(TEXT("52_Pakete_im_Lieferbereich")); }), 0.3f, false);
			}
		});
		Schedule(42.0f, [this, Courier, CountParcels]()
		{
			// Saved with parcels lying around: after loading they are still the same parcels with the same contents.
			AWEGameMode* GM = GetWorld()->GetAuthGameMode<AWEGameMode>();
			const bool bSaved = GM && GM->SaveToSlot(TEXT("P21G"));
			const bool bLoaded = GM && GM->LoadFromSlot(TEXT("P21G"));
			int32 Known = 0;
			const TArray<AWEPickupItem*> Parcels = CountParcels();
			for (const AWEPickupItem* P : Parcels) { Known += Courier() && Courier()->FindContents(P->GetItem().InstanceId) ? 1 : 0; }
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 courier after save %d / load %d: %d parcels, %d with known contents"), bSaved ? 1 : 0, bLoaded ? 1 : 0, Parcels.Num(), Known);
		});
		Schedule(44.0f, [this, PC, CountParcels]()
		{
			const TArray<AWEPickupItem*> Parcels = CountParcels();
			AWECharacter* C = GetCharacter();
			if (Parcels.Num() == 0 || !C || !PC) { return; }
			AWEPickupItem* P = Parcels[0];
			PC->ServerDevTeleport(P->GetActorLocation() + FVector(130.f, -90.f, 100.f), FRotator::ZeroRotator);
			FText Text, Reason;
			const bool bOffer = P->GetAlternativeInteraction(C, Text, Reason);
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 parcel offer %d '%s'"), bOffer ? 1 : 0, *Text.ToString());
			int32 Before = 0;
			for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It) { ++Before; }
			P->InteractAlternative(C);
			FTimerHandle H;
			GetWorld()->GetTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [this, Before, CountParcels]()
			{
				int32 After = 0;
				TArray<FString> Names;
				for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It) { ++After; }
				UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 parcel opened: world items %d -> %d, parcels left %d"), Before, After, CountParcels().Num());
				Shot(TEXT("53_Paket_geoeffnet"));
			}), 0.6f, false);
		});
		Schedule(47.0f, [this]() { Quit(); });
		return;
	}
	if (ShotName == TEXT("P21F"))
	{
		// --- 7. Tool wall: hang everything up, take two tools back (outlines stay), reach check ---------------------
		Schedule(1.0f, [PC]() { if (PC) { PC->ConsoleCommand(TEXT("DisableAllScreenMessages")); PC->ServerDevAddMoney(5000); } });
		Schedule(2.0f, [this, PC]()
		{
			if (PC) { PC->ServerDevTeleport(FVector(-60.f, -60.f, 100.f), FRotator(0.f, -90.f, 0.f)); }
			if (UWEBuildComponent* B = GetBuild())
			{
				B->SetBuildMode(true);
				PlaceFurnitureAt(TEXT("ToolWall"), 60.f, -230.f, 90.f);
				B->SetBuildMode(false);
			}
			for (const TCHAR* Id : { TEXT("Screwdriver"), TEXT("SideCutter"), TEXT("CordlessDrill"), TEXT("Multimeter"), TEXT("SolderingIron"), TEXT("AirDuster"), TEXT("BitRatchet") })
			{
				if (PC) { PC->ServerDevGiveItem(FName(Id), 1, EWEItemCondition::New); }
			}
		});
		auto Wall = [this]() { return Cast<AWEToolWall>(FindFurniture(TEXT("ToolWall"))); };
		auto WallShot = [this, PC, Wall](const TCHAR* Name, float Distance)
		{
			AWEToolWall* W = Wall();
			if (!W || !PC) { return; }
			if (!CatCamera.IsValid())
			{
				FActorSpawnParameters Params;
				Params.ObjectFlags |= RF_Transient;
				CatCamera = GetWorld()->SpawnActor<ACameraActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
				CatCamera->GetCameraComponent()->SetFieldOfView(60.f);
				CatCamera->GetCameraComponent()->bConstrainAspectRatio = false;
			}
			const FVector Target = W->GetActorTransform().TransformPosition(FVector(-5.f, 0.f, 125.f));
			const FVector From = Target + W->GetActorForwardVector() * Distance + FVector(0.f, 0.f, 10.f);
			CatCamera->SetActorLocationAndRotation(From, (Target - From).Rotation());
			PC->SetViewTarget(CatCamera.Get());
			const FString Label(Name);
			FTimerHandle H;
			GetWorld()->GetTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [this, Label]() { Shot(Label); }), 0.3f, false);
		};
		Schedule(4.0f, [this, Wall]()
		{
			AWEToolWall* W = Wall();
			AWECharacter* C = GetCharacter();
			const int32 Hung = W ? W->HangUpTools(C) : -1;
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 wall hung %d, hanging %d, screwdriver speed %.2f (from the wall), multimeter %.2f"), Hung, W ? W->GetHangingCount() : -1,
				AWEWorkbench::FindToolSpeed(C, EWEToolType::Screwdriver), AWEWorkbench::FindToolSpeed(C, EWEToolType::Multimeter));
		});
		Schedule(5.0f, [WallShot]() { WallShot(TEXT("40_Werkzeugwand"), 190.f); });
		Schedule(6.5f, [WallShot]() { WallShot(TEXT("40b_Werkzeugwand_nah"), 95.f); });
		Schedule(8.0f, [this, PC, Wall]()
		{
			// Stand in front of the wall and look at the first hook: [Q] offers that tool.
			AWEToolWall* W = Wall();
			if (W && PC)
			{
				PC->SetViewTarget(PC->GetPawn());
				PC->ServerDevTeleport(W->GetActorTransform().TransformPosition(FVector(110.f, 0.f, 0.f)) + FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator);
			}
		});
		Schedule(8.6f, [this, Wall]()
		{
			AWEToolWall* W = Wall();
			if (!W) { return; }
			LookAtPoint(W->GetActorTransform().TransformPosition(W->GetHookLocation(0) + FVector(3.f, 0.f, -13.f)));
		});
		Schedule(9.2f, [this, Wall]()
		{
			AWEToolWall* W = Wall();
			AWECharacter* C = GetCharacter();
			FText Text, Reason;
			const bool bOffer = W && W->GetAlternativeInteraction(C, Text, Reason);
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 wall aim: hook %d offer %d '%s'"), W ? W->FindAimedTool(C) : -2, bOffer ? 1 : 0, *Text.ToString());
			if (bOffer) { W->InteractAlternative(C); }
			if (W) { W->TakeTool(C, 3); }
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 wall after taking: hanging %d, inventory tools %d"), W ? W->GetHangingCount() : -1,
				[C]() { int32 N = 0; for (const FWEInventorySlotData& S : C->GetPlayerInventory()->GetAllSlots()) { N += S.Item.Definition && S.Item.Definition->ToolType != EWEToolType::None ? 1 : 0; } return N; }());
		});
		Schedule(9.6f, [this, PC, Wall]()
		{
			// Out of the picture.
			if (AWEToolWall* W = Wall(); W && PC) { PC->ServerDevTeleport(W->GetActorTransform().TransformPosition(FVector(150.f, 200.f, 0.f)) + FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator); }
		});
		Schedule(10.2f, [WallShot]() { WallShot(TEXT("41_Umrisse"), 120.f); });
		Schedule(11.5f, [this, PC]()
		{
			if (PC) { PC->SetViewTarget(PC->GetPawn()); PC->ServerDevTeleport(FVector(900.f, 600.f, 100.f), FRotator::ZeroRotator); }
		});
		Schedule(12.5f, [this]()
		{
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 wall far away: soldering iron %.2f (on the wall, expect 0), multimeter %.2f (taken, expect 1)"),
				AWEWorkbench::FindToolSpeed(GetCharacter(), EWEToolType::SolderingIron), AWEWorkbench::FindToolSpeed(GetCharacter(), EWEToolType::Multimeter));
			Quit();
		});
		return;
	}
	if (ShotName == TEXT("P21E"))
	{
		// --- 6. Employees at work: carrying, reaching into shelf and press, at the controls; then a coffee break -----
		Schedule(1.0f, [PC]() { if (PC) { PC->ConsoleCommand(TEXT("DisableAllScreenMessages")); } });
		Schedule(2.0f, [this, PC]() { if (PC) { PC->ServerDevAddMoney(20000); } TeleportToTerminal(0.f); });
		Schedule(3.0f, [this]()
		{
			// One of each role (the applicant list is random).
			if (UWEStaffComponent* Staff = UWEStaffComponent::Get(this))
			{
				for (const EWEStaffRole Role : { EWEStaffRole::WarehouseWorker, EWEStaffRole::MachineOperator })
				{
					FWEEmployeeProfile Profile;
					Profile.Name = Role == EWEStaffRole::WarehouseWorker ? TEXT("Nina Wolf") : TEXT("Georg Klein");
					Profile.Role = Role;
					Profile.WageCentsPerMinute = 90;
					Staff->SpawnEmployee(Profile, FTransform(FVector(250.f, Role == EWEStaffRole::WarehouseWorker ? 120.f : 200.f, 100.f)));
				}
			}
		});
		Schedule(4.0f, [this, PC]()
		{
			if (PC) { PC->ServerDevTeleport(FVector(-60.f, -60.f, 100.f), FRotator(0.f, -90.f, 0.f)); }
			if (UWEBuildComponent* B = GetBuild())
			{
				B->SetBuildMode(true);
				PlaceFurnitureAt(TEXT("HeavyShelf"), 60.f, -210.f, 90.f);
				PlaceFurnitureAt(TEXT("Press"), 60.f, -60.f, 0.f);
				B->SetBuildMode(false);
			}
			if (PC) { PC->ServerDevGiveItem(TEXT("SteelScrap"), 25, EWEItemCondition::Used); }
		});
		Schedule(5.0f, [this, PC]()
		{
			AWEPlaceholderProp* Shelf = FindFurniture(TEXT("HeavyShelf"));
			if (Shelf) { TransferToStorage(TEXT("SteelScrap"), Shelf->GetStorage()); }
			if (PC)
			{
				PC->RequestAssignTask(FindEmployee(static_cast<uint8>(EWEStaffRole::WarehouseWorker)), Shelf, FindFurniture(TEXT("Press")));
				PC->RequestAssignTask(FindEmployee(static_cast<uint8>(EWEStaffRole::MachineOperator)), Shelf, FindFurniture(TEXT("Press")));
				PC->ServerDevTeleport(FVector(300.f, 240.f, 100.f), FRotator(0.f, 180.f, 0.f));
			}
		});
		// A camera beside the person, at chest height, whenever the pose we want shows up.
		TSharedRef<TSet<FString>> Taken = MakeShared<TSet<FString>>();
		TSharedRef<float> LastShot = MakeShared<float>(-10.f);
		auto Watch = [this, PC, Taken, LastShot](AWEEmployee* E, EWEWorkPose Pose, const TCHAR* Key, const TCHAR* Name)
		{
			const float Now = GetWorld()->GetTimeSeconds();
			if (!E || !PC || Taken->Contains(Key) || E->GetActivity() != Pose || !E->GetAvatar() || E->GetAvatar()->GetWorkPoseBlend() < 0.95f || Now - *LastShot < 1.2f)
			{
				return;
			}
			*LastShot = Now;
			Taken->Add(Key);
			if (!CatCamera.IsValid())
			{
				FActorSpawnParameters Params;
				Params.ObjectFlags |= RF_Transient;
				CatCamera = GetWorld()->SpawnActor<ACameraActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
				CatCamera->GetCameraComponent()->SetFieldOfView(60.f);
				CatCamera->GetCameraComponent()->bConstrainAspectRatio = false;
			}
			const FVector Target = E->GetActorLocation() + FVector(0.f, 0.f, 20.f);
			// The first side with a clear view (not from inside the shelf).
			FVector From = Target + E->GetActorForwardVector() * 190.f + E->GetActorRightVector() * 120.f + FVector(0.f, 0.f, 25.f);
			for (const FVector2D Offset : { FVector2D(190.f, 120.f), FVector2D(190.f, -120.f), FVector2D(-190.f, 120.f), FVector2D(-190.f, -120.f), FVector2D(190.f, 0.f) })
			{
				const FVector Try = Target + E->GetActorForwardVector() * Offset.X + E->GetActorRightVector() * Offset.Y + FVector(0.f, 0.f, 25.f);
				FCollisionQueryParams Params(SCENE_QUERY_STAT(WEShotView), false, E);
				if (!GetWorld()->SweepTestByChannel(Target, Try, FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(12.f), Params))
				{
					From = Try;
					break;
				}
			}
			CatCamera->SetActorLocationAndRotation(From, (Target - From).Rotation());
			PC->SetViewTarget(CatCamera.Get());
			const FString Label(Name);
			FTimerHandle H;
			GetWorld()->GetTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [this, PC, Label, Key = FString(Key), E]()
			{
				Shot(Label);
				UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 staff %s: activity %d break %d at %s"), *Key, E ? static_cast<int32>(E->GetActivity()) : -1, E && E->IsOnBreak() ? 1 : 0,
					E ? *E->GetActorLocation().ToString() : TEXT("-"));
			}), 0.15f, false);
			FTimerHandle Back;
			GetWorld()->GetTimerManager().SetTimer(Back, FTimerDelegate::CreateWeakLambda(this, [PC]() { if (PC && PC->GetPawn()) { PC->SetViewTarget(PC->GetPawn()); } }), 0.6f, false);
		};
		Schedule(6.0f, [this, Watch]()
		{
			FTimerHandle Handle;
			GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Watch]()
			{
				AWEEmployee* Worker = FindEmployee(static_cast<uint8>(EWEStaffRole::WarehouseWorker));
				AWEEmployee* Operator = FindEmployee(static_cast<uint8>(EWEStaffRole::MachineOperator));
				Watch(Worker, EWEWorkPose::Carry, TEXT("carry"), TEXT("30_Traegt"));
				Watch(Worker, EWEWorkPose::Reach, TEXT("reach"), TEXT("31_Legt_ein"));
				Watch(Operator, EWEWorkPose::Operate, TEXT("operate"), TEXT("32_Bedient_Presse"));
				Watch(Worker, EWEWorkPose::Drink, TEXT("drink"), TEXT("33_Kaffeepause"));
				Watch(Operator, EWEWorkPose::Drink, TEXT("drink2"), TEXT("33b_Kaffeepause"));
				Watch(Worker, EWEWorkPose::Talk, TEXT("talk"), TEXT("34_Plausch"));
				Watch(Operator, EWEWorkPose::Talk, TEXT("talk2"), TEXT("34b_Plausch"));
				Watch(Worker, EWEWorkPose::Sit, TEXT("sit"), TEXT("35_Sitzt"));
				Watch(Operator, EWEWorkPose::Sit, TEXT("sit2"), TEXT("35b_Sitzt"));
			}), 0.7f, true);
		});
		Schedule(30.0f, [this]()
		{
			for (TActorIterator<AWEEmployee> It(GetWorld()); It; ++It)
			{
				It->StartBreak();
			}
		});
		// Alone on a second break: coffee, then a seat.
		Schedule(72.0f, [this]() { if (AWEEmployee* E = FindEmployee(static_cast<uint8>(EWEStaffRole::WarehouseWorker))) { E->StartBreak(); } });
		// The navmesh from above (where the staff can walk between the furniture).
		Schedule(116.0f, [this, PC]()
		{
			if (!PC)
			{
				return;
			}
			PC->ConsoleCommand(TEXT("show Navigation"));
			if (!CatCamera.IsValid())
			{
				FActorSpawnParameters Params;
				Params.ObjectFlags |= RF_Transient;
				CatCamera = GetWorld()->SpawnActor<ACameraActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
				CatCamera->GetCameraComponent()->bConstrainAspectRatio = false;
			}
			// (Under the ceiling, wide angle.)
			CatCamera->GetCameraComponent()->SetFieldOfView(112.f);
			CatCamera->SetActorLocationAndRotation(FVector(0.f, 0.f, 270.f), FRotator(-90.f, 0.f, 0.f));
			PC->SetViewTarget(CatCamera.Get());
		});
		Schedule(117.2f, [this]() { Shot(TEXT("99_Navmesh")); });
		Schedule(118.0f, [this]() { LogStaff(TEXT("P21 after break")); Quit(); });
		return;
	}
	if (ShotName == TEXT("P21C"))
	{
		// --- 3. Visible conditions: the same devices from new to defective in a row on the floor ------------------
		Schedule(1.0f, [PC]() { if (PC) { PC->ConsoleCommand(TEXT("DisableAllScreenMessages")); } });
		Schedule(2.0f, [this]()
		{
			struct FLook { EWEItemCondition Condition; uint8 Quality; };
			const FLook Looks[] = { { EWEItemCondition::New, 100 }, { EWEItemCondition::VeryGood, 85 }, { EWEItemCondition::Used, 60 },
				{ EWEItemCondition::Used, 25 }, { EWEItemCondition::Damaged, 30 }, { EWEItemCondition::Defective, 10 } };
			int32 Row = 0;
			for (const TCHAR* Id : { TEXT("CrtMonitor"), TEXT("PortableRadio"), TEXT("OldComputer") })
			{
				UWEItemDefinition* Def = UWEItemDefinition::FindById(FName(Id));
				for (int32 i = 0; Def && i < UE_ARRAY_COUNT(Looks); ++i)
				{
					FWEItemInstance Item = FWEItemInstance::Make(Def, 1, Looks[i].Condition, Looks[i].Quality);
					AWEPickupItem* Spawned = AWEPickupItem::SpawnWorldItem(GetWorld(), Item, FTransform(FRotator(0.f, 90.f, 0.f), FVector(-200.f + i * 60.f, 40.f + Row * 70.f, 30.f)));
					UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 look %s %d/%d: %s"), Id, static_cast<int32>(Looks[i].Condition), Looks[i].Quality,
						Spawned ? *Spawned->GetItem().GetWearText().ToString() : TEXT("-"));
				}
				++Row;
			}
		});
		ShotFrom(3.0f, FVector(-50.f, -120.f, 230.f), FVector(-50.f, 110.f, 10.f), TEXT("20_Zustaende"));
		Schedule(7.0f, [this]() { Quit(); });
		return;
	}
	if (ShotName == TEXT("P21B"))
	{
		// --- 2b. A radio repair by hand: multimeter, screws into the bowl, the spare part out of its box and film ---
		Schedule(1.0f, [PC]() { if (PC) { PC->ConsoleCommand(TEXT("DisableAllScreenMessages")); } });
		Schedule(2.0f, [PC]()
		{
			if (!PC) { return; }
			PC->ServerDevAddMoney(60000);
			for (const TCHAR* Tool : { TEXT("Screwdriver"), TEXT("Multimeter"), TEXT("Pliers"), TEXT("SideCutter"), TEXT("SolderingIron") })
			{
				PC->ServerDevGiveItem(Tool, 1, EWEItemCondition::Used);
			}
			if (const UWEItemDefinition* Radio = UWEItemDefinition::FindById(TEXT("PortableRadio")); Radio && Radio->Repair)
			{
				for (const FWERepairFault& Fault : Radio->Repair->Faults)
				{
					if (Fault.Part) { PC->ServerDevGiveItem(Fault.Part->ItemId, 1, EWEItemCondition::New); }
				}
			}
		});
		// A defective radio straight onto the bench (small devices would go into the inventory).
		Schedule(2.6f, [this]()
		{
			AWEWorkbench* B = FindBench();
			const UWEItemDefinition* Radio = UWEItemDefinition::FindById(TEXT("PortableRadio"));
			if (B && Radio)
			{
				AWEPickupItem* Item = AWEPickupItem::SpawnWorldItem(GetWorld(), FWEItemInstance::Make(const_cast<UWEItemDefinition*>(Radio), 1, EWEItemCondition::Defective, 40), FTransform(B->GetWorkSpotLocation() + FVector(0.f, 0.f, 20.f)));
				B->PlaceItem(Item);
			}
		});
		Schedule(4.4f, [this]() { TeleportToBench(0.f); });
		Schedule(5.0f, [this]() { if (AWEWorkbench* B = FindBench()) { LookAtPoint(B->GetWorkSpotLocation()); } });
		Schedule(6.6f, [this, PC]() { if (PC && FindBench()) { PC->OpenWorkView(FindBench(), true); } });
		Schedule(7.6f, [this, LogHand]() { Shot(TEXT("10_Radio_Arbeitsansicht")); LogHand(TEXT("open")); });
		Schedule(8.0f, [this]() { GetWorld()->GetTimerManager().SetTimer(WorkViewTimer, this, &UWEDevAutomationComponent::WorkViewTick, 0.25f, true); });
		// Pictures of the moments that matter, whenever they come.
		TSharedRef<TSet<FString>> Taken = MakeShared<TSet<FString>>();
		Schedule(8.1f, [this, View, LogHand, Taken]()
		{
			FTimerHandle Handle;
			GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, View, LogHand, Taken]()
			{
				const UWEWorkViewWidget* V = View();
				const AWEWorkRig* Rig = V ? V->GetRig() : nullptr;
				const int32 Active = Rig ? Rig->GetActiveTask() : INDEX_NONE;
				if (Rig && Rig->IsCelebrating() && !Taken->Contains(TEXT("success")))
				{
					Taken->Add(TEXT("success"));
					Shot(TEXT("17b_Erfolg"));
					LogHand(TEXT("success"));
					return;
				}
				if (!Rig || !Rig->GetPlan().Tasks.IsValidIndex(Active))
				{
					FVector Carry;
					if (Rig && Rig->GetCarryPoint(Carry) && !Taken->Contains(TEXT("carry")))
					{
						Taken->Add(TEXT("carry"));
						Shot(TEXT("13_Schraube_am_Magnet"));
						LogHand(TEXT("carry"));
					}
					return;
				}
				const EWEWorkTaskKind Kind = Rig->GetPlan().Tasks[Active].Kind;
				const float P = Rig->GetActiveProgress();
				auto Once = [&](const TCHAR* Key, const TCHAR* Name, bool bWhen)
				{
					if (bWhen && !Taken->Contains(Key))
					{
						Taken->Add(Key);
						Shot(Name);
						LogHand(Key);
					}
				};
				Once(TEXT("probe"), TEXT("11_Messen"), Kind == EWEWorkTaskKind::Probe && P > 0.3f);
				Once(TEXT("screw"), TEXT("12_Schrauben"), Kind == EWEWorkTaskKind::Screw && P > 0.5f);
				Once(TEXT("box"), TEXT("14_Ersatzteil_aus_Karton"), Kind == EWEWorkTaskKind::InsertPart && P > 0.15f && P < 0.3f);
				Once(TEXT("film"), TEXT("15_Schutzfolie"), Kind == EWEWorkTaskKind::InsertPart && P > 0.38f && P < 0.48f);
				Once(TEXT("insert"), TEXT("16_Einsetzen"), Kind == EWEWorkTaskKind::InsertPart && P > 0.75f);
				Once(TEXT("test"), TEXT("17_Funktionstest"), Kind == EWEWorkTaskKind::PowerTest && P > 0.4f && Taken->Contains(TEXT("insert")));
				Once(TEXT("air"), TEXT("21_Druckluft"), Kind == EWEWorkTaskKind::AirBlow && P > 0.5f);
				Once(TEXT("wipe"), TEXT("22_Alte_Paste"), Kind == EWEWorkTaskKind::Wipe && P > 0.4f);
				Once(TEXT("paste"), TEXT("23_Neue_Paste"), Kind == EWEWorkTaskKind::PasteApply && P > 0.6f);
			}), 0.05f, true);
		});
		Schedule(26.0f, [this, LogHand]() { Shot(TEXT("18_Schraubenschale")); LogHand(TEXT("bowl")); });
		// After the repair: the maintenance steps (air duster and thermal paste are in the inventory).
		Schedule(2.2f, [PC]() { if (PC) { PC->ServerDevGiveItem(TEXT("AirDuster"), 1, EWEItemCondition::New); PC->ServerDevGiveItem(TEXT("ThermalPaste"), 2, EWEItemCondition::New); } });
		Schedule(60.0f, [this, LogHand]()
		{
			GetWorld()->GetTimerManager().ClearTimer(WorkViewTimer);
			Shot(TEXT("19_Ende"));
			LogHand(TEXT("end"));
			const AWEWorkbench* B = FindBench();
			const AWEPickupItem* Radio = B ? B->GetPlacedItem() : nullptr;
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 radio after repair: %s cond %d broken %d tasks by hand %d"), Radio ? *Radio->GetDisplayName().ToString() : TEXT("none"),
				Radio ? static_cast<int32>(Radio->GetItem().Condition) : -1, Radio && Radio->GetItem().IsBroken() ? 1 : 0, WorkViewTasks);
		});
		Schedule(61.0f, [this]() { Quit(); });
		return;
	}
	auto LogCam = [View](const TCHAR* Step)
	{
		if (const UWEWorkViewWidget* V = View())
		{
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 camera %s: zoom %.2f target %s %s"), Step, V->GetZoomFactor(), *V->GetOrbitTarget().ToString(), *V->DescribeState());
		}
		else
		{
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 camera %s: no work view"), Step);
		}
	};

	if (ShotName == TEXT("P21W"))
	{
		// --- Real mouse clicks (Slate events) on the parts of a PC in the work view: does a click pick the part? -------
		Schedule(1.0f, [PC]() { if (PC) { PC->ConsoleCommand(TEXT("DisableAllScreenMessages")); } });
		Schedule(2.0f, [PC]() { if (PC) { PC->ServerDevGiveItem(TEXT("Screwdriver"), 1, EWEItemCondition::Used); PC->ServerDevGiveItem(TEXT("OldComputer"), 1, EWEItemCondition::Used); } });
		Schedule(3.0f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestInteractWith(FindNearestItem(TEXT("Computer"))); } });
		Schedule(4.0f, [this]() { TeleportToBench(0.f); });
		Schedule(5.0f, [this]() { if (AWEWorkbench* B = FindBench()) { LookAtPoint(B->GetWorkSpotLocation()); } });
		Schedule(5.6f, [this]() { PlaceHeldOnBench(); });
		Schedule(6.6f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestPrimaryInteraction(); } });
		auto ClickPart = [this, PC, View](int32 Part, const TCHAR* Label)
		{
			UWEWorkViewWidget* V = View();
			const AWEWorkbench* B = FindBench();
			const AWEAssemblyView* Asm = B ? B->GetAssemblyView() : nullptr;
			FVector2D Pixel;
			if (!V || !Asm || !PC || !PC->ProjectWorldLocationToScreen(Asm->GetPartCenter(Part), Pixel, true) || !GEngine->GameViewport)
			{
				UE_LOG(LogWorkshopEmpire, Display, TEXT("P21W %s: cannot click"), Label);
				return;
			}
			const float Scale = UWidgetLayoutLibrary::GetViewportScale(V);
			const FVector2D Abs = V->GetCachedGeometry().LocalToAbsolute(Pixel / FMath::Max(Scale, 0.01f));
			TSharedPtr<SWindow> Window = GEngine->GameViewport->GetWindow();
			FSlateApplication& App = FSlateApplication::Get();
			App.SetCursorPos(Abs);
			App.ProcessMouseMoveEvent(FPointerEvent(0, Abs, Abs, TSet<FKey>(), FKey(), 0.f, FModifierKeysState()));
			TSet<FKey> Pressed;
			Pressed.Add(EKeys::LeftMouseButton);
			App.ProcessMouseButtonDownEvent(Window.IsValid() ? Window->GetNativeWindow() : nullptr, FPointerEvent(0, Abs, Abs, Pressed, EKeys::LeftMouseButton, 0.f, FModifierKeysState()));
			App.ProcessMouseButtonUpEvent(FPointerEvent(0, Abs, Abs, TSet<FKey>(), EKeys::LeftMouseButton, 0.f, FModifierKeysState()));
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21W %s: clicked part %d at %s (pixel %s): %s"), Label, Part, *Abs.ToString(), *Pixel.ToString(), *V->DescribeState());
		};
		Schedule(9.0f, [this, View]() { Shot(TEXT("50_Vor_Klick")); });
		Schedule(9.5f, [ClickPart]() { ClickPart(0, TEXT("side panel")); });
		Schedule(10.5f, [this, View]() { Shot(TEXT("51_Nach_Klick")); if (View()) { UE_LOG(LogWorkshopEmpire, Display, TEXT("P21W after: %s"), *View()->DescribeState()); } });
		// Then press and hold the mouse on the first screw (as a player does) until it is out.
		TSharedRef<FVector2D> HoldAt = MakeShared<FVector2D>(FVector2D::ZeroVector);
		Schedule(12.0f, [PC, View, HoldAt]()
		{
			UWEWorkViewWidget* V = View();
			const int32 Task = V ? V->DevFindAvailableTask() : INDEX_NONE;
			FVector2D Pixel;
			if (!V || Task == INDEX_NONE || !V->GetRig() || !PC->ProjectWorldLocationToScreen(V->GetRig()->GetTaskLocation(Task), Pixel, true))
			{
				UE_LOG(LogWorkshopEmpire, Display, TEXT("P21W hold: no screw to hold"));
				return;
			}
			*HoldAt = V->GetCachedGeometry().LocalToAbsolute(Pixel / FMath::Max(UWidgetLayoutLibrary::GetViewportScale(V), 0.01f));
			FSlateApplication& App = FSlateApplication::Get();
			App.SetCursorPos(*HoldAt);
			App.ProcessMouseMoveEvent(FPointerEvent(0, *HoldAt, *HoldAt, TSet<FKey>(), FKey(), 0.f, FModifierKeysState()));
			TSet<FKey> Pressed;
			Pressed.Add(EKeys::LeftMouseButton);
			TSharedPtr<SWindow> Window = GEngine->GameViewport->GetWindow();
			App.ProcessMouseButtonDownEvent(Window.IsValid() ? Window->GetNativeWindow() : nullptr, FPointerEvent(0, *HoldAt, *HoldAt, Pressed, EKeys::LeftMouseButton, 0.f, FModifierKeysState()));
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21W hold: pressed on task %d: %s"), Task, *V->DescribeState());
		});
		Schedule(13.0f, [this]() { Shot(TEXT("52_Schraube_halten")); });
		Schedule(15.5f, [View, HoldAt]()
		{
			FSlateApplication::Get().ProcessMouseButtonUpEvent(FPointerEvent(0, *HoldAt, *HoldAt, TSet<FKey>(), EKeys::LeftMouseButton, 0.f, FModifierKeysState()));
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P21W hold: released: %s"), View() ? *View()->DescribeState() : TEXT("-"));
		});
		Schedule(16.5f, [this]() { Quit(); });
		return;
	}
	Schedule(1.0f, [PC]() { if (PC) { PC->ConsoleCommand(TEXT("DisableAllScreenMessages")); } });
	// --- 1. Repair camera: the PC on the bench, zoom towards the cursor, close-up on a screw -----------------
	Schedule(2.0f, [PC]() { if (PC) { PC->ServerDevGiveItem(TEXT("Screwdriver"), 1, EWEItemCondition::Used); PC->ServerDevAddMoney(60000); } });
	Schedule(2.4f, [PC]() { if (PC) { PC->ServerDevGiveItem(TEXT("OldComputer"), 1, EWEItemCondition::Used); } });
	Schedule(3.0f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestInteractWith(FindNearestItem(TEXT("Computer"))); } });
	Schedule(4.0f, [this]() { TeleportToBench(0.f); });
	Schedule(5.0f, [this]() { if (AWEWorkbench* B = FindBench()) { LookAtPoint(B->GetWorkSpotLocation()); } });
	Schedule(5.6f, [this]() { PlaceHeldOnBench(); });
	Schedule(6.6f, [this]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestPrimaryInteraction(); } });
	Schedule(8.0f, [this, LogCam]() { Shot(TEXT("01_Arbeitsansicht")); LogCam(TEXT("start")); });
	Schedule(8.4f, [View]() { if (UWEWorkViewWidget* V = View()) { V->DevZoom(4.f, true); } });
	Schedule(9.8f, [this, LogCam]() { Shot(TEXT("02_Herangezoomt")); LogCam(TEXT("zoomed")); });
	Schedule(10.2f, [this, View]()
	{
		UWEWorkViewWidget* V = View();
		const AWEWorkbench* B = FindBench();
		if (V && B && B->GetAssembly() && B->GetPlacedItem())
		{
			V->DevSelectPart(B->GetAssembly()->GetDefaultPart(B->GetPlacedItem()->GetItem().RemovedParts));
		}
	});
	Schedule(11.6f, [this, View, LogCam]()
	{
		if (UWEWorkViewWidget* V = View()) { V->DevWorkNext(); V->DevFocusTask(V->DevFindAvailableTask() == INDEX_NONE ? 0 : V->DevFindAvailableTask()); }
		LogCam(TEXT("part"));
	});
	Schedule(13.2f, [this, LogCam]() { Shot(TEXT("03_Nahansicht_Schraube")); LogCam(TEXT("close-up")); });
	Schedule(13.6f, [View]() { if (UWEWorkViewWidget* V = View()) { V->DevZoom(-8.f, false); } });
	Schedule(15.0f, [this, LogCam]() { Shot(TEXT("04_Wieder_weit")); LogCam(TEXT("back")); });

	// --- 2. Tools in the hand: the screwdriver on the PC's side panel screws, screws into the bowl ----------
	Schedule(15.5f, [this]() { GetWorld()->GetTimerManager().SetTimer(WorkViewTimer, this, &UWEDevAutomationComponent::WorkViewTick, 0.25f, true); });
	Schedule(15.6f, [View]() { if (UWEWorkViewWidget* V = View()) { V->DevFocusPoint(FVector::ZeroVector, true); } });
	TSharedRef<TSet<FString>> TakenA = MakeShared<TSet<FString>>();
	Schedule(15.7f, [this, View, LogHand, TakenA]()
	{
		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, View, LogHand, TakenA]()
		{
			const UWEWorkViewWidget* V = View();
			const AWEWorkRig* Rig = V ? V->GetRig() : nullptr;
			const AWEToolHand* Hand = V ? V->GetHand() : nullptr;
			FVector Carry;
			if (Rig && Hand && Hand->IsHolding() && Rig->GetPlan().Tasks.IsValidIndex(Rig->GetActiveTask()) && Rig->GetPlan().Tasks[Rig->GetActiveTask()].Kind == EWEWorkTaskKind::Screw
				&& Rig->GetActiveProgress() > 0.5f && !TakenA->Contains(TEXT("screw")))
			{
				TakenA->Add(TEXT("screw"));
				Shot(TEXT("05_Schraubendreher_an_Schraube"));
				LogHand(TEXT("screw"));
			}
			else if (Rig && Hand && Hand->IsHolding() && Rig->GetCarryPoint(Carry) && TakenA->Contains(TEXT("screw")) && !TakenA->Contains(TEXT("carry")))
			{
				TakenA->Add(TEXT("carry"));
				Shot(TEXT("06_Schraube_zur_Schale"));
				LogHand(TEXT("carry"));
			}
		}), 0.05f, true);
	});
	Schedule(22.0f, [this, LogHand]() { Shot(TEXT("07_Seitenteil")); LogHand(TEXT("later")); });
	Schedule(28.4f, [this, View]() { if (UWEWorkViewWidget* V = View(); V && FindBench()) { V->DevFocusPoint(FindBench()->GetScrewBowlLocation(), false); } });
	Schedule(30.0f, [this, LogHand]()
	{
		GetWorld()->GetTimerManager().ClearTimer(WorkViewTimer);
		Shot(TEXT("08_Schrauben_in_Schale"));
		LogHand(TEXT("end"));
		UE_LOG(LogWorkshopEmpire, Display, TEXT("P21 tasks by hand: %d"), WorkViewTasks);
	});
	Schedule(31.0f, [this]() { Quit(); });
#endif
}

void UWEDevAutomationComponent::RunSmokeRoleP21(bool bActor)
{
#if !UE_BUILD_SHIPPING
	// Phase 21 over the network (dedicated server): Y1 orders (courier), builds a tool wall, hangs up and takes a tool
	// through the server; Y2 only watches what replicates (rules, courier with parcels, parcels, hanging tools).
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	const FString Tag = bActor ? TEXT("Y1") : TEXT("Y2");
	auto Log = [this, Tag](const TCHAR* Step)
	{
		int32 Parcels = 0;
		for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It) { Parcels += UWECourierComponent::IsParcel(It->GetItem()) ? 1 : 0; }
		int32 Couriers = 0, Carrying = 0;
		for (TActorIterator<AWECourier> It(GetWorld()); It; ++It) { ++Couriers; Carrying += It->GetParcelCount(); }
		int32 Hanging = -1;
		for (TActorIterator<AWEToolWall> It(GetWorld()); It; ++It) { Hanging = It->GetHangingCount(); }
		int32 Tools = 0;
		if (const AWECharacter* C = GetCharacter())
		{
			for (const FWEInventorySlotData& S : C->GetPlayerInventory()->GetAllSlots()) { Tools += S.Item.Definition && S.Item.Definition->ToolType != EWEToolType::None ? 1 : 0; }
		}
		const UWECourierComponent* Courier = UWECourierComponent::Get(this);
		UE_LOG(LogWENet, Display, TEXT("SMOKE[%s] %s: rules='%s' pending=%d couriers=%d carrying=%d parcels=%d wall=%d myTools=%d"), *Tag, Step,
			*UWEGameRulesComponent::GetRules(this).Describe().ToString(), Courier ? Courier->GetPendingCount() : -1, Couriers, Carrying, Parcels, Hanging, Tools);
	};
	Schedule(8.f, [Log]() { Log(TEXT("0 start")); });
	if (bActor)
	{
		Schedule(10.f, [this, PC]()
		{
			if (!PC) { return; }
			PC->ServerDevAddMoney(500);
			PC->ServerDevPhase21(1);
			PC->ServerDevTeleport(FVector(-60.f, -60.f, 100.f), FRotator(0.f, -90.f, 0.f));
			PC->ServerDevGiveItem(TEXT("Screwdriver"), 1, EWEItemCondition::New);
			PC->ServerDevGiveItem(TEXT("SideCutter"), 1, EWEItemCondition::New);
		});
		Schedule(12.f, [this]()
		{
			if (UWEBuildComponent* B = GetBuild())
			{
				B->SetBuildMode(true);
				PlaceFurnitureAt(TEXT("ToolWall"), 60.f, -230.f, 90.f);
				B->SetBuildMode(false);
			}
		});
		Schedule(14.f, [this, PC]()
		{
			// In front of the wall, [R]: hang everything up (the server moves the items).
			AWEToolWall* Wall = nullptr;
			for (TActorIterator<AWEToolWall> It(GetWorld()); It; ++It) { Wall = *It; }
			if (Wall && PC)
			{
				PC->ServerDevTeleport(Wall->GetActorTransform().TransformPosition(FVector(110.f, 0.f, 0.f)) + FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator);
			}
		});
		Schedule(15.f, [this]()
		{
			AWEToolWall* Wall = nullptr;
			for (TActorIterator<AWEToolWall> It(GetWorld()); It; ++It) { Wall = *It; }
			if (AWECharacter* C = GetCharacter(); C && Wall) { C->GetInteractionComponent()->RequestSecondaryInteractWith(Wall); }
		});
		Schedule(17.f, [Log]() { Log(TEXT("1 hung up")); });
		Schedule(18.f, [this]()
		{
			// Look at the first hook, [Q]: the server takes the tool it sees the player look at.
			AWEToolWall* Wall = nullptr;
			for (TActorIterator<AWEToolWall> It(GetWorld()); It; ++It) { Wall = *It; }
			if (Wall) { LookAtPoint(Wall->GetActorTransform().TransformPosition(Wall->GetHookLocation(0) + FVector(3.f, 0.f, -13.f))); }
		});
		Schedule(19.5f, [this]()
		{
			AWEToolWall* Wall = nullptr;
			for (TActorIterator<AWEToolWall> It(GetWorld()); It; ++It) { Wall = *It; }
			if (AWECharacter* C = GetCharacter(); C && Wall) { C->GetInteractionComponent()->RequestAlternativeInteractWith(Wall); }
		});
		Schedule(21.f, [Log]() { Log(TEXT("2 took one")); });
	}
	Schedule(16.f, [Log]() { Log(TEXT("3 courier walking")); });
	Schedule(45.f, [Log]() { Log(TEXT("4 delivered")); });
	Schedule(47.f, [this]() { Quit(); });
#endif
}