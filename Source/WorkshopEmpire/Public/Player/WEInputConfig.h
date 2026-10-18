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

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> SecondaryAction;

	/** [Q]: alternative action, e.g. the next repair step on the workbench. */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> AlternativeAction;

	/** [B]: build mode on/off (Phase 8). */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> BuildAction;

	/** [X]: in build mode remove furniture / cancel moving. */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> RemoveAction;

	/** Mouse wheel: in build mode choose the furniture. */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> CycleAction;

	/** [T]: setting of the focused object (machine recipe, storage output). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SettingAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> InventoryAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> QuickSaveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> QuickLoadAction;

	/** [Esc]: in-game menu with players, invite, save, leave (Phase 13). */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> PauseAction;

	/** Phase 18c: [V] first / third person. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ViewAction;
};
