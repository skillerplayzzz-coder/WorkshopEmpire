#include "Economy/WETraderTerminal.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WEMarketComponent.h"
#include "Economy/WETraderDefinition.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Framework/WEPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Interaction/WEInteractionComponent.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Net/UnrealNetwork.h"
#include "Player/WECharacter.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "WELog.h"
#include "World/WEPlaceholderVisuals.h"

#define LOCTEXT_NAMESPACE "WETraderTerminal"

namespace WETerminal
{
	void AddBox(UInstancedStaticMeshComponent* ISM, const FVector& Center, const FVector& Size)
	{
		ISM->AddInstance(FTransform(FRotator::ZeroRotator, Center, Size / 100.f));
	}

	FString Describe(const FWEItemInstance& Item)
	{
		return Item.Quantity > 1
			? FString::Printf(TEXT("%d× %s"), Item.Quantity, *Item.GetDisplayName().ToString())
			: Item.GetDisplayName().ToString();
	}
}

AWETraderTerminal::AWETraderTerminal()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(5.f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlaceholderMat(WEPlaceholder::MaterialPath);
	PlaceholderMaterial = PlaceholderMat.Object;

	Body = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Body"));
	Body->SetMobility(EComponentMobility::Static);
	Body->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RootComponent = Body;
	Screen = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Screen"));
	Screen->SetupAttachment(Body);
	Screen->SetMobility(EComponentMobility::Static);
	Screen->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	if (CubeMesh.Succeeded())
	{
		Body->SetStaticMesh(CubeMesh.Object);
		Screen->SetStaticMesh(CubeMesh.Object);
	}
}

void AWETraderTerminal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWETraderTerminal, StockRemaining);
}

void AWETraderTerminal::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Small desk with a monitor; front faces +X.
	Body->ClearInstances();
	Screen->ClearInstances();
	WETerminal::AddBox(Body, FVector(0.f, 0.f, 73.f), FVector(60.f, 110.f, 4.f));
	for (const float SY : { -1.f, 1.f })
	{
		WETerminal::AddBox(Body, FVector(0.f, SY * 50.f, 35.5f), FVector(55.f, 5.f, 71.f));
	}
	WETerminal::AddBox(Screen, FVector(-12.f, 0.f, 105.f), FVector(4.f, 55.f, 34.f));
	WETerminal::AddBox(Screen, FVector(-14.f, 0.f, 80.f), FVector(10.f, 10.f, 10.f));
	WEPlaceholder::ApplyTint(Body, PlaceholderMaterial, FLinearColor(0.25f, 0.27f, 0.30f));
	WEPlaceholder::ApplyTint(Screen, PlaceholderMaterial, FLinearColor(0.05f, 0.25f, 0.55f));
}

void AWETraderTerminal::BeginPlay()
{
	Super::BeginPlay();
	WEPlaceholder::ApplyTint(Body, PlaceholderMaterial, FLinearColor(0.25f, 0.27f, 0.30f));
	WEPlaceholder::ApplyTint(Screen, PlaceholderMaterial, FLinearColor(0.05f, 0.25f, 0.55f));

	if (HasAuthority())
	{
		if (StockRemaining.Num() != NumOffers())
		{
			Restock();
		}
		float Minutes = 0.f;
		for (const UWETraderDefinition* Trader : Traders)
		{
			if (Trader && Trader->RestockMinutes > 0.f)
			{
				Minutes = Minutes > 0.f ? FMath::Min(Minutes, Trader->RestockMinutes) : Trader->RestockMinutes;
			}
		}
		if (Minutes > 0.f)
		{
			GetWorldTimerManager().SetTimer(RestockTimer, this, &AWETraderTerminal::Restock, Minutes * 60.f, true);
		}
	}
}

void AWETraderTerminal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RestockTimer);
	Super::EndPlay(EndPlayReason);
}

// --- Interaction ---------------------------------------------------------------------------------

FText AWETraderTerminal::GetInteractionText(const AWECharacter* Interactor) const
{
	return LOCTEXT("Open", "Online-Marktplatz öffnen");
}

bool AWETraderTerminal::CanInteract(const AWECharacter* Interactor, FText& OutReason) const
{
	return Interactor != nullptr;
}

void AWETraderTerminal::Interact(AWECharacter* Interactor)
{
	if (AWEPlayerController* PC = Interactor ? Cast<AWEPlayerController>(Interactor->GetController()) : nullptr)
	{
		PC->ClientOpenTrade(this);
	}
}

