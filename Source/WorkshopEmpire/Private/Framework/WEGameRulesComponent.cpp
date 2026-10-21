#include "Framework/WEGameRulesComponent.h"

#include "Framework/WEGameState.h"
#include "Net/UnrealNetwork.h"
#include "WELog.h"

#define LOCTEXT_NAMESPACE "WEGameRules"

FWEGameRules FWEGameRules::FromPreset(EWEDifficultyPreset InPreset)
{
	FWEGameRules R;
	R.Preset = InPreset;
	R.bCourier = true;
	switch (InPreset)
	{
	case EWEDifficultyPreset::Relaxed:
		R.bDiagnosisRequired = false;
		R.bRepairMistakes = false;
		R.bCustomerReviews = false;
		R.Economy = EWEEconomyLevel::Relaxed;
		R.Complexity = EWERepairComplexity::Simple;
		break;
	case EWEDifficultyPreset::Expert:
		R.bDiagnosisRequired = true;
		R.bRepairMistakes = true;
		R.bCustomerReviews = true;
		R.Economy = EWEEconomyLevel::Hard;
		R.Complexity = EWERepairComplexity::Detailed;
		break;
	case EWEDifficultyPreset::Realistic:
	case EWEDifficultyPreset::Custom:
	default:
		R.bDiagnosisRequired = true;
		R.bRepairMistakes = true;
		R.bCustomerReviews = true;
		R.Economy = EWEEconomyLevel::Normal;
		R.Complexity = EWERepairComplexity::Normal;
		break;
	}
	return R;
}

EWEDifficultyPreset FWEGameRules::DetectPreset() const
{
	for (const EWEDifficultyPreset P : { EWEDifficultyPreset::Relaxed, EWEDifficultyPreset::Realistic, EWEDifficultyPreset::Expert })
	{
		FWEGameRules Candidate = FromPreset(P);
		Candidate.bCourier = bCourier;
		Candidate.Preset = Preset;
		if (Candidate == *this)
		{
			return P;
		}
	}
	return EWEDifficultyPreset::Custom;
}

float FWEGameRules::BuyPriceFactor() const
{
	return Economy == EWEEconomyLevel::Relaxed ? 0.85f : Economy == EWEEconomyLevel::Hard ? 1.15f : 1.f;
}

float FWEGameRules::SellPriceFactor() const
{
	return Economy == EWEEconomyLevel::Relaxed ? 1.1f : Economy == EWEEconomyLevel::Hard ? 0.9f : 1.f;
}

float FWEGameRules::RepairTimeFactor() const
{
	return Complexity == EWERepairComplexity::Simple ? 0.7f : Complexity == EWERepairComplexity::Detailed ? 1.3f : 1.f;
}

float FWEGameRules::MistakeChance() const
{
	if (!bRepairMistakes)
	{
		return 0.f;
	}
	return Complexity == EWERepairComplexity::Detailed ? 0.12f : Complexity == EWERepairComplexity::Simple ? 0.03f : 0.07f;
}

FString FWEGameRules::ToOption() const
{
	// Digits in a fixed order: preset, diagnosis, mistakes, reviews, economy, complexity, courier.
	return FString::Printf(TEXT("%d%d%d%d%d%d%d"), static_cast<int32>(Preset), bDiagnosisRequired ? 1 : 0, bRepairMistakes ? 1 : 0, bCustomerReviews ? 1 : 0,
		static_cast<int32>(Economy), static_cast<int32>(Complexity), bCourier ? 1 : 0);
}

bool FWEGameRules::FromOption(const FString& Option, FWEGameRules& Out)
{
	if (Option.Len() != 7)
	{
		return false;
	}
	auto Digit = [&Option](int32 i) { return FChar::IsDigit(Option[i]) ? Option[i] - TEXT('0') : -1; };
	for (int32 i = 0; i < 7; ++i)
	{
		if (Digit(i) < 0)
		{
			return false;
		}
	}
	Out.Preset = static_cast<EWEDifficultyPreset>(FMath::Clamp(Digit(0), 0, 3));
	Out.bDiagnosisRequired = Digit(1) != 0;
	Out.bRepairMistakes = Digit(2) != 0;
	Out.bCustomerReviews = Digit(3) != 0;
	Out.Economy = static_cast<EWEEconomyLevel>(FMath::Clamp(Digit(4), 0, 2));
	Out.Complexity = static_cast<EWERepairComplexity>(FMath::Clamp(Digit(5), 0, 2));
	Out.bCourier = Digit(6) != 0;
	return true;
}

