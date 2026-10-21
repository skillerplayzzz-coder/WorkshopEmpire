#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WEGameRulesComponent.generated.h"

UENUM(BlueprintType)
enum class EWEDifficultyPreset : uint8
{
	Relaxed,
	Realistic,
	Expert,
	/** Options picked one by one (also: saves from before Phase 21). */
	Custom
};

UENUM(BlueprintType)
enum class EWEEconomyLevel : uint8
{
	Relaxed,
	Normal,
	Hard
};

UENUM(BlueprintType)
enum class EWERepairComplexity : uint8
{
	Simple,
	Normal,
	Detailed
};

/**
 * Phase 21: the rules of one company, chosen when it is founded (difficulty). Saved with the game and replicated.
 * The defaults are the game as it was before Phase 21 (also used by old saves and test worlds).
 */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEGameRules
{
	GENERATED_BODY()

	UPROPERTY() EWEDifficultyPreset Preset = EWEDifficultyPreset::Custom;
	/** Off: faults are known as soon as the device lies on the bench (no search needed). */
	UPROPERTY() bool bDiagnosisRequired = true;
	/** On: a slipping screwdriver or a plug that did not click in - the step has to be done again. */
	UPROPERTY() bool bRepairMistakes = false;
	/** On: customers rate the work; the company's reputation brings more and better orders. */
	UPROPERTY() bool bCustomerReviews = true;
	UPROPERTY() EWEEconomyLevel Economy = EWEEconomyLevel::Normal;
	UPROPERTY() EWERepairComplexity Complexity = EWERepairComplexity::Normal;
	/** On: orders from the mail-order traders come by parcel courier instead of appearing at once. */
	UPROPERTY() bool bCourier = false;

	static FWEGameRules FromPreset(EWEDifficultyPreset InPreset);
	/** The preset these options match (Custom if none). */
	EWEDifficultyPreset DetectPreset() const;

	/** Prices: what buying costs and selling brings, compared with Normal. */
	float BuyPriceFactor() const;
	float SellPriceFactor() const;
	/** Repair and disassembly step times. */
	float RepairTimeFactor() const;
	/** Chance that a remove/install step goes wrong (before the tool's quality). 0 without mistakes. */
	float MistakeChance() const;

	/** "?WERules=..." value for the travel URL and back. */
	FString ToOption() const;
	static bool FromOption(const FString& Option, FWEGameRules& Out);

	static FText GetPresetName(EWEDifficultyPreset InPreset);
	static FText GetPresetDescription(EWEDifficultyPreset InPreset);
	static FText GetEconomyName(EWEEconomyLevel Level);
	static FText GetComplexityName(EWERepairComplexity Level);
	/** One line for the save list / pause menu: "Realistisch" or "Individuell: …". */
	FText Describe() const;

	bool operator==(const FWEGameRules& O) const
	{
		return bDiagnosisRequired == O.bDiagnosisRequired && bRepairMistakes == O.bRepairMistakes && bCustomerReviews == O.bCustomerReviews
			&& Economy == O.Economy && Complexity == O.Complexity && bCourier == O.bCourier && Preset == O.Preset;
	}
	bool operator!=(const FWEGameRules& O) const { return !(*this == O); }
};

/** Phase 21: holds the company's rules on the game state (server sets them, everybody reads them). */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWEGameRulesComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEGameRulesComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static UWEGameRulesComponent* Get(const UObject* WorldContext);
	/** The rules of the world WorldContext is in (defaults when there is none). */
	static FWEGameRules GetRules(const UObject* WorldContext);

	const FWEGameRules& GetCurrent() const { return Rules; }
	/** Server: new rules (a new company, or a loaded save). */
	void SetRules(const FWEGameRules& InRules);

protected:
	UPROPERTY(Replicated)
	FWEGameRules Rules;
};
