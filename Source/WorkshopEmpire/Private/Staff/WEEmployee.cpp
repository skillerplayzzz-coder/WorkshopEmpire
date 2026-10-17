#include "Staff/WEEmployee.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Player/WEMannequin.h"
#include "Player/WEFootstepComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Framework/WEPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Logistics/WEVehicle.h"
#include "Machines/WEMachine.h"
#include "Machines/WEMachineDefinition.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "Player/WECharacter.h"
#include "Staff/WEStaffComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "WELog.h"
#include "World/WEPlaceholderProp.h"
#include "World/WEPlaceholderVisuals.h"

#define LOCTEXT_NAMESPACE "WEEmployee"

bool AWEEmployee::bInstantTravel = false;

namespace WEEmployeeConst
{
	constexpr float ArriveRadius = 45.f;
	constexpr float NextToDistance = 140.f;
	constexpr float StandOff = 55.f;
	constexpr float CapsuleHalfHeight = 90.f;
	constexpr float MaintainBelow = 40.f;
}

AWEEmployee::AWEEmployee()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;

	GetCapsuleComponent()->InitCapsuleSize(32.f, WEEmployeeConst::CapsuleHalfHeight);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(WEPlaceholder::MaterialPath);
	PlaceholderMaterial = Material.Object;

	auto MakePart = [this](const TCHAR* Name, UStaticMesh* PartMesh, const FVector& Location, const FVector& Scale)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(GetCapsuleComponent());
		Part->SetStaticMesh(PartMesh);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Part;
	};
	// Animated worker (mannequin in work clothes of the role colour) and what he carries in front of him.
	WEMannequin::Configure(GetMesh(), WEEmployeeConst::CapsuleHalfHeight);
	CreateDefaultSubobject<UWEFootstepComponent>(TEXT("Footsteps"))->VolumeMultiplier = 0.45f;
	CarriedMesh = MakePart(TEXT("Carried"), Cube.Object, FVector(42.f, 0.f, 8.f), FVector(0.3f));
	CarriedMesh->SetVisibility(false);
}

void AWEEmployee::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWEEmployee, Profile);
	DOREPLIFETIME(AWEEmployee, Task);
	DOREPLIFETIME(AWEEmployee, State);
	DOREPLIFETIME(AWEEmployee, StateDetail);
	DOREPLIFETIME(AWEEmployee, Carried);
	DOREPLIFETIME(AWEEmployee, Tint);
	DOREPLIFETIME(AWEEmployee, bAway);
	DOREPLIFETIME(AWEEmployee, bPaused);
	DOREPLIFETIME(AWEEmployee, bUnpaid);
}

void AWEEmployee::BeginPlay()
{
	Super::BeginPlay();
	OnRep_Visuals();
	// Only the server steers; clients just see the replicated movement.
	SetActorTickEnabled(HasAuthority());
}

void AWEEmployee::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		ReleaseMachine();
	}
	Super::EndPlay(EndPlayReason);
}

void AWEEmployee::OnRep_Visuals()
{
	// Manny or Quinn by name, so the same person always looks the same on every machine.
	WEMannequin::ApplyBody(GetMesh(), GetTypeHash(Profile.Name) % 2 == 1);
	WEMannequin::Tint(GetMesh(), Tint);
	const UWEItemDefinition* Def = Carried.Definition;
	CarriedMesh->SetVisibility(Def != nullptr && !bAway);
	if (Def)
	{
		FVector Scale = Def->WorldScale;
		Scale *= Scale.GetMax() > 0.45f ? 0.45f / Scale.GetMax() : 1.f;
		CarriedMesh->SetRelativeScale3D(Scale);
		WEPlaceholder::ApplyTint(CarriedMesh, PlaceholderMaterial, Def->Tint);
	}
	SetActorHiddenInGame(bAway);
	SetActorEnableCollision(!bAway);
}

// --- Texts ---------------------------------------------------------------------------------------

FText AWEEmployee::GetRoleText(EWEStaffRole Role)
{
	switch (Role)
	{
	case EWEStaffRole::WarehouseWorker: return LOCTEXT("RoleWarehouse", "Lagerarbeiter");
	case EWEStaffRole::MachineOperator: return LOCTEXT("RoleOperator", "Maschinenbediener");
	case EWEStaffRole::Driver:          return LOCTEXT("RoleDriver", "Fahrer");
	}
	return FText::GetEmpty();
}

