#include "World/WECustomer.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WEMarketComponent.h"
#include "EngineUtils.h"
#include "Framework/WEPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "Player/WEAvatarComponent.h"
#include "Player/WECharacter.h"
#include "Player/WEFootstepComponent.h"
#include "Player/WEMannequin.h"
#include "WELog.h"
#include "World/WEDayNightComponent.h"
#include "World/WEGarageDoor.h"

#define LOCTEXT_NAMESPACE "WECustomer"

namespace WECustomerConst
{
	constexpr float HalfHeight = 90.f;
	constexpr float ArriveRadius = 45.f;
}

AWECustomer::AWECustomer()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;
	GetCapsuleComponent()->InitCapsuleSize(32.f, WECustomerConst::HalfHeight);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 420.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 150.f;
	WEMannequin::Configure(GetMesh(), WECustomerConst::HalfHeight);
	Avatar = CreateDefaultSubobject<UWEAvatarComponent>(TEXT("Avatar"));
	Avatar->SetupAttachment(GetCapsuleComponent());
	CreateDefaultSubobject<UWEFootstepComponent>(TEXT("Footsteps"))->VolumeMultiplier = 0.4f;
}

void AWECustomer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWECustomer, Look);
	DOREPLIFETIME(AWECustomer, CustomerName);
	DOREPLIFETIME(AWECustomer, Wish);
	DOREPLIFETIME(AWECustomer, State);
	DOREPLIFETIME(AWECustomer, Item);
	DOREPLIFETIME(AWECustomer, PriceCents);
}

void AWECustomer::BeginPlay()
{
	Super::BeginPlay();
	OnRep_Look();
}

void AWECustomer::OnRep_Look()
{
	// The mannequin underneath drives the bones of the character body (like the players' figures).
	WEMannequin::ApplyBody(GetMesh(), Look.Body == 1);
	if (Avatar && GetNetMode() != NM_DedicatedServer)
	{
		Avatar->SetDriver(GetMesh());
		Avatar->ApplyLook(Look);
	}
}

void AWECustomer::Setup(const FString& InName, EWECustomerWish InWish, const FWEItemInstance& InItem, int64 InPriceCents, const FVector& InCounter, const FVector& InExit, int32 LookSeed)
{
	CustomerName = InName;
	Wish = InWish;
	Item = InItem;
	PriceCents = InPriceCents;
	Counter = InCounter;
	Exit = InExit;
	Look = WELook::Random(LookSeed);
	State = EWECustomerState::Arriving;
	OnRep_Look();
	ForceNetUpdate();
}

FText AWECustomer::GetWishText() const
{
	const FText ItemName = Item.Definition ? Item.Definition->DisplayName : FText::GetEmpty();
	switch (Wish)
	{
	case EWECustomerWish::Repair:
		return FText::Format(LOCTEXT("WishRepair", "Bringt {0} zur Reparatur ({1}) – Lohn {2}"), ItemName, WEItems::GetConditionText(Item.Condition), FText::FromString(GetPrice().ToDisplayString()));
	case EWECustomerWish::SellToUs:
		return FText::Format(LOCTEXT("WishSell", "Möchte {0} verkaufen ({1}) für {2}"), ItemName, WEItems::GetConditionText(Item.Condition), FText::FromString(GetPrice().ToDisplayString()));
	default:
		return FText::Format(LOCTEXT("WishBuy", "Sucht {0} (mindestens gebraucht) – zahlt {1}"), ItemName, FText::FromString(GetPrice().ToDisplayString()));
	}
}

FText AWECustomer::GetInteractionText(const AWECharacter* Interactor) const
{
	switch (Wish)
	{
	case EWECustomerWish::Repair:   return FText::Format(LOCTEXT("TakeRepair", "Reparatur annehmen ({0})"), FText::FromString(GetPrice().ToDisplayString()));
	case EWECustomerWish::SellToUs: return FText::Format(LOCTEXT("TakeSell", "Ankaufen für {0}"), FText::FromString(GetPrice().ToDisplayString()));
	default:                        return FText::Format(LOCTEXT("TakeBuy", "Verkaufen für {0}"), FText::FromString(GetPrice().ToDisplayString()));
	}
}

