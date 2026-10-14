#include "Economy/WEAuctionComponent.h"

#include "Economy/WECompanyComponent.h"
#include "Economy/WEMarketComponent.h"
#include "Economy/WETraderTerminal.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/WEGameState.h"
#include "Framework/WEPlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "WELog.h"

#define LOCTEXT_NAMESPACE "WEAuction"

UWEAuctionComponent::UWEAuctionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	static ConstructorHelpers::FObjectFinder<UWEAuctionCatalog> DefaultCatalog(TEXT("/Game/WorkshopEmpire/Data/Economy/DA_AuctionCatalog.DA_AuctionCatalog"));
	Catalog = DefaultCatalog.Object;
	Random.Initialize(777);
}

void UWEAuctionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWEAuctionComponent, Lots);
	DOREPLIFETIME(UWEAuctionComponent, RecentResults);
}

void UWEAuctionComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority())
	{
		Random.Initialize(FMath::Rand());
		UpdateAuctions();
		GetWorld()->GetTimerManager().SetTimer(UpdateTimer, this, &UWEAuctionComponent::UpdateAuctions, 1.f, true);
	}
}

void UWEAuctionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimer);
	}
	Super::EndPlay(EndPlayReason);
}

UWEAuctionComponent* UWEAuctionComponent::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AWEGameState* GameState = World ? World->GetGameState<AWEGameState>() : nullptr;
	return GameState ? GameState->GetAuctions() : nullptr;
}

float UWEAuctionComponent::GetServerTime() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}
	if (GetOwner()->HasAuthority())
	{
		return World->GetTimeSeconds();
	}
	const AGameStateBase* GameState = World->GetGameState();
	return GameState ? static_cast<float>(GameState->GetServerWorldTimeSeconds()) : World->GetTimeSeconds();
}

void UWEAuctionComponent::OnRep_Lots()
{
	OnAuctionsChanged.Broadcast();
}

const FWEAuctionLot* UWEAuctionComponent::FindLot(const FGuid& Id) const
{
	return Lots.FindByPredicate([&Id](const FWEAuctionLot& L) { return L.Id == Id; });
}

FWEMoney UWEAuctionComponent::GetIncrement(const FWEMoney& Bid)
{
	// 5 % of the bid, at least 1 €, rounded up to 50 cents.
	const int64 Raw = FMath::Max<int64>(100, FMath::CeilToInt64(Bid.GetCents() * 0.05));
	return FWEMoney::FromCents(((Raw + 49) / 50) * 50);
}

FWEMoney UWEAuctionComponent::GetNextBid(const FWEAuctionLot& Lot)
{
	return Lot.NumBids == 0 ? Lot.CurrentBid : FWEMoney::FromCents(Lot.CurrentBid.GetCents() + GetIncrement(Lot.CurrentBid).GetCents());
}

FString UWEAuctionComponent::PickBidderName()
{
	if (Catalog && Catalog->BidderNames.Num() > 0)
	{
		return Catalog->BidderNames[Random.RandRange(0, Catalog->BidderNames.Num() - 1)];
	}
	return TEXT("Mitbieter");
}

EWEAuctionResult UWEAuctionComponent::PlaceBid(const FGuid& LotId, const FString& PlayerName, FWEMoney& OutBid)
{
	FWEAuctionLot* Lot = Lots.FindByPredicate([&LotId](const FWEAuctionLot& L) { return L.Id == LotId; });
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!GetOwner()->HasAuthority() || !Lot || !Company || Lot->EndsAt <= GetServerTime())
	{
		return EWEAuctionResult::NotFound;
	}
	if (Lot->bCompanyLeads)
	{
		return EWEAuctionResult::AlreadyLeading;
	}
	const FWEMoney Bid = GetNextBid(*Lot);
	if (!Company->CanAfford(Bid))
	{
		return EWEAuctionResult::NotEnoughMoney;
	}
	OutBid = Bid;
	Lot->CurrentBid = Bid;
	Lot->bCompanyLeads = true;
	Lot->LeaderName = PlayerName;
	++Lot->NumBids;
	const float Now = GetServerTime();
	Lot->EndsAt = FMath::Max(Lot->EndsAt, Now + AntiSnipeSeconds);

	// The rival answers at once while his hidden limit allows it.
	EWEAuctionResult Result = EWEAuctionResult::Leading;
	const FWEMoney Counter = FWEMoney::FromCents(Bid.GetCents() + GetIncrement(Bid).GetCents());
	if (const int64* Limit = BidderMaxCents.Find(LotId); Limit && Counter.GetCents() <= *Limit)
	{
		Lot->CurrentBid = Counter;
		Lot->bCompanyLeads = false;
		FString* Rival = RivalNames.Find(LotId);
		Lot->LeaderName = Rival ? *Rival : RivalNames.Add(LotId, PickBidderName());
		++Lot->NumBids;
		Result = EWEAuctionResult::Outbid;
	}
	UE_LOG(LogWENet, Log, TEXT("Auction '%s': %s bids %s -> %s"), *Lot->Title.ToString(), *PlayerName, *Bid.ToDisplayString(),
		Result == EWEAuctionResult::Outbid ? *FString::Printf(TEXT("outbid by %s (%s)"), *Lot->LeaderName, *Lot->CurrentBid.ToDisplayString()) : TEXT("leading"));
	OnRep_Lots();
	return Result;
}

