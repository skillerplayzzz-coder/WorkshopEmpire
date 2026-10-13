#include "Framework/WEGameState.h"

#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WEMarketComponent.h"
#include "Progression/WEProgressionComponent.h"

AWEGameState::AWEGameState()
{
	Company = CreateDefaultSubobject<UWECompanyComponent>(TEXT("Company"));
	Market = CreateDefaultSubobject<UWEMarketComponent>(TEXT("Market"));
	Contracts = CreateDefaultSubobject<UWEContractComponent>(TEXT("Contracts"));
	Progression = CreateDefaultSubobject<UWEProgressionComponent>(TEXT("Progression"));
}
