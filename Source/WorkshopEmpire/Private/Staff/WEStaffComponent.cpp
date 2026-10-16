#include "Staff/WEStaffComponent.h"

#include "Economy/WECompanyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/WEGameState.h"
#include "Framework/WEPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Logistics/WEVehicle.h"
#include "Net/UnrealNetwork.h"
#include "Player/WECharacter.h"
#include "Staff/WEEmployee.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "WELog.h"

#define LOCTEXT_NAMESPACE "WEStaff"

UWEStaffComponent::UWEStaffComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	static ConstructorHelpers::FObjectFinder<UWEStaffCatalog> DefaultCatalog(TEXT("/Game/WorkshopEmpire/Data/Staff/DA_StaffCatalog.DA_StaffCatalog"));
	static ConstructorHelpers::FObjectFinder<UWETourCatalog> DefaultTours(TEXT("/Game/WorkshopEmpire/Data/Staff/DA_TourCatalog.DA_TourCatalog"));
	Catalog = DefaultCatalog.Object;
	TourCatalog = DefaultTours.Object;
	Random.Initialize(1357);
}

void UWEStaffComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWEStaffComponent, Applicants);
	DOREPLIFETIME(UWEStaffComponent, OwedWages);
	DOREPLIFETIME(UWEStaffComponent, NextPayrollAt);
}

void UWEStaffComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority())
	{
		Random.Initialize(FMath::Rand());
		RefreshApplicants();
		FTimerManager& Timers = GetWorld()->GetTimerManager();
		Timers.SetTimer(StepTimer, this, &UWEStaffComponent::OnStepTimer, StepSeconds, true);
		Timers.SetTimer(PayrollTimer, this, &UWEStaffComponent::OnPayrollTimer, PayrollIntervalSeconds, true);
		Timers.SetTimer(ApplicantTimer, this, &UWEStaffComponent::OnApplicantTimer, ApplicantRefreshSeconds, true);
		NextPayrollAt = GetWorld()->GetTimeSeconds() + PayrollIntervalSeconds;
	}
}

void UWEStaffComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StepTimer);
		World->GetTimerManager().ClearTimer(PayrollTimer);
		World->GetTimerManager().ClearTimer(ApplicantTimer);
	}
	Super::EndPlay(EndPlayReason);
}

UWEStaffComponent* UWEStaffComponent::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AWEGameState* GameState = World ? World->GetGameState<AWEGameState>() : nullptr;
	return GameState ? GameState->GetStaff() : nullptr;
}

TArray<AWEEmployee*> UWEStaffComponent::GetEmployees() const
{
	TArray<AWEEmployee*> Result;
	for (TActorIterator<AWEEmployee> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && !It->IsActorBeingDestroyed())
		{
			Result.Add(*It);
		}
	}
	return Result;
}

FWEMoney UWEStaffComponent::GetWagesPerMinute() const
{
	int64 Cents = 0;
	for (const AWEEmployee* Employee : GetEmployees())
	{
		Cents += Employee->GetProfile().WageCentsPerMinute;
	}
	return FWEMoney::FromCents(Cents);
}

float UWEStaffComponent::GetSecondsToPayroll() const
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const float Now = GameState ? GameState->GetServerWorldTimeSeconds() : 0.f;
	return FMath::Max(0.f, NextPayrollAt - Now);
}

FText UWEStaffComponent::GetRoleDescription(EWEStaffRole Role) const
{
	const FWEStaffRoleDefinition* Definition = Catalog ? Catalog->FindRole(Role) : nullptr;
	return Definition ? Definition->Description : FText::GetEmpty();
}

FWEEmployeeProfile UWEStaffComponent::MakeApplicant()
{
	FWEEmployeeProfile Profile;
	if (!Catalog || Catalog->Roles.Num() == 0)
	{
		return Profile;
	}
	const FWEStaffRoleDefinition& Role = Catalog->Roles[Random.RandRange(0, Catalog->Roles.Num() - 1)];
	const FString First = Catalog->FirstNames.Num() > 0 ? Catalog->FirstNames[Random.RandRange(0, Catalog->FirstNames.Num() - 1)] : TEXT("Alex");
	const FString Last = Catalog->LastNames.Num() > 0 ? Catalog->LastNames[Random.RandRange(0, Catalog->LastNames.Num() - 1)] : TEXT("Muster");
	Profile.Name = FString::Printf(TEXT("%s %s"), *First, *Last);
	Profile.Role = Role.Role;
	// Mostly beginners, sometimes experienced (faster, more expensive).
	const float Roll = Random.FRand();
	Profile.Skill = Roll < 0.6f ? 1 : Roll < 0.9f ? 2 : 3;
	Profile.WageCentsPerMinute = FMath::RoundToInt32(Role.WageCentsPerMinute * (1.f + 0.25f * (Profile.Skill - 1)));
	return Profile;
}

