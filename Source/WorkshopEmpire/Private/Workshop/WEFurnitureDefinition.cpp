#include "Workshop/WEFurnitureDefinition.h"

#define LOCTEXT_NAMESPACE "WEFurniture"

EWEFurnitureCategory WEFurniture::GetCategory(const FWEFurnitureDefinition& Definition)
{
	if (Definition.bWorkbench || Definition.PropType == EWEPlaceholderPropType::Workbench)
	{
		return EWEFurnitureCategory::Workbenches;
	}
	if (Definition.Machine)
	{
		return EWEFurnitureCategory::Machines;
	}
	if (Definition.PropType == EWEPlaceholderPropType::Conveyor || Definition.bProductionStorage)
	{
		return EWEFurnitureCategory::Equipment;
	}
	return Definition.bHasStorage ? EWEFurnitureCategory::Storage : EWEFurnitureCategory::Decoration;
}

FText WEFurniture::GetCategoryName(EWEFurnitureCategory Category)
{
	switch (Category)
	{
	case EWEFurnitureCategory::Workbenches: return LOCTEXT("Workbenches", "Werkbänke");
	case EWEFurnitureCategory::Storage:     return LOCTEXT("Storage", "Lagerung");
	case EWEFurnitureCategory::Machines:    return LOCTEXT("Machines", "Maschinen");
	case EWEFurnitureCategory::Equipment:   return LOCTEXT("Equipment", "Werkstattausstattung");
	case EWEFurnitureCategory::Decoration:  return LOCTEXT("Decoration", "Dekoration");
	}
	return FText::GetEmpty();
}

FName WEFurniture::GetCategoryIcon(EWEFurnitureCategory Category)
{
	switch (Category)
	{
	case EWEFurnitureCategory::Workbenches: return TEXT("Hammer");
	case EWEFurnitureCategory::Storage:     return TEXT("Box");
	case EWEFurnitureCategory::Machines:    return TEXT("Factory");
	case EWEFurnitureCategory::Equipment:   return TEXT("Gear");
	case EWEFurnitureCategory::Decoration:  return TEXT("Lamp");
	}
	return TEXT("Box");
}

#undef LOCTEXT_NAMESPACE
