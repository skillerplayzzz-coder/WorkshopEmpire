// Phase 20 trailer (-WEShots=P20T on L_Garage, best with -UseFixedTimeStep -FPS=30): camera moves through the game's
// scenes and every frame is written to Saved/Trailer/f_00000.png; Tools/Release/make_trailer.ps1 turns them into a video.
#include "Dev/WEDevAutomationComponent.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Containers/Ticker.h"
#include "EngineUtils.h"
#include "Framework/WEPlayerController.h"
#include "Economy/WETraderTerminal.h"
#include "Logistics/WEVehicle.h"
#include "Staff/WEEmployee.h"
#include "UI/WEHUDWidget.h"
#include "UI/WETradeWidget.h"
#include "UnrealClient.h"
#include "WELog.h"
#include "Workshop/WEBrandComponent.h"
#include "World/WEAtmosphere.h"
#include "World/WEDayNightComponent.h"
#include "World/WEGarageDoor.h"
#include "Player/WECharacter.h"

namespace WETrailer
{
	struct FScene
	{
		float Start = 0.f;
		float Duration = 5.f;
		FVector From0, From1, Look0, Look1;
		bool bUI = false;
		/** Camera relative to an actor found at the scene start (employee, cat), else world positions. */
		TFunction<AActor*()> Anchor;
	};
}