FText AWEEmployee::GetRoleText() const
{
	return GetRoleText(Profile.Role);
}

FText AWEEmployee::GetStateText() const
{
	FText Text;
	switch (State)
	{
	case EWEEmployeeState::Idle:    Text = Task.Kind == EWETaskKind::None ? LOCTEXT("StNoTask", "Wartet auf eine Aufgabe") : LOCTEXT("StIdle", "Bereit"); break;
	case EWEEmployeeState::Walking: Text = LOCTEXT("StWalking", "Unterwegs"); break;
	case EWEEmployeeState::Working: Text = LOCTEXT("StWorking", "Arbeitet"); break;
	case EWEEmployeeState::Waiting: Text = LOCTEXT("StWaiting", "Wartet"); break;
	case EWEEmployeeState::Paused:  Text = LOCTEXT("StPaused", "Pause"); break;
	case EWEEmployeeState::Unpaid:  Text = LOCTEXT("StUnpaid", "Arbeitet nicht – Lohn ausstehend"); break;
	case EWEEmployeeState::Away:    Text = LOCTEXT("StAway", "Auf Tour"); break;
	}
	return StateDetail.IsEmpty() ? Text : FText::Format(LOCTEXT("StateWithDetail", "{0}: {1}"), Text, StateDetail);
}

static FText StaffLabelOf(const AActor* Actor)
{
	if (const AWEPlaceholderProp* Prop = Cast<AWEPlaceholderProp>(Actor))
	{
		return Prop->GetFurnitureName();
	}
	return Actor ? FText::FromString(Actor->GetName()) : NSLOCTEXT("WEEmployee", "NoActor", "–");
}

FText AWEEmployee::GetTaskText() const
{
	switch (Task.Kind)
	{
	case EWETaskKind::Transport:
		return FText::Format(LOCTEXT("TaskTransport", "Transport: {0} → {1} ({2})"), StaffLabelOf(Task.Source), StaffLabelOf(Task.Target),
			Task.Filter ? Task.Filter->DisplayName : LOCTEXT("FilterAll", "alles Passende"));
	case EWETaskKind::OperateMachine:
		return Task.Source
			? FText::Format(LOCTEXT("TaskOperateStore", "Bedient: {0} (Lager: {1})"), StaffLabelOf(Task.Target), StaffLabelOf(Task.Source))
			: FText::Format(LOCTEXT("TaskOperate", "Bedient: {0}"), StaffLabelOf(Task.Target));
	case EWETaskKind::DriveVehicle:
		return FText::Format(LOCTEXT("TaskDrive", "Fährt: {0}"), StaffLabelOf(Task.Target));
	default:
		return LOCTEXT("TaskNone", "Keine Aufgabe");
	}
}

FText AWEEmployee::GetWageText() const
{
	return FText::Format(LOCTEXT("Wage", "{0}/min"), FText::FromString(FWEMoney::FromCents(Profile.WageCentsPerMinute).ToDisplayString()));
}

EWETaskKind AWEEmployee::GetTaskKindForRole(EWEStaffRole Role)
{
	switch (Role)
	{
	case EWEStaffRole::WarehouseWorker: return EWETaskKind::Transport;
	case EWEStaffRole::MachineOperator: return EWETaskKind::OperateMachine;
	case EWEStaffRole::Driver:          return EWETaskKind::DriveVehicle;
	}
	return EWETaskKind::None;
}

UWEInventoryComponent* AWEEmployee::GetTakeInventory(AActor* Actor)
{
	if (AWEMachine* Machine = Cast<AWEMachine>(Actor))
	{
		return Machine->GetOutputStorage();
	}
	const AWEPlaceholderProp* Prop = Cast<AWEPlaceholderProp>(Actor);
	return Prop && Prop->HasStorage() ? Prop->GetStorage() : nullptr;
}

bool AWEEmployee::IsValidSource(const AActor* Actor) const
{
	switch (Profile.Role)
	{
	case EWEStaffRole::WarehouseWorker: return Actor && GetTakeInventory(const_cast<AActor*>(Actor)) != nullptr;
	case EWEStaffRole::MachineOperator: return Actor && !Actor->IsA<AWEMachine>() && GetTakeInventory(const_cast<AActor*>(Actor)) != nullptr;
	default: return false;
	}
}

