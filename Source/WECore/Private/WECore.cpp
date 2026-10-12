#include "WECore.h"
#include "WELog.h"

void FWECoreModule::StartupModule()
{
	UE_LOG(LogWorkshopEmpire, Log, TEXT("WECore module started."));
}

void FWECoreModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FWECoreModule, WECore)
