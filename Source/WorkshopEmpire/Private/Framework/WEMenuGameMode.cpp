#include "Framework/WEMenuGameMode.h"

#include "Camera/CameraActor.h"
#include "Dev/WEDevAutomationComponent.h"
#include "EngineUtils.h"
#include "Online/WESessionSubsystem.h"
#include "UI/WEMenuWidgets.h"
#include "WELog.h"

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
	}
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
