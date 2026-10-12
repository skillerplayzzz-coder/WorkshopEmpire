#include "Framework/WEPlayerState.h"

#include "Inventory/WEInventoryComponent.h"

AWEPlayerState::AWEPlayerState()
{
	Inventory = CreateDefaultSubobject<UWEInventoryComponent>(TEXT("Inventory"));
	Inventory->Configure(NSLOCTEXT("WEPlayerState", "Inventory", "Inventar"), 20, 25.f);
}
