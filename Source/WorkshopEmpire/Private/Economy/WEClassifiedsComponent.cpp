#include "Economy/WEClassifiedsComponent.h"

#include "Economy/WECompanyComponent.h"
#include "Economy/WEMarketComponent.h"
#include "Economy/WETraderTerminal.h"
#include "Engine/World.h"
#include "Framework/WEGameState.h"
#include "GameFramework/GameStateBase.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "WELog.h"

namespace WEClassifieds
{
	/** What private sellers offer: id, chance weight, worst / best condition index. */
	struct FPoolEntry { const TCHAR* Id; int32 Weight; int32 BestCondition; int32 WorstCondition; };
	const FPoolEntry Pool[] = {
		{ TEXT("OldComputer"), 6, 1, 4 }, { TEXT("CrtMonitor"), 2, 2, 4 }, { TEXT("GraphicsCard"), 5, 1, 4 }, { TEXT("GraphicsCardPro"), 4, 0, 3 },
		{ TEXT("RamModule"), 5, 1, 3 }, { TEXT("RamModule8"), 4, 0, 3 }, { TEXT("Processor"), 4, 1, 3 }, { TEXT("ProcessorFast"), 3, 0, 3 },
		{ TEXT("PowerSupply"), 4, 1, 4 }, { TEXT("Mainboard"), 3, 1, 4 }, { TEXT("SsdDrive"), 3, 1, 3 }, { TEXT("SsdFast"), 3, 0, 2 },
		{ TEXT("HardDrive"), 2, 1, 3 }, { TEXT("CpuCooler"), 2, 1, 3 }, { TEXT("CordlessDrill"), 2, 0, 3 }, { TEXT("BitRatchet"), 2, 0, 2 },
		{ TEXT("DigitalMultimeter"), 1, 0, 2 }, { TEXT("VintageRadio"), 1, 2, 4 }, { TEXT("RareGpu"), 1, 2, 4 },
	};
	const TCHAR* FirstNames[] = { TEXT("Jens"), TEXT("Sabine"), TEXT("Murat"), TEXT("Petra"), TEXT("Dieter"), TEXT("Lena"), TEXT("Kevin"),
		TEXT("Monika"), TEXT("Stefan"), TEXT("Aylin"), TEXT("Uwe"), TEXT("Jana"), TEXT("Holger"), TEXT("Svenja"), TEXT("Rolf") };
	const TCHAR* Places[] = { TEXT("Nordstadt"), TEXT("Hafenviertel"), TEXT("Altstadt"), TEXT("Gewerbegebiet Süd"), TEXT("Neubausiedlung"),
		TEXT("am Bahnhof"), TEXT("Oststadt") };
	const TCHAR* Notes[] = {
		TEXT("Lag lange im Keller, sollte noch gehen."), TEXT("Funktioniert, keine Garantie, Privatverkauf."), TEXT("Nur Abholung? Nein, ich schicke es auch."),
		TEXT("Aus Haushaltsauflösung, nicht getestet."), TEXT("Brauche den Platz, Preis ist verhandelbar."), TEXT("Wie neu, kaum benutzt."),
		TEXT("Festpreis! … na ja, fast."), TEXT("Von meinem Sohn, der hat jetzt was Neues.") };

	constexpr float AdLifetimeMin = 600.f, AdLifetimeMax = 1200.f;
	constexpr float DeliveryMin = 45.f, DeliveryMax = 120.f;
	constexpr float BuyerMin = 25.f, BuyerMax = 90.f;
}

UWEClassifiedsComponent::UWEClassifiedsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Random.Initialize(FMath::Rand());
}

void UWEClassifiedsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWEClassifiedsComponent, Ads);
	DOREPLIFETIME(UWEClassifiedsComponent, Deliveries);
	DOREPLIFETIME(UWEClassifiedsComponent, Listings);
}

void UWEClassifiedsComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority())
	{
		Update();
		GetWorld()->GetTimerManager().SetTimer(UpdateTimer, this, &UWEClassifiedsComponent::Update, 3.f, true);
	}
}

void UWEClassifiedsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimer);
	}
	Super::EndPlay(EndPlayReason);
}

