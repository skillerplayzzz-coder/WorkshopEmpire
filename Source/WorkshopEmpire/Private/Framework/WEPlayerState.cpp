#include "Framework/WEPlayerState.h"

#include "Inventory/WEInventoryComponent.h"
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
		ApplyInventoryBonus(Progression->GetInventoryExtraKg());
	}
}

void AWEPlayerState::ApplyInventoryBonus(float ExtraKg)
{
	Inventory->Configure(NSLOCTEXT("WEPlayerState", "Inventory", "Inventar"), BaseSlots, BaseWeightKg + FMath::Max(0.f, ExtraKg));
	Inventory->NotifyChanged();
}
