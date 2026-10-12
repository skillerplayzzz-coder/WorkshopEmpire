#include "Economy/WETraderDefinition.h"

float UWETraderDefinition::GetBuyMultiplier(EWEItemCategory Category) const
{
	for (const FWETraderCategoryRate& Rate : Buys)
	{
		if (Rate.Category == Category)
		{
			return Rate.Multiplier;
		}
	}
	return 0.f;
}
