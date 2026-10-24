#include "Economy/WECourierComponent.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Economy/WETraderTerminal.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Framework/WEGameRulesComponent.h"
#include "Framework/WEGameState.h"
#include "Framework/WEPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Inventory/WEInventoryComponent.h"
#include "UObject/UObjectIterator.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "Player/WEAvatarComponent.h"
#include "Player/WECharacter.h"
#include "Player/WEFootstepComponent.h"
#include "Player/WEMannequin.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WELog.h"
#include "World/WEGarageDoor.h"
#include "World/WEDayNightComponent.h"

#define LOCTEXT_NAMESPACE "WECourier"

namespace WECourierConst
{
	constexpr float HalfHeight = 90.f;
	constexpr float ArriveRadius = 45.f;
	/** Usual time from ordering to the courier (Economy "Entspannt": quicker). */
	constexpr float DeliverySeconds = 75.f;
	const TCHAR* ParcelMesh = TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_Parcel.SM_Gen_Parcel");
	const TCHAR* OpenSound = TEXT("/Game/WorkshopEmpire/Audio/P21/SFX_K_cloth1.SFX_K_cloth1");

	void TellEveryone(UWorld* World, const FText& Message)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (AWEPlayerController* PC = Cast<AWEPlayerController>(It->Get()))
			{
				PC->ClientShowMessage(Message);
			}
		}
	}
}

// --- Component ----------------------------------------------------------------------------------------------------

UWECourierComponent::UWECourierComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UWECourierComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWECourierComponent, PendingCount);
	DOREPLIFETIME(UWECourierComponent, NextVisitAt);
}

void UWECourierComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(UpdateTimer, FTimerDelegate::CreateUObject(this, &UWECourierComponent::Update), 1.f, true);
	}
}

UWECourierComponent* UWECourierComponent::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AWEGameState* GameState = World ? World->GetGameState<AWEGameState>() : nullptr;
	return GameState ? GameState->GetCourier() : nullptr;
}

bool UWECourierComponent::IsParcel(const FWEItemInstance& Item)
{
	return Item.Definition && Item.Definition->ItemId == FName(TEXT("Parcel"));
}

bool UWECourierComponent::ShouldShip(const FWEItemInstance& Item) const
{
	return Item.Definition && UWEGameRulesComponent::GetRules(this).bCourier && Item.Definition->WeightKg <= MaxParcelKg && !IsParcel(Item);
}

void UWECourierComponent::Order(const TArray<FWEItemInstance>& Items, const FString& Sender, float DelaySeconds)
{
	if (Items.Num() == 0)
	{
		return;
	}
	Order(Items[0], Sender, DelaySeconds);
	// (Quality overhaul: &Items[1] of a single item tripped the array bounds check - buying one tool crashed the game.)
	if (Items.Num() > 1 && Pending.Num() > 0 && Pending.Last().Item.InstanceId == Items[0].InstanceId)
	{
		Pending.Last().More.Append(&Items[1], Items.Num() - 1);
	}
}

bool UWECourierComponent::IsDeliveryTime() const
{
	const UWEDayNightComponent* Clock = UWEDayNightComponent::Get(this);
	const float Minute = Clock ? Clock->GetMinuteOfDay() : 600.f;
	return Minute >= 7.f * 60.f && Minute < 19.f * 60.f;
}

void UWECourierComponent::Order(const FWEItemInstance& Item, const FString& Sender, float DelaySeconds)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Item.Definition || Item.Quantity <= 0)
	{
		return;
	}
	const bool bQuick = UWEGameRulesComponent::GetRules(this).Economy == EWEEconomyLevel::Relaxed;
	FWECourierOrder& New = Pending.AddDefaulted_GetRef();
	New.Item = Item;
	New.Sender = Sender;
	New.DueAt = GetWorld()->GetTimeSeconds() + (DelaySeconds >= 0.f ? DelaySeconds : WECourierConst::DeliverySeconds * (bQuick ? 0.6f : 1.f) * FMath::FRandRange(0.85f, 1.2f));
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Courier: ordered %d x %s from %s (due in %.0fs)"), Item.Quantity, *Item.GetDisplayName().ToString(), *Sender,
		New.DueAt - GetWorld()->GetTimeSeconds());
	RefreshReplicated();
}

float UWECourierComponent::GetNextVisitIn() const
{
	return NextVisitAt < 0.f || !GetWorld() ? -1.f : FMath::Max(0.f, NextVisitAt - GetWorld()->GetTimeSeconds());
}

