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
	DOREPLIFETIME(UWECompanyComponent, CategoryTotals);
	DOREPLIFETIME(UWECompanyComponent, CategoryLastHour);
}

EWEBookingCategory UWECompanyComponent::Classify(const FString& Description)
{
	struct FRule { const TCHAR* Prefix; EWEBookingCategory Category; };
	// Order matters: longer prefixes first.
	static const FRule Rules[] = {
		{ TEXT("Verkaufsfahrt"), EWEBookingCategory::Sales },
		{ TEXT("Verkauf"), EWEBookingCategory::Sales },
		{ TEXT("Auftrag"), EWEBookingCategory::Orders },
		{ TEXT("Industrieauftrag"), EWEBookingCategory::Orders },
		{ TEXT("Liefervertrag"), EWEBookingCategory::Orders },
		{ TEXT("Vertragsstrafe"), EWEBookingCategory::Orders },
		{ TEXT("Einkauf"), EWEBookingCategory::Purchases },
		{ TEXT("Großeinkauf"), EWEBookingCategory::Purchases },
		{ TEXT("Auktion"), EWEBookingCategory::Purchases },
		{ TEXT("Einrichtung"), EWEBookingCategory::Equipment },
		{ TEXT("Rückbuchung Einrichtung"), EWEBookingCategory::Equipment },
		{ TEXT("Werkstatt"), EWEBookingCategory::Expansion },
		{ TEXT("Rückbuchung Werkstatt"), EWEBookingCategory::Expansion },
		{ TEXT("Gebäude verkauft"), EWEBookingCategory::Expansion },
		{ TEXT("Gestaltung"), EWEBookingCategory::Equipment },
		{ TEXT("Löhne"), EWEBookingCategory::Wages },
		{ TEXT("Stromrechnung"), EWEBookingCategory::Power },
		{ TEXT("Tankfüllung"), EWEBookingCategory::Logistics },
		{ TEXT("Rückbuchung Tankfüllung"), EWEBookingCategory::Logistics },
	};
	EWEBookingCategory Best = EWEBookingCategory::Other;
	int32 BestLength = 0;
	for (const FRule& Rule : Rules)
	{
		const int32 Length = FCString::Strlen(Rule.Prefix);
		if (Length > BestLength && Description.StartsWith(Rule.Prefix))
		{
			Best = Rule.Category;
			BestLength = Length;
		}
	}
	return Best;
}

FText UWECompanyComponent::GetCategoryName(EWEBookingCategory Category)
{
	switch (Category)
	{
	case EWEBookingCategory::Sales:     return NSLOCTEXT("WECompany", "CatSales", "Verkäufe");
	case EWEBookingCategory::Orders:    return NSLOCTEXT("WECompany", "CatOrders", "Aufträge und Verträge");
	case EWEBookingCategory::Purchases: return NSLOCTEXT("WECompany", "CatPurchases", "Einkauf");
	case EWEBookingCategory::Equipment: return NSLOCTEXT("WECompany", "CatEquipment", "Einrichtung und Maschinen");
	case EWEBookingCategory::Expansion: return NSLOCTEXT("WECompany", "CatExpansion", "Ausbau, Gebäude, Fahrzeuge");
	case EWEBookingCategory::Wages:     return NSLOCTEXT("WECompany", "CatWages", "Löhne");
	case EWEBookingCategory::Power:     return NSLOCTEXT("WECompany", "CatPower", "Strom");
	case EWEBookingCategory::Logistics: return NSLOCTEXT("WECompany", "CatLogistics", "Logistik");
	default:                            return NSLOCTEXT("WECompany", "CatOther", "Sonstiges");
	}
}

int64 UWECompanyComponent::GetCategoryTotalCents(EWEBookingCategory Category) const
{
	const int32 Index = static_cast<int32>(Category);
	return CategoryTotals.IsValidIndex(Index) ? CategoryTotals[Index] : 0;
}

int64 UWECompanyComponent::GetCategoryLastHourCents(EWEBookingCategory Category) const
{
	const int32 Index = static_cast<int32>(Category);
	return CategoryLastHour.IsValidIndex(Index) ? CategoryLastHour[Index] : 0;
}

void UWECompanyComponent::SetCategoryTotals(const TArray<int64>& InTotals)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	CategoryTotals = InTotals;
	CategoryTotals.SetNumZeroed(static_cast<int32>(EWEBookingCategory::Count));
	HourEntries.Reset();
	UpdateLastHour();
}

void UWECompanyComponent::UpdateLastHour()
{
	const float Now = GetWorld() ? static_cast<float>(GetWorld()->GetTimeSeconds()) : 0.f;
	HourEntries.RemoveAll([Now](const FHourEntry& E) { return Now - E.Time > 3600.f; });
	CategoryLastHour.Init(0, static_cast<int32>(EWEBookingCategory::Count));
	for (const FHourEntry& E : HourEntries)
	{
		CategoryLastHour[E.Category] += E.Cents;
	}
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
	// Books by category (all time and the last hour).
	const uint8 Category = static_cast<uint8>(Classify(Description));
	CategoryTotals.SetNumZeroed(static_cast<int32>(EWEBookingCategory::Count));
	CategoryTotals[Category] += Amount.GetCents();
	HourEntries.Add({ GetWorld() ? static_cast<float>(GetWorld()->GetTimeSeconds()) : 0.f, Category, Amount.GetCents() });
	UpdateLastHour();
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