bool AWETraderTerminal::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	OutInfo.Title = LOCTEXT("Title", "Online-Marktplatz");
	OutInfo.Description = LOCTEXT("Desc", "Ein alter Rechner mit Internetanschluss. Hier wird gekauft, verkauft und es kommen Kundenaufträge herein.");
	for (const UWETraderDefinition* Trader : Traders)
	{
		if (Trader)
		{
			OutInfo.Details.Add(Trader->DisplayName);
		}
	}
	return true;
}

bool AWETraderTerminal::IsWithinUseRange(const AWECharacter* Character) const
{
	return UWEInteractionComponent::IsWithinReach(Character, this, UseRange);
}

// --- Offers --------------------------------------------------------------------------------------

int32 AWETraderTerminal::NumOffers() const
{
	int32 Count = 0;
	for (const UWETraderDefinition* Trader : Traders)
	{
		Count += Trader ? Trader->Stock.Num() : 0;
	}
	return Count;
}

bool AWETraderTerminal::ResolveOffer(int32 OfferIndex, const UWETraderDefinition*& OutTrader, const FWETraderStockEntry*& OutEntry) const
{
	int32 Index = OfferIndex;
	for (const UWETraderDefinition* Trader : Traders)
	{
		if (!Trader)
		{
			continue;
		}
		if (Index < Trader->Stock.Num())
		{
			OutTrader = Trader;
			OutEntry = &Trader->Stock[Index];
			return Index >= 0 && OutEntry->Item != nullptr;
		}
		Index -= Trader->Stock.Num();
	}
	return false;
}

TArray<FWEMarketOffer> AWETraderTerminal::GetOffers() const
{
	TArray<FWEMarketOffer> Offers;
	const UWEMarketComponent* Market = UWEMarketComponent::Get(this);
	for (int32 i = 0; i < NumOffers(); ++i)
	{
		const UWETraderDefinition* Trader = nullptr;
		const FWETraderStockEntry* Entry = nullptr;
		if (!ResolveOffer(i, Trader, Entry))
		{
			continue;
		}
		FWEMarketOffer& Offer = Offers.AddDefaulted_GetRef();
		Offer.OfferIndex = i;
		Offer.TraderName = Trader->DisplayName;
		Offer.Item = FWEItemInstance::Make(Entry->Item, 1, Entry->Condition, Entry->Quality);
		Offer.Remaining = StockRemaining.IsValidIndex(i) ? StockRemaining[i] : 0;
		Offer.UnitPrice = UWEMarketComponent::GetBuyPrice(Offer.Item, Entry->PriceMultiplier, Market);
	}
	return Offers;
}

FWEMoney AWETraderTerminal::GetBestSellPrice(const FWEItemInstance& Item, const UWETraderDefinition** OutTrader) const
{
	const UWEMarketComponent* Market = UWEMarketComponent::Get(this);
	FWEMoney Best;
	const UWETraderDefinition* BestTrader = nullptr;
	for (const UWETraderDefinition* Trader : Traders)
	{
		const FWEMoney Price = UWEMarketComponent::GetSellPrice(Item, Trader, Market);
		if (Price > Best)
		{
			Best = Price;
			BestTrader = Trader;
		}
	}
	if (OutTrader)
	{
		*OutTrader = BestTrader;
	}
	return Best;
}

void AWETraderTerminal::Restock()
{
	if (!HasAuthority())
	{
		return;
	}
	StockRemaining.SetNum(NumOffers());
	for (int32 i = 0; i < StockRemaining.Num(); ++i)
	{
		const UWETraderDefinition* Trader = nullptr;
		const FWETraderStockEntry* Entry = nullptr;
		StockRemaining[i] = ResolveOffer(i, Trader, Entry) ? Entry->Quantity : 0;
	}
	ForceNetUpdate();
}

void AWETraderTerminal::SetStockRemaining(const TArray<int32>& Values)
{
	if (HasAuthority() && Values.Num() == NumOffers())
	{
		StockRemaining = Values;
		ForceNetUpdate();
	}
}

// --- Trades (server) -----------------------------------------------------------------------------

FString AWETraderTerminal::PlayerNameOf(const AWECharacter* Character) const
{
	const APlayerState* PS = Character ? Character->GetPlayerState() : nullptr;
	return PS ? PS->GetPlayerName() : FString();
}