void UWEAuctionComponent::AddResult(const FString& Text)
{
	RecentResults.Insert(Text, 0);
	if (RecentResults.Num() > 4)
	{
		RecentResults.SetNum(4);
	}
}

bool UWEAuctionComponent::FinishLot(const FGuid& LotId)
{
	const int32 Index = Lots.IndexOfByPredicate([&LotId](const FWEAuctionLot& L) { return L.Id == LotId; });
	if (!GetOwner()->HasAuthority() || Index == INDEX_NONE)
	{
		return false;
	}
	const FWEAuctionLot Lot = Lots[Index];
	Lots.RemoveAt(Index);
	BidderMaxCents.Remove(LotId);
	RivalNames.Remove(LotId);

	bool bWon = false;
	FString Text;
	if (!Lot.bCompanyLeads)
	{
		Text = Lot.NumBids > 0 ? FString::Printf(TEXT("%s: an %s für %s"), *Lot.Title.ToString(), *Lot.LeaderName, *Lot.CurrentBid.ToDisplayString())
			: FString::Printf(TEXT("%s: ohne Gebot beendet"), *Lot.Title.ToString());
	}
	else if (UWECompanyComponent* Company = UWECompanyComponent::Get(this);
		Company && Company->TryWithdraw(Lot.CurrentBid, FString::Printf(TEXT("Auktion gewonnen: %s"), *Lot.Title.ToString()), Lot.LeaderName))
	{
		bWon = true;
		AWEDeliveryZone* Zone = AWEDeliveryZone::Find(GetWorld());
		for (const FWEItemInstance& Item : Lot.Items)
		{
			// Non-stackable stacks become single items.
			const int32 Count = Item.Definition && Item.Definition->GetMaxStackSize() > 1 ? 1 : Item.Quantity;
			for (int32 i = 0; i < Count; ++i)
			{
				FWEItemInstance One = FWEItemInstance::Make(Item.Definition, Count > 1 ? 1 : Item.Quantity, Item.Condition, Item.Quality);
				const FTransform Drop = Zone ? Zone->NextDropTransform() : FTransform(FVector(0.f, 0.f, 120.f));
				AWEPickupItem::SpawnWorldItem(GetWorld(), One, Drop);
			}
		}
		Text = FString::Printf(TEXT("Gewonnen: %s für %s – steht auf der Lieferpalette"), *Lot.Title.ToString(), *Lot.CurrentBid.ToDisplayString());
	}
	else
	{
		Text = FString::Printf(TEXT("%s: Zuschlag verfallen – zu wenig Geld auf dem Firmenkonto"), *Lot.Title.ToString());
	}
	AddResult(Text);
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Auction finished: %s"), *Text);

	// Tell every player (company-wide event).
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(It->Get()); PC && (Lot.bCompanyLeads || Lot.NumBids > 0))
		{
			PC->ClientShowMessage(FText::FromString(TEXT("Auktion – ") + Text));
		}
	}
	OnRep_Lots();
	return bWon;
}

