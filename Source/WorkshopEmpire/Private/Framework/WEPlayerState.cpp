#include "Framework/WEPlayerState.h"

#include "Inventory/WEInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Progression/WEProgressionComponent.h"

AWEPlayerState::AWEPlayerState()
{
	Inventory = CreateDefaultSubobject<UWEInventoryComponent>(TEXT("Inventory"));
	Inventory->Configure(NSLOCTEXT("WEPlayerState", "Inventory", "Inventar"), BaseSlots, BaseWeightKg);
}

void AWEPlayerState::BeginPlay()
{
	Super::BeginPlay();
	// Players joining later get the company's inventory upgrades right away.
	if (const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this))
	{
		ApplyInventoryBonus(Progression->GetInventoryExtraKg(), Progression->GetInventoryExtraSlots(), Progression->AreToolsWeightless());
	}
}

void AWEPlayerState::ApplyInventoryBonus(float ExtraKg, int32 ExtraSlots, bool bToolsWeightless)
{
	Inventory->Configure(NSLOCTEXT("WEPlayerState", "Inventory", "Inventar"), BaseSlots + FMath::Max(0, ExtraSlots), BaseWeightKg + FMath::Max(0.f, ExtraKg));
	Inventory->SetToolsWeightless(bToolsWeightless);
	Inventory->NotifyChanged();
}

void AWEPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWEPlayerState, bIsHost);
}