void AWETraderTerminal::Notify(AWECharacter* Character, const FText& Message) const
{
	if (AWEPlayerController* PC = Character ? Cast<AWEPlayerController>(Character->GetController()) : nullptr)
	{
		PC->ClientShowMessage(Message);
	}
}

FText AWETraderTerminal::ResultText(EWETradeResult Result)
{
	switch (Result)
	{
	case EWETradeResult::Success:        return LOCTEXT("ResOk", "Erledigt");
	case EWETradeResult::Duplicate:      return LOCTEXT("ResDup", "Anfrage wurde bereits bearbeitet");
	case EWETradeResult::OutOfReach:     return LOCTEXT("ResReach", "Zu weit vom Terminal entfernt");
	case EWETradeResult::InvalidItem:    return LOCTEXT("ResItem", "Gegenstand nicht verfügbar");
	case EWETradeResult::NoBuyer:        return LOCTEXT("ResNoBuyer", "Niemand kauft das");
	case EWETradeResult::NotEnoughMoney: return LOCTEXT("ResMoney", "Nicht genug Geld auf dem Firmenkonto");
	case EWETradeResult::OutOfStock:     return LOCTEXT("ResStock", "Nicht mehr vorrätig");
	case EWETradeResult::ContractFailed: return LOCTEXT("ResContract", "Auftrag nicht möglich");
	}
	return FText::GetEmpty();
}

EWETradeResult AWETraderTerminal::SellFromInventory(AWECharacter* Character, UWEInventoryComponent* Inventory, int32 Slot, int32 Quantity, const FGuid& RequestId)
{
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!HasAuthority() || !Company || !Character)
	{
		return EWETradeResult::InvalidItem;
	}
	if (!IsWithinUseRange(Character))
	{
		return EWETradeResult::OutOfReach;
	}
	const FWEItemInstance* Item = Inventory && Inventory->CanBeAccessedBy(Character) ? Inventory->GetItemInSlot(Slot) : nullptr;
	if (!Item)
	{
		return EWETradeResult::InvalidItem;
	}
	FWEItemInstance Selling = *Item;
	Selling.Quantity = Quantity <= 0 ? Item->Quantity : FMath::Min(Quantity, Item->Quantity);
	const UWETraderDefinition* Trader = nullptr;
	const FWEMoney Price = GetBestSellPrice(Selling, &Trader);
	if (!Trader || Price.IsZero())
	{
		Notify(Character, ResultText(EWETradeResult::NoBuyer));
		return EWETradeResult::NoBuyer;
	}
	if (!Company->RegisterRequest(RequestId))
	{
		return EWETradeResult::Duplicate;
	}

	// Goods first, then money: if removing fails, nothing is paid.
	FWEItemInstance Removed;
	if (Inventory->RemoveFromSlot(Slot, Selling.Quantity, Removed) != EWEInventoryResult::Success)
	{
		return EWETradeResult::InvalidItem;
	}
	const FString What = WETerminal::Describe(Removed);
	Company->Deposit(Price, FString::Printf(TEXT("Verkauf: %s an %s"), *What, *Trader->DisplayName.ToString()), PlayerNameOf(Character));
	Notify(Character, FText::FromString(FString::Printf(TEXT("+%s  für %s"), *Price.ToDisplayString(), *What)));
	UE_LOG(LogWENet, Log, TEXT("%s sold %s for %s"), *Character->GetName(), *What, *Price.ToDisplayString());
	return EWETradeResult::Success;
}

EWETradeResult AWETraderTerminal::SellHeldItem(AWECharacter* Character, const FGuid& RequestId)
{
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	AWEPickupItem* Held = Character ? Character->GetHeldItem() : nullptr;
	if (!HasAuthority() || !Company || !Held)
	{
		return EWETradeResult::InvalidItem;
	}
	if (!IsWithinUseRange(Character))
	{
		return EWETradeResult::OutOfReach;
	}
	const FWEItemInstance Item = Held->GetItem();
	const UWETraderDefinition* Trader = nullptr;
	const FWEMoney Price = GetBestSellPrice(Item, &Trader);
	if (!Trader || Price.IsZero())
	{
		Notify(Character, ResultText(EWETradeResult::NoBuyer));
		return EWETradeResult::NoBuyer;
	}
	if (!Company->RegisterRequest(RequestId))
	{
		return EWETradeResult::Duplicate;
	}
	Character->ReleaseHeldItem();
	Held->Destroy();
	const FString What = WETerminal::Describe(Item);
	Company->Deposit(Price, FString::Printf(TEXT("Verkauf: %s an %s"), *What, *Trader->DisplayName.ToString()), PlayerNameOf(Character));
	Notify(Character, FText::FromString(FString::Printf(TEXT("+%s  für %s"), *Price.ToDisplayString(), *What)));
	UE_LOG(LogWENet, Log, TEXT("%s sold carried %s for %s"), *Character->GetName(), *What, *Price.ToDisplayString());
	return EWETradeResult::Success;
}

void AWETraderTerminal::DeliverPurchase(AWECharacter* Character, const FWEItemInstance& Item)
{
	FWEItemInstance Rest = Item;
	if (UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr; Inventory && Item.Definition->bFitsInInventory)
	{
		int32 Added = 0;
		Inventory->TryAddItem(Item, Added);
		Rest.Quantity -= Added;
	}
	if (Rest.Quantity <= 0)
	{
		return;
	}
	FTransform Drop = GetActorTransform();
	Drop.SetLocation(GetActorLocation() + GetActorForwardVector() * 80.f + FVector(0.f, 0.f, 40.f));
	if (AWEDeliveryZone* Zone = AWEDeliveryZone::Find(GetWorld()))
	{
		Drop = Zone->NextDropTransform();
	}
	AWEPickupItem::SpawnWorldItem(GetWorld(), Rest, Drop);
}

EWETradeResult AWETraderTerminal::Buy(AWECharacter* Character, int32 OfferIndex, int32 Quantity, const FGuid& RequestId)
{
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!HasAuthority() || !Company || !Character)
	{
		return EWETradeResult::InvalidItem;
	}
	if (!IsWithinUseRange(Character))
	{
		return EWETradeResult::OutOfReach;
	}
	const UWETraderDefinition* Trader = nullptr;
	const FWETraderStockEntry* Entry = nullptr;
	if (!ResolveOffer(OfferIndex, Trader, Entry))
	{
		return EWETradeResult::InvalidItem;
	}
	Quantity = FMath::Max(1, Quantity);
	if (!StockRemaining.IsValidIndex(OfferIndex) || StockRemaining[OfferIndex] < Quantity)
	{
		Notify(Character, ResultText(EWETradeResult::OutOfStock));
		return EWETradeResult::OutOfStock;
	}
	const FWEItemInstance Bought = FWEItemInstance::Make(Entry->Item, Quantity, Entry->Condition, Entry->Quality);
	const FWEMoney Price = UWEMarketComponent::GetBuyPrice(Bought, Entry->PriceMultiplier, UWEMarketComponent::Get(this));
	if (!Company->CanAfford(Price))
	{
		Notify(Character, ResultText(EWETradeResult::NotEnoughMoney));
		return EWETradeResult::NotEnoughMoney;
	}
	if (!Company->RegisterRequest(RequestId))
	{
		return EWETradeResult::Duplicate;
	}

	// Money and stock first (both checked above), then hand over the goods.
	const FString What = WETerminal::Describe(Bought);
	if (!Company->TryWithdraw(Price, FString::Printf(TEXT("Einkauf: %s bei %s"), *What, *Trader->DisplayName.ToString()), PlayerNameOf(Character)))
	{
		return EWETradeResult::NotEnoughMoney;
	}
	StockRemaining[OfferIndex] -= Quantity;
	ForceNetUpdate();

	// Non-stackable items become separate instances.
	if (Entry->Item->GetMaxStackSize() > 1)
	{
		DeliverPurchase(Character, Bought);
	}
	else
	{
		for (int32 i = 0; i < Quantity; ++i)
		{
			DeliverPurchase(Character, FWEItemInstance::Make(Entry->Item, 1, Entry->Condition, Entry->Quality));
		}
	}
	const bool bToPallet = !Entry->Item->bFitsInInventory;
	Notify(Character, FText::FromString(FString::Printf(TEXT("-%s  %s gekauft%s"), *Price.ToDisplayString(), *What,
		bToPallet ? TEXT(" (steht auf der Lieferpalette)") : TEXT(""))));
	UE_LOG(LogWENet, Log, TEXT("%s bought %s for %s"), *Character->GetName(), *What, *Price.ToDisplayString());
	return EWETradeResult::Success;
}

EWETradeResult AWETraderTerminal::AcceptContract(AWECharacter* Character, const FGuid& ContractId, const FGuid& RequestId)
{
	UWEContractComponent* Contracts = UWEContractComponent::Get(this);
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!HasAuthority() || !Contracts || !Company || !Character)
	{
		return EWETradeResult::ContractFailed;
	}
	if (!IsWithinUseRange(Character))
	{
		return EWETradeResult::OutOfReach;
	}
	if (!Company->RegisterRequest(RequestId))
	{
		return EWETradeResult::Duplicate;
	}
	const EWEContractResult Result = Contracts->Accept(ContractId, PlayerNameOf(Character));
	if (Result != EWEContractResult::Success)
	{
		Notify(Character, Result == EWEContractResult::TooManyAccepted
			? LOCTEXT("TooMany", "Es laufen schon genug Aufträge")
			: ResultText(EWETradeResult::ContractFailed));
		return EWETradeResult::ContractFailed;
	}
	Notify(Character, LOCTEXT("Accepted", "Auftrag angenommen"));
	return EWETradeResult::Success;
}

EWETradeResult AWETraderTerminal::DeliverContract(AWECharacter* Character, const FGuid& ContractId, const FGuid& RequestId)
{
	UWEContractComponent* Contracts = UWEContractComponent::Get(this);
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!HasAuthority() || !Contracts || !Company || !Character)
	{
		return EWETradeResult::ContractFailed;
	}
	if (!IsWithinUseRange(Character))
	{
		return EWETradeResult::OutOfReach;
	}
	const FWEContract* Contract = Contracts->FindContract(ContractId);
	const FWEMoney Reward = Contract ? Contract->Reward : FWEMoney();
	if (!Company->RegisterRequest(RequestId))
	{
		return EWETradeResult::Duplicate;
	}
	const EWEContractResult Result = Contracts->Deliver(ContractId, Character);
	if (Result != EWEContractResult::Success)
	{
		Notify(Character, Result == EWEContractResult::MissingItems
			? LOCTEXT("Missing", "Passende Gegenstände fehlen im Inventar")
			: ResultText(EWETradeResult::ContractFailed));
		return EWETradeResult::ContractFailed;
	}
	Notify(Character, FText::Format(LOCTEXT("Delivered", "Auftrag erledigt: +{0}"), FText::FromString(Reward.ToDisplayString())));
	return EWETradeResult::Success;
}

// --- Delivery zone ------------------------------------------------------------------------------

AWEDeliveryZone::AWEDeliveryZone()
{
	PrimaryActorTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlaceholderMat(WEPlaceholder::MaterialPath);
	PlaceholderMaterial = PlaceholderMat.Object;

	Pallet = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pallet"));
	Pallet->SetMobility(EComponentMobility::Static);
	Pallet->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Pallet->SetRelativeScale3D(FVector(1.2f, 0.8f, 0.12f)); // place the actor 6 cm above the floor
	RootComponent = Pallet;
	if (CubeMesh.Succeeded())
	{
		Pallet->SetStaticMesh(CubeMesh.Object);
	}
}

void AWEDeliveryZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	WEPlaceholder::ApplyTint(Pallet, PlaceholderMaterial, FLinearColor(0.55f, 0.42f, 0.25f));
}

void AWEDeliveryZone::BeginPlay()
{
	Super::BeginPlay();
	WEPlaceholder::ApplyTint(Pallet, PlaceholderMaterial, FLinearColor(0.55f, 0.42f, 0.25f));
}

FTransform AWEDeliveryZone::NextDropTransform()
{
	// 2x2 spots on the pallet, items are dropped from a little above and settle by physics.
	const int32 Spot = NextIndex++ % 4;
	const FVector Local((Spot % 2 == 0 ? -30.f : 30.f), (Spot < 2 ? -20.f : 20.f), 60.f + (NextIndex / 4 % 3) * 40.f);
	// Rotation + location only: the root (pallet) carries a non-uniform scale.
	return FTransform(GetActorRotation(), GetActorLocation() + GetActorRotation().RotateVector(Local));
}

AWEDeliveryZone* AWEDeliveryZone::Find(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<AWEDeliveryZone> It(const_cast<UWorld*>(World));
	return It ? *It : nullptr;
}

#undef LOCTEXT_NAMESPACE