void UWEStaffComponent::RefreshApplicants()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	Applicants.Reset();
	for (int32 i = 0; i < NumApplicants; ++i)
	{
		Applicants.Add(MakeApplicant());
	}
	// Every role at least once, so a player is never stuck without a driver for days.
	if (Catalog && Applicants.Num() >= 3 && Catalog->Roles.Num() >= 3)
	{
		TSet<EWEStaffRole> Seen;
		for (const FWEEmployeeProfile& A : Applicants)
		{
			Seen.Add(A.Role);
		}
		for (int32 i = 0; i < Catalog->Roles.Num() && Seen.Num() < Catalog->Roles.Num(); ++i)
		{
			const EWEStaffRole Missing = Catalog->Roles[i].Role;
			if (!Seen.Contains(Missing))
			{
				for (FWEEmployeeProfile& A : Applicants)
				{
					int32 Same = 0;
					for (const FWEEmployeeProfile& B : Applicants)
					{
						Same += B.Role == A.Role ? 1 : 0;
					}
					if (Same > 1)
					{
						A.Role = Missing;
						A.WageCentsPerMinute = FMath::RoundToInt32(Catalog->Roles[i].WageCentsPerMinute * (1.f + 0.25f * (A.Skill - 1)));
						Seen.Add(Missing);
						break;
					}
				}
			}
		}
	}
	GetOwner()->ForceNetUpdate();
}

FVector UWEStaffComponent::GetSpawnLocation() const
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("WE_StaffSpawn")))
		{
			return It->GetActorLocation();
		}
	}
	return FVector(300.f, 0.f, 100.f);
}

