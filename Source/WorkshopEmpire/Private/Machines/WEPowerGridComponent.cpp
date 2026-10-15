#include "Machines/WEPowerGridComponent.h"

#include "Economy/WECompanyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/WEGameState.h"
#include "Machines/WEMachine.h"
#include "Machines/WEMachineDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Progression/WEProgressionComponent.h"
#include "TimerManager.h"
#include "WELog.h"

UWEPowerGridComponent::UWEPowerGridComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UWEPowerGridComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWEPowerGridComponent, LoadKw);
	DOREPLIFETIME(UWEPowerGridComponent, UnbilledKWh);
	DOREPLIFETIME(UWEPowerGridComponent, LastBill);
	DOREPLIFETIME(UWEPowerGridComponent, MachineCount);
}

void UWEPowerGridComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(StepTimer, this, &UWEPowerGridComponent::OnStepTimer, StepSeconds, true);
		GetWorld()->GetTimerManager().SetTimer(BillTimer, this, &UWEPowerGridComponent::OnBillTimer, BillIntervalSeconds, true);
	}
}

void UWEPowerGridComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StepTimer);
		World->GetTimerManager().ClearTimer(BillTimer);
	}
	Super::EndPlay(EndPlayReason);
}

UWEPowerGridComponent* UWEPowerGridComponent::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AWEGameState* GameState = World ? World->GetGameState<AWEGameState>() : nullptr;
	return GameState ? GameState->GetPowerGrid() : nullptr;
}

float UWEPowerGridComponent::GetCapacityKw() const
{
	const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	return BaseCapacityKw + (Progression ? Progression->GetPowerCapacityBonusKw() : 0.f);
}

void UWEPowerGridComponent::OnStepTimer()
{
	StepMachines(StepSeconds);
}

void UWEPowerGridComponent::StepMachines(float DeltaSeconds)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	TArray<AWEMachine*> Machines;
	for (TActorIterator<AWEMachine> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && !It->IsActorBeingDestroyed() && It->GetMachineDefinition())
		{
			Machines.Add(*It);
		}
	}

	// 1) Running cycles advance. 2) Count the load. 3) Waiting machines start while there is power.
	for (AWEMachine* Machine : Machines)
	{
		Machine->AdvanceCycle(DeltaSeconds);
	}
	float Load = 0.f;
	for (AWEMachine* Machine : Machines)
	{
		Load += Machine->IsRunning() ? Machine->GetPowerDrawKw() : 0.f;
	}
	const float Capacity = GetCapacityKw();
	for (AWEMachine* Machine : Machines)
	{
		if (Machine->IsRunning() || !Machine->IsOn())
		{
			continue;
		}
		const int32 Recipe = Machine->FindStartableRecipe();
		if (Recipe == INDEX_NONE)
		{
			continue;
		}
		const float Needed = Machine->GetMachineDefinition()->PowerKw;
		if (Load + Needed > Capacity + KINDA_SMALL_NUMBER)
		{
			Machine->SetWaitingForPower();
			continue;
		}
		Machine->StartCycle(Recipe);
		Load += Machine->IsRunning() ? Needed : 0.f;
	}
	// Machines that are on but wait draw their idle power.
	float Total = 0.f;
	for (AWEMachine* Machine : Machines)
	{
		Total += Machine->GetPowerDrawKw();
	}
	LoadKw = Total;
	MachineCount = Machines.Num();
	UnbilledKWh += Total * DeltaSeconds / 3600.f;
}

void UWEPowerGridComponent::OnBillTimer()
{
	Bill();
}

FWEMoney UWEPowerGridComponent::Bill()
{
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!GetOwner()->HasAuthority() || !Company)
	{
		return FWEMoney();
	}
	const double Exact = UnbilledKWh * CentsPerKWh + CentRemainder;
	const int64 Cents = static_cast<int64>(Exact);
	CentRemainder = Exact - Cents;
	const float KWh = UnbilledKWh;
	UnbilledKWh = 0.f;
	if (Cents <= 0)
	{
		return FWEMoney();
	}
	// The account never goes below zero: an unpaid rest is simply lost to the utility (kept simple).
	const FWEMoney Amount = FWEMoney::FromCents(FMath::Min(Cents, Company->GetBalance().GetCents()));
	if (!Amount.IsZero())
	{
		Company->TryWithdraw(Amount, FString::Printf(TEXT("Stromrechnung (%.2f kWh)"), KWh), FString());
	}
	LastBill = Amount;
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Power bill: %.3f kWh -> %s"), KWh, *Amount.ToDisplayString());
	return Amount;
}