void UWECourierComponent::RefreshReplicated()
{
	PendingCount = Pending.Num();
	float Earliest = -1.f;
	for (const FWECourierOrder& O : Pending)
	{
		Earliest = Earliest < 0.f ? O.DueAt : FMath::Min(Earliest, O.DueAt);
	}
	NextVisitAt = Earliest < 0.f ? -1.f : FMath::Max(Earliest, LastVisit + MinVisitGap);
	if (GetOwner())
	{
		GetOwner()->ForceNetUpdate();
	}
}

void UWECourierComponent::Update()
{
	if (Pending.Num() == 0 || ActiveCourier.IsValid())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	// Not more often than every few minutes, and not at night.
	if (Now - LastVisit < MinVisitGap || !IsDeliveryTime())
	{
		return;
	}
	for (const FWECourierOrder& O : Pending)
	{
		if (O.DueAt <= Now)
		{
			SendCourier(false);
			return;
		}
	}
}

void UWECourierComponent::DevSendCourierNow()
{
	if (Pending.Num() > 0 && !ActiveCourier.IsValid())
	{
		SendCourier(true);
	}
}

void UWECourierComponent::SendCourier(bool bIgnoreTimes)
{
	UWorld* World = GetWorld();
	const float Now = World->GetTimeSeconds();
	// Everything due (and what would be due shortly) in one trip, up to what fits in the arms.
	TArray<FWECourierOrder> Trip;
	for (int32 i = 0; i < Pending.Num() && Trip.Num() < MaxParcelsPerTrip;)
	{
		if (bIgnoreTimes || Pending[i].DueAt <= Now + 20.f)
		{
			Trip.Add(Pending[i]);
			Pending.RemoveAt(i);
		}
		else
		{
			++i;
		}
	}
	if (Trip.Num() == 0)
	{
		return;
	}
	LastVisit = Now;
	// From the street to the delivery pallet (like the walk-in customers), back to the street afterwards.
	// (The door closest to the delivery pallet: with expansions there are several.)
	const AWEDeliveryZone* Zone = AWEDeliveryZone::Find(World);
	const AWEGarageDoor* Door = nullptr;
	for (TActorIterator<AWEGarageDoor> It(World); It; ++It)
	{
		if (!Door || (Zone && FVector::DistSquared(It->GetActorLocation(), Zone->GetActorLocation()) < FVector::DistSquared(Door->GetActorLocation(), Zone->GetActorLocation())))
		{
			Door = *It;
		}
	}
	const FVector DoorAt = Door ? Door->GetActorLocation() : FVector(410.f, 0.f, 0.f);
	FVector DropAt = Zone ? Zone->GetActorLocation() : DoorAt + FVector(170.f, 0.f, 0.f);
	DropAt.Z = DoorAt.Z;
	const FVector Street(DoorAt.X + FMath::FRandRange(250.f, 650.f), -1290.f, DoorAt.Z);
	const FVector Exit(DoorAt.X + (FMath::RandBool() ? -900.f : 1500.f), -1290.f, DoorAt.Z);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AWECourier* Courier = World->SpawnActor<AWECourier>(Street + FVector(0.f, 0.f, 95.f), FRotator(0.f, 90.f, 0.f), Params);
	if (!Courier)
	{
		Pending.Append(Trip);
		return;
	}
	Courier->Setup(Trip, DropAt, Exit, FMath::Rand(), DoorAt);
	ActiveCourier = Courier;
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Courier: on the way with %d parcel(s), %d still pending"), Trip.Num(), Pending.Num());
	WECourierConst::TellEveryone(World, FText::Format(LOCTEXT("OnTheWay", "Der Paketbote kommt: {0} {0}|plural(one=Paket,other=Pakete) für die Werkstatt."), Trip.Num()));
	RefreshReplicated();
}

