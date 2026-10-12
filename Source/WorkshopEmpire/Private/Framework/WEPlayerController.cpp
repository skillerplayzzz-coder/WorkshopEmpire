#include "Framework/WEPlayerController.h"

#include "WELog.h"

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
}