UWEClassifiedsComponent* UWEClassifiedsComponent::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AWEGameState* GameState = World ? World->GetGameState<AWEGameState>() : nullptr;
	return GameState ? GameState->GetClassifieds() : nullptr;
}

float UWEClassifiedsComponent::GetServerTime() const
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

void UWEClassifiedsComponent::OnRep_Lists()
{
	OnChanged.Broadcast();
}

void UWEClassifiedsComponent::Changed()
{
	OnRep_Lists();
}

bool UWEClassifiedsComponent::AddAd(const FWEItemInstance* Fixed, int64 AskingCents, int64 LowestCents)
{
	if (!GetOwner()->HasAuthority())
	{
		return false;
	}
	using namespace WEClassifieds;
	FWEClassified Ad;
	Ad.Id = FGuid::NewGuid();
	if (Fixed)
	{
		Ad.Item = *Fixed;
	}
	else
	{
		int32 Total = 0;
		for (const FPoolEntry& E : Pool)
		{
			Total += UWEItemDefinition::FindById(E.Id) ? E.Weight : 0;
		}
		int32 Pick = Random.RandRange(1, FMath::Max(1, Total));
		for (const FPoolEntry& E : Pool)
		{
			UWEItemDefinition* Def = UWEItemDefinition::FindById(E.Id);
			if (!Def || (Pick -= E.Weight) > 0)
			{
				continue;
			}
			const EWEItemCondition Condition = static_cast<EWEItemCondition>(Random.RandRange(E.BestCondition, E.WorstCondition));
			Ad.Item = FWEItemInstance::Make(Def, 1, Condition, static_cast<uint8>(Random.RandRange(25, 85)));
			break;
		}
	}
	if (!Ad.Item.IsValid())
	{
		return false;
	}
	// Private sellers want a bit more than it is worth (some much more); the lowest price is often below the value.
	const int64 Value = FMath::Max<int64>(100, UWEMarketComponent::GetMarketValue(Ad.Item, UWEMarketComponent::Get(this)).GetCents());
	const int64 Asking = AskingCents > 0 ? AskingCents : FMath::RoundToInt64(Value * Random.FRandRange(0.85f, 1.35f) / 50.0) * 50;
	const int64 Lowest = LowestCents > 0 ? LowestCents : FMath::Max<int64>(50, FMath::RoundToInt64(Asking * Random.FRandRange(0.6f, 0.88f)));
	Ad.Asking = FWEMoney::FromCents(FMath::Max<int64>(Asking, 50));
	Ad.Lowest = FWEMoney::FromCents(FMath::Min(Lowest, Ad.Asking.GetCents()));
	Ad.Seller = FString::Printf(TEXT("%s, %s"), FirstNames[Random.RandRange(0, UE_ARRAY_COUNT(FirstNames) - 1)], Places[Random.RandRange(0, UE_ARRAY_COUNT(Places) - 1)]);
	Ad.Text = Notes[Random.RandRange(0, UE_ARRAY_COUNT(Notes) - 1)];
	Ad.ExpiresAt = GetServerTime() + Random.FRandRange(AdLifetimeMin, AdLifetimeMax);
	Ads.Add(Ad);
	Changed();
	return true;
}

void UWEClassifiedsComponent::Update()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	const float Now = GetServerTime();
	bool bChanged = false;
	bChanged |= Ads.RemoveAll([Now](const FWEClassified& A) { return A.ExpiresAt <= Now; }) > 0;
	for (int32 i = Deliveries.Num() - 1; i >= 0; --i)
	{
		if (Deliveries[i].ArrivesAt <= Now)
		{
			Deliver(Deliveries[i].Item);
			Deliveries.RemoveAt(i);
			bChanged = true;
		}
	}
	for (FWEOwnListing& Listing : Listings)
	{
		if (Listing.NextBuyerAt <= Now)
		{
			BuyerTurn(Listing, Now);
			bChanged = true;
		}
	}
	while (Ads.Num() < MaxAds && AddAd())
	{
		bChanged = true;
	}
	if (bChanged)
	{
		Changed();
	}
}

