#include "Economy/WECompanyComponent.h"

#include "Engine/World.h"
#include "Framework/WEGameState.h"
#include "Net/UnrealNetwork.h"
#include "WELog.h"

UWECompanyComponent::UWECompanyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UWECompanyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWECompanyComponent, Balance);
	DOREPLIFETIME(UWECompanyComponent, RecentTransactions);
}

UWECompanyComponent* UWECompanyComponent::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AWEGameState* GameState = World ? World->GetGameState<AWEGameState>() : nullptr;
	return GameState ? GameState->GetCompany() : nullptr;
}

void UWECompanyComponent::OnRep_Company()
{
	OnCompanyChanged.Broadcast();
}

void UWECompanyComponent::AddRecord(const FWEMoney& Amount, const FString& Description, const FString& PlayerName)
{
	FWETransactionRecord Record;
	Record.Amount = Amount;
	Record.Description = Description;
	Record.PlayerName = PlayerName;
	RecentTransactions.Insert(Record, 0);
	if (RecentTransactions.Num() > MaxRecentTransactions)
	{
		RecentTransactions.SetNum(MaxRecentTransactions);
	}
}

bool UWECompanyComponent::Deposit(const FWEMoney& Amount, const FString& Description, const FString& PlayerName)
{
	FWEMoney NewBalance;
	if (!GetOwner()->HasAuthority() || Amount.IsNegative() || !FWEMoney::TryAdd(Balance, Amount, NewBalance))
	{
		return false;
	}
	Balance = NewBalance;
	AddRecord(Amount, Description, PlayerName);
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Company +%s (%s) -> %s"), *Amount.ToDisplayString(), *Description, *Balance.ToDisplayString());
	OnRep_Company();
	return true;
}

bool UWECompanyComponent::TryWithdraw(const FWEMoney& Amount, const FString& Description, const FString& PlayerName)
{
	FWEMoney NewBalance;
	if (!GetOwner()->HasAuthority() || Amount.IsNegative() || !CanAfford(Amount) || !FWEMoney::TrySubtract(Balance, Amount, NewBalance))
	{
		return false;
	}
	Balance = NewBalance;
	AddRecord(FWEMoney::FromCents(-Amount.GetCents()), Description, PlayerName);
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Company -%s (%s) -> %s"), *Amount.ToDisplayString(), *Description, *Balance.ToDisplayString());
	OnRep_Company();
	return true;
}

void UWECompanyComponent::SetBalance(const FWEMoney& NewBalance)
{
	if (GetOwner()->HasAuthority() && !NewBalance.IsNegative())
	{
		Balance = NewBalance;
		RecentTransactions.Reset();
		OnRep_Company();
	}
}

bool UWECompanyComponent::RegisterRequest(const FGuid& RequestId)
{
	if (!RequestId.IsValid() || ProcessedRequests.Contains(RequestId))
	{
		UE_LOG(LogWENet, Warning, TEXT("Company: duplicate or invalid request %s ignored"), *RequestId.ToString());
		return false;
	}
	ProcessedRequests.Add(RequestId);
	ProcessedOrder.Add(RequestId);
	if (ProcessedOrder.Num() > MaxRememberedRequests)
	{
		ProcessedRequests.Remove(ProcessedOrder[0]);
		ProcessedOrder.RemoveAt(0);
	}
	return true;
}
