#include "Items/WEItemInstance.h"

#include "Items/WEItemDefinition.h"
#include "Workshop/WEAssemblyDefinition.h"
#include "Workshop/WEBenchmark.h"

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
		&& InstalledParts == Other.InstalledParts && PartVariants == Other.PartVariants;
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
	if (!IsBroken() && !NeedsFunctionTest())
	{
		// Phase 19: better parts (graphics card, processor ...) make a working PC worth more.
		Factor *= WEBenchmark::ValueFactor(*this);
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

#define LOCTEXT_NAMESPACE "WEItemWear"

FWEItemWear FWEItemInstance::GetWear() const
{
	FWEItemWear Wear;
	if (!Definition || Condition == EWEItemCondition::New || IsPersonalEquipment())
	{
		return Wear;
	}
	const EWEItemCategory Cat = Definition->Category;
	const bool bDevice = Cat == EWEItemCategory::Electronics || Cat == EWEItemCategory::PowerTool || Cat == EWEItemCategory::Motor
		|| Cat == EWEItemCategory::Machine || Cat == EWEItemCategory::VehiclePart || Cat == EWEItemCategory::Collectible || Cat == EWEItemCategory::RareFind;
	if (!bDevice)
	{
		return Wear;
	}
	// The same item always looks the same (seeded by its id); worse quality = more wear.
	FRandomStream R(static_cast<int32>(GetTypeHash(InstanceId)));
	const float Base = 1.f - Quality / 100.f;
	const float R1 = R.FRand(), R2 = R.FRand(), R3 = R.FRand(), R4 = R.FRand(), R5 = R.FRand(), R6 = R.FRand();
	Wear.Dirt = FMath::Clamp(Base * 1.15f * (0.45f + 0.55f * R1) + (IsBroken() ? 0.15f : 0.f), 0.f, 1.f);
	Wear.Scratches = FMath::Clamp(Base * (0.3f + 0.9f * R2), 0.f, 1.f);
	Wear.Yellowing = Cat == EWEItemCategory::Electronics && R3 < 0.45f ? FMath::Clamp(Base * 1.3f * R4, 0.f, 0.9f) : 0.f;
	// Not every defect can be seen: only some broken devices show burnt or cracked spots.
	if (Condition == EWEItemCondition::Damaged && R6 < 0.5f)
	{
		Wear.Damage = 0.35f + 0.3f * R5;
	}
	else if (Condition >= EWEItemCondition::Defective && R6 < 0.6f)
	{
		Wear.Damage = 0.5f + 0.4f * R5;
	}
	if (Condition == EWEItemCondition::VeryGood)
	{
		Wear.Dirt *= 0.4f;
		Wear.Scratches *= 0.3f;
	}
	if (Repair.HasFlag(FWERepairState::Cleaned))
	{
		Wear.Dirt *= 0.06f;
	}
	if (Repair.HasFlag(FWERepairState::Refurbished))
	{
		Wear.Dirt *= 0.3f;
		Wear.Scratches *= 0.3f;
		Wear.Yellowing *= 0.5f;
	}
	return Wear;
}

FText FWEItemInstance::GetWearText() const
{
	const FWEItemWear W = GetWear();
	TArray<FText> Parts;
	if (W.Dirt > 0.45f) { Parts.Add(LOCTEXT("VeryDirty", "stark verschmutzt")); }
	else if (W.Dirt > 0.22f) { Parts.Add(LOCTEXT("Dirty", "verschmutzt")); }
	if (W.Scratches > 0.35f) { Parts.Add(LOCTEXT("Scratched", "verkratzt")); }
	if (W.Yellowing > 0.25f) { Parts.Add(LOCTEXT("Yellowed", "vergilbt")); }
	if (W.Damage > 0.f) { Parts.Add(LOCTEXT("VisiblyDamaged", "sichtbar beschädigt")); }
	if (Parts.Num() == 0)
	{
		return Condition <= EWEItemCondition::VeryGood || (W.Dirt < 0.1f && W.Scratches < 0.12f) ? LOCTEXT("LikeNew", "Neuwertig") : LOCTEXT("UsedLook", "Gebraucht");
	}
	FString Joined = FText::Join(FText::FromString(TEXT(", ")), Parts).ToString();
	Joined[0] = FChar::ToUpper(Joined[0]);
	return FText::FromString(Joined);
}

#undef LOCTEXT_NAMESPACE