namespace
{
	/** A stack in the inventory (or the item in the hands) that a buying customer takes: right part, working. */
	bool FindForBuyer(const AWECharacter* Character, const UWEItemDefinition* Def, int32& OutSlot, bool& bOutHeld)
	{
		OutSlot = INDEX_NONE;
		bOutHeld = false;
		if (const AWEPickupItem* Held = Character ? Character->GetHeldItem() : nullptr;
			Held && Held->GetItem().Definition == Def && !Held->GetItem().IsBroken() && !Held->GetItem().IsSaleProtected() && !Held->GetItem().IsCustomerItem())
		{
			bOutHeld = true;
			return true;
		}
		const UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr;
		for (const FWEInventorySlotData& Slot : Inventory ? Inventory->GetAllSlots() : TArray<FWEInventorySlotData>())
		{
			if (Slot.Item.Definition == Def && !Slot.Item.IsBroken() && !Slot.Item.IsSaleProtected() && !Slot.Item.IsCustomerItem() && !Slot.Item.IsPartiallyDisassembled())
			{
				OutSlot = Slot.SlotIndex;
				return true;
			}
		}
		return false;
	}
}

bool AWECustomer::CanInteract(const AWECharacter* Interactor, FText& OutReason) const
{
	if (State != EWECustomerState::Waiting)
	{
		OutReason = State == EWECustomerState::Arriving ? LOCTEXT("Coming", "Kommt gerade an …") : LOCTEXT("Going", "Geht gerade");
		return false;
	}
	if (Wish == EWECustomerWish::SellToUs)
	{
		const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
		if (!Company || !Company->CanAfford(GetPrice()))
		{
			OutReason = LOCTEXT("NoMoney", "Nicht genug Geld");
			return false;
		}
	}
	if (Wish == EWECustomerWish::BuyFromUs)
	{
		int32 Slot = INDEX_NONE;
		bool bHeld = false;
		if (!FindForBuyer(Interactor, Item.Definition, Slot, bHeld))
		{
			OutReason = FText::Format(LOCTEXT("NotHere", "Du hast kein {0} dabei"), Item.Definition ? Item.Definition->DisplayName : FText::GetEmpty());
			return false;
		}
	}
	return true;
}

