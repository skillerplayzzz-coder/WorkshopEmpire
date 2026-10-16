#pragma once

#include "CoreMinimal.h"
#include "World/WEPlaceholderProp.h"
#include "WEProductionStorage.generated.h"

class UWEItemDefinition;
struct FWEItemInstance;

/**
 * Material store / Produktionslager (Phase 10): a big storage box in a production line. Conveyors
 * deliver into it like into any storage; unlike a shelf it also feeds a conveyor whose start touches it
 * ("Ausgabe"): everything, only one material ([T] chooses), or nothing.
 * Takes bulky items too (bales).
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEProductionStorage : public AWEPlaceholderProp
{
	GENERATED_BODY()

public:
	AWEProductionStorage();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IWEInteractable
	virtual bool GetSettingInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const override;
	virtual void InteractSetting(AWECharacter* Interactor) override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;

	/** "Alles", "Aus" or the material name. */
	FText GetOutputText() const;
	bool IsOutputOn() const { return bOutputOn; }
	const UWEItemDefinition* GetOutputFilter() const { return OutputFilter; }

	// --- Server ---------------------------------------------------------------------------------------

	/** Everything -> each material in the store -> off -> everything. */
	void CycleOutput();
	void SetOutput(bool bInOn, UWEItemDefinition* Filter);
	/**
	 * Material flow: the first stack that passes the filter for a conveyor at the output. If the line
	 * ends at a machine, only what that machine takes right now leaves the store (no clogged belts).
	 */
	bool TakeForBelt(FWEItemInstance& OutItem, const AWEPlaceholderProp* LineDestination = nullptr);

protected:
	virtual void RebuildVisuals() override;

	UPROPERTY(Replicated)
	bool bOutputOn = true;

	/** Only this material goes out (None = everything). */
	UPROPERTY(Replicated)
	TObjectPtr<UWEItemDefinition> OutputFilter;
};