bool AWEEmployee::IsValidTarget(const AActor* Actor) const
{
	switch (Profile.Role)
	{
	case EWEStaffRole::WarehouseWorker: return Actor && (Actor->IsA<AWEMachine>() || GetTakeInventory(const_cast<AActor*>(Actor)) != nullptr);
	case EWEStaffRole::MachineOperator: return Actor && Actor->IsA<AWEMachine>();
	case EWEStaffRole::Driver:          return Actor && Actor->IsA<AWEVehicle>();
	}
	return false;
}

// --- Interaction ---------------------------------------------------------------------------------

FText AWEEmployee::GetInteractionText(const AWECharacter* Interactor) const
{
	return FText::Format(LOCTEXT("Assign", "Aufgabe zuweisen ({0}, {1})"), FText::FromString(Profile.Name), GetRoleText());
}

bool AWEEmployee::CanInteract(const AWECharacter* Interactor, FText& OutReason) const
{
	return Interactor && !bAway;
}

bool AWEEmployee::GetAlternativeInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const
{
	if (!Interactor || bAway)
	{
		return false;
	}
	OutText = bPaused ? LOCTEXT("Resume", "Weiterarbeiten") : LOCTEXT("Pause", "Pause machen");
	return true;
}

void AWEEmployee::InteractAlternative(AWECharacter* Interactor)
{
	SetPaused(!bPaused);
}

bool AWEEmployee::GetExtraInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const
{
	if (!Interactor || bAway)
	{
		return false;
	}
	OutText = LOCTEXT("Fire", "Entlassen (2× drücken)");
	return true;
}

void AWEEmployee::InteractExtra(AWECharacter* Interactor)
{
	if (UWEStaffComponent* Staff = UWEStaffComponent::Get(this))
	{
		Staff->Fire(this, Interactor);
	}
}

bool AWEEmployee::GetSettingInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const
{
	if (!Interactor || Task.Kind != EWETaskKind::Transport)
	{
		return false;
	}
	OutText = FText::Format(LOCTEXT("FilterSetting", "Material: {0}"), Task.Filter ? Task.Filter->DisplayName : LOCTEXT("FilterAllShort", "alles Passende"));
	return true;
}

void AWEEmployee::InteractSetting(AWECharacter* Interactor)
{
	CycleFilter();
}

bool AWEEmployee::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	OutInfo.Title = FText::FromString(Profile.Name);
	OutInfo.Description = FText::Format(LOCTEXT("ExDesc", "{0} · Erfahrung {1}/3 · Lohn {2}"), GetRoleText(), FText::AsNumber(Profile.Skill), GetWageText());
	OutInfo.Details.Reset();
	OutInfo.Details.Add(GetTaskText());
	OutInfo.Details.Add(FText::Format(LOCTEXT("ExState", "Zustand: {0}"), GetStateText()));
	if (Carried.IsValid())
	{
		OutInfo.Details.Add(FText::Format(LOCTEXT("ExCarried", "Trägt: {0} × {1}"), FText::AsNumber(Carried.Quantity), Carried.GetDisplayName()));
	}
	OutInfo.Details.Add(LOCTEXT("ExHelp", "[E] Aufgabe zuweisen · [Q] Pause · [T] Material · [X] Entlassen"));
	return true;
}

// --- Server: setup and orders ------------------------------------------------------------------------

void AWEEmployee::Setup(const FWEEmployeeProfile& InProfile, const FLinearColor& InTint)
{
	Profile = InProfile;
	Tint = InTint;
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * Profile.GetSpeedFactor();
	OnRep_Visuals();
	ForceNetUpdate();
}

