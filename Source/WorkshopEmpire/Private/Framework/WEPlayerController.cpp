#include "Framework/WEPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Dev/WEDevAutomationComponent.h"
#include "UI/WEHUDWidget.h"
#include "WELog.h"

AWEPlayerController::AWEPlayerController()
{
	HUDWidgetClass = UWEHUDWidget::StaticClass();
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
				HUDWidget->AddToViewport();
			}
		}

		if (UWEDevAutomationComponent::IsRequested())
		{
			UWEDevAutomationComponent* Automation = NewObject<UWEDevAutomationComponent>(this, TEXT("DevAutomation"));
			Automation->RegisterComponent();
		}
	}
}