void UWECourierComponent::DropParcels(const TArray<FWECourierOrder>& Orders, const FVector& Near, bool bAnnounce, bool bAtPallet)
{
	UWorld* World = GetWorld();
	UWEItemDefinition* ParcelDef = UWEItemDefinition::FindById(TEXT("Parcel"));
	AWEDeliveryZone* Zone = bAtPallet ? AWEDeliveryZone::Find(World) : nullptr;
	int32 Index = 0;
	for (const FWECourierOrder& O : Orders)
	{
		FWEItemInstance Parcel = FWEItemInstance::Make(ParcelDef, 1, EWEItemCondition::New, 100);
		if (!ParcelDef)
		{
			Parcel = O.Item; // no parcel asset: the thing itself
		}
		else
		{
			FWEParcelContents& Contents = Parcels.AddDefaulted_GetRef();
			Contents.ParcelId = Parcel.InstanceId;
			Contents.Items = { O.Item };
			Contents.Items.Append(O.More);
			Contents.Sender = O.Sender;
		}
		// (In front of a shut door: side by side on the ground, a little apart from earlier ones.)
		FTransform At = Zone ? Zone->NextDropTransform() : FTransform(Near + FVector(FMath::FRandRange(-25.f, 25.f), FMath::FRandRange(-60.f, 60.f), 30.f));
		AWEPickupItem::SpawnWorldItem(World, Parcel, At);
		++Index;
	}
	LastVisit = World->GetTimeSeconds();
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Courier: %d parcel(s) set down"), Orders.Num());
	if (bAnnounce)
	{
		AnnounceDelivered(Orders.Num(), bAtPallet);
	}
	RefreshReplicated();
}

void UWECourierComponent::AnnounceDelivered(int32 Count, bool bAtPallet)
{
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Courier: %d parcel(s) delivered (%s)"), Count, bAtPallet ? TEXT("pallet") : TEXT("in front of the shut door"));
	WECourierConst::TellEveryone(GetWorld(), bAtPallet
		? FText::Format(LOCTEXT("Delivered", "{0} {0}|plural(one=Paket,other=Pakete) im Lieferbereich abgestellt – aufheben und mit [Q] öffnen."), Count)
		: FText::Format(LOCTEXT("DeliveredOutside", "Tor zu: {0} {0}|plural(one=Paket,other=Pakete) vor dem Tor abgestellt – aufheben und mit [Q] öffnen."), Count));
}

const FWEParcelContents* UWECourierComponent::FindContents(const FGuid& ParcelId) const
{
	return Parcels.FindByPredicate([&ParcelId](const FWEParcelContents& P) { return P.ParcelId == ParcelId; });
}

TArray<FWEItemInstance> UWECourierComponent::OpenParcel(AWEPickupItem* Parcel, AWECharacter* By)
{
	TArray<FWEItemInstance> Found;
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Parcel || !IsParcel(Parcel->GetItem()) || Parcel->IsActorBeingDestroyed())
	{
		return Found;
	}
	const int32 Index = Parcels.IndexOfByPredicate([Parcel](const FWEParcelContents& P) { return P.ParcelId == Parcel->GetItem().InstanceId; });
	FString Sender;
	if (Index != INDEX_NONE)
	{
		Found = Parcels[Index].Items;
		Sender = Parcels[Index].Sender;
		Parcels.RemoveAt(Index);
	}
	const FVector Base = Parcel->GetActorLocation();
	TArray<FString> Names;
	for (int32 i = 0; i < Found.Num(); ++i)
	{
		AWEPickupItem::SpawnWorldItem(GetWorld(), Found[i], FTransform(Parcel->GetActorRotation(), Base + FVector((i % 2) * 30.f, (i / 2) * 30.f, 20.f + i * 10.f)));
		Names.Add(Found[i].Quantity > 1 ? FString::Printf(TEXT("%d× %s"), Found[i].Quantity, *Found[i].GetDisplayName().ToString()) : Found[i].GetDisplayName().ToString());
	}
	if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, WECourierConst::OpenSound, nullptr, LOAD_Quiet | LOAD_NoWarn))
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Base);
	}
	UE_LOG(LogWENet, Log, TEXT("%s opened a parcel from %s: %s"), *GetNameSafe(By), *Sender, *FString::Join(Names, TEXT(", ")));
	if (AWEPlayerController* PC = By ? Cast<AWEPlayerController>(By->GetController()) : nullptr)
	{
		PC->ClientShowMessage(Names.Num() > 0
			? FText::Format(LOCTEXT("Opened", "Paket geöffnet: {0}"), FText::FromString(FString::Join(Names, TEXT(", "))))
			: LOCTEXT("OpenedEmpty", "Das Paket ist leer."));
	}
	Parcel->Destroy();
	return Found;
}

