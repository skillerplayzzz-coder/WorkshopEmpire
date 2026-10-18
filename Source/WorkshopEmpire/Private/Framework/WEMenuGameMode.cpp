#include "Framework/WEMenuGameMode.h"

#include "Camera/CameraActor.h"
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
	if (UWEDevAutomationComponent::IsRequested())
	{
		UWEDevAutomationComponent* Automation = NewObject<UWEDevAutomationComponent>(this, TEXT("DevAutomation"));
		Automation->RegisterComponent();
	}
}