bool UWEAuctionComponent::AddLot(int32 TemplateIndex)
{
	if (!GetOwner()->HasAuthority() || !Catalog || Catalog->Templates.Num() == 0)
	{
		return false;
	}
	if (TemplateIndex == INDEX_NONE)
	{
		// Weighted, but not the same kind of lot twice at the same time.
		auto WeightOf = [this](const FWEAuctionTemplate& T)
		{
			const bool bRunning = Lots.ContainsByPredicate([&T](const FWEAuctionLot& L) { return L.Title.EqualTo(T.Title); });
			return bRunning ? 0.f : FMath::Max(0.f, T.Weight);
		};
		float Total = 0.f;
		for (const FWEAuctionTemplate& T : Catalog->Templates)
		{
			Total += WeightOf(T);
		}
		if (Total <= 0.f)
		{
			return false;
		}
		float Pick = Random.FRand() * Total;
		for (int32 i = 0; i < Catalog->Templates.Num(); ++i)
		{
			const float Weight = WeightOf(Catalog->Templates[i]);
			if (Weight > 0.f)
			{
				TemplateIndex = i;
				if ((Pick -= Weight) <= 0.f)
				{
					break;
				}
			}
		}
	}
	if (!Catalog->Templates.IsValidIndex(TemplateIndex))
	{
		return false;
	}
	const FWEAuctionTemplate& Template = Catalog->Templates[TemplateIndex];
	const UWEMarketComponent* Market = UWEMarketComponent::Get(this);

	FWEAuctionLot Lot;
	Lot.Id = FGuid::NewGuid();
	Lot.Title = Template.Title;
	int64 Value = 0;
	for (const FWEAuctionLotItem& Entry : Template.Items)
	{
		if (Entry.Item)
		{
			const FWEItemInstance Item = FWEItemInstance::Make(Entry.Item, Entry.Quantity, Entry.Condition, Entry.Quality);
			Lot.Items.Add(Item);
			Value += UWEMarketComponent::GetMarketValue(Item, Market).GetCents();
		}
	}
	if (Lot.Items.Num() == 0)
	{
		return false;
	}
	Lot.EstimatedValue = FWEMoney::FromCents(Value);
	Lot.CurrentBid = FWEMoney::FromCents(FMath::Max<int64>(100, ((FMath::RoundToInt64(Value * Template.StartFactor) + 49) / 50) * 50));
	Lot.EndsAt = GetServerTime() + Template.DurationMinutes * 60.f;
	const float LimitFactor = Random.FRandRange(FMath::Min(Template.BidderMinFactor, Template.BidderMaxFactor), FMath::Max(Template.BidderMinFactor, Template.BidderMaxFactor));
	BidderMaxCents.Add(Lot.Id, FMath::Max(Lot.CurrentBid.GetCents(), FMath::RoundToInt64(Value * LimitFactor)));
	Lots.Add(Lot);
	OnRep_Lots();
	return true;
}

void UWEAuctionComponent::UpdateAuctions()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	const float Now = GetServerTime();
	TArray<FGuid> Due;
	for (const FWEAuctionLot& Lot : Lots)
	{
		if (Lot.EndsAt <= Now)
		{
			Due.Add(Lot.Id);
		}
	}
	for (const FGuid& Id : Due)
	{
		FinishLot(Id);
	}
	while (Lots.Num() < MaxLots && AddLot())
	{
	}
}

void UWEAuctionComponent::SetLots(const TArray<FWEAuctionLot>& InLots, const TArray<int64>& InBidderMaxCents)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	Lots = InLots;
	BidderMaxCents.Reset();
	RivalNames.Reset();
	for (int32 i = 0; i < Lots.Num(); ++i)
	{
		BidderMaxCents.Add(Lots[i].Id, InBidderMaxCents.IsValidIndex(i) ? InBidderMaxCents[i] : Lots[i].CurrentBid.GetCents());
		if (!Lots[i].bCompanyLeads && !Lots[i].LeaderName.IsEmpty())
		{
			RivalNames.Add(Lots[i].Id, Lots[i].LeaderName);
		}
	}
	OnRep_Lots();
}

int64 UWEAuctionComponent::GetBidderMaxCents(const FGuid& LotId) const
{
	const int64* Found = BidderMaxCents.Find(LotId);
	return Found ? *Found : 0;
}

void UWEAuctionComponent::SetBidderMaxCents(const FGuid& LotId, int64 Cents)
{
	BidderMaxCents.Add(LotId, Cents);
}

#undef LOCTEXT_NAMESPACE
