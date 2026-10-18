#include "Items/WEItemInstance.h"

#include "Items/WEItemDefinition.h"
#include "Workshop/WEAssemblyDefinition.h"

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
	// Marks must match: a locked stack never swallows loose goods (and the other way round).
	return Definition && Definition == Other.Definition && Definition->bStackable && Condition == Other.Condition
		&& DisassemblyStep == Other.DisassemblyStep && Repair == Other.Repair && Marks == Other.Marks && RemovedParts == Other.RemovedParts
		&& InstalledParts == Other.InstalledParts;
}

bool FWEItemInstance::IsPersonalEquipment() const
{
	return Definition && (Definition->ToolType != EWEToolType::None || Definition->CarryBonusKg > 0.f || Definition->Category == EWEItemCategory::Tool);
}

bool FWEItemInstance::NeedsSaleConfirmation() const
{
	if (!Definition)
	{
		return false;
	}
	// Rare finds and collectibles, or anything worth 150 € or more per piece.
	FWEItemInstance One = *this;
	One.Quantity = 1;
	return Definition->Rarity >= EWEItemRarity::Rare || Definition->Category == EWEItemCategory::Collectible
		|| Definition->Category == EWEItemCategory::RareFind || One.GetEstimatedValue().GetCents() >= 15000;
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
	if (IsBroken() || NeedsFunctionTest())
	{
		// A broken complex device is worth much less than a working one (repairing must pay off).
		// Rebuilt but not tested yet: nobody knows whether it runs, so it sells like a broken one.
		Factor *= Definition->BrokenValueMultiplier;
	}
	if (Definition->HasAssembly() && RemovedParts != 0)
	{
		// Rebuilt without the optional parts (CD drive, case fan): a little less.
		Factor *= FMath::Max(0.5, 1.0 - 0.1 * Definition->Assembly->CountMissingOptional(RemovedParts));
	}
	const double Cents = static_cast<double>(Definition->BaseValue.GetCents()) * Factor * Quantity;
	return FWEMoney::FromCents(static_cast<int64>(FMath::RoundToDouble(Cents)));
}

bool FWEItemInstance::HasBrokenInstalledPart() const
{
	for (int32 i = 0; i < MaxInstalledParts; ++i)
	{
		if (GetInstalledCondition(i) >= static_cast<int32>(EWEItemCondition::Damaged))
		{
			return true;
		}
	}
	return false;
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
	if (NeedsFunctionTest())
	{
		return FText::Format(NSLOCTEXT("WEItemInstance", "Untested", "{0} (ungetestet)"), Definition->DisplayName);
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