void UWECourierComponent::GetSaveState(TArray<FWECourierOrder>& OutOrders, TArray<FWEParcelContents>& OutParcels) const
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	OutOrders = Pending;
	// The courier on his way: his parcels count as still pending (they come with the next visit after loading).
	if (const AWECourier* Courier = ActiveCourier.Get(); Courier)
	{
		OutOrders.Append(Courier->GetOrdersForSave());
	}
	for (FWECourierOrder& O : OutOrders)
	{
		O.DueAt = FMath::Max(0.f, O.DueAt - Now); // remaining seconds
	}
	// Only parcels that still exist (lying around or in a storage): one that was destroyed otherwise is forgotten.
	TSet<FGuid> Existing;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AWEPickupItem> It(World); It; ++It)
		{
			Existing.Add(It->GetItem().InstanceId);
		}
		for (TObjectIterator<UWEInventoryComponent> It; It; ++It)
		{
			if (It->GetWorld() == World)
			{
				for (const FWEInventorySlotData& Slot : It->GetAllSlots())
				{
					Existing.Add(Slot.Item.InstanceId);
				}
			}
		}
	}
	OutParcels = Parcels.FilterByPredicate([&Existing](const FWEParcelContents& P) { return Existing.Contains(P.ParcelId); });
}

void UWECourierComponent::SetSaveState(const TArray<FWECourierOrder>& InOrders, const TArray<FWEParcelContents>& InParcels)
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	// Loaded while a courier walks around: his parcels are in the loaded orders again - he must not bring them twice.
	if (AWECourier* Courier = ActiveCourier.Get())
	{
		Courier->Destroy();
	}
	ActiveCourier.Reset();
	Pending = InOrders;
	for (FWECourierOrder& O : Pending)
	{
		O.DueAt += Now;
	}
	Parcels = InParcels;
	RefreshReplicated();
}

// --- The courier --------------------------------------------------------------------------------------------------

AWECourier::AWECourier()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;
	GetCapsuleComponent()->InitCapsuleSize(32.f, WECourierConst::HalfHeight);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 420.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 170.f;
	WEMannequin::Configure(GetMesh(), WECourierConst::HalfHeight);
	Avatar = CreateDefaultSubobject<UWEAvatarComponent>(TEXT("Avatar"));
	Avatar->SetupAttachment(GetCapsuleComponent());
	CreateDefaultSubobject<UWEFootstepComponent>(TEXT("Footsteps"))->VolumeMultiplier = 0.4f;
}

void AWECourier::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWECourier, Look);
	DOREPLIFETIME(AWECourier, State);
	DOREPLIFETIME(AWECourier, Carrying);
}

void AWECourier::BeginPlay()
{
	Super::BeginPlay();
	OnRep_Look();
	OnRep_Carrying();
}

void AWECourier::OnRep_Look()
{
	WEMannequin::ApplyBody(GetMesh(), Look.Body == 1);
	if (Avatar && GetNetMode() != NM_DedicatedServer)
	{
		Avatar->SetDriver(GetMesh());
		Avatar->ApplyLook(Look);
	}
}

void AWECourier::OnRep_Carrying()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// The stack of parcels in front of the chest (arms in the carry pose).
	UStaticMesh* BoxMesh = LoadObject<UStaticMesh>(nullptr, WECourierConst::ParcelMesh, nullptr, LOAD_Quiet | LOAD_NoWarn);
	while (Stack.Num() < Carrying)
	{
		UStaticMeshComponent* Box = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		Box->SetStaticMesh(BoxMesh);
		Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Box->SetupAttachment(GetCapsuleComponent());
		Box->RegisterComponent();
		const int32 i = Stack.Num();
		// Smaller boxes on top, a little askew.
		const float Scale = 0.85f - i * 0.1f;
		Box->SetRelativeLocationAndRotation(FVector(34.f, (i % 2) * 3.f - 1.5f, 2.f + i * 21.f * 0.9f - i * 2.f), FRotator(0.f, 90.f + (i % 2 ? 6.f : -4.f), 0.f));
		Box->SetRelativeScale3D(FVector(Scale));
		Stack.Add(Box);
	}
	for (int32 i = 0; i < Stack.Num(); ++i)
	{
		Stack[i]->SetVisibility(i < Carrying);
	}
	if (Avatar)
	{
		Avatar->SetWorkPose(Carrying > 0 ? EWEWorkPose::Carry : State == EWECourierState::Dropping ? EWEWorkPose::Reach : EWEWorkPose::None);
	}
}