FText FWEGameRules::GetPresetName(EWEDifficultyPreset InPreset)
{
	switch (InPreset)
	{
	case EWEDifficultyPreset::Relaxed:   return LOCTEXT("Relaxed", "Entspannt");
	case EWEDifficultyPreset::Realistic: return LOCTEXT("Realistic", "Realistisch");
	case EWEDifficultyPreset::Expert:    return LOCTEXT("Expert", "Experte");
	default:                             return LOCTEXT("Custom", "Individuell");
	}
}

FText FWEGameRules::GetPresetDescription(EWEDifficultyPreset InPreset)
{
	switch (InPreset)
	{
	case EWEDifficultyPreset::Relaxed:
		return LOCTEXT("RelaxedDesc", "Fehler sind sofort bekannt, nichts geht schief, keine Bewertungen, günstige Preise, kurze Reparaturen.");
	case EWEDifficultyPreset::Realistic:
		return LOCTEXT("RealisticDesc", "Fehler suchen, ab und zu rutscht der Schraubendreher ab, Kunden bewerten, normale Preise.");
	case EWEDifficultyPreset::Expert:
		return LOCTEXT("ExpertDesc", "Wie Realistisch, dazu knappe Preise, längere Reparaturen, öfter mehrere Fehler und mehr Fehlgriffe.");
	default:
		return LOCTEXT("CustomDesc", "Jede Option einzeln gewählt.");
	}
}

FText FWEGameRules::GetEconomyName(EWEEconomyLevel Level)
{
	switch (Level)
	{
	case EWEEconomyLevel::Relaxed: return LOCTEXT("EcoRelaxed", "Entspannt");
	case EWEEconomyLevel::Hard:    return LOCTEXT("EcoHard", "Schwer");
	default:                       return LOCTEXT("EcoNormal", "Normal");
	}
}

FText FWEGameRules::GetComplexityName(EWERepairComplexity Level)
{
	switch (Level)
	{
	case EWERepairComplexity::Simple:   return LOCTEXT("CxSimple", "Einfach");
	case EWERepairComplexity::Detailed: return LOCTEXT("CxDetailed", "Detailliert");
	default:                            return LOCTEXT("CxNormal", "Normal");
	}
}

FText FWEGameRules::Describe() const
{
	const EWEDifficultyPreset Detected = DetectPreset();
	if (Detected != EWEDifficultyPreset::Custom)
	{
		return GetPresetName(Detected);
	}
	auto OnOff = [](bool b) { return b ? LOCTEXT("On", "an") : LOCTEXT("Off", "aus"); };
	return FText::Format(LOCTEXT("CustomLine", "Individuell: Diagnose {0}, Fehler {1}, Bewertungen {2}, Wirtschaft {3}, Reparaturen {4}"),
		OnOff(bDiagnosisRequired), OnOff(bRepairMistakes), OnOff(bCustomerReviews), GetEconomyName(Economy), GetComplexityName(Complexity));
}

UWEGameRulesComponent::UWEGameRulesComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UWEGameRulesComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWEGameRulesComponent, Rules);
}

UWEGameRulesComponent* UWEGameRulesComponent::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AWEGameState* GameState = World ? World->GetGameState<AWEGameState>() : nullptr;
	return GameState ? GameState->GetRules() : nullptr;
}

FWEGameRules UWEGameRulesComponent::GetRules(const UObject* WorldContext)
{
	const UWEGameRulesComponent* Component = Get(WorldContext);
	return Component ? Component->Rules : FWEGameRules();
}

void UWEGameRulesComponent::SetRules(const FWEGameRules& InRules)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	Rules = InRules;
	GetOwner()->ForceNetUpdate();
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Game rules: %s (%s)"), *Rules.Describe().ToString(), *Rules.ToOption());
}

#undef LOCTEXT_NAMESPACE
