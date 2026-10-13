#include "Items/WEItemInstance.h"

#include "Items/WEItemDefinition.h"

FWEItemInstance FWEItemInstance::Make(UWEItemDefinition* InDefinition, int32 InQuantity, EWEItemCondition InCondition, uint8 InQuality)
{
	FWEItemInstance Item;
	Item.Definition = InDefinition;
	Item.InstanceId = FGuid::NewGuid();
	Item.Quantity = FMath::Max(1, InQuantity);
	Item.Condition = InCondition;
	Item.Quality = FMath::Min<uint8>(InQuality, 100);
	return Item;
}

bool FWEItemInstance::CanStackWith(const FWEItemInstance& Other) const
{
	return Definition && Definition == Other.Definition && Definition->bStackable && Condition == Other.Condition
		&& DisassemblyStep == Other.DisassemblyStep && Repair == Other.Repair;
}

float FWEItemInstance::GetUnitWeightKg() const
{
	return Definition ? Definition->WeightKg : 0.f;
}

FWEMoney FWEItemInstance::GetEstimatedValue() const
{
	if (!Definition)
	{
		return FWEMoney();
	}
	double Factor = WEItems::GetConditionValueFactor(Condition) * (0.75 + 0.5 * (Quality / 100.0));
	if (IsBroken())
	{
		// A broken complex device is worth much less than a working one (repairing must pay off).
		Factor *= Definition->BrokenValueMultiplier;
	}
	const double Cents = static_cast<double>(Definition->BaseValue.GetCents()) * Factor * Quantity;
	return FWEMoney::FromCents(static_cast<int64>(FMath::RoundToDouble(Cents)));
}

FText FWEItemInstance::GetDisplayName() const
{
	if (!Definition)
	{
		return FText::GetEmpty();
	}
	if (IsPartiallyDisassembled())
	{
		return FText::Format(NSLOCTEXT("WEItemInstance", "PartlyDisassembled", "{0} (teilweise zerlegt)"), Definition->DisplayName);
	}
	if (Repair.IsInProgress())
	{
		return FText::Format(NSLOCTEXT("WEItemInstance", "InRepair", "{0} (in Reparatur)"), Definition->DisplayName);
	}
	return Definition->DisplayName;
}

FWEItemInstance FWEItemInstance::SplitOff(int32 Count)
{
	Count = FMath::Clamp(Count, 0, Quantity);
	FWEItemInstance Part = *this;
	Part.Quantity = Count;
	Part.InstanceId = FGuid::NewGuid();
	Quantity -= Count;
	return Part;
}