bool AWEEmployee::AssignTask(const FWEEmployeeTask& NewTask)
{
	if (!HasAuthority() || NewTask.Kind != GetTaskKindForRole(Profile.Role) || !IsValidTarget(NewTask.Target)
		|| (NewTask.Kind == EWETaskKind::Transport && (!IsValidSource(NewTask.Source) || NewTask.Source == NewTask.Target))
		|| (NewTask.Source && !IsValidSource(NewTask.Source)))
	{
		return false;
	}
	ReturnCarried();
	ReleaseMachine();
	StopWalking();
	Task = NewTask;
	Phase = Task.Kind == EWETaskKind::OperateMachine ? EPhase::AtMachine : EPhase::GoToSource;
	PhaseTimer = 0.f;
	WaitTimer = 0.f;
	SetState(EWEEmployeeState::Idle);
	ForceNetUpdate();
	UE_LOG(LogWENet, Log, TEXT("%s (%s): %s"), *GetName(), *Profile.Name, *GetTaskText().ToString());
	return true;
}

void AWEEmployee::ClearTask()
{
	if (!HasAuthority())
	{
		return;
	}
	ReturnCarried();
	ReleaseMachine();
	StopWalking();
	Task = FWEEmployeeTask();
	SetState(EWEEmployeeState::Idle);
	ForceNetUpdate();
}

void AWEEmployee::SetPaused(bool bInPaused)
{
	if (!HasAuthority())
	{
		return;
	}
	bPaused = bInPaused;
	if (bPaused)
	{
		StopWalking();
		ReleaseMachine();
		SetState(EWEEmployeeState::Paused);
	}
	else
	{
		SetState(EWEEmployeeState::Idle);
	}
}

void AWEEmployee::SetUnpaid(bool bInUnpaid)
{
	if (!HasAuthority() || bUnpaid == bInUnpaid)
	{
		return;
	}
	bUnpaid = bInUnpaid;
	if (bUnpaid)
	{
		StopWalking();
		ReleaseMachine();
		SetState(EWEEmployeeState::Unpaid);
	}
	else
	{
		SetState(bPaused ? EWEEmployeeState::Paused : EWEEmployeeState::Idle);
	}
}

void AWEEmployee::CycleFilter()
{
	if (!HasAuthority() || Task.Kind != EWETaskKind::Transport)
	{
		return;
	}
	TArray<UWEItemDefinition*> Choices;
	if (const UWEInventoryComponent* Inventory = GetTakeInventory(Task.Source))
	{
		for (const FWEInventorySlotData& Slot : Inventory->GetAllSlots())
		{
			if (Slot.Item.Definition)
			{
				Choices.AddUnique(Slot.Item.Definition);
			}
		}
	}
	const int32 Index = Task.Filter ? Choices.IndexOfByKey(Task.Filter.Get()) : INDEX_NONE;
	Task.Filter = Choices.IsValidIndex(Index + 1) && (Task.Filter == nullptr || Index != INDEX_NONE) ? Choices[Index + 1] : nullptr;
	ForceNetUpdate();
}

void AWEEmployee::SetAway(bool bInAway, const FVector& ReturnLocation)
{
	if (!HasAuthority())
	{
		return;
	}
	bAway = bInAway;
	StopWalking();
	if (!bAway && !ReturnLocation.IsZero())
	{
		SetActorLocation(ReturnLocation + FVector(0.f, 0.f, WEEmployeeConst::CapsuleHalfHeight + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
	}
	SetState(bAway ? EWEEmployeeState::Away : EWEEmployeeState::Idle);
	OnRep_Visuals();
	ForceNetUpdate();
}

bool AWEEmployee::IsReadyToDrive() const
{
	return Task.Kind == EWETaskKind::DriveVehicle && !bAway && !bPaused && !bUnpaid && IsNextTo(Task.Target);
}

void AWEEmployee::RestoreCarried(const FWEItemInstance& Item)
{
	Carried = Item;
	OnRep_Visuals();
}

void AWEEmployee::SetState(EWEEmployeeState NewState, const FText& Detail)
{
	if (State != NewState || !StateDetail.EqualTo(Detail))
	{
		State = NewState;
		StateDetail = Detail;
		ForceNetUpdate();
	}
}

// --- Server: walking ---------------------------------------------------------------------------------

FVector AWEEmployee::GetStandPoint(const AActor* Actor) const
{
	if (!Actor)
	{
		return GetActorLocation();
	}
	// The free spot next to the actor that is closest to the employee (front, back or the sides).
	const AWEPlaceholderProp* Prop = Cast<AWEPlaceholderProp>(Actor);
	const FTransform Transform = Actor->GetActorTransform();
	const FVector Half = Prop ? Prop->GetSize() * 0.5f : Actor->GetComponentsBoundingBox().GetExtent();
	const FVector Center = Prop ? Transform.GetLocation() : Actor->GetComponentsBoundingBox().GetCenter();
	const FVector Candidates[] = {
		FVector(Half.X + WEEmployeeConst::StandOff, 0.f, 0.f), FVector(-Half.X - WEEmployeeConst::StandOff, 0.f, 0.f),
		FVector(0.f, Half.Y + WEEmployeeConst::StandOff, 0.f), FVector(0.f, -Half.Y - WEEmployeeConst::StandOff, 0.f) };
	FVector Best = Center;
	float BestDistance = TNumericLimits<float>::Max();
	for (const FVector& Local : Candidates)
	{
		FVector Point = Center + Transform.GetRotation().RotateVector(Local);
		Point.Z = WEEmployeeConst::CapsuleHalfHeight + 2.f;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(WEStandPoint), false, this);
		const bool bBlocked = GetWorld() && GetWorld()->OverlapAnyTestByChannel(Point, FQuat::Identity, ECC_Pawn,
			FCollisionShape::MakeCapsule(28.f, WEEmployeeConst::CapsuleHalfHeight - 10.f), Params);
		const float Distance = FVector::DistSquared2D(Point, GetActorLocation()) + (bBlocked ? 1.e8f : 0.f);
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Point;
		}
	}
	return Best;
}