void UWEClassifiedsComponent::BuyerTurn(FWEOwnListing& Listing, float Now)
{
	using namespace WEClassifieds;
	// Buyers come quicker and offer more the fairer the price is (compared with the market value).
	const int64 Value = FMath::Max<int64>(100, UWEMarketComponent::GetMarketValue(Listing.Item, UWEMarketComponent::Get(this)).GetCents());
	const float Ratio = Listing.Price.GetCents() / static_cast<float>(Value);
	Listing.NextBuyerAt = Now + Random.FRandRange(BuyerMin, BuyerMax) * FMath::Clamp(Ratio, 0.6f, 2.5f);
	if (Ratio > 1.6f && Random.FRand() < 0.7f)
	{
		return; // too expensive: nobody bites this time
	}
	const float Share = Ratio <= 0.9f ? 1.f : Random.FRandRange(FMath::Clamp(1.25f - Ratio * 0.35f, 0.55f, 0.95f), 1.f);
	const int64 Offer = FMath::RoundToInt64(Listing.Price.GetCents() * Share / 50.0) * 50;
	if (Offer > Listing.Offer.GetCents())
	{
		Listing.Offer = FWEMoney::FromCents(FMath::Max<int64>(50, Offer));
		Listing.Buyer = FString::Printf(TEXT("%s aus der %s"), FirstNames[Random.RandRange(0, UE_ARRAY_COUNT(FirstNames) - 1)], Places[Random.RandRange(0, UE_ARRAY_COUNT(Places) - 1)]);
	}
}

void UWEClassifiedsComponent::Deliver(const FWEItemInstance& Item)
{
	FTransform Drop(FVector(0.f, 0.f, 100.f));
	if (AWEDeliveryZone* Zone = AWEDeliveryZone::Find(GetWorld()))
	{
		Drop = Zone->NextDropTransform();
	}
	AWEPickupItem::SpawnWorldItem(GetWorld(), Item, Drop);
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Classifieds: %s delivered"), *Item.GetDisplayName().ToString());
}

EWEClassifiedResult UWEClassifiedsComponent::MakeOffer(const FGuid& Id, int64 Cents, const FString& PlayerName)
{
	FWEClassified* Ad = Ads.FindByPredicate([&Id](const FWEClassified& A) { return A.Id == Id; });
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!GetOwner()->HasAuthority() || !Ad || !Company)
	{
		return EWEClassifiedResult::NotFound;
	}
	if (Ad->Offers >= MaxOffersPerAd)
	{
		return EWEClassifiedResult::Ignored;
	}
	Cents = FMath::Min(Cents, Ad->Asking.GetCents());
	if (Cents >= Ad->Lowest.GetCents() || (!Ad->Counter.IsZero() && Cents >= Ad->Counter.GetCents()))
	{
		const FWEMoney Price = FWEMoney::FromCents(Cents);
		if (!Company->TryWithdraw(Price, FString::Printf(TEXT("Einkauf Kleinanzeige: %s"), *Ad->Item.GetDisplayName().ToString()), PlayerName))
		{
			return EWEClassifiedResult::NoMoney;
		}
		FWEClassifiedDelivery& D = Deliveries.AddDefaulted_GetRef();
		D.Item = Ad->Item;
		D.ArrivesAt = GetServerTime() + Random.FRandRange(WEClassifieds::DeliveryMin, WEClassifieds::DeliveryMax);
		Ads.RemoveAll([&Id](const FWEClassified& A) { return A.Id == Id; });
		Changed();
		return EWEClassifiedResult::Accepted;
	}
	++Ad->Offers;
	// Far too low: no answer. Otherwise the seller meets halfway (never below the lowest price).
	if (Cents < Ad->Lowest.GetCents() * 0.7)
	{
		Changed();
		return EWEClassifiedResult::Declined;
	}
	const int64 Middle = FMath::RoundToInt64((Cents + Ad->Asking.GetCents()) * 0.5 / 50.0) * 50;
	const int64 Counter = FMath::Clamp<int64>(Middle, Ad->Lowest.GetCents(), Ad->Counter.IsZero() ? Ad->Asking.GetCents() : Ad->Counter.GetCents());
	Ad->Counter = FWEMoney::FromCents(Counter);
	Changed();
	return EWEClassifiedResult::Countered;
}

