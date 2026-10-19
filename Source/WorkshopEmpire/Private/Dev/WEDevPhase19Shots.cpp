// Phase 19 screenshot runs: -WEShots=P19C in the main menu (wardrobe: every outfit and face of both bodies in the
// character creator), -WEShots=P19G in the garage (night, walk-in customers, classifieds, PC orders, mirror, forklift).
#include "Dev/WEDevAutomationComponent.h"

#include "Framework/WEMenuGameMode.h"
#include "Player/WEAvatarComponent.h"
#include "Player/WEAvatarPreview.h"
#include "Player/WECharacterLook.h"
#include "UI/WEMenuWidgets.h"
#include "WELog.h"
#include "EngineUtils.h"
#include "Economy/WEContractComponent.h"
#include "Framework/WEPlayerController.h"
#include "Interaction/WEInteractionComponent.h"
#include "Player/WECharacter.h"
#include "UI/WEMirrorWidget.h"
#include "UI/WETradeWidget.h"
#include "World/WECustomer.h"
#include "World/WEDayNightComponent.h"
#include "World/WEMirror.h"
#include "Logistics/WEForklift.h"

void UWEDevAutomationComponent::RunPhase19Shots()
{
#if !UE_BUILD_SHIPPING
	if (!IsInMenu())
	{
		RunPhase19GameShots();
		return;
	}
	AWEMenuPlayerController* MenuPC = Cast<AWEMenuPlayerController>(GetOwner());
	auto Menu = [MenuPC]() { return MenuPC ? MenuPC->GetMenu() : nullptr; };
	// Steps a field of the creator until it shows the wanted option.
	auto SetField = [Menu](EWELookField Field, int32 Value)
	{
		UWEMainMenuWidget* M = Menu();
		for (int32 i = 0; M && i < 12 && M->GetEditLook().Get(Field) != Value; ++i)
		{
			M->StepLook(static_cast<int32>(Field), 1);
		}
	};
	auto LogGarments = [Menu](const TCHAR* Name)
	{
		const AWEAvatarPreview* P = Menu() ? Menu()->GetPreview() : nullptr;
		const UWEAvatarComponent* A = P ? P->GetAvatar() : nullptr;
		FString List;
		for (const FName& G : A ? A->GetGarments() : TArray<FName>())
		{
			List += G.ToString() + TEXT(" ");
		}
		UE_LOG(LogWorkshopEmpire, Display, TEXT("P19C %s: look %s, garments %s"), Name, Menu() ? *Menu()->GetEditLook().ToString() : TEXT("-"), *List);
	};
	float T = 2.5f;
	Schedule(T, [Menu]() { if (Menu()) { Menu()->ShowPage(5); } });
	T += 2.0f;
	static const TCHAR* OutfitNames[] = { TEXT("Overall"), TEXT("Polo"), TEXT("Hoodie"), TEXT("TShirt"), TEXT("Jacke") };
	for (int32 Body = 0; Body < 2; ++Body)
	{
		const TCHAR* BodyName = Body == 0 ? TEXT("Mann") : TEXT("Frau");
		for (int32 Outfit = 0; Outfit < 5; ++Outfit)
		{
			Schedule(T, [=]()
			{
				SetField(EWELookField::Body, Body);
				SetField(EWELookField::Hair, Body == 0 ? 1 : 4);
				SetField(EWELookField::Headwear, 0);
				SetField(EWELookField::Glasses, 0);
				SetField(EWELookField::Outfit, Outfit);
				SetField(EWELookField::OutfitColor, (Outfit + Body) % 5);
				SetField(EWELookField::Shoes, Outfit % 3);
			});
			T += 1.6f;
			const FString Name = FString::Printf(TEXT("%s_%d_%s"), BodyName, Outfit, OutfitNames[Outfit]);
			Schedule(T, [this, Name, LogGarments]() { LogGarments(*Name); Shot(Name); });
			T += 0.4f;
			if (Outfit == 0 || Outfit == 2 || Outfit == 4)
			{
				Schedule(T, [Menu]() { if (Menu()) { Menu()->TurnPreview(150.f); } });
				T += 1.4f;
				Schedule(T, [this, Name]() { Shot(Name + TEXT("_Ruecken")); });
				T += 0.3f;
				Schedule(T, [Menu]() { if (Menu()) { Menu()->TurnPreview(-150.f); } });
				T += 0.3f;
			}
		}
		// Faces close up.
		const int32 Faces = WELook::NumOptionsFor(EWELookField::Face, Body);
		for (int32 Face = 0; Face < Faces; ++Face)
		{
			Schedule(T, [=]()
			{
				SetField(EWELookField::Face, Face);
				if (UWEMainMenuWidget* M = Menu(); M && M->GetEditLook().Face == Face && Face == 0)
				{
					// Already on it: step away and back so the creator switches to the close-up.
					M->StepLook(static_cast<int32>(EWELookField::Face), 1);
					M->StepLook(static_cast<int32>(EWELookField::Face), -1);
				}
			});
			T += 1.5f;
			const FString Name = FString::Printf(TEXT("%s_Gesicht_%d"), BodyName, Face);
			Schedule(T, [this, Name, LogGarments]() { LogGarments(*Name); Shot(Name); });
			T += 0.3f;
		}
	}
	Schedule(T + 1.f, [this]() { Quit(); });
#endif
}