bool AWEEmployee::IsNextTo(const AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}
	const FVector Location = GetActorLocation();
	if (const AWEPlaceholderProp* Prop = Cast<AWEPlaceholderProp>(Actor))
	{
		// Furniture, machines, vans: distance to the (rotated) footprint.
		const FVector Local = Prop->GetActorTransform().InverseTransformPosition(Location);
		const FVector Half = Prop->GetSize() * 0.5f;
		const FVector2D Outside(FMath::Max(0.f, FMath::Abs(Local.X) - Half.X), FMath::Max(0.f, FMath::Abs(Local.Y) - Half.Y));
		return Outside.Size() <= WEEmployeeConst::NextToDistance;
	}
	const FBox Box = Actor->GetComponentsBoundingBox(true);
	const FVector Closest(FMath::Clamp(Location.X, Box.Min.X, Box.Max.X), FMath::Clamp(Location.Y, Box.Min.Y, Box.Max.Y), Location.Z);
	return FVector::Dist2D(Closest, Location) <= WEEmployeeConst::NextToDistance;
}

bool AWEEmployee::WalkTo(const AActor* Actor)
{
	if (!Actor)
	{
		return false;
	}
	if (IsNextTo(Actor) && (!bWalking || WalkTarget.Get() != Actor || PathIndex >= Path.Num()))
	{
		StopWalking();
		return true;
	}
	if (bInstantTravel)
	{
		SetActorLocation(GetStandPoint(Actor), false, nullptr, ETeleportType::TeleportPhysics);
		StopWalking();
		return true;
	}
	if (!bWalking || WalkTarget.Get() != Actor)
	{
		const FVector Goal = GetStandPoint(Actor);
		Path.Reset();
		const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		if (Now < NoPathUntil)
		{
			return false; // waiting for a way (e.g. the door is closed)
		}
		UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		const bool bHasNavData = Nav && Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) != nullptr;
		if (bHasNavData)
		{
			if (const UNavigationPath* NavPath = Nav->FindPathToLocationSynchronously(GetWorld(), GetActorLocation(), Goal, this);
				NavPath && NavPath->IsValid() && !NavPath->IsPartial())
			{
				Path.Append(NavPath->PathPoints);
				if (Path.Num() > 0)
				{
					Path.RemoveAt(0); // own position
				}
			}
		}
		const bool bNavPath = Path.Num() > 0;
		if (bHasNavData && !bNavPath)
		{
			// There is a navmesh but no way there: wait instead of walking through walls.
			SetState(EWEEmployeeState::Waiting, LOCTEXT("NoWay", "kein Weg frei (Tor zu?)"));
			NoPathUntil = Now + RetrySeconds;
			return false;
		}
		if (Path.Num() == 0)
		{
			Path.Add(Goal);
		}
		UE_LOG(LogWENet, Log, TEXT("%s walks to %s (%d points, %s)"), *GetName(), *Actor->GetName(), Path.Num(), bNavPath ? TEXT("navmesh") : TEXT("direct"));
		PathIndex = 0;
		WalkTarget = Actor;
		bWalking = true;
		StuckTimer = 0.f;
		LastCheckLocation = GetActorLocation();
		SetState(EWEEmployeeState::Walking);
	}
	return false;
}