void AWECustomer::Interact(AWECharacter* Interactor)
{
	FText Reason;
	if (!HasAuthority() || !CanInteract(Interactor, Reason))
	{
		return;
	}
	const FString PlayerName = Interactor && Interactor->GetPlayerState() ? Interactor->GetPlayerState()->GetPlayerName() : FString();
	const FVector Drop = GetActorLocation() + GetActorForwardVector() * 70.f - FVector(0.f, 0.f, WECustomerConst::HalfHeight - 30.f);
	switch (Wish)
	{
	case EWECustomerWish::Repair:
	{
		// A repair order like one from the PC: the device is handed over right here.
		UWEContractComponent* Contracts = UWEContractComponent::Get(this);
		FWEItemInstance Device = Item;
		Device.Repair.SetFlag(FWERepairState::CustomerItem);
		if (!Contracts || !Contracts->AddWalkInRepair(FText::Format(LOCTEXT("WalkInTitle", "Laufkunde {0}: {1}"), FText::FromString(CustomerName), Item.Definition->DisplayName),
			Device, GetPrice(), PlayerName))
		{
			Leave(LOCTEXT("TooMany", "Zu viele Aufträge – der Kunde geht wieder."), Interactor);
			return;
		}
		AWEPickupItem::SpawnWorldItem(GetWorld(), Device, FTransform(GetActorRotation(), Drop));
		Leave(LOCTEXT("RepairTaken", "Reparatur angenommen – das Gerät steht an der Tür, der Auftrag ist im Ferrit OS."), Interactor);
		break;
	}
	case EWECustomerWish::SellToUs:
	{
		UWECompanyComponent* Company = UWECompanyComponent::Get(this);
		if (Company && Company->TryWithdraw(GetPrice(), FString::Printf(TEXT("Einkauf Laufkunde: %s"), *Item.GetDisplayName().ToString()), PlayerName))
		{
			AWEPickupItem::SpawnWorldItem(GetWorld(), Item, FTransform(GetActorRotation(), Drop));
			Leave(FText::Format(LOCTEXT("Bought", "Angekauft: {0}"), Item.GetDisplayName()), Interactor);
		}
		break;
	}
	case EWECustomerWish::BuyFromUs:
	{
		int32 Slot = INDEX_NONE;
		bool bHeld = false;
		UWECompanyComponent* Company = UWECompanyComponent::Get(this);
		if (!Company || !FindForBuyer(Interactor, Item.Definition, Slot, bHeld))
		{
			return;
		}
		if (bHeld)
		{
			if (AWEPickupItem* Held = Interactor->ReleaseHeldItem())
			{
				Held->Destroy();
			}
		}
		else
		{
			FWEItemInstance Removed;
			Interactor->GetPlayerInventory()->RemoveFromSlot(Slot, 1, Removed);
		}
		Company->Deposit(GetPrice(), FString::Printf(TEXT("Verkauf an Laufkunde: %s"), *Item.Definition->DisplayName.ToString()), PlayerName);
		Leave(FText::Format(LOCTEXT("Sold", "Verkauft an {0}: +{1}"), FText::FromString(CustomerName), FText::FromString(GetPrice().ToDisplayString())), Interactor);
		break;
	}
	}
}

bool AWECustomer::GetAlternativeInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const
{
	if (State != EWECustomerState::Waiting)
	{
		return false;
	}
	OutText = LOCTEXT("SendAway", "Ablehnen");
	return true;
}

void AWECustomer::InteractAlternative(AWECharacter* Interactor)
{
	if (HasAuthority() && State == EWECustomerState::Waiting)
	{
		Leave(LOCTEXT("SentAway", "Der Kunde geht wieder."), Interactor);
	}
}

bool AWECustomer::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	OutInfo.Title = FText::Format(LOCTEXT("ExTitle", "Kunde: {0}"), FText::FromString(CustomerName));
	OutInfo.Description = GetWishText();
	return true;
}

void AWECustomer::Leave(const FText& Message, AWECharacter* Interactor)
{
	State = EWECustomerState::Leaving;
	ForceNetUpdate();
	if (AWEPlayerController* PC = Interactor ? Cast<AWEPlayerController>(Interactor->GetController()) : nullptr)
	{
		PC->ClientShowMessage(Message);
	}
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Walk-in %s leaves: %s"), *CustomerName, *Message.ToString());
}

bool AWECustomer::MoveTo(const FVector& Goal, float DeltaSeconds)
{
	if (bInstantTravel)
	{
		SetActorLocation(Goal + FVector(0.f, 0.f, WECustomerConst::HalfHeight), false, nullptr, ETeleportType::TeleportPhysics);
		return true;
	}
	if (!PathGoal.Equals(Goal, 1.f))
	{
		PathGoal = Goal;
		Path.Reset();
		PathIndex = 0;
		if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			if (const UNavigationPath* NavPath = Nav->FindPathToLocationSynchronously(GetWorld(), GetActorLocation(), Goal, this); NavPath && NavPath->IsValid())
			{
				Path.Append(NavPath->PathPoints);
			}
		}
		if (Path.Num() == 0)
		{
			Path.Add(Goal);
		}
		StuckTimer = 0.f;
		LastCheck = GetActorLocation();
	}
	while (Path.IsValidIndex(PathIndex) && FVector::Dist2D(Path[PathIndex], GetActorLocation()) < WECustomerConst::ArriveRadius)
	{
		++PathIndex;
	}
	if (!Path.IsValidIndex(PathIndex))
	{
		return true;
	}
	FVector To = Path[PathIndex] - GetActorLocation();
	To.Z = 0.f;
	AddMovementInput(To.GetSafeNormal());
	StuckTimer += DeltaSeconds;
	if (StuckTimer > 2.f)
	{
		// Somebody in the way: step on to the next point.
		if (FVector::Dist2D(LastCheck, GetActorLocation()) < 25.f)
		{
			SetActorLocation(Path[PathIndex] + FVector(0.f, 0.f, WECustomerConst::HalfHeight), false, nullptr, ETeleportType::TeleportPhysics);
		}
		StuckTimer = 0.f;
		LastCheck = GetActorLocation();
	}
	return false;
}

void AWECustomer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority())
	{
		return;
	}
	switch (State)
	{
	case EWECustomerState::Arriving:
		if (MoveTo(Counter, DeltaSeconds))
		{
			State = EWECustomerState::Waiting;
			WaitTime = 0.f;
			ForceNetUpdate();
			// The door may be shut: tell everybody somebody is waiting outside.
			for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
			{
				if (AWEPlayerController* PC = Cast<AWEPlayerController>(It->Get()))
				{
					PC->ClientShowMessage(FText::Format(LOCTEXT("AtDoor", "Kundschaft am Tor: {0} – {1}"), FText::FromString(CustomerName), GetWishText()));
				}
			}
		}
		break;
	case EWECustomerState::Waiting:
		WaitTime += DeltaSeconds;
		if (WaitTime > PatienceSeconds)
		{
			UE_LOG(LogWorkshopEmpire, Log, TEXT("Walk-in %s waited too long"), *CustomerName);
			State = EWECustomerState::Leaving;
			ForceNetUpdate();
		}
		break;
	case EWECustomerState::Leaving:
		if (MoveTo(Exit, DeltaSeconds))
		{
			Destroy();
		}
		break;
	}
}

// --- Spawning ---------------------------------------------------------------------------------------------------

namespace WEWalkIn
{
	AWECustomer* TrySpawn(UWorld* World, bool bForce)
	{
		if (!World || World->GetNetMode() == NM_Client)
		{
			return nullptr;
		}
		const UWEDayNightComponent* Clock = UWEDayNightComponent::Get(World);
		int32 Count = 0;
		for (TActorIterator<AWECustomer> It(World); It; ++It)
		{
			++Count;
		}
		if (!bForce && (!Clock || !Clock->IsOpen() || Count >= MaxCustomers))
		{
			return nullptr;
		}
		// Where they come from (the sidewalk at the gate) and where they wait (in front of the garage door).
		const AWEGarageDoor* Door = nullptr;
		for (TActorIterator<AWEGarageDoor> It(World); It && !Door; ++It)
		{
			Door = *It;
		}
		const FVector DoorAt = Door ? Door->GetActorLocation() : FVector(410.f, 0.f, 0.f);
		const FVector Counter = DoorAt + FVector(170.f, FMath::FRandRange(-60.f, 60.f) - 60.f * Count, 0.f);
		const FVector Street(DoorAt.X + FMath::FRandRange(250.f, 650.f), -1290.f, DoorAt.Z);
		const FVector Exit(DoorAt.X + (FMath::RandBool() ? -900.f : 1500.f), -1290.f, DoorAt.Z);

		// What they want.
		static const TCHAR* RepairItems[] = { TEXT("OldComputer"), TEXT("CrtMonitor"), TEXT("OldTelevision"), TEXT("PortableRadio"), TEXT("GameConsole"), TEXT("CordlessDrill") };
		static const TCHAR* SellItems[] = { TEXT("OldComputer"), TEXT("CrtMonitor"), TEXT("VintageRadio"), TEXT("ElectricMotor"), TEXT("CableBox"), TEXT("PortableRadio"), TEXT("GameConsole"), TEXT("OldTelevision") };
		static const TCHAR* BuyItems[] = { TEXT("RamModule"), TEXT("GraphicsCard"), TEXT("PowerSupply"), TEXT("SsdDrive"), TEXT("CopperCable"), TEXT("Processor"), TEXT("Speaker"), TEXT("DrillBattery") };
		static const TCHAR* Names[] = { TEXT("Herr Krause"), TEXT("Frau Yilmaz"), TEXT("Herr Schulte"), TEXT("Frau Becker"), TEXT("Tom"), TEXT("Frau Novak"),
			TEXT("Herr Albers"), TEXT("Leonie"), TEXT("Herr Okafor"), TEXT("Frau Brandt") };
		const int32 Roll = FMath::RandRange(0, 99);
		const EWECustomerWish Wish = Roll < 40 ? EWECustomerWish::Repair : Roll < 70 ? EWECustomerWish::SellToUs : EWECustomerWish::BuyFromUs;
		FWEItemInstance Item;
		int64 Cents = 0;
		const UWEMarketComponent* Market = UWEMarketComponent::Get(World);
		if (Wish == EWECustomerWish::Repair)
		{
			Item = FWEItemInstance::Make(UWEItemDefinition::FindById(RepairItems[FMath::RandRange(0, UE_ARRAY_COUNT(RepairItems) - 1)]), 1,
				FMath::RandBool() ? EWEItemCondition::Damaged : EWEItemCondition::Defective, static_cast<uint8>(FMath::RandRange(30, 55)));
			// Labour fee like the repair orders on the PC.
			if (Item.Definition)
			{
				FWEItemInstance Repaired = Item;
				Repaired.Condition = EWEItemCondition::Used;
				Cents = FMath::RoundToInt64(UWEMarketComponent::GetMarketValue(Repaired, Market).GetCents() * 0.5 / 50.0) * 50;
			}
		}
		else if (Wish == EWECustomerWish::SellToUs)
		{
			Item = FWEItemInstance::Make(UWEItemDefinition::FindById(SellItems[FMath::RandRange(0, UE_ARRAY_COUNT(SellItems) - 1)]), 1,
				static_cast<EWEItemCondition>(FMath::RandRange(2, 4)), static_cast<uint8>(FMath::RandRange(25, 70)));
			// Private people sell cheap.
			Cents = Item.Definition ? FMath::RoundToInt64(UWEMarketComponent::GetMarketValue(Item, Market).GetCents() * FMath::FRandRange(0.45f, 0.75f) / 50.0) * 50 : 0;
		}
		else
		{
			Item = FWEItemInstance::Make(UWEItemDefinition::FindById(BuyItems[FMath::RandRange(0, UE_ARRAY_COUNT(BuyItems) - 1)]), 1, EWEItemCondition::Used, 50);
			// They pay more than a trader would.
			Cents = Item.Definition ? FMath::RoundToInt64(UWEMarketComponent::GetMarketValue(Item, Market).GetCents() * FMath::FRandRange(1.1f, 1.35f) / 50.0) * 50 : 0;
		}
		if (!Item.Definition)
		{
			return nullptr;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AWECustomer* Customer = World->SpawnActor<AWECustomer>(Street + FVector(0.f, 0.f, 95.f), FRotator(0.f, 90.f, 0.f), Params);
		if (Customer)
		{
			Customer->Setup(Names[FMath::RandRange(0, UE_ARRAY_COUNT(Names) - 1)], Wish, Item, FMath::Max<int64>(100, Cents), Counter, Exit, FMath::Rand());
			UE_LOG(LogWorkshopEmpire, Log, TEXT("Walk-in customer: %s"), *Customer->GetWishText().ToString());
		}
		return Customer;
	}
}

#undef LOCTEXT_NAMESPACE