void UWEDevAutomationComponent::RunPhase19GameShots()
{
#if !UE_BUILD_SHIPPING
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	auto Mirror = [this]() -> AWEMirror*
	{
		for (TActorIterator<AWEMirror> It(GetWorld()); It; ++It)
		{
			return *It;
		}
		return nullptr;
	};
	auto Log = [this, PC](const TCHAR* Step)
	{
		const AWECharacter* Me = GetCharacter();
		const UWEDayNightComponent* Clock = UWEDayNightComponent::Get(this);
		int32 Customers = 0;
		for (TActorIterator<AWECustomer> It(GetWorld()); It; ++It)
		{
			++Customers;
		}
		const AWEMirror* M = nullptr;
		for (TActorIterator<AWEMirror> It(GetWorld()); It; ++It)
		{
			M = *It;
		}
		UE_LOG(LogWorkshopEmpire, Display, TEXT("P19G %s: frame %llu at %.2f s, look %s, garments %d, clock %s, customers %d, mirror captures %d, mirror ui %d"), Step, (unsigned long long)GFrameCounter, FPlatformTime::Seconds(),
			Me ? *Me->GetLook().ToString() : TEXT("-"), Me && Me->GetAvatar() ? Me->GetAvatar()->GetGarments().Num() : -1,
			Clock ? *Clock->GetClockText().ToString() : TEXT("-"), Customers, M ? M->GetCaptureCount() : -1, PC && PC->IsMirrorOpen() ? 1 : 0);
	};
	// 1) In front of the mirror: see yourself, then change clothes there.
	Schedule(3.0f, [this, PC, Mirror]()
	{
		if (const AWEMirror* M = Mirror(); M && PC)
		{
			const FVector Front = M->GetActorLocation() + M->GetActorForwardVector() * 230.f + FVector(0.f, 0.f, 100.f);
			PC->ServerDevTeleport(Front, (M->GetActorLocation() + FVector(0.f, 0.f, 115.f) - Front).Rotation());
		}
	});
	Schedule(3.6f, [this, Mirror]() { if (const AWEMirror* M = Mirror()) { LookAtPoint(M->GetActorLocation() + FVector(0.f, 0.f, 115.f)); } });
	Schedule(5.0f, [this, Log]() { Log(TEXT("mirror")); Shot(TEXT("01_Spiegel")); });
	Schedule(5.4f, [this, Mirror]() { if (AWECharacter* C = GetCharacter()) { C->GetInteractionComponent()->RequestInteractWith(Mirror()); } });
	Schedule(6.4f, [this, PC, Log]()
	{
		if (UWEMirrorWidget* W = PC ? PC->GetMirrorWidget() : nullptr; W && PC->IsMirrorOpen())
		{
			W->StepLook(static_cast<int32>(EWELookField::Outfit), 1);
			W->StepLook(static_cast<int32>(EWELookField::OutfitColor), 1);
			W->StepLook(static_cast<int32>(EWELookField::Headwear), 1);
		}
		Log(TEXT("mirror editor"));
	});
	Schedule(8.0f, [this, Log]() { Log(TEXT("changed")); Shot(TEXT("02_Spiegel_Umziehen")); });
	Schedule(8.4f, [PC]() { if (PC && PC->GetMirrorWidget()) { PC->GetMirrorWidget()->StepLook(static_cast<int32>(EWELookField::Outfit), 1); } });
	Schedule(9.6f, [this]() { Shot(TEXT("03_Spiegel_Jacke")); });
	Schedule(10.0f, [PC]() { if (PC) { PC->CloseMirror(); } });
	// 2) A walk-in customer at the door (spawned on the sidewalk, walks over).
	Schedule(10.5f, [this, PC]()
	{
		WEWalkIn::TrySpawn(GetWorld(), true);
		if (PC)
		{
			PC->ServerDevTeleport(FVector(900.f, -260.f, 100.f), FRotator(0.f, 150.f, 0.f));
		}
	});
	Schedule(24.0f, [this]()
	{
		for (TActorIterator<AWECustomer> It(GetWorld()); It; ++It)
		{
			LookAtPoint(It->GetActorLocation() + FVector(0.f, 0.f, 40.f));
		}
	});
	Schedule(25.0f, [this, Log]() { Log(TEXT("customer")); Shot(TEXT("04_Laufkundschaft")); });
	// 3) Ferrit OS: classifieds and PC orders.
	Schedule(25.5f, [this, PC]()
	{
		if (!PC)
		{
			return;
		}
		PC->ServerDevAddMoney(3000);
		const UWEContractComponent* Contracts = UWEContractComponent::Get(this);
		const UWEContractCatalog* Catalog = Contracts ? Contracts->GetCatalog() : nullptr;
		for (int32 i = 0; Catalog && i < Catalog->Templates.Num(); ++i)
		{
			if (Catalog->Templates[i].Kind == EWEContractKind::BuildPc || Catalog->Templates[i].Kind == EWEContractKind::UpgradePc)
			{
				PC->ServerDevAddContractOffer(i);
			}
		}
		TeleportToTerminal(0.f);
	});
	Schedule(26.5f, [this, PC]() { LookAt(FindTerminal()); if (PC) { PC->OpenTrade(FindTerminal()); } });
	auto Trade = [PC]() { return PC ? PC->GetTradeWidget() : nullptr; };
	Schedule(27.5f, [Trade]() { if (Trade()) { Trade()->ShowPage(8); } });
	Schedule(28.6f, [this]() { Shot(TEXT("05_Kleinanzeigen")); });
	Schedule(29.0f, [Trade]() { if (Trade()) { Trade()->ShowPage(4); } });
	Schedule(30.0f, [this]() { Shot(TEXT("06_PC_Auftraege")); });
	Schedule(30.4f, [PC]() { if (PC) { PC->CloseTrade(); } });
	// 4) Evening and night: sun goes down, street lights come on, the clock in the HUD.
	Schedule(31.0f, [this]() { if (UWEDayNightComponent* Clock = UWEDayNightComponent::Get(this)) { Clock->SetTime(1, 19.f * 60.f + 30.f); } });
	ShotFrom(31.2f, FVector(900.f, 300.f, 170.f), FVector(-200.f, -100.f, 120.f), TEXT("07_Abend"));
	Schedule(35.0f, [this]() { if (UWEDayNightComponent* Clock = UWEDayNightComponent::Get(this)) { Clock->SetTime(1, 22.f * 60.f); } });
	ShotFrom(35.2f, FVector(900.f, 300.f, 170.f), FVector(-200.f, -100.f, 120.f), TEXT("08_Nacht"));
	ShotFrom(39.0f, FVector(600.f, -900.f, 220.f), FVector(1500.f, -1300.f, 200.f), TEXT("09_Nacht_Strasse"));
	Schedule(43.0f, [this]() { if (UWEDayNightComponent* Clock = UWEDayNightComponent::Get(this)) { Clock->SetTime(2, 9.f * 60.f); } });
	// 5) The forklift (bought as an upgrade).
	Schedule(43.5f, [this]()
	{
		for (TActorIterator<AWEForklift> It(GetWorld()); It; ++It)
		{
			It->SetUnlocked(true); // as if bought
		}
	});
	ShotFrom(44.5f, FVector(120.f, -20.f, 150.f), FVector(-110.f, -195.f, 90.f), TEXT("10_Gabelstapler"));
	Schedule(48.5f, [this, Log]() { Log(TEXT("end")); Quit(); });
#endif
}