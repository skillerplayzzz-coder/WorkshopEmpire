#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "WEGameState.generated.h"

class UWECompanyComponent;
class UWEContractComponent;
class UWEMarketComponent;
class UWEProgressionComponent;

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

protected:
	UPROPERTY(VisibleAnywhere, Category = "Company")
	TObjectPtr<UWECompanyComponent> Company;

	UPROPERTY(VisibleAnywhere, Category = "Company")
	TObjectPtr<UWEMarketComponent> Market;

	UPROPERTY(VisibleAnywhere, Category = "Company")
	TObjectPtr<UWEContractComponent> Contracts;

	UPROPERTY(VisibleAnywhere, Category = "Company")
	TObjectPtr<UWEProgressionComponent> Progression;
};
