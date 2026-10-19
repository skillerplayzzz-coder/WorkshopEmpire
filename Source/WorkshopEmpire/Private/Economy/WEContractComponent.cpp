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
#include "Workshop/WEBenchmark.h"

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
	DOREPLIFETIME(UWEContractComponent, RatingSum);
	DOREPLIFETIME(UWEContractComponent, RatingCount);
	DOREPLIFETIME(UWEContractComponent, LastStars);
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
	if (Contract.Kind == EWEContractKind::UpgradePc)
	{
		// Phase 19: the customer's own PC, complete, tested and fast enough.
		return Contract.CustomerItemId.IsValid() && Item.InstanceId == Contract.CustomerItemId && !Item.IsPartiallyDisassembled()
			&& WEBenchmark::Score(Item) >= Contract.MinBenchmark;
	}
	return Contract.CustomerItemId.IsValid() && Item.InstanceId == Contract.CustomerItemId && !Item.IsBroken()
		&& Item.Condition <= Contract.MinCondition && Item.Repair.Faults == 0 && !Item.IsPartiallyDisassembled() && !Item.NeedsFunctionTest();
}

bool UWEContractComponent::IsDeliverablePc(const FWEContract& Contract, const FWEItemInstance& Item)
{
	if (Contract.Kind == EWEContractKind::UpgradePc)
	{
		return IsRepairedCustomerItem(Contract, Item);
	}
	// Built from parts: one of ours, working, tested, fast enough.
	return Contract.Kind == EWEContractKind::BuildPc && Item.Definition == Contract.Item && !Item.IsCustomerItem() && !Item.IsSaleProtected()
		&& !Item.IsPartiallyDisassembled() && WEBenchmark::Score(Item) >= Contract.MinBenchmark;
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
	if (Contract.IsPcOrder())
	{
		const AWEPickupItem* Held = Character ? Character->GetHeldItem() : nullptr;
		return Held && IsDeliverablePc(Contract, Held->GetItem()) ? 1 : 0;
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
	if (Contract.IsPcOrder())
	{
		// Rounded to 50 points, as a customer would say it.
		Contract.MinBenchmark = FMath::RoundToInt(FMath::RandRange(Template.MinBenchmark, FMath::Max(Template.MinBenchmark, Template.MaxBenchmark)) / 50.f) * 50;
	}

	// Reward: market value of exactly the minimum acceptable items, times the template factor.
	const FWEItemInstance Reference = FWEItemInstance::Make(Template.Item, Contract.Quantity, Template.MinCondition, FMath::Max<uint8>(50, Template.MinQuality));
	const FWEMoney Value = UWEMarketComponent::GetMarketValue(Reference, UWEMarketComponent::Get(this));
	double Factor = Template.RewardFactor;
	if (Contract.IsPcOrder())
	{
		// A faster PC pays more; an upgrade pays the parts and the work (the customer keeps the PC).
		const int32 Stock = FMath::Max(1, WEBenchmark::StockScore(Template.Item));
		const double Ratio = Contract.MinBenchmark / static_cast<double>(Stock);
		Factor *= Contract.Kind == EWEContractKind::BuildPc ? FMath::Max(1.0, Ratio) : FMath::Max(0.35, Ratio - 0.6);
	}
	Contract.Reward = FWEMoney::FromCents(FMath::Max<int64>(100, FMath::RoundToInt64(Value.GetCents() * Factor)));

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

bool UWEContractComponent::AddWalkInRepair(const FText& Title, const FWEItemInstance& Device, const FWEMoney& Reward, const FString& PlayerName)
{
	const int32 Accepted = Contracts.FilterByPredicate([](const FWEContract& C) { return C.State == EWEContractState::Accepted && !C.bIndustrial; }).Num();
	if (!GetOwner()->HasAuthority() || !Device.Definition || Accepted >= MaxAccepted + 1)
	{
		return false;
	}
	FWEContract Contract;
	Contract.Id = FGuid::NewGuid();
	Contract.Title = Title;
	Contract.Item = Device.Definition;
	Contract.Kind = EWEContractKind::Repair;
	Contract.CustomerCondition = Device.Condition;
	Contract.MinCondition = EWEItemCondition::Used;
	Contract.Quantity = 1;
	Contract.Reward = Reward;
	Contract.State = EWEContractState::Accepted;
	Contract.AcceptedBy = PlayerName;
	Contract.DeliverySeconds = 45.f * 60.f;
	Contract.ExpiresAt = GetServerTime() + Contract.DeliverySeconds;
	Contract.CustomerItemId = Device.InstanceId;
	Contracts.Add(Contract);
	OnRep_Contracts();
	return true;
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
	if (Contract->Kind == EWEContractKind::Repair || Contract->Kind == EWEContractKind::UpgradePc)
	{
		// The customer drops the device off on the delivery pallet (or in front of the terminal).
		FWEItemInstance Device = FWEItemInstance::Make(Contract->Item, 1, Contract->CustomerCondition, static_cast<uint8>(FMath::RandRange(30, 50)));
		Device.Repair.SetFlag(FWERepairState::CustomerItem);
		if (Contract->Kind == EWEContractKind::UpgradePc)
		{
			// Works, but too slow: parts may be swapped (the old ones stay in the workshop).
			Device.Repair.SetFlag(FWERepairState::CustomerUpgrade);
			Device.Repair.SetFlag(FWERepairState::FaultsRolled);
			Device.Condition = EWEItemCondition::Used;
		}
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

	// Phase 19: the customer rates the job.
	const AWEPickupItem* HeldNow = Character->GetHeldItem();
	// (Plain deliveries of goods are not rated: the customer gets what was ordered.)
	const int32 Stars = Contract.Kind != EWEContractKind::Supply ? RateDelivery(Contract, HeldNow ? &HeldNow->GetItem() : nullptr, Contract.ExpiresAt - GetServerTime()) : 0;
	if (Contract.Kind == EWEContractKind::Repair || Contract.IsPcOrder())
	{
		// The repaired / upgraded / built PC goes to the customer.
		if (AWEPickupItem* Held = Character->ReleaseHeldItem())
		{
			Held->Destroy();
		}
	}
	// Take exactly the requested amount from matching stacks.
	int32 Remaining = Contract.Kind == EWEContractKind::Repair || Contract.IsPcOrder() ? 0 : Contract.Quantity;
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
	CompleteDelivery(Index, PS ? PS->GetPlayerName() : FString(), Stars);
	return EWEContractResult::Success;
}

int32 UWEContractComponent::RateDelivery(const FWEContract& Contract, const FWEItemInstance* Delivered, float SecondsLeft)
{
	int32 Stars = 4;
	if (Contract.IsPcOrder() && Delivered)
	{
		Stars = WEBenchmark::StarsFor(WEBenchmark::Score(*Delivered), Contract.MinBenchmark);
	}
	else if (Contract.Kind == EWEContractKind::Repair && Delivered)
	{
		// A tidy, well-working device makes the customer happy.
		Stars = Delivered->Quality >= 70 ? 5 : Delivered->Quality >= 50 ? 4 : 3;
	}
	// Last minute: one star less.
	if (Contract.DeliverySeconds > 0.f && SecondsLeft < Contract.DeliverySeconds * 0.15f)
	{
		--Stars;
	}
	return FMath::Clamp(Stars, 1, 5);
}

FWEMoney UWEContractComponent::RewardWithBonus(const FWEMoney& Reward, int32 Stars)
{
	const double Bonus = Stars >= 5 ? 1.25 : Stars == 4 ? 1.1 : 1.0;
	return FWEMoney::FromCents(FMath::RoundToInt64(Reward.GetCents() * Bonus));
}

void UWEContractComponent::CompleteDelivery(int32 Index, const FString& PlayerName, int32 Stars)
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
	const FWEMoney Reward = Stars > 0 ? RewardWithBonus(Contract.Reward, Stars) : Contract.Reward;
	const FString Title = Contract.Title.ToString();
	if (Stars > 0)
	{
		RatingSum += Stars;
		++RatingCount;
		LastStars = Stars;
		Booking += FString::Printf(TEXT(" (%d Sterne)"), Stars);
	}
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
