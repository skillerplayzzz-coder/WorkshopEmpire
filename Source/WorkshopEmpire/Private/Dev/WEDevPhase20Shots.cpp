// Phase 20 screenshot run (-WEShots=P20 on L_Garage): company logo and work clothes, employees as real figures, and
// later the new repairs, the atmosphere and the settings.
#include "Dev/WEDevAutomationComponent.h"

#include "EngineUtils.h"
#include "Framework/WEPlayerController.h"
#include "Interaction/WEInteractionComponent.h"
#include "Logistics/WEVehicle.h"
#include "Player/WEAvatarComponent.h"
#include "Player/WECharacter.h"
#include "Staff/WEEmployee.h"
#include "UI/WETradeWidget.h"
#include "WELog.h"
#include "Workshop/WEBrandComponent.h"
#include "Workshop/WELogoComponent.h"
#include "World/WELogoActor.h"
#include "Components/ScrollBox.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/WEInputConfig.h"
#include "UI/WEMenuWidgets.h"
#include "World/WEAtmosphere.h"
#include "World/WEDayNightComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "World/WEGarageDoor.h"

void UWEDevAutomationComponent::RunPhase20Shots()
{
#if !UE_BUILD_SHIPPING
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	if (ShotName == TEXT("P20T"))
	{
		RunTrailer();
		return;
	}
	if (ShotName == TEXT("P20W"))
	{
		// Quick weather look (tuning): rain at noon from the yard.
		Schedule(2.0f, [this]() { if (UWEDayNightComponent* D = UWEDayNightComponent::Get(this)) { D->SetWeather(EWEWeather::Rain, true); } });
		ShotFrom(2.5f, FVector(900.f, -300.f, 170.f), FVector(420.f, 0.f, 250.f), TEXT("Regen"));
		Schedule(6.0f, [this]() { if (UWEDayNightComponent* D = UWEDayNightComponent::Get(this)) { D->SetWeather(EWEWeather::Storm, true); } });
		Schedule(8.5f, [this]() { Shot(TEXT("Sturm")); });
		Schedule(9.5f, [this]() { Quit(); });
		return;
	}
	auto Van = [this]() { TActorIterator<AWEVehicle> It(GetWorld()); return It ? *It : nullptr; };
	auto Log = [this](const TCHAR* Step)
	{
		const FWECompanyBrand Brand = UWEBrandComponent::GetBrandOf(this);
		int32 Logos = 0, Employees = 0, EmployeeGarments = 0;
		FString Names;
		for (TObjectIterator<UWELogoComponent> It; It; ++It)
		{
			if (It->GetWorld() == GetWorld())
			{
				++Logos;
				Names += It->GetShownName() + TEXT("/");
			}
		}
		for (TActorIterator<AWEEmployee> It(GetWorld()); It; ++It)
		{
			++Employees;
			EmployeeGarments += It->GetAvatar() ? It->GetAvatar()->GetGarments().Num() : 0;
		}
		UE_LOG(LogWorkshopEmpire, Display, TEXT("P20 %s: brand symbol %d colours %d/%d workwear %d name '%s', logos %d (%s), employees %d with %d garments"), Step,
			Brand.Symbol, Brand.SymbolColor, Brand.Background, Brand.WorkwearColor, *Brand.Name, Logos, *Names, Employees, EmployeeGarments);
	};
	auto SignLogo = [this]() -> AActor*
	{
		for (TActorIterator<AWELogoActor> It(GetWorld()); It; ++It)
		{
			if (FMath::Abs(It->GetActorRotation().Yaw + 90.f) < 5.f)
			{
				return *It;
			}
		}
		return nullptr;
	};
	Schedule(3.0f, [this, PC]() { if (PC) { PC->ServerDevAddMoney(8000); } TeleportToTerminal(0.f); });
	Schedule(4.0f, [this, PC]()
	{
		BuyUpgradeAtTerminal(TEXT("OldVan"));
		if (PC)
		{
			AWETraderTerminal* Terminal = const_cast<AWETraderTerminal*>(FindTerminal());
			PC->RequestHire(Terminal, 0);
			PC->RequestHire(Terminal, 1);
		}
	});
	Schedule(6.0f, [Log]() { Log(TEXT("default")); });
	ShotFrom(6.2f, FVector(820.f, 0.f, 180.f), FVector(423.f, 0.f, 270.f), TEXT("01_Logo_Tor"));
	Schedule(9.5f, [this, Van]() { if (const AWEVehicle* V = Van()) { UE_LOG(LogWorkshopEmpire, Display, TEXT("P20 van at %s yaw %.0f"), *V->GetActorLocation().ToString(), V->GetActorRotation().Yaw); } });
	ShotFrom(9.6f, FVector(520.f, -560.f, 170.f), FVector(780.f, -560.f, 120.f), TEXT("02_Logo_Transporter"));
	Schedule(13.0f, [this, SignLogo]()
	{
		if (const AActor* S = SignLogo())
		{
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P20 sign logo at %s"), *S->GetActorLocation().ToString());
		}
	});
	ShotFrom(13.2f, FVector(370.f, -1520.f, 230.f), FVector(370.f, -1217.f, 260.f), TEXT("03_Firmenschild"));
	// The logo editor in the Ferrit OS, then a new brand: white gear on red, green work clothes.
	Schedule(16.5f, [this, PC]() { TeleportToTerminal(0.f); });
	Schedule(17.5f, [this, PC]() { LookAt(FindTerminal()); if (PC) { PC->OpenTrade(FindTerminal()); } });
	Schedule(18.5f, [PC]() { if (PC && PC->GetTradeWidget()) { PC->GetTradeWidget()->ShowPage(6); PC->GetTradeWidget()->ShowStyleTab(4); } });
	Schedule(19.6f, [this]() { Shot(TEXT("04_Logo_Editor")); });
	Schedule(20.0f, [PC]()
	{
		FWECompanyBrand Brand;
		Brand.Symbol = 1;
		Brand.SymbolColor = 1;
		Brand.Background = 4;
		Brand.WorkwearColor = 2;
		Brand.Name = TEXT("Schraubwerk Müller");
		if (PC) { PC->RequestSetBrand(Brand); }
	});
	Schedule(21.2f, [this, Log]() { Shot(TEXT("05_Logo_Editor_neu")); Log(TEXT("changed")); });
	Schedule(21.6f, [PC]() { if (PC) { PC->CloseTrade(); } });
	ShotFrom(22.0f, FVector(820.f, 0.f, 180.f), FVector(423.f, 0.f, 270.f), TEXT("06_Logo_Tor_neu"));
	ShotFrom(25.5f, FVector(520.f, -560.f, 170.f), FVector(780.f, -560.f, 120.f), TEXT("07_Transporter_neu"));
	ShotFrom(29.0f, FVector(370.f, -1520.f, 230.f), FVector(370.f, -1217.f, 260.f), TEXT("08_Firmenschild_neu"));
	// The employees: real figures in the green company overall with the logo.
	Schedule(32.5f, [this, PC]()
	{
		for (TActorIterator<AWEEmployee> It(GetWorld()); It; ++It)
		{
			const FVector At = It->GetActorLocation() + It->GetActorForwardVector() * 170.f + FVector(0.f, 0.f, 40.f);
			if (PC) { PC->ServerDevTeleport(At, FRotator::ZeroRotator); }
			break;
		}
	});
	Schedule(33.2f, [this]()
	{
		for (TActorIterator<AWEEmployee> It(GetWorld()); It; ++It)
		{
			LookAtPoint(It->GetActorLocation() + FVector(0.f, 0.f, 30.f));
			break;
		}
	});
	Schedule(34.5f, [this, Log]() { Shot(TEXT("09_Mitarbeiter")); Log(TEXT("employees")); });

	// --- Atmosphere: coffee, radio, the cat, rain and thunderstorm ---------------------------------------
	auto Me = [PC]() { return PC ? Cast<AWECharacter>(PC->GetPawn()) : nullptr; };
	auto Cat = [this]() { TActorIterator<AWEWorkshopCat> It(GetWorld()); return It ? *It : nullptr; };
	auto Radio = [this]() { TActorIterator<AWEWorkshopRadio> It(GetWorld()); return It ? *It : nullptr; };
	auto Weather = [this](const TCHAR* Step)
	{
		const UWEDayNightComponent* D = UWEDayNightComponent::Get(this);
		UE_LOG(LogWorkshopEmpire, Display, TEXT("P20 weather %s: %s rain %.2f drops %d indoors %d lightning %d"), Step,
			D ? *D->GetWeatherText().ToString() : TEXT("-"), D ? D->GetRainAmount() : -1.f, D ? D->GetRainParticleCount() : -1,
			D && D->IsViewIndoors() ? 1 : 0, D ? D->GetLightningCount() : -1);
	};
	Schedule(36.0f, [this, Me]()
	{
		TActorIterator<AWECoffeeMachine> It(GetWorld());
		AWECharacter* C = Me();
		const float WalkBefore = C ? C->GetCharacterMovement()->MaxWalkSpeed : 0.f;
		if (It && C)
		{
			It->Interact(C);
		}
		UE_LOG(LogWorkshopEmpire, Display, TEXT("P20 coffee: machine %d cups %d boost %.0fs walk %.0f -> %.0f work x%.2f"), It ? 1 : 0, It ? It->GetCupsServed() : -1,
			C ? C->GetCoffeeBoostRemaining() : -1.f, WalkBefore, C ? C->GetCharacterMovement()->MaxWalkSpeed : 0.f, C ? C->GetWorkBoost() : 0.f);
	});
	ShotFrom(36.5f, FVector(-250.f, 10.f, 165.f), FVector(-385.f, 0.f, 100.f), TEXT("10_Radio_Kaffee"));
	Schedule(39.5f, [Radio, Me]()
	{
		AWEWorkshopRadio* R = Radio();
		const int32 Before = R ? R->GetTrack() : -1;
		if (R) { R->InteractAlternative(Me()); }
		UE_LOG(LogWorkshopEmpire, Display, TEXT("P20 radio: found %d on %d playing %d track %d -> %d"), R ? 1 : 0, R && R->IsOn() ? 1 : 0,
			R && R->IsPlayingLocally() ? 1 : 0, Before + 1, R ? R->GetTrack() + 1 : -1);
	});
	// The cat close up: a camera beside it at cat height (the player's eyes are too high for a cat).
	auto CatCam = [this, PC, Cat]()
	{
		const AWEWorkshopCat* K = Cat();
		if (!K || !PC)
		{
			return;
		}
		if (!CatCamera.IsValid())
		{
			FActorSpawnParameters Params;
			Params.ObjectFlags |= RF_Transient;
			CatCamera = GetWorld()->SpawnActor<ACameraActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
			CatCamera->GetCameraComponent()->SetFieldOfView(55.f);
			CatCamera->GetCameraComponent()->bConstrainAspectRatio = false;
		}
		const FVector Target = K->GetActorLocation() + FVector(0.f, 0.f, -2.f);
		const FVector From = Target + K->GetActorRightVector() * 115.f + K->GetActorForwardVector() * 45.f + FVector(0.f, 0.f, 28.f);
		CatCamera->SetActorLocationAndRotation(From, (Target - From).Rotation());
		PC->SetViewTarget(CatCamera.Get());
	};
	Schedule(40.0f, [Cat, CatCam]()
	{
		if (AWEWorkshopCat* K = Cat())
		{
			K->SetCatState(EWECatState::Sit, 30.f);
		}
		CatCam();
	});
	Schedule(41.8f, [this, Cat]()
	{
		Shot(TEXT("11_Katze_sitzt"));
		const AWEWorkshopCat* K = Cat();
		UE_LOG(LogWorkshopEmpire, Display, TEXT("P20 cat: found %d at %s state %d"), K ? 1 : 0, K ? *K->GetActorLocation().ToString() : TEXT("-"), K ? static_cast<int32>(K->GetCatState()) : -1);
	});
	Schedule(42.2f, [Cat, Me]()
	{
		AWEWorkshopCat* K = Cat();
		if (K && Me()) { K->Interact(Me()); }
		UE_LOG(LogWorkshopEmpire, Display, TEXT("P20 cat petted: count %d state %d"), K ? K->GetPetCount() : -1, K ? static_cast<int32>(K->GetCatState()) : -1);
	});
	Schedule(42.5f, [CatCam]() { CatCam(); });
	Schedule(43.4f, [this]() { Shot(TEXT("12_Katze_gestreichelt")); });
	Schedule(44.0f, [Cat]() { if (AWEWorkshopCat* K = Cat()) { K->SetCatState(EWECatState::Sleep, 60.f); } });
	Schedule(46.4f, [CatCam]() { CatCam(); });
	Schedule(46.6f, [this]() { Shot(TEXT("13_Katze_schlaeft")); });
	Schedule(46.9f, [Cat]()
	{
		if (AWEWorkshopCat* K = Cat())
		{
			K->SetCatState(EWECatState::Walk, 20.f);
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P20 cat walk from %s"), *K->GetActorLocation().ToString());
		}
	});
	Schedule(47.9f, [CatCam]() { CatCam(); });
	Schedule(48.0f, [this]() { Shot(TEXT("13b_Katze_laeuft")); });
	Schedule(48.2f, [this, PC, Me]()
	{
		if (PC && PC->GetPawn()) { PC->SetViewTarget(PC->GetPawn()); }
		// Open the garage door: rain outside, dry inside.
		if (AWEGarageDoor* Door = Cast<AWEGarageDoor>(FindDoor()); Door && Me()) { Door->Interact(Me()); }
	});	Schedule(48.3f, [this]() { if (UWEDayNightComponent* D = UWEDayNightComponent::Get(this)) { D->SetWeather(EWEWeather::Rain, true); } });
	ShotFrom(48.4f, FVector(900.f, -300.f, 170.f), FVector(420.f, 0.f, 150.f), TEXT("14_Regen_Hof"));
	Schedule(51.8f, [Weather]() { Weather(TEXT("rain outside")); });
	ShotFrom(52.0f, FVector(230.f, 170.f, 165.f), FVector(700.f, -150.f, 150.f), TEXT("15_Regen_vom_Tor"));
	Schedule(55.2f, [Weather, Cat]()
	{
		Weather(TEXT("rain inside"));
		if (const AWEWorkshopCat* K = Cat())
		{
			UE_LOG(LogWorkshopEmpire, Display, TEXT("P20 cat walk now at %s state %d"), *K->GetActorLocation().ToString(), static_cast<int32>(K->GetCatState()));
		}
	});
	Schedule(55.6f, [this]()
	{
		if (UWEDayNightComponent* D = UWEDayNightComponent::Get(this))
		{
			D->SetTime(D->GetDay(), 21.5f * 60.f);
			D->SetWeather(EWEWeather::Storm, true);
		}
	});
	ShotFrom(56.0f, FVector(900.f, -300.f, 170.f), FVector(420.f, 0.f, 150.f), TEXT("16_Gewitter_Nacht"));
	Schedule(59.6f, [this]() { if (UWEDayNightComponent* D = UWEDayNightComponent::Get(this)) { D->TriggerLightning(); } });
	Schedule(59.78f, [this]() { Shot(TEXT("17_Blitz")); });
	Schedule(62.0f, [Weather]() { Weather(TEXT("storm")); });
	Schedule(62.4f, [this]()
	{
		if (UWEDayNightComponent* D = UWEDayNightComponent::Get(this))
		{
			D->SetWeather(EWEWeather::Clear, true);
			D->SetTime(D->GetDay(), 11.f * 60.f);
		}
		TeleportToTerminal(0.f);
	});

	// --- Settings: sliders, motion blur, key assignment ---------------------------------------------------
	Schedule(63.5f, [PC]() { if (PC) { PC->OpenPauseMenu(); if (PC->GetPauseMenu()) { PC->GetPauseMenu()->ShowSettings(true); } } });
	Schedule(64.8f, [this]() { Shot(TEXT("18_Einstellungen")); });
	Schedule(65.2f, [PC]()
	{
		UWESettingsPanel* Panel = PC && PC->GetPauseMenu() ? PC->GetPauseMenu()->GetSettingsPanel() : nullptr;
		if (UScrollBox* Scroll = Panel ? Cast<UScrollBox>(Panel->GetParent()) : nullptr)
		{
			Scroll->ScrollToEnd();
		}
	});
	Schedule(66.3f, [this]() { Shot(TEXT("19_Tastenbelegung")); });
	Schedule(66.8f, [PC, Me]()
	{
		UWESettingsPanel* Panel = PC && PC->GetPauseMenu() ? PC->GetPauseMenu()->GetSettingsPanel() : nullptr;
		const int32 Interact = WEKeys::GetEntries().IndexOfByPredicate([](const WEKeys::FEntry& E) { return E.Id == TEXT("Interact"); });
		bool bOk = false;
		if (Panel)
		{
			Panel->BeginKeyCapture(Interact);
			bOk = Panel->CaptureKey(EKeys::G);
		}
		const AWECharacter* C = Me();
		const UWEInputConfig* Config = C ? C->GetInputConfig() : nullptr;
		UE_LOG(LogWorkshopEmpire, Display, TEXT("P20 keys: panel %d set %d interact now %s, mapping %s, examine %s"), Panel ? 1 : 0, bOk ? 1 : 0,
			*WEKeys::GetKey(TEXT("Interact")).ToString(), Config ? *Config->GetKeyForAction(Config->InteractAction).ToString() : TEXT("-"),
			*WEKeys::GetKey(TEXT("Examine")).ToString());
		// F was taken by "Untersuchen": the two swap.
		if (Panel)
		{
			Panel->BeginKeyCapture(Interact);
			Panel->CaptureKey(EKeys::F);
		}
		UE_LOG(LogWorkshopEmpire, Display, TEXT("P20 keys swap: interact %s examine %s"), *WEKeys::GetKey(TEXT("Interact")).ToString(), *WEKeys::GetKey(TEXT("Examine")).ToString());
	});
	Schedule(67.3f, [this]() { Shot(TEXT("20_Taste_getauscht")); });
	Schedule(67.8f, [PC, Me]()
	{
		WEKeys::ResetAll();
		const AWECharacter* C = Me();
		const UWEInputConfig* Config = C ? C->GetInputConfig() : nullptr;
		UE_LOG(LogWorkshopEmpire, Display, TEXT("P20 keys reset: interact %s mapping %s"), *WEKeys::GetKey(TEXT("Interact")).ToString(),
			Config ? *Config->GetKeyForAction(Config->InteractAction).ToString() : TEXT("-"));
		if (PC) { PC->ClosePauseMenu(); }
	});
	Schedule(69.0f, [this]() { Quit(); });
#endif
}