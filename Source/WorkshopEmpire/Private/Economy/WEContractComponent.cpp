#include "Economy/WEContractComponent.h"
#include "Progression/WETutorialComponent.h"
#include "Progression/WEMilestoneComponent.h"
#include "Progression/WEProgressionComponent.h"

#include "Economy/WECompanyComponent.h"
#include "Economy/WEMarketComponent.h"
#include "Economy/WETraderTerminal.h"
#include "EngineUtils.h"
#include "Items/WEPickupItem.h"
#include "Workshop/WEWorkbench.h"
#include "Engine/World.h"
#include "Framework/WEGameState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "Player/WECharacter.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "WELog.h"

UWEContractComponent::UWEContractComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	static ConstructorHelpers::FObjectFinder<UWEContractCatalog> DefaultCatalog(TEXT("/Game/WorkshopEmpire/Data/Economy/DA_ContractCatalog.DA_ContractCatalog"));
	Catalog = DefaultCatalog.Object;
}

void UWEContractComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWEContractComponent, Contracts);
}

void UWEContractComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority())
	{
		UpdateContracts();
		GetWorld()->GetTimerManager().SetTimer(UpdateTimer, this, &UWEContractComponent::UpdateContracts, 5.f, true);
	}
}

void UWEContractComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimer);
	}
	Super::EndPlay(EndPlayReason);
}

UWEContractComponent* UWEContractComponent::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AWEGameState* GameState = World ? World->GetGameState<AWEGameState>() : nullptr;
	return GameState ? GameState->GetContracts() : nullptr;
}

float UWEContractComponent::GetServerTime() const
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

void UWEContractComponent::OnRep_Contracts()
{
	OnContractsChanged.Broadcast();
}

const FWEContract* UWEContractComponent::FindContract(const FGuid& Id) const
{
	return Contracts.FindByPredicate([&Id](const FWEContract& C) { return C.Id == Id; });
}

bool UWEContractComponent::IsRepairedCustomerItem(const FWEContract& Contract, const FWEItemInstance& Item)
{
	return Contract.CustomerItemId.IsValid() && Item.InstanceId == Contract.CustomerItemId && !Item.IsBroken()
		&& Item.Condition <= Contract.MinCondition && Item.Repair.Faults == 0 && !Item.IsPartiallyDisassembled();
}

bool UWEContractComponent::Matches(const FWEContract& Contract, const FWEItemInstance& Item)
{
	// Lower enum value = better condition. A device with an open repair (faults) does not count.
	// Phase 17.5: locked, favourite and unreleased personal equipment never goes to a customer.
	return Item.Definition == Contract.Item && Item.Condition <= Contract.MinCondition && Item.Quality >= Contract.MinQuality
		&& !Item.IsPartiallyDisassembled() && !Item.IsCustomerItem() && (!Item.Repair.IsInProgress() || !Item.IsBroken())
		&& !Item.IsSaleProtected() && !Item.NeedsFunctionTest();
}

int32 UWEContractComponent::CountDeliverable(const FWEContract& Contract, const AWECharacter* Character)
{
	if (Contract.Kind == EWEContractKind::Repair)
	{
		const AWEPickupItem* Held = Character ? Character->GetHeldItem() : nullptr;
		return Held && IsRepairedCustomerItem(Contract, Held->GetItem()) ? 1 : 0;
	}
	const UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr;
	int32 Count = 0;
	if (Inventory)
	{
		for (const FWEInventorySlotData& Slot : Inventory->GetAllSlots())
		{
			if (Matches(Contract, Slot.Item))
			{
				Count += Slot.Item.Quantity;
			}
		}
	}
	// Bulky items (e.g. a working PC) are delivered in the hands (Phase 7).
	if (const AWEPickupItem* Held = Character ? Character->GetHeldItem() : nullptr; Held && Matches(Contract, Held->GetItem()))
	{
		Count += Held->GetItem().Quantity;
	}
	return Count;
}

int32 UWEContractComponent::GetCompanyTier() const
{
	const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	return Progression ? Progression->GetCompanyTier() : 1;
}

