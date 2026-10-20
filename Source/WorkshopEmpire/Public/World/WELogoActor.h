#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WELogoActor.generated.h"

/** Phase 20: the company logo placed in the map (above the garage door, on the company sign). Faces its +X. */
UCLASS()
class WORKSHOPEMPIRE_API AWELogoActor : public AActor
{
	GENERATED_BODY()

public:
	AWELogoActor();

	UPROPERTY(VisibleAnywhere, Category = "Logo")
	TObjectPtr<class UWELogoComponent> Logo;
};
