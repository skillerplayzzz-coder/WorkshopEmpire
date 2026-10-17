#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "WEGameState.generated.h"

class UWEAuctionComponent;
class UWEWorkshopLayoutComponent;
class UWEPowerGridComponent;
class UWEStaffComponent;
class UWECompanyComponent;
class UWEContractComponent;
class UWEMarketComponent;
class UWEProgressionComponent;
class UWEMilestoneComponent;

/**
 * Replicated state shared by every player of the session: the shared company (account),
 * the market (demand), the customer orders and the progression (upgrades, guide). Mutations only happen on the server; clients read
 * replicated values and send requests through their PlayerController.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AWEGameState();

	UWECompanyComponent* GetCompany() const { return Company; }
	UWEMarketComponent* GetMarket() const { return Market; }
	UWEContractComponent* GetContracts() const { return Contracts; }
	UWEProgressionComponent* GetProgression() const { return Progression; }
	UWEAuctionComponent* GetAuctions() const { return Auctions; }
	UWEWorkshopLayoutComponent* GetLayout() const { return Layout; }
	UWEPowerGridComponent* GetPowerGrid() const { return PowerGrid; }
	UWEStaffComponent* GetStaff() const { return Staff; }
	UWEMilestoneComponent* GetMilestones() const { return Milestones; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Company")
	TObjectPtr<UWECompanyComponent> Company;

	UPROPERTY(VisibleAnywhere, Category = "Company")
	TObjectPtr<UWEMarketComponent> Market;

	UPROPERTY(VisibleAnywhere, Category = "Company")
	TObjectPtr<UWEContractComponent> Contracts;

	UPROPERTY(VisibleAnywhere, Category = "Company")
	TObjectPtr<UWEProgressionComponent> Progression;

	UPROPERTY(VisibleAnywhere, Category = "Company")
	TObjectPtr<UWEAuctionComponent> Auctions;

	UPROPERTY(VisibleAnywhere, Category = "Company")
	TObjectPtr<UWEWorkshopLayoutComponent> Layout;

	UPROPERTY(VisibleAnywhere, Category = "Company")
	TObjectPtr<UWEPowerGridComponent> PowerGrid;

	UPROPERTY(VisibleAnywhere, Category = "Company")
	TObjectPtr<UWEStaffComponent> Staff;

	UPROPERTY(VisibleAnywhere, Category = "Company")
	TObjectPtr<UWEMilestoneComponent> Milestones;
};