bool UWEContractComponent::AddOffer(int32 TemplateIndex, bool bIndustrial)
{
	if (!GetOwner()->HasAuthority() || !Catalog || Catalog->Templates.Num() == 0)
	{
		return false;
	}
	if (TemplateIndex == INDEX_NONE)
	{
		// Only what fits the company: its tier, the board (normal / industrial), not offered twice at once.
		const int32 Tier = GetCompanyTier();
		TArray<int32> Eligible;
		for (int32 i = 0; i < Catalog->Templates.Num(); ++i)
		{
			const FWEContractTemplate& T = Catalog->Templates[i];
			const bool bAlready = Contracts.ContainsByPredicate([&T](const FWEContract& C) { return C.Title.EqualTo(T.Title); });
			if (T.bIndustrial == bIndustrial && T.MinTier <= Tier && !(bIndustrial && bAlready))
			{
				Eligible.Add(i);
			}
		}
		if (Eligible.Num() == 0)
		{
			return false;
		}
		TemplateIndex = Eligible[FMath::RandRange(0, Eligible.Num() - 1)];
	}
	if (!Catalog->Templates.IsValidIndex(TemplateIndex) || !Catalog->Templates[TemplateIndex].Item)
	{
		return false;
	}
	const FWEContractTemplate& Template = Catalog->Templates[TemplateIndex];

	FWEContract Contract;
	Contract.Id = FGuid::NewGuid();
	Contract.Title = Template.Title;
	Contract.Item = Template.Item;
	Contract.Kind = Template.Kind;
	Contract.CustomerCondition = Template.CustomerCondition;
	Contract.Quantity = Template.Kind == EWEContractKind::Repair ? 1
		: FMath::RandRange(FMath::Min(Template.MinQuantity, Template.MaxQuantity), FMath::Max(Template.MinQuantity, Template.MaxQuantity));
	Contract.MinCondition = Template.MinCondition;
	Contract.MinQuality = Template.MinQuality;
	Contract.DeliverySeconds = Template.DurationMinutes * 60.f;
	Contract.ExpiresAt = GetServerTime() + OfferLifetimeSeconds;
	Contract.bIndustrial = Template.bIndustrial;
	Contract.Rounds = FMath::Max(1, Template.Rounds);

	// Reward: market value of exactly the minimum acceptable items, times the template factor.
	const FWEItemInstance Reference = FWEItemInstance::Make(Template.Item, Contract.Quantity, Template.MinCondition, FMath::Max<uint8>(50, Template.MinQuality));
	const FWEMoney Value = UWEMarketComponent::GetMarketValue(Reference, UWEMarketComponent::Get(this));
	Contract.Reward = FWEMoney::FromCents(FMath::Max<int64>(100, FMath::RoundToInt64(Value.GetCents() * Template.RewardFactor)));

	Contracts.Add(Contract);
	OnRep_Contracts();
	return true;
}

void UWEContractComponent::UpdateContracts()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	const float Now = GetServerTime();
	bool bChanged = false;
	for (int32 i = Contracts.Num() - 1; i >= 0; --i)
	{
		if (Contracts[i].ExpiresAt <= Now)
		{
			UE_LOG(LogWorkshopEmpire, Log, TEXT("Contract '%s' expired (%s)"), *Contracts[i].Title.ToString(),
				Contracts[i].State == EWEContractState::Accepted ? TEXT("accepted") : TEXT("offer"));
			// Industrial orders and supply contracts: a missed delivery costs a penalty (never below zero).
			if (UWECompanyComponent* Company = UWECompanyComponent::Get(this);
				Company && Contracts[i].State == EWEContractState::Accepted && !Contracts[i].GetPenalty().IsZero())
			{
				const FWEMoney Penalty = FWEMoney::FromCents(FMath::Min(Contracts[i].GetPenalty().GetCents(), Company->GetBalance().GetCents()));
				if (!Penalty.IsZero())
				{
					Company->TryWithdraw(Penalty, FString::Printf(TEXT("Vertragsstrafe: %s"), *Contracts[i].Title.ToString()), Contracts[i].AcceptedBy);
				}
			}
			if (Contracts[i].Kind == EWEContractKind::Repair && Contracts[i].CustomerItemId.IsValid())
			{
				RemoveCustomerItem(Contracts[i].CustomerItemId);
			}
			Contracts.RemoveAt(i);
			bChanged = true;
		}
	}
	int32 Offers = Contracts.FilterByPredicate([](const FWEContract& C) { return C.State == EWEContractState::Offered && !C.bIndustrial; }).Num();
	while (Offers < MaxOffers && AddOffer())
	{
		++Offers;
		bChanged = true;
	}
	int32 IndustrialOffers = Contracts.FilterByPredicate([](const FWEContract& C) { return C.State == EWEContractState::Offered && C.bIndustrial; }).Num();
	while (IndustrialOffers < MaxIndustrialOffers && AddOffer(INDEX_NONE, true))
	{
		++IndustrialOffers;
		bChanged = true;
	}
	if (bChanged)
	{
		OnRep_Contracts();
	}
}