EWEClassifiedResult UWEClassifiedsComponent::AcceptCounter(const FGuid& Id, const FString& PlayerName)
{
	const FWEClassified* Ad = Ads.FindByPredicate([&Id](const FWEClassified& A) { return A.Id == Id; });
	if (!Ad || Ad->Counter.IsZero())
	{
		return EWEClassifiedResult::NotFound;
	}
	return MakeOffer(Id, Ad->Counter.GetCents(), PlayerName);
}

EWEClassifiedResult UWEClassifiedsComponent::ListItem(UWEInventoryComponent* Inventory, int32 Slot, int64 PriceCents, const FString& PlayerName)
{
	const FWEItemInstance* Item = Inventory ? Inventory->GetItemInSlot(Slot) : nullptr;
	if (!GetOwner()->HasAuthority() || !Item)
	{
		return EWEClassifiedResult::NotFound;
	}
	if (Item->IsCustomerItem() || Item->IsSaleProtected() || Listings.Num() >= MaxListings || PriceCents <= 0)
	{
		return EWEClassifiedResult::NotAllowed;
	}
	FWEItemInstance Taken;
	if (Inventory->RemoveFromSlot(Slot, Item->Quantity, Taken) != EWEInventoryResult::Success)
	{
		return EWEClassifiedResult::NotFound;
	}
	FWEOwnListing& Listing = Listings.AddDefaulted_GetRef();
	Listing.Id = FGuid::NewGuid();
	Listing.Item = Taken;
	Listing.Price = FWEMoney::FromCents(PriceCents);
	Listing.NextBuyerAt = GetServerTime() + Random.FRandRange(10.f, 30.f);
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Classifieds: %s listed %s for %s"), *PlayerName, *Taken.GetDisplayName().ToString(), *Listing.Price.ToDisplayString());
	Changed();
	return EWEClassifiedResult::Accepted;
}

EWEClassifiedResult UWEClassifiedsComponent::AcceptBuyer(const FGuid& ListingId, const FString& PlayerName)
{
	const int32 Index = Listings.IndexOfByPredicate([&ListingId](const FWEOwnListing& L) { return L.Id == ListingId; });
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!GetOwner()->HasAuthority() || Index == INDEX_NONE || !Company || Listings[Index].Offer.IsZero())
	{
		return EWEClassifiedResult::NotFound;
	}
	const FWEOwnListing Sold = Listings[Index];
	Listings.RemoveAt(Index);
	Company->Deposit(Sold.Offer, FString::Printf(TEXT("Verkauf Kleinanzeige: %s an %s"), *Sold.Item.GetDisplayName().ToString(), *Sold.Buyer), PlayerName);
	Changed();
	return EWEClassifiedResult::Accepted;
}

EWEClassifiedResult UWEClassifiedsComponent::Withdraw(const FGuid& ListingId, UWEInventoryComponent* Inventory)
{
	const int32 Index = Listings.IndexOfByPredicate([&ListingId](const FWEOwnListing& L) { return L.Id == ListingId; });
	if (!GetOwner()->HasAuthority() || Index == INDEX_NONE)
	{
		return EWEClassifiedResult::NotFound;
	}
	FWEItemInstance Item = Listings[Index].Item;
	Listings.RemoveAt(Index);
	int32 Added = 0;
	if (Inventory)
	{
		Inventory->TryAddItem(Item, Added);
	}
	if (Added < Item.Quantity)
	{
		Item.Quantity -= Added;
		Deliver(Item);
	}
	Changed();
	return EWEClassifiedResult::Accepted;
}

void UWEClassifiedsComponent::SetState(const TArray<FWEClassifiedDelivery>& InDeliveries, const TArray<FWEOwnListing>& InListings)
{
	if (GetOwner()->HasAuthority())
	{
		Deliveries = InDeliveries;
		Listings = InListings;
		Changed();
	}
}

void UWEClassifiedsComponent::DeliverAllNow()
{
	for (FWEClassifiedDelivery& D : Deliveries)
	{
		D.ArrivesAt = -1.f;
	}
	Update();
}
