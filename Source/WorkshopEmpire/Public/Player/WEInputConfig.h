#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputCoreTypes.h"
#include "WEInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * Input actions and default key mapping of the first-person character.
 *
 * Can be authored as a Data Asset later (for remapping / gamepad); until then
 * CreateDefault() builds the same setup at runtime so the project needs no binary input assets.
 */
UCLASS(BlueprintType)
class WORKSHOPEMPIRE_API UWEInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Builds actions + mapping context with the default keyboard/mouse layout. */
	static UWEInputConfig* CreateDefault(UObject* Outer);

	/** First keyboard/mouse key mapped to Action in MappingContext, or EKeys::Invalid. */
	FKey GetKeyForAction(const UInputAction* Action) const;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ExamineAction;
};
