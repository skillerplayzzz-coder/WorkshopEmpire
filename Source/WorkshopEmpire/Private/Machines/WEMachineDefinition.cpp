#include "Machines/WEMachineDefinition.h"

#include "Items/WEItemDefinition.h"

bool FWEMachineRecipe::UsesItem(const UWEItemDefinition* Item) const
{
	if (!Item)
	{
		return false;
	}
	if (SecondInputItem && SecondInputItem == Item)
	{
		return true;
	}
	return InputItem ? InputItem == Item : InputCategories.Contains(Item->Category);
}

bool UWEMachineDefinition::AcceptsItem(const UWEItemDefinition* Item) const
{
	for (const FWEMachineRecipe& Recipe : Recipes)
	{
		if (Recipe.UsesItem(Item))
		{
			return true;
		}
	}
	return false;
}