EWEContractResult UWEContractComponent::Accept(const FGuid& Id, const FString& PlayerName)
{
	FWEContract* Contract = Contracts.FindByPredicate([&Id](const FWEContract& C) { return C.Id == Id; });
	if (!GetOwner()->HasAuthority() || !Contract)
	{
		return EWEContractResult::NotFound;
	}
	if (Contract->State != EWEContractState::Offered)
	{
		return EWEContractResult::WrongState;
	}
	const bool bIndustrial = Contract->bIndustrial;
	const int32 Accepted = Contracts.FilterByPredicate([bIndustrial](const FWEContract& C) { return C.State == EWEContractState::Accepted && C.bIndustrial == bIndustrial; }).Num();
	if (Accepted >= (bIndustrial ? MaxIndustrialAccepted : MaxAccepted))
	{
		return EWEContractResult::TooManyAccepted;
	}
	if (Contract->Kind == EWEContractKind::Repair)
	{
		// The customer drops the device off on the delivery pallet (or in front of the terminal).
		FWEItemInstance Device = FWEItemInstance::Make(Contract->Item, 1, Contract->CustomerCondition, static_cast<uint8>(FMath::RandRange(30, 50)));
		Device.Repair.SetFlag(FWERepairState::CustomerItem);
		FTransform Drop(FVector(0.f, 0.f, 100.f));
		if (AWEDeliveryZone* Zone = AWEDeliveryZone::Find(GetWorld()))
		{
			Drop = Zone->NextDropTransform();
		}
		else if (TActorIterator<AWETraderTerminal> Terminal(GetWorld()); Terminal)
		{
			Drop = FTransform(Terminal->GetActorLocation() + Terminal->GetActorForwardVector() * 80.f + FVector(0.f, 0.f, 40.f));
		}
		if (!AWEPickupItem::SpawnWorldItem(GetWorld(), Device, Drop))
		{
			return EWEContractResult::NotFound;
		}
		Contract->CustomerItemId = Device.InstanceId;
	}
	Contract->State = EWEContractState::Accepted;
	Contract->AcceptedBy = PlayerName;
	Contract->ExpiresAt = GetServerTime() + Contract->DeliverySeconds;
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Contract '%s' accepted by %s"), *Contract->Title.ToString(), *PlayerName);
	if (UWETutorialComponent* Tutorial = UWETutorialComponent::Get(this)) { Tutorial->NotifyEvent(EWETutorialEvent::ContractAccepted); }
	OnRep_Contracts();
	return EWEContractResult::Success;
}

EWEContractResult UWEContractComponent::Deliver(const FGuid& Id, AWECharacter* Character)
{
	const int32 Index = Contracts.IndexOfByPredicate([&Id](const FWEContract& C) { return C.Id == Id; });
	UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr;
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!GetOwner()->HasAuthority() || Index == INDEX_NONE || !Inventory || !Company)
	{
		return EWEContractResult::NotFound;
	}
	const FWEContract Contract = Contracts[Index];
	if (Contract.State != EWEContractState::Accepted)
	{
		return EWEContractResult::WrongState;
	}
	if (CountDeliverable(Contract, Character) < Contract.Quantity)
	{
		return EWEContractResult::MissingItems;
	}

	if (Contract.Kind == EWEContractKind::Repair)
	{
		// The repaired device goes back to the customer.
		if (AWEPickupItem* Held = Character->ReleaseHeldItem())
		{
			Held->Destroy();
		}
	}
	// Take exactly the requested amount from matching stacks.
	int32 Remaining = Contract.Kind == EWEContractKind::Repair ? 0 : Contract.Quantity;
	for (const FWEInventorySlotData& Slot : Inventory->GetAllSlots())
	{
		if (Remaining <= 0)
		{
			break;
		}
		const FWEItemInstance& Item = Slot.Item;
		if (Matches(Contract, Item))
		{
			FWEItemInstance Removed;
			const int32 Take = FMath::Min(Remaining, Item.Quantity);
			Inventory->RemoveFromSlot(Slot.SlotIndex, Take, Removed);
			Remaining -= Take;
		}
	}

	if (Remaining > 0)
	{
		// The rest is in the hands (bulky item).
		AWEPickupItem* Held = Character->GetHeldItem();
		if (Held && Matches(Contract, Held->GetItem()) && Character->ReleaseHeldItem())
		{
			Remaining -= Held->GetItem().Quantity;
			Held->Destroy();
		}
	}
	const APlayerState* PS = Character->GetPlayerState();
	CompleteDelivery(Index, PS ? PS->GetPlayerName() : FString());
	return EWEContractResult::Success;
}