void AWEEmployee::StopWalking()
{
	bWalking = false;
	Path.Reset();
	PathIndex = 0;
	WalkTarget.Reset();
}

void AWEEmployee::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || !bWalking || !Path.IsValidIndex(PathIndex))
	{
		return;
	}
	FVector ToNext = Path[PathIndex] - GetActorLocation();
	ToNext.Z = 0.f;
	if (ToNext.Size() < WEEmployeeConst::ArriveRadius)
	{
		++PathIndex;
		return;
	}
	AddMovementInput(ToNext.GetSafeNormal());

	// Stuck behind something (no navigation data, someone in the way): step over it.
	StuckTimer += DeltaSeconds;
	if (StuckTimer >= 1.5f)
	{
		if (FVector::Dist2D(LastCheckLocation, GetActorLocation()) < 25.f)
		{
			UE_LOG(LogWENet, Log, TEXT("%s stuck, stepping to the next path point"), *GetName());
			SetActorLocation(Path[PathIndex] + FVector(0.f, 0.f, 2.f), false, nullptr, ETeleportType::TeleportPhysics);
		}
		StuckTimer = 0.f;
		LastCheckLocation = GetActorLocation();
	}
}

// --- Server: work ------------------------------------------------------------------------------------

void AWEEmployee::StepWork(float DeltaSeconds)
{
	if (!HasAuthority() || bAway)
	{
		return;
	}
	if (bPaused || bUnpaid)
	{
		return;
	}
	UnpaidSeconds += DeltaSeconds;
	switch (Task.Kind)
	{
	case EWETaskKind::Transport:      StepTransport(DeltaSeconds); break;
	case EWETaskKind::OperateMachine: StepOperate(DeltaSeconds); break;
	case EWETaskKind::DriveVehicle:   StepDrive(DeltaSeconds); break;
	default:                          SetState(EWEEmployeeState::Idle); break;
	}
}

int32 AWEEmployee::GetAddableAt(const AActor* Destination, const FWEItemInstance& Item) const
{
	AActor* Target = const_cast<AActor*>(Destination);
	if (AWEMachine* Machine = Cast<AWEMachine>(Target))
	{
		// Same rule as a conveyor: only what the machine (its chosen recipe) uses.
		FWEItemInstance One = Item;
		One.Quantity = 1;
		return Machine->CanAcceptFromBelt(One) ? Machine->GetStorage()->GetAddableQuantity(Item) : 0;
	}
	const UWEInventoryComponent* Inventory = GetTakeInventory(Target);
	return Inventory ? Inventory->GetAddableQuantity(Item) : 0;
}

bool AWEEmployee::TakeFrom(AActor* Source, const AActor* Destination, const UWEItemDefinition* OnlyItem)
{
	UWEInventoryComponent* Inventory = GetTakeInventory(Source);
	if (!Inventory || Carried.IsValid())
	{
		return false;
	}
	for (const FWEInventorySlotData& Slot : Inventory->GetAllSlots())
	{
		const FWEItemInstance& Item = Slot.Item;
		if (!Item.Definition || Item.IsCustomerItem() || (OnlyItem && Item.Definition != OnlyItem) || (!OnlyItem && Task.Filter && Item.Definition != Task.Filter))
		{
			continue;
		}
		const float UnitKg = FMath::Max(0.001f, Item.Definition->WeightKg);
		int32 Quantity = 0;
		if (!Item.Definition->bFitsInInventory)
		{
			Quantity = UnitKg <= CarryBulkyKg ? 1 : 0;
		}
		else
		{
			Quantity = FMath::Min(Item.Quantity, FMath::Max(1, FMath::FloorToInt(CarryKg / UnitKg)));
		}
		if (Quantity <= 0)
		{
			continue;
		}
		if (!OnlyItem)
		{
			Quantity = FMath::Min(Quantity, GetAddableAt(Destination, Item));
			if (Quantity <= 0)
			{
				continue; // the target does not take this right now
			}
		}
		FWEItemInstance Taken;
		if (Inventory->RemoveFromSlot(Slot.SlotIndex, Quantity, Taken) == EWEInventoryResult::Success)
		{
			Carried = Taken;
			OnRep_Visuals();
			ForceNetUpdate();
			return true;
		}
	}
	return false;
}