void AWECourier::Setup(const TArray<FWECourierOrder>& InOrders, const FVector& InDrop, const FVector& InExit, int32 LookSeed, const FVector& InDoor)
{
	Orders = InOrders;
	Exit = InExit;
	DoorAt = InDoor;
	// Outside, in front of the door (the street side is +X of the garage doors).
	Outside = InDoor + FVector(140.f, 0.f, 0.f);
	Look = WELook::Random(LookSeed);
	State = EWECourierState::Arriving;
	Carrying = Orders.Num();
	// A free spot in front of the pallet (on the navmesh).
	Drop = InDrop;
	if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation OnNav;
		for (const float Radius : { 110.f, 150.f, 200.f })
		{
			const FVector Try = InDrop + (GetActorLocation() - InDrop).GetSafeNormal2D() * Radius;
			if (Nav->ProjectPointToNavigation(Try, OnNav, FVector(60.f, 60.f, 200.f)))
			{
				Drop = OnNav.Location;
				break;
			}
		}
	}
	OnRep_Look();
	OnRep_Carrying();
	ForceNetUpdate();
}

TArray<FWECourierOrder> AWECourier::GetOrdersForSave() const
{
	// Parcels are put down from the last one: the first Carrying are still in the arms.
	return TArray<FWECourierOrder>(Orders.GetData(), FMath::Min(Carrying, Orders.Num()));
}

bool AWECourier::MoveTo(const FVector& Goal, float DeltaSeconds)
{
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
	while (Path.IsValidIndex(PathIndex) && FVector::Dist2D(Path[PathIndex], GetActorLocation()) < WECourierConst::ArriveRadius)
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
		if (FVector::Dist2D(LastCheck, GetActorLocation()) < 25.f)
		{
			SetActorLocation(Path[PathIndex] + FVector(0.f, 0.f, WECourierConst::HalfHeight), false, nullptr, ETeleportType::TeleportPhysics);
		}
		StuckTimer = 0.f;
		LastCheck = GetActorLocation();
	}
	return false;
}

void AWECourier::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority())
	{
		return;
	}
	StateTime += DeltaSeconds;
	LifeTime += DeltaSeconds;
	switch (State)
	{
	case EWECourierState::Arriving:
	{
		// A shut door: he leaves the parcels outside in front of it instead of walking through it.
		if (!bOutside && !DoorAt.IsZero() && GetActorLocation().X > DoorAt.X + 20.f)
		{
			const AWEGarageDoor* Door = nullptr;
			for (TActorIterator<AWEGarageDoor> It(GetWorld()); It; ++It)
			{
				Door = FVector::Dist2D(It->GetActorLocation(), DoorAt) < 50.f ? *It : Door;
			}
			bOutside = Door && !Door->IsOpen();
		}
		// (Never longer than two minutes on the way: then the parcels are simply there.)
		if (MoveTo(bOutside ? Outside : Drop, DeltaSeconds) || LifeTime > 120.f)
		{
			State = EWECourierState::Dropping;
			StateTime = 0.f;
			if (const AWEDeliveryZone* Zone = AWEDeliveryZone::Find(GetWorld()))
			{
				FVector To = Zone->GetActorLocation() - GetActorLocation();
				To.Z = 0.f;
				SetActorRotation(To.Rotation());
			}
			OnRep_Carrying();
			ForceNetUpdate();
		}
		break;
	}
	case EWECourierState::Dropping:
		// Bend down, put them on the pallet one by one.
		if (Carrying > 0 && StateTime > 0.6f)
		{
			--Carrying;
			StateTime = 0.f;
			UWECourierComponent* Component = UWECourierComponent::Get(this);
			if (Component && Orders.IsValidIndex(Carrying))
			{
				Component->DropParcels({ Orders[Carrying] }, bOutside ? Outside : Drop, false, !bOutside);
			}
			if (Carrying == 0)
			{
				if (Component)
				{
					Component->AnnounceDelivered(Orders.Num(), !bOutside);
				}
				Orders.Reset();
			}
			OnRep_Carrying();
			ForceNetUpdate();
		}
		else if (Carrying == 0 && StateTime > 1.f)
		{
			State = EWECourierState::Leaving;
			OnRep_Carrying();
			ForceNetUpdate();
		}
		break;
	case EWECourierState::Leaving:
		if (MoveTo(Exit, DeltaSeconds) || LifeTime > 240.f)
		{
			Destroy();
		}
		break;
	}
}

#undef LOCTEXT_NAMESPACE