void UWEContractComponent::CompleteDelivery(int32 Index, const FString& PlayerName)
{
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!Contracts.IsValidIndex(Index) || !Company)
	{
		return;
	}
	FWEContract& Contract = Contracts[Index];
	++Contract.RoundsDone;
	FString Booking;
	if (Contract.Rounds > 1)
	{
		Booking = FString::Printf(TEXT("Liefervertrag: %s (Lieferung %d/%d)"), *Contract.Title.ToString(), Contract.RoundsDone, Contract.Rounds);
	}
	else
	{
		Booking = FString::Printf(TEXT("%s: %s"), Contract.bIndustrial ? TEXT("Industrieauftrag") : TEXT("Auftrag"), *Contract.Title.ToString());
	}
	const FWEMoney Reward = Contract.Reward;
	const FString Title = Contract.Title.ToString();
	// Phase 16: long-term goals count every completed order (supply contracts once, at the last delivery) and collectors' pieces.
	if (UWEMilestoneComponent* Milestones = UWEMilestoneComponent::Get(this))
	{
		Milestones->AddRareFind(Contract.Item);
		if (Contract.RoundsDone >= Contract.Rounds)
		{
			Milestones->AddCount(Contract.bIndustrial ? EWEMilestoneKind::IndustrialOrders : EWEMilestoneKind::OrdersCompleted);
		}
	}
	if (Contract.RoundsDone < Contract.Rounds)
	{
		// Supply contract: the next delivery is due within the same time again.
		Contract.ExpiresAt = GetServerTime() + Contract.DeliverySeconds;
	}
	else
	{
		Contracts.RemoveAt(Index);
	}
	Company->Deposit(Reward, Booking, PlayerName);
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Contract '%s' delivered, reward %s"), *Title, *Reward.ToDisplayString());
	OnRep_Contracts();
}

int32 UWEContractComponent::CountInStorage(const FWEContract& Contract, const UWEInventoryComponent* Storage)
{
	int32 Count = 0;
	for (const FWEInventorySlotData& Slot : Storage ? Storage->GetAllSlots() : TArray<FWEInventorySlotData>())
	{
		Count += Matches(Contract, Slot.Item) ? Slot.Item.Quantity : 0;
	}
	return Count;
}

EWEContractResult UWEContractComponent::DeliverFromStorage(const FGuid& Id, UWEInventoryComponent* Storage, const FString& PlayerName)
{
	const int32 Index = Contracts.IndexOfByPredicate([&Id](const FWEContract& C) { return C.Id == Id; });
	if (!GetOwner()->HasAuthority() || Index == INDEX_NONE || !Storage)
	{
		return EWEContractResult::NotFound;
	}
	const FWEContract Contract = Contracts[Index];
	if (Contract.State != EWEContractState::Accepted || Contract.Kind != EWEContractKind::Supply)
	{
		return EWEContractResult::WrongState;
	}
	if (CountInStorage(Contract, Storage) < Contract.Quantity)
	{
		return EWEContractResult::MissingItems;
	}
	int32 Remaining = Contract.Quantity;
	const TArray<FWEInventorySlotData> Slots = Storage->GetAllSlots();
	for (const FWEInventorySlotData& Slot : Slots)
	{
		if (Remaining > 0 && Matches(Contract, Slot.Item))
		{
			FWEItemInstance Removed;
			const int32 Take = FMath::Min(Remaining, Slot.Item.Quantity);
			Storage->RemoveFromSlot(Slot.SlotIndex, Take, Removed);
			Remaining -= Take;
		}
	}
	CompleteDelivery(Index, PlayerName);
	return EWEContractResult::Success;
}

void UWEContractComponent::RemoveCustomerItem(const FGuid& ItemId)
{
	for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It)
	{
		AWEPickupItem* Item = *It;
		if (Item->GetItem().InstanceId != ItemId)
		{
			continue;
		}
		if (AWECharacter* Holder = Item->GetHolder())
		{
			Holder->ReleaseHeldItem();
		}
		if (AWEWorkbench* Bench = Item->GetPlacedOn())
		{
			Bench->ResetForLoad();
		}
		UE_LOG(LogWorkshopEmpire, Log, TEXT("Customer device %s taken back (order expired)"), *Item->GetDisplayName().ToString());
		Item->Destroy();
		return;
	}
}

void UWEContractComponent::SetContracts(const TArray<FWEContract>& InContracts)
{
	if (GetOwner()->HasAuthority())
	{
		Contracts = InContracts;
		OnRep_Contracts();
	}
}
