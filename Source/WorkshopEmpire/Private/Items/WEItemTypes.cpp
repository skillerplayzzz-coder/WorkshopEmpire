#include "Items/WEItemTypes.h"

#define LOCTEXT_NAMESPACE "WEItems"

// UMETA display names are editor-only metadata, so runtime texts are spelled out here.

FText WEItems::GetConditionText(EWEItemCondition Condition)
{
	switch (Condition)
	{
	case EWEItemCondition::New:       return LOCTEXT("CondNew", "Neu");
	case EWEItemCondition::VeryGood:  return LOCTEXT("CondVeryGood", "Sehr gut");
	case EWEItemCondition::Used:      return LOCTEXT("CondUsed", "Gebraucht");
	case EWEItemCondition::Damaged:   return LOCTEXT("CondDamaged", "Beschädigt");
	case EWEItemCondition::Defective: return LOCTEXT("CondDefective", "Defekt");
	case EWEItemCondition::Scrap:     return LOCTEXT("CondScrap", "Schrott");
	}
	return FText::GetEmpty();
}

FText WEItems::GetCategoryText(EWEItemCategory Category)
{
	switch (Category)
	{
	case EWEItemCategory::Electronics:        return LOCTEXT("CatElectronics", "Elektronik");
	case EWEItemCategory::HouseholdAppliance: return LOCTEXT("CatHousehold", "Haushaltsgerät");
	case EWEItemCategory::PowerTool:          return LOCTEXT("CatPowerTool", "Elektrowerkzeug");
	case EWEItemCategory::Motor:              return LOCTEXT("CatMotor", "Motor");
	case EWEItemCategory::Machine:            return LOCTEXT("CatMachine", "Maschine");
	case EWEItemCategory::VehiclePart:        return LOCTEXT("CatVehiclePart", "Fahrzeugteil");
	case EWEItemCategory::ScrapMetal:         return LOCTEXT("CatScrap", "Metallschrott");
	case EWEItemCategory::RawMaterial:        return LOCTEXT("CatRaw", "Rohstoff");
	case EWEItemCategory::SparePart:          return LOCTEXT("CatSpare", "Ersatzteil");
	case EWEItemCategory::Product:            return LOCTEXT("CatProduct", "Fertigprodukt");
	case EWEItemCategory::Collectible:        return LOCTEXT("CatCollectible", "Sammlerstück");
	case EWEItemCategory::RareFind:           return LOCTEXT("CatRare", "Seltener Fund");
	case EWEItemCategory::Tool:               return LOCTEXT("CatTool", "Werkzeug");
	}
	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE

float WEItems::GetConditionValueFactor(EWEItemCondition Condition)
{
	switch (Condition)
	{
	case EWEItemCondition::New:       return 1.0f;
	case EWEItemCondition::VeryGood:  return 0.8f;
	case EWEItemCondition::Used:      return 0.55f;
	case EWEItemCondition::Damaged:   return 0.3f;
	case EWEItemCondition::Defective: return 0.15f;
	case EWEItemCondition::Scrap:     return 0.05f;
	}
	return 1.f;
}

FLinearColor WEItems::GetConditionColor(EWEItemCondition Condition)
{
	switch (Condition)
	{
	case EWEItemCondition::New:       return FLinearColor(0.25f, 0.85f, 0.35f);
	case EWEItemCondition::VeryGood:  return FLinearColor(0.55f, 0.85f, 0.30f);
	case EWEItemCondition::Used:      return FLinearColor(0.90f, 0.85f, 0.35f);
	case EWEItemCondition::Damaged:   return FLinearColor(0.95f, 0.60f, 0.20f);
	case EWEItemCondition::Defective: return FLinearColor(0.95f, 0.30f, 0.20f);
	case EWEItemCondition::Scrap:     return FLinearColor(0.55f, 0.50f, 0.50f);
	}
	return FLinearColor::White;
}