void UWEDevAutomationComponent::RunTrailer()
{
#if !UE_BUILD_SHIPPING
	using WETrailer::FScene;
	AWEPlayerController* PC = Cast<AWEPlayerController>(GetOwner());
	UWorld* World = GetWorld();
	auto DayNight = [this]() { return UWEDayNightComponent::Get(this); };
	auto Cat = [World]() { TActorIterator<AWEWorkshopCat> It(World); return It ? *It : nullptr; };
	auto Employee = [World]() -> AActor* { TActorIterator<AWEEmployee> It(World); return It ? *It : nullptr; };

	// Set up the company: money, the van, two employees in the company colours, the brand.
	Schedule(1.0f, [this, PC]()
	{
		if (PC) { PC->ConsoleCommand(TEXT("DisableAllScreenMessages")); PC->ServerDevAddMoney(8000); }
		TeleportToTerminal(0.f);
	});
	Schedule(2.0f, [this, PC]()
	{
		BuyUpgradeAtTerminal(TEXT("OldVan"));
		if (PC)
		{
			AWETraderTerminal* Terminal = const_cast<AWETraderTerminal*>(FindTerminal());
			PC->RequestHire(Terminal, 0);
			PC->RequestHire(Terminal, 1);
			FWECompanyBrand Brand;
			Brand.Symbol = 1;
			Brand.SymbolColor = 1;
			Brand.Background = 4;
			Brand.WorkwearColor = 2;
			Brand.Name = TEXT("Schraubwerk Müller");
			PC->RequestSetBrand(Brand);
		}
	});

	constexpr float T0 = 8.f;
	TArray<FScene> Scenes;
	auto Add = [&Scenes](float Duration, FVector F0, FVector F1, FVector L0, FVector L1, bool bUI = false, TFunction<AActor*()> Anchor = nullptr)
	{
		FScene S;
		S.Start = Scenes.Num() ? Scenes.Last().Start + Scenes.Last().Duration : 0.f;
		S.Duration = Duration;
		S.From0 = F0; S.From1 = F1; S.Look0 = L0; S.Look1 = L1;
		S.bUI = bUI;
		S.Anchor = MoveTemp(Anchor);
		Scenes.Add(MoveTemp(S));
	};
	// 0 Morning: towards the garage with the logo above the door.
	Add(5.5f, FVector(1250.f, 450.f, 280.f), FVector(920.f, 170.f, 210.f), FVector(423.f, 0.f, 250.f), FVector(423.f, 0.f, 225.f));
	// 1 Inside: along the workbench with radio and coffee machine.
	Add(5.0f, FVector(-140.f, -190.f, 165.f), FVector(-170.f, 150.f, 150.f), FVector(-385.f, -60.f, 100.f), FVector(-385.f, 60.f, 100.f));
	// 2 The cat (relative to it: low orbit).
	Add(4.5f, FVector(60.f, 110.f, 25.f), FVector(110.f, -40.f, 30.f), FVector(0.f, 0.f, 0.f), FVector(0.f, 0.f, 0.f), false, Cat);
	// 3 Employees in the company overall.
	Add(4.5f, FVector(230.f, 80.f, 90.f), FVector(170.f, -60.f, 80.f), FVector(0.f, 0.f, 60.f), FVector(0.f, 0.f, 50.f), false, Employee);
	// 4 The logo editor in the Ferrit OS (UI).
	Add(4.5f, FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector, true);
	// 5 The van with the logo.
	Add(4.5f, FVector(500.f, -800.f, 170.f), FVector(560.f, -380.f, 160.f), FVector(780.f, -560.f, 120.f), FVector(780.f, -560.f, 120.f));
	// 6 Rain in the yard, the garage behind it.
	Add(5.5f, FVector(950.f, -180.f, 150.f), FVector(760.f, -60.f, 160.f), FVector(423.f, 0.f, 210.f), FVector(423.f, 0.f, 190.f));
	// 7 Night storm with lightning.
	Add(6.0f, FVector(1350.f, -750.f, 300.f), FVector(1100.f, -420.f, 250.f), FVector(420.f, 0.f, 220.f), FVector(420.f, 0.f, 200.f));
	// 8 Evening: the company sign.
	Add(5.0f, FVector(300.f, -1650.f, 210.f), FVector(370.f, -1460.f, 240.f), FVector(370.f, -1217.f, 260.f), FVector(370.f, -1217.f, 270.f));
	const float Total = Scenes.Last().Start + Scenes.Last().Duration;

	// Scene changes (world, weather, time of day).
	auto At = [this](float Time, TFunction<void()> Fn) { Schedule(T0 + Time, MoveTemp(Fn)); };
	At(-0.5f, [this, PC, DayNight]()
	{
		if (UWEDayNightComponent* D = DayNight()) { D->SetTime(D->GetDay(), 8.5f * 60.f); D->SetWeather(EWEWeather::Clear, true); }
		if (PC && PC->GetHUDWidget()) { PC->GetHUDWidget()->SetVisibility(ESlateVisibility::Collapsed); }
		if (AWECharacter* C = GetCharacter()) { C->SetActorHiddenInGame(true); }
		UE_LOG(LogWorkshopEmpire, Display, TEXT("TRAILER start"));
	});
	At(Scenes[2].Start - 0.3f, [Cat]()
	{
		if (AWEWorkshopCat* K = Cat())
		{
			K->SetActorLocation(FVector(-200.f, 30.f, 20.f), false, nullptr, ETeleportType::TeleportPhysics);
			K->SetActorRotation(FRotator(0.f, 150.f, 0.f));
			K->SetCatState(EWECatState::Sit, 30.f);
		}
	});
	At(Scenes[4].Start - 0.3f, [this, PC]()
	{
		TeleportToTerminal(0.f);
		if (PC)
		{
			PC->SetViewTarget(PC->GetPawn());
			LookAt(FindTerminal());
			PC->OpenTrade(FindTerminal());
			if (PC->GetTradeWidget()) { PC->GetTradeWidget()->ShowPage(6); PC->GetTradeWidget()->ShowStyleTab(4); }
		}
	});
	At(Scenes[5].Start - 0.1f, [PC]() { if (PC) { PC->CloseTrade(); } });
	At(Scenes[6].Start - 0.3f, [this, DayNight]()
	{
		if (UWEDayNightComponent* D = DayNight()) { D->SetTime(D->GetDay(), 14.f * 60.f); D->SetWeather(EWEWeather::Rain, true); }
		if (AWEGarageDoor* Door = Cast<AWEGarageDoor>(FindDoor()); Door && GetCharacter()) { Door->Interact(GetCharacter()); }
		// The two employees step aside (into the workshop).
		float Y = -150.f;
		for (TActorIterator<AWEEmployee> It(GetWorld()); It; ++It, Y += 300.f)
		{
			It->SetActorLocation(FVector(-150.f, Y, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
		}
	});
	At(Scenes[7].Start - 0.3f, [DayNight]()
	{
		if (UWEDayNightComponent* D = DayNight()) { D->SetTime(D->GetDay(), 21.75f * 60.f); D->SetWeather(EWEWeather::Storm, true); }
	});
	At(Scenes[7].Start + 1.5f, [DayNight]() { if (UWEDayNightComponent* D = DayNight()) { D->TriggerLightning(); UE_LOG(LogWorkshopEmpire, Display, TEXT("TRAILER lightning")); } });
	At(Scenes[7].Start + 4.4f, [DayNight]() { if (UWEDayNightComponent* D = DayNight()) { D->TriggerLightning(); UE_LOG(LogWorkshopEmpire, Display, TEXT("TRAILER lightning")); } });
	At(Scenes[8].Start - 0.3f, [DayNight]()
	{
		if (UWEDayNightComponent* D = DayNight()) { D->SetTime(D->GetDay(), 19.4f * 60.f); D->SetWeather(EWEWeather::Clear, true); }
	});

	// Every frame: move the camera, write the frame.
	At(0.f, [this, PC, Scenes, Total]()
	{
		const float Start = GetWorld()->GetTimeSeconds();
		TSharedRef<int32> Frame = MakeShared<int32>(0);
		TSharedRef<int32> SceneIndex = MakeShared<int32>(-1);
		TSharedRef<FVector> AnchorPos = MakeShared<FVector>(FVector::ZeroVector);
		TSharedRef<FRotator> AnchorRot = MakeShared<FRotator>(FRotator::ZeroRotator);
		const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Trailer"));
		IFileManager::Get().DeleteDirectory(*Dir, false, true);
		IFileManager::Get().MakeDirectory(*Dir, true);
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
		Cam->GetCameraComponent()->SetFieldOfView(62.f);
		Cam->GetCameraComponent()->bConstrainAspectRatio = false;
		// Cuts between bright yard and dark workshop: the eye adapts at once instead of over seconds.
		FPostProcessSettings& Post = Cam->GetCameraComponent()->PostProcessSettings;
		Post.bOverride_AutoExposureSpeedUp = true;
		Post.AutoExposureSpeedUp = 12.f;
		Post.bOverride_AutoExposureSpeedDown = true;
		Post.AutoExposureSpeedDown = 12.f;
		Cam->GetCameraComponent()->PostProcessBlendWeight = 1.f;
		TWeakObjectPtr<UWorld> WeakWorld = GetWorld();
		TrailerTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
			[this, PC, Scenes, Total, Start, Frame, SceneIndex, AnchorPos, AnchorRot, Dir, Cam, WeakWorld](float) -> bool
		{
			if (!WeakWorld.IsValid() || !PC)
			{
				return false;
			}
			const float T = WeakWorld->GetTimeSeconds() - Start;
			if (T >= Total)
			{
				UE_LOG(LogWorkshopEmpire, Display, TEXT("TRAILER done: %d frames"), *Frame);
				Quit();
				return false;
			}
			int32 Index = 0;
			while (Index + 1 < Scenes.Num() && T >= Scenes[Index + 1].Start) { ++Index; }
			const FScene& S = Scenes[Index];
			if (Index != *SceneIndex)
			{
				*SceneIndex = Index;
				AActor* Anchor = S.Anchor ? S.Anchor() : nullptr;
				*AnchorPos = Anchor ? Anchor->GetActorLocation() : FVector::ZeroVector;
				*AnchorRot = Anchor ? FRotator(0.f, Anchor->GetActorRotation().Yaw, 0.f) : FRotator::ZeroRotator;
				UE_LOG(LogWorkshopEmpire, Display, TEXT("TRAILER scene %d at frame %d"), Index, *Frame);
			}
			if (!S.bUI)
			{
				const float A = FMath::SmoothStep(0.f, 1.f, (T - S.Start) / S.Duration);
				const FVector From = *AnchorPos + AnchorRot->RotateVector(FMath::Lerp(S.From0, S.From1, A));
				const FVector Look = *AnchorPos + AnchorRot->RotateVector(FMath::Lerp(S.Look0, S.Look1, A));
				Cam->SetActorLocationAndRotation(From, (Look - From).Rotation());
				if (PC->GetViewTarget() != Cam)
				{
					PC->SetViewTarget(Cam);
				}
			}
			FScreenshotRequest::RequestScreenshot(FPaths::Combine(Dir, FString::Printf(TEXT("f_%05d.png"), *Frame)), S.bUI, false);
			++*Frame;
			return true;
		}));
	});
#endif
}
