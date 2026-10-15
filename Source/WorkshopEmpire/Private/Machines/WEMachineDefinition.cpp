#include "Machines/WEMachineDefinition.h"

#include "Items/WEItemDefinition.h"

bool UWEMachineDefinition::AcceptsItem(const UWEItemDefinition* Item) const
{
	if (!Item)
	{
		return false;
	}
	for (const FWEMachineRecipe& Recipe : Recipes)
	{
		if (Recipe.InputItem ? Recipe.InputItem == Item : Recipe.InputCategories.Contains(Item->Category))
		{
			return true;
		}
	}
	return false;
}
