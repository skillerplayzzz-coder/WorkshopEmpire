#include "Framework/WEMenuGameMode.h"

#include "Camera/CameraActor.h"
#include "UI/WEIntroWidget.h"
#include "UnrealClient.h"
#include "Dev/WEDevAutomationComponent.h"
#include "EngineUtils.h"
#include "Online/WESessionSubsystem.h"
#include "UI/WEMenuWidgets.h"
#include "UI/WESettings.h"
#include "WELog.h"

void AWEMenuPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	// Phase 17.5: the camera drifts slowly over the workshop scene (a living background, no cuts).
	if (ACameraActor* Camera = MenuCamera.Get())
	{
		MenuTime += DeltaTime;
		const float S = FMath::Sin(MenuTime * 0.07f);
		const float C = FMath::Cos(MenuTime * 0.05f);
		const FVector Offset = CameraStart.GetRotation().GetRightVector() * (S * 70.f) + FVector(0.f, 0.f, C * 12.f);
		Camera->SetActorLocation(CameraStart.GetLocation() + Offset);
		Camera->SetActorRotation(CameraStart.Rotator() + FRotator(C * 0.8f, S * -3.f, 0.f));
	}
}

AWEMenuGameMode::AWEMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AWEMenuPlayerController::StaticClass();
}

void AWEMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}
	if (TActorIterator<ACameraActor> Camera(GetWorld()); Camera)
	{
		SetViewTarget(*Camera);
		MenuCamera = *Camera;
		CameraStart = Camera->GetActorTransform();
	}
	WESettings::ApplyGameValues(WESettings::Load());
	UWESessionSubsystem* Session = UWESessionSubsystem::Get(this);
	if (Session)
	{
		++Session->DevMenuVisits;
		Session->NotifyMenuOpened();
	}
	Menu = CreateWidget<UWEMainMenuWidget>(this, UWEMainMenuWidget::StaticClass());
	if (Menu)
	{
		Menu->AddToViewport(0);
		const FText Message = Session ? Session->ConsumePendingMessage() : FText::GetEmpty();
		Menu->ShowMessage(Message);
		if (!Message.IsEmpty())
		{
			UE_LOG(LogWENet, Display, TEXT("Main menu message: %s"), *Message.ToString());
		}
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(Menu->TakeWidget());
		SetInputMode(Mode);
		SetShowMouseCursor(true);
	}
	// Intro video when the game starts (first visit of the menu only; not in automated runs; -NoIntro skips it).
	const bool bIntroShot = FParse::Param(FCommandLine::Get(), TEXT("WEIntroShot"));
	const bool bFirstVisit = !Session || Session->DevMenuVisits <= 1;
	if (Menu && bFirstVisit && UWEIntroWidget::HasMovie() && !FParse::Param(FCommandLine::Get(), TEXT("NoIntro"))
		&& (bIntroShot || !UWEDevAutomationComponent::IsRequested()))
	{
		Intro = CreateWidget<UWEIntroWidget>(this, UWEIntroWidget::StaticClass());
		Intro->AddToViewport(100);
		FInputModeUIOnly IntroMode;
		IntroMode.SetWidgetToFocus(Intro->TakeWidget());
		SetInputMode(IntroMode);
		SetShowMouseCursor(false);
		TWeakObjectPtr<AWEMenuPlayerController> WeakThis(this);
		Intro->Start([WeakThis]()
		{
			AWEMenuPlayerController* PC = WeakThis.Get();
			if (!PC || !PC->Menu)
			{
				return;
			}
			FInputModeUIOnly Back;
			Back.SetWidgetToFocus(PC->Menu->TakeWidget());
			PC->SetInputMode(Back);
			PC->SetShowMouseCursor(true);
			PC->Intro = nullptr;
		});
		if (bIntroShot)
		{
			// Dev check: pictures during the intro and after it, then quit.
			FTimerHandle T1, T2, T3;
			GetWorldTimerManager().SetTimer(T1, []() { FScreenshotRequest::RequestScreenshot(TEXT("IntroShot_01_Video"), true, false); }, 1.6f, false);
			GetWorldTimerManager().SetTimer(T2, [this]() { FScreenshotRequest::RequestScreenshot(TEXT("IntroShot_01b_Video"), true, false); UE_LOG(LogWENet, Display, TEXT("INTRO playing=%d"), Intro && Intro->IsPlaying() ? 1 : 0); }, 3.f, false);
			GetWorldTimerManager().SetTimer(T3, [this]() { if (Intro) { Intro->Finish(); } }, 8.f, false);
			FTimerHandle T4, T5;
			GetWorldTimerManager().SetTimer(T4, []() { FScreenshotRequest::RequestScreenshot(TEXT("IntroShot_02_Menu"), true, false); }, 10.f, false);
			GetWorldTimerManager().SetTimer(T5, [this]() { ConsoleCommand(TEXT("quit")); }, 12.f, false);
		}
	}
	if (UWEDevAutomationComponent::IsRequested())
	{
		UWEDevAutomationComponent* Automation = NewObject<UWEDevAutomationComponent>(this, TEXT("DevAutomation"));
		Automation->RegisterComponent();
	}
}
