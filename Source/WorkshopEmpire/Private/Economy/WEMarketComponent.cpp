#include "Economy/WEMarketComponent.h"

#include "Economy/WETraderDefinition.h"
#include "Engine/World.h"
#include "Framework/WEGameState.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace WEMarket
{
	constexpr int32 NumCategories = static_cast<int32>(EWEItemCategory::Tool) + 1;

	FWEMoney Scale(const FWEMoney& Value, double Factor)
	{
		return FWEMoney::FromCents(static_cast<int64>(FMath::RoundToDouble(static_cast<double>(Value.GetCents()) * Factor)));
	}
}

UWEMarketComponent::UWEMarketComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Demand.Init(1.f, WEMarket::NumCategories);
}

void UWEMarketComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWEMarketComponent, Demand);
}

void UWEMarketComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority() && DriftIntervalSeconds > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(DriftTimer, this, &UWEMarketComponent::DriftDemand, DriftIntervalSeconds, true);
	}
}

void UWEMarketComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DriftTimer);
	}
	Super::EndPlay(EndPlayReason);
}

UWEMarketComponent* UWEMarketComponent::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AWEGameState* GameState = World ? World->GetGameState<AWEGameState>() : nullptr;
	return GameState ? GameState->GetMarket() : nullptr;
}

float UWEMarketComponent::GetDemand(EWEItemCategory Category) const
{
	const int32 Index = static_cast<int32>(Category);
	return Demand.IsValidIndex(Index) ? Demand[Index] : 1.f;
}

void UWEMarketComponent::SetDemand(EWEItemCategory Category, float Value)
{
	const int32 Index = static_cast<int32>(Category);
	if (Demand.IsValidIndex(Index))
	{
		Demand[Index] = FMath::Clamp(Value, MinDemand, MaxDemand);
	}
}

void UWEMarketComponent::SetAllDemand(const TArray<float>& Values)
{
	for (int32 i = 0; i < Demand.Num(); ++i)
	{
		Demand[i] = Values.IsValidIndex(i) ? FMath::Clamp(Values[i], MinDemand, MaxDemand) : 1.f;
	}
}

void UWEMarketComponent::DriftDemand()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	for (float& Value : Demand)
	{
		// Small steps with a pull back towards 1.0, so prices move but never run away.
		Value = FMath::Clamp(Value + FMath::FRandRange(-0.06f, 0.06f) + (1.f - Value) * 0.1f, MinDemand, MaxDemand);
	}
}

FWEMoney UWEMarketComponent::GetMarketValue(const FWEItemInstance& Item, const UWEMarketComponent* Market)
{
	if (!Item.IsValid())
	{
		return FWEMoney();
	}
	const float DemandFactor = Market ? Market->GetDemand(Item.Definition->Category) : 1.f;
	return WEMarket::Scale(Item.GetEstimatedValue(), DemandFactor);
}

FWEMoney UWEMarketComponent::GetSellPrice(const FWEItemInstance& Item, const UWETraderDefinition* Trader, const UWEMarketComponent* Market)
{
	if (!Item.IsValid() || !Trader || Item.IsPartiallyDisassembled())
	{
		return FWEMoney(); // nobody buys half-disassembled junk at a normal price
	}
	const float TraderFactor = Trader->GetBuyMultiplier(Item.Definition->Category);
	if (TraderFactor <= 0.f)
	{
		return FWEMoney();
	}
	return WEMarket::Scale(GetMarketValue(Item, Market), SellShare * TraderFactor);
}

FWEMoney UWEMarketComponent::GetBuyPrice(const FWEItemInstance& Item, float PriceMultiplier, const UWEMarketComponent* Market)
{
	if (!Item.IsValid())
	{
		return FWEMoney();
	}
	const FWEMoney Price = WEMarket::Scale(GetMarketValue(Item, Market), BuyMarkup * FMath::Max(0.01f, PriceMultiplier));
	return Price.GetCents() < 1 ? FWEMoney::FromCents(1) : Price;
}