AWEEmployee* UWEStaffComponent::SpawnEmployee(const FWEEmployeeProfile& Profile, const FTransform& Transform, FName ActorName)
{
	UWorld* World = GetWorld();
	if (!World || !GetOwner()->HasAuthority())
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Name = ActorName.IsNone() ? FName(*FString::Printf(TEXT("Employee_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Short))) : ActorName;
	Params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	if (!ActorName.IsNone())
	{
		if (UObject* Old = StaticFindObjectFast(nullptr, World->PersistentLevel, ActorName); Old && (!IsValid(Old) || Cast<AActor>(Old)->IsActorBeingDestroyed()))
		{
			Old->Rename(*MakeUniqueObjectName(World->PersistentLevel, Old->GetClass(), TEXT("FiredEmployee")).ToString(), nullptr,
				REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty);
		}
	}
	AWEEmployee* Employee = World->SpawnActor<AWEEmployee>(AWEEmployee::StaticClass(), Transform, Params);
	if (Employee)
	{
		const FWEStaffRoleDefinition* Role = Catalog ? Catalog->FindRole(Profile.Role) : nullptr;
		Employee->Setup(Profile, Role ? Role->Tint : FLinearColor(0.2f, 0.35f, 0.6f));
	}
	return Employee;
}

EWEStaffResult UWEStaffComponent::Hire(AWECharacter* By, int32 ApplicantIndex, const FGuid& RequestId, AWEEmployee** OutEmployee)
{
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!GetOwner()->HasAuthority() || !Applicants.IsValidIndex(ApplicantIndex) || !Applicants[ApplicantIndex].IsValid() || !Company)
	{
		return EWEStaffResult::Invalid;
	}
	if (Catalog && GetEmployees().Num() >= Catalog->MaxEmployees)
	{
		if (AWEPlayerController* PC = By ? Cast<AWEPlayerController>(By->GetController()) : nullptr)
		{
			PC->ClientShowMessage(FText::Format(LOCTEXT("TooMany", "Mehr als {0} Mitarbeiter passen nicht in die Werkstatt"), FText::AsNumber(Catalog->MaxEmployees)));
		}
		return EWEStaffResult::TooManyEmployees;
	}
	if (!Company->RegisterRequest(RequestId))
	{
		return EWEStaffResult::Duplicate;
	}
	const FWEEmployeeProfile Profile = Applicants[ApplicantIndex];
	const FVector Spawn = GetSpawnLocation() + FVector(Random.FRandRange(-40.f, 40.f), Random.FRandRange(-60.f, 60.f), 0.f);
	AWEEmployee* Employee = SpawnEmployee(Profile, FTransform(FRotator(0.f, 180.f, 0.f), Spawn));
	if (!Employee)
	{
		return EWEStaffResult::Invalid;
	}
	Applicants[ApplicantIndex] = MakeApplicant();
	GetOwner()->ForceNetUpdate();
	if (AWEPlayerController* PC = By ? Cast<AWEPlayerController>(By->GetController()) : nullptr)
	{
		PC->ClientShowMessage(FText::Format(LOCTEXT("Hired", "{0} eingestellt ({1}) – am Tor. [E] auf {0}: Aufgabe zuweisen"),
			FText::FromString(Profile.Name), AWEEmployee::GetRoleText(Profile.Role)));
	}
	UE_LOG(LogWENet, Log, TEXT("%s hired %s (%s, skill %d, %d ct/min)"), By ? *By->GetName() : TEXT("?"), *Profile.Name,
		*AWEEmployee::GetRoleText(Profile.Role).ToString(), Profile.Skill, Profile.WageCentsPerMinute);
	if (OutEmployee)
	{
		*OutEmployee = Employee;
	}
	return EWEStaffResult::Success;
}

bool UWEStaffComponent::Fire(AWEEmployee* Employee, AWECharacter* By)
{
	if (!GetOwner()->HasAuthority() || !IsValid(Employee) || Employee->GetState() == EWEEmployeeState::Away)
	{
		return false;
	}
	// The time worked is still paid at the next payroll.
	if (UWECompanyComponent* Company = UWECompanyComponent::Get(this))
	{
		const int64 Earned = FMath::RoundToInt64(Employee->GetUnpaidSeconds() / 60.f * Employee->GetProfile().WageCentsPerMinute);
		OwedWages = FWEMoney::FromCents(OwedWages.GetCents() + Earned);
	}
	Employee->ClearTask(); // puts back what he carries
	if (AWEPlayerController* PC = By ? Cast<AWEPlayerController>(By->GetController()) : nullptr)
	{
		PC->ClientShowMessage(FText::Format(LOCTEXT("Fired", "{0} entlassen"), FText::FromString(Employee->GetProfile().Name)));
	}
	UE_LOG(LogWENet, Log, TEXT("%s fired %s"), By ? *By->GetName() : TEXT("?"), *Employee->GetProfile().Name);
	Employee->Destroy();
	GetOwner()->ForceNetUpdate();
	return true;
}

bool UWEStaffComponent::Payroll()
{
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!GetOwner()->HasAuthority() || !Company)
	{
		return false;
	}
	NextPayrollAt = GetWorld()->GetTimeSeconds() + PayrollIntervalSeconds;
	const TArray<AWEEmployee*> Employees = GetEmployees();
	int64 Cents = OwedWages.GetCents();
	for (const AWEEmployee* Employee : Employees)
	{
		Cents += FMath::RoundToInt64(Employee->GetUnpaidSeconds() / 60.f * Employee->GetProfile().WageCentsPerMinute);
	}
	if (Cents <= 0)
	{
		for (AWEEmployee* Employee : Employees)
		{
			Employee->SetUnpaid(false);
		}
		return true;
	}
	const FWEMoney Total = FWEMoney::FromCents(Cents);
	const bool bPaid = Company->CanAfford(Total) && Company->TryWithdraw(Total, FString::Printf(TEXT("Löhne (%d Mitarbeiter)"), Employees.Num()), FString());
	for (AWEEmployee* Employee : Employees)
	{
		Employee->ResetUnpaidSeconds();
		Employee->SetUnpaid(!bPaid);
	}
	OwedWages = bPaid ? FWEMoney() : Total;
	GetOwner()->ForceNetUpdate();
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Payroll: %s %s"), *Total.ToDisplayString(), bPaid ? TEXT("paid") : TEXT("NOT paid (not enough money)"));
	if (!bPaid)
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (AWEPlayerController* PC = Cast<AWEPlayerController>(It->Get()))
			{
				PC->ClientShowMessage(FText::Format(LOCTEXT("Unpaid", "Löhne ({0}) nicht bezahlt – die Mitarbeiter legen die Arbeit nieder"), FText::FromString(Total.ToDisplayString())));
			}
		}
	}
	return bPaid;
}

void UWEStaffComponent::Step(float DeltaSeconds)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	for (AWEEmployee* Employee : GetEmployees())
	{
		Employee->StepWork(DeltaSeconds);
	}
	for (TActorIterator<AWEVehicle> It(GetWorld()); It; ++It)
	{
		It->StepTour(DeltaSeconds);
	}
}

void UWEStaffComponent::RestoreState(const TArray<FWEEmployeeProfile>& InApplicants, const FWEMoney& InOwed)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	if (InApplicants.Num() > 0)
	{
		Applicants = InApplicants;
	}
	OwedWages = InOwed;
	GetOwner()->ForceNetUpdate();
}

void UWEStaffComponent::OnStepTimer()
{
	Step(StepSeconds);
}

void UWEStaffComponent::OnPayrollTimer()
{
	Payroll();
}

void UWEStaffComponent::OnApplicantTimer()
{
	RefreshApplicants();
}

#undef LOCTEXT_NAMESPACE
