#include "World/WELogoActor.h"

#include "Workshop/WELogoComponent.h"

AWELogoActor::AWELogoActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Logo = CreateDefaultSubobject<UWELogoComponent>(TEXT("Logo"));
	RootComponent = Logo;
}
