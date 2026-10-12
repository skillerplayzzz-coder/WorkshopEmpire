#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WEInteractable.generated.h"

class AWECharacter;

/** Data shown in the [F] "Untersuchen" panel. */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEExamineInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Examine")
	FText Title;

	UPROPERTY(BlueprintReadOnly, Category = "Examine")
	FText Description;

	/** Additional "Label: Value" lines (weight, condition, ...). */
	UPROPERTY(BlueprintReadOnly, Category = "Examine")
	TArray<FText> Details;
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UWEInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything the player can look at and use with [E] (and optionally examine with [F]).
 *
 * Network contract:
 *  - GetInteractionText / CanInteract / GetExamineInfo are called on the owning client to build
 *    prompts, and again on the server to validate. They must only read replicated or map data.
 *  - Interact is called on the server only (via UWEInteractionComponent::ServerInteract).
 */
class WORKSHOPEMPIRE_API IWEInteractable
{
	GENERATED_BODY()

public:
	/** Verb shown after the key, e.g. "Aufheben" or "Tor öffnen". */
	virtual FText GetInteractionText(const AWECharacter* Interactor) const = 0;

	/** Whether Interactor may use this right now. OutReason is shown instead of the prompt when false. */
	virtual bool CanInteract(const AWECharacter* Interactor, FText& OutReason) const = 0;

	/** Server only. Performs the interaction after validation. */
	virtual void Interact(AWECharacter* Interactor) = 0;

	/** Fills the examine panel. Return false if this object cannot be examined. */
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const { return false; }

	/** Optional second action on [R] (e.g. "Vom Tisch nehmen"). Return false if there is none right now. */
	virtual bool GetSecondaryInteraction(const AWECharacter* Interactor, FText& OutText) const { return false; }

	/** Server only. Performs the secondary action after validation. */
	virtual void InteractSecondary(AWECharacter* Interactor) {}
};