bool AWEEmployee::PutInto(AActor* Target)
{
	if (!Carried.IsValid())
	{
		return true;
	}
	const int32 Addable = Cast<AWEMachine>(Target) ? GetAddableAt(Target, Carried) : (GetTakeInventory(Target) ? GetTakeInventory(Target)->GetAddableQuantity(Carried) : 0);
	UWEInventoryComponent* Inventory = Cast<AWEMachine>(Target) ? Cast<AWEMachine>(Target)->GetStorage() : GetTakeInventory(Target);
	if (!Inventory || Addable <= 0)
	{
		return false;
	}
	FWEItemInstance Part = Carried;
	Part.Quantity = FMath::Min(Carried.Quantity, Addable);
	int32 Added = 0;
	Inventory->TryAddItem(Part, Added);
	Carried.Quantity -= Added;
	if (Carried.Quantity <= 0)
	{
		Carried = FWEItemInstance();
	}
	OnRep_Visuals();
	ForceNetUpdate();
	return !Carried.IsValid();
}

void AWEEmployee::ReturnCarried()
{
	if (!Carried.IsValid())
	{
		return;
	}
	// Back where it came from if possible, otherwise put down at his feet.
	if (UWEInventoryComponent* Inventory = GetTakeInventory(Task.Source))
	{
		int32 Added = 0;
		Inventory->TryAddItem(Carried, Added);
		Carried.Quantity -= Added;
	}
	if (Carried.Quantity > 0 && GetWorld())
	{
		AWEPickupItem::SpawnWorldItem(GetWorld(), Carried, FTransform(GetActorLocation() + GetActorForwardVector() * 60.f - FVector(0.f, 0.f, 60.f)));
	}
	Carried = FWEItemInstance();
	OnRep_Visuals();
}

void AWEEmployee::StepTransport(float DeltaSeconds)
{
	AActor* Source = Task.Source;
	AActor* Target = Task.Target;
	if (!IsValid(Source) || !IsValid(Target))
	{
		SetState(EWEEmployeeState::Waiting, LOCTEXT("Gone", "Quelle oder Ziel fehlt"));
		return;
	}
	if (WaitTimer > 0.f)
	{
		WaitTimer -= DeltaSeconds;
		return;
	}
	switch (Phase)
	{
	case EPhase::GoToSource:
		if (Carried.IsValid())
		{
			Phase = EPhase::GoToTarget;
		}
		else if (WalkTo(Source))
		{
			Phase = EPhase::Take;
			PhaseTimer = HandlingSeconds / Profile.GetSpeedFactor();
			SetState(EWEEmployeeState::Working);
		}
		break;
	case EPhase::Take:
		PhaseTimer -= DeltaSeconds;
		if (PhaseTimer <= 0.f)
		{
			if (TakeFrom(Source, Target))
			{
				Phase = EPhase::GoToTarget;
			}
			else
			{
				SetState(EWEEmployeeState::Waiting, LOCTEXT("SourceEmpty", "nichts Passendes in der Quelle"));
				WaitTimer = RetrySeconds;
				Phase = EPhase::GoToSource;
			}
		}
		break;
	case EPhase::GoToTarget:
		if (WalkTo(Target))
		{
			Phase = EPhase::Put;
			PhaseTimer = HandlingSeconds / Profile.GetSpeedFactor();
			SetState(EWEEmployeeState::Working);
		}
		break;
	case EPhase::Put:
		PhaseTimer -= DeltaSeconds;
		if (PhaseTimer <= 0.f)
		{
			if (PutInto(Target))
			{
				Phase = EPhase::GoToSource;
			}
			else
			{
				SetState(EWEEmployeeState::Waiting, LOCTEXT("TargetFull", "Ziel nimmt nichts an"));
				WaitTimer = RetrySeconds;
			}
		}
		break;
	default:
		Phase = EPhase::GoToSource;
		break;
	}
}

