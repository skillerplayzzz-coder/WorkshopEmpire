#include "Items/WELootTable.h"

#include "Items/WEItemDefinition.h"

FWEItemInstance UWELootTable::RollOne(FRandomStream& Random) const
{
	float Total = 0.f;
	for (const FWELootEntry& Entry : Entries)
	{
		Total += Entry.Item ? FMath::Max(0.f, Entry.Weight) : 0.f;
	}
	if (Total <= 0.f)
	{
		return FWEItemInstance();
	}
	float Pick = Random.FRand() * Total;
	for (const FWELootEntry& Entry : Entries)
	{
		const float Weight = Entry.Item ? FMath::Max(0.f, Entry.Weight) : 0.f;
		if (Weight <= 0.f || (Pick -= Weight) > 0.f)
		{
			continue;
		}
		const int32 Best = static_cast<int32>(FMath::Min(Entry.BestCondition, Entry.WorstCondition));
		const int32 Worst = static_cast<int32>(FMath::Max(Entry.BestCondition, Entry.WorstCondition));
		const int32 Quantity = Random.RandRange(FMath::Min(Entry.MinQuantity, Entry.MaxQuantity), FMath::Max(Entry.MinQuantity, Entry.MaxQuantity));
		const int32 Quality = Random.RandRange(FMath::Min(Entry.MinQuality, Entry.MaxQuality), FMath::Max(Entry.MinQuality, Entry.MaxQuality));
		FWEItemInstance Item = FWEItemInstance::Make(Entry.Item, FMath::Clamp(Quantity, 1, Entry.Item->GetMaxStackSize()),
			static_cast<EWEItemCondition>(Random.RandRange(Best, Worst)), static_cast<uint8>(Quality));
		return Item;
	}
	return FWEItemInstance();
}

TArray<FWEItemInstance> UWELootTable::Roll(FRandomStream& Random) const
{
	TArray<FWEItemInstance> Result;
	const int32 Rolls = Random.RandRange(FMath::Min(MinRolls, MaxRolls), FMath::Max(MinRolls, MaxRolls));
	for (int32 i = 0; i < Rolls; ++i)
	{
		const FWEItemInstance Item = RollOne(Random);
		if (Item.IsValid())
		{
			Result.Add(Item);
		}
	}
	return Result;
}
