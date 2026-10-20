#include "Framework/WEGameState.h"
#include "Economy/WEClassifiedsComponent.h"
#include "World/WEDayNightComponent.h"
#include "Workshop/WEBrandComponent.h"

#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WEMarketComponent.h"
#include "Progression/WEMilestoneComponent.h"
#include "Progression/WEProgressionComponent.h"
#include "Economy/WEAuctionComponent.h"
#include "Workshop/WEWorkshopLayoutComponent.h"
#include "Machines/WEPowerGridComponent.h"
#include "Staff/WEStaffComponent.h"
#include "Progression/WETutorialComponent.h"
#include "Workshop/WEWorkshopStyleComponent.h"

AWEGameState::AWEGameState()
{
	Company = CreateDefaultSubobject<UWECompanyComponent>(TEXT("Company"));
	Market = CreateDefaultSubobject<UWEMarketComponent>(TEXT("Market"));
	Contracts = CreateDefaultSubobject<UWEContractComponent>(TEXT("Contracts"));
	Progression = CreateDefaultSubobject<UWEProgressionComponent>(TEXT("Progression"));
	Auctions = CreateDefaultSubobject<UWEAuctionComponent>(TEXT("Auctions"));
	Layout = CreateDefaultSubobject<UWEWorkshopLayoutComponent>(TEXT("Layout"));
	PowerGrid = CreateDefaultSubobject<UWEPowerGridComponent>(TEXT("PowerGrid"));
	Staff = CreateDefaultSubobject<UWEStaffComponent>(TEXT("Staff"));
	Milestones = CreateDefaultSubobject<UWEMilestoneComponent>(TEXT("Milestones"));
	Tutorial = CreateDefaultSubobject<UWETutorialComponent>(TEXT("Tutorial"));
	WorkshopStyle = CreateDefaultSubobject<UWEWorkshopStyleComponent>(TEXT("WorkshopStyle"));
	Classifieds = CreateDefaultSubobject<UWEClassifiedsComponent>(TEXT("Classifieds"));
	DayNight = CreateDefaultSubobject<UWEDayNightComponent>(TEXT("DayNight"));
	Brand = CreateDefaultSubobject<UWEBrandComponent>(TEXT("Brand"));
}