void AWEEmployee::ReleaseMachine()
{
	if (AWEMachine* Machine = OperatedMachine.Get())
	{
		Machine->SetOperatorBonus(1.f);
	}
	OperatedMachine.Reset();
}

void AWEEmployee::StepOperate(float DeltaSeconds)
{
	AWEMachine* Machine = Cast<AWEMachine>(Task.Target);
	AActor* Store = Task.Source;
	if (!IsValid(Machine))
	{
		SetState(EWEEmployeeState::Waiting, LOCTEXT("NoMachine", "Maschine fehlt"));
		return;
	}
	if (WaitTimer > 0.f)
	{
		WaitTimer -= DeltaSeconds;
	}
	const UWEMachineDefinition* Definition = Machine->GetMachineDefinition();
	switch (Phase)
	{
	case EPhase::AtMachine:
	{
		if (!WalkTo(Machine))
		{
			ReleaseMachine();
			return;
		}
		OperatedMachine = Machine;
		Machine->SetOperatorBonus(OperatorBonus);
		if (!Machine->IsOn())
		{
			Machine->SetPower(true);
		}
		// Maintenance first, then clear the output into the store.
		const bool bNeedsKit = Definition && Definition->MaintenanceItem && Machine->GetMaintenance() < WEEmployeeConst::MaintainBelow;
		if (bNeedsKit && IsValid(Store) && WaitTimer <= 0.f)
		{
			const UWEInventoryComponent* StoreInventory = GetTakeInventory(Store);
			if (StoreInventory && StoreInventory->CountItems(Definition->MaintenanceItem) > 0)
			{
				ReleaseMachine();
				Phase = EPhase::FetchKit;
				break;
			}
			SetState(EWEEmployeeState::Waiting, LOCTEXT("NoKit", "kein Wartungsset im Lager"));
			WaitTimer = RetrySeconds;
			break;
		}
		if (IsValid(Store) && !Machine->IsOutputConnected() && Machine->GetOutputStorage()->GetAllSlots().Num() > 0 && WaitTimer <= 0.f)
		{
			if (TakeFrom(Machine, Store))
			{
				ReleaseMachine();
				Phase = EPhase::CarryProducts;
				break;
			}
		}
		SetState(EWEEmployeeState::Working, FText::Format(LOCTEXT("Operating", "bedient {0}"), StaffLabelOf(Machine)));
		break;
	}
	case EPhase::FetchKit:
		if (WalkTo(Store))
		{
			if (!TakeFrom(Store, Machine, Definition ? Definition->MaintenanceItem.Get() : nullptr))
			{
				WaitTimer = RetrySeconds;
			}
			Phase = EPhase::BringKit;
		}
		break;
	case EPhase::BringKit:
		if (WalkTo(Machine))
		{
			if (Carried.IsValid() && Definition && Carried.Definition == Definition->MaintenanceItem)
			{
				Carried.Quantity -= 1;
				if (Carried.Quantity <= 0)
				{
					Carried = FWEItemInstance();
				}
				Machine->MaintainWithKit();
				OnRep_Visuals();
				ForceNetUpdate();
			}
			Phase = EPhase::AtMachine;
		}
		break;
	case EPhase::CarryProducts:
		if (WalkTo(Store))
		{
			if (!PutInto(Store))
			{
				SetState(EWEEmployeeState::Waiting, LOCTEXT("StoreFull", "Lager voll"));
				WaitTimer = RetrySeconds;
				ReturnCarried();
			}
			Phase = EPhase::AtMachine;
		}
		break;
	default:
		Phase = EPhase::AtMachine;
		break;
	}
}

void AWEEmployee::StepDrive(float DeltaSeconds)
{
	AWEVehicle* Vehicle = Cast<AWEVehicle>(Task.Target);
	if (!IsValid(Vehicle) || !Vehicle->IsUnlocked())
	{
		SetState(EWEEmployeeState::Waiting, LOCTEXT("NoVan", "kein Fahrzeug"));
		return;
	}
	if (WalkTo(Vehicle))
	{
		SetState(EWEEmployeeState::Working, LOCTEXT("ReadyToDrive", "bereit für eine Tour"));
	}
}

#undef LOCTEXT_NAMESPACE
