#include "Workshop/WEBenchmark.h"

#include "Items/WEItemDefinition.h"
#include "Items/WEItemInstance.h"
#include "Workshop/WEAssemblyDefinition.h"
#include "Workshop/WEDisassemblyDefinition.h"

namespace
{
	/** The part's own item and how many pieces sit there (RAM: two modules). */
	const FWEDisassemblyOutput* PartOutput(const UWEItemDefinition* Def, int32 Part)
	{
		const FWEDisassemblyStep* Step = Def && Def->Disassembly ? Def->Disassembly->GetStep(Part) : nullptr;
		if (!Step)
		{
			return nullptr;
		}
		for (const FWEDisassemblyOutput& Out : Step->Outputs)
		{
			if (Out.Item && Out.bInheritCondition)
			{
				return &Out;
			}
		}
		return nullptr;
	}

	float ConditionFactor(int32 ConditionIndex)
	{
		switch (static_cast<EWEItemCondition>(FMath::Clamp(ConditionIndex, 0, 5)))
		{
		case EWEItemCondition::New:      return 1.f;
		case EWEItemCondition::VeryGood: return 0.95f;
		case EWEItemCondition::Used:     return 0.85f;
		default:                         return 0.5f;
		}
	}
}

namespace WEBenchmark
{
	const UWEItemDefinition* GetPartItem(const FWEItemInstance& Device, int32 Part)
	{
		const UWEItemDefinition* Def = Device.Definition;
		const UWEAssemblyDefinition* Assembly = Def && Def->HasAssembly() ? Def->Assembly.Get() : nullptr;
		const FWEDisassemblyOutput* Out = PartOutput(Def, Part);
		if (!Assembly || !Out || !Assembly->Parts.IsValidIndex(Part))
		{
			return nullptr;
		}
		const int32 Variant = Device.GetPartVariant(Part);
		const TArray<TObjectPtr<UWEItemDefinition>>& Alts = Assembly->Parts[Part].Alternatives;
		return Variant > 0 && Alts.IsValidIndex(Variant - 1) && Alts[Variant - 1] ? Alts[Variant - 1].Get() : Out->Item.Get();
	}

	int32 RawScore(const FWEItemInstance& Device)
	{
		const UWEItemDefinition* Def = Device.Definition;
		const UWEAssemblyDefinition* Assembly = Def && Def->HasAssembly() ? Def->Assembly.Get() : nullptr;
		int32 Total = 0;
		for (int32 i = 0; Assembly && i < Assembly->Parts.Num(); ++i)
		{
			const FWEDisassemblyOutput* Out = PartOutput(Def, i);
			const UWEItemDefinition* PartItem = GetPartItem(Device, i);
			if (Out && PartItem && !Device.IsPartRemoved(i))
			{
				Total += PartItem->BenchmarkPoints * FMath::Max(1, Out->MinQuantity);
			}
		}
		return Total;
	}

	int32 Score(const FWEItemInstance& Device)
	{
		const UWEItemDefinition* Def = Device.Definition;
		const UWEAssemblyDefinition* Assembly = Def && Def->HasAssembly() ? Def->Assembly.Get() : nullptr;
		if (!Assembly || Device.IsBroken() || Device.NeedsFunctionTest() || !Assembly->GetMissingForTest(Device.RemovedParts).IsEmpty())
		{
			return 0;
		}
		// Quality of the device as a whole (dust, wear) on top of each part's condition.
		const float Quality = 0.8f + 0.4f * (Device.Quality / 100.f);
		float Total = 0.f;
		for (int32 i = 0; i < Assembly->Parts.Num(); ++i)
		{
			const FWEDisassemblyOutput* Out = PartOutput(Def, i);
			const UWEItemDefinition* PartItem = GetPartItem(Device, i);
			if (!Out || !PartItem || Device.IsPartRemoved(i) || PartItem->BenchmarkPoints <= 0)
			{
				continue;
			}
			const int32 Installed = Device.GetInstalledCondition(i);
			const int32 Condition = Installed != INDEX_NONE ? Installed : static_cast<int32>(Device.Condition);
			Total += PartItem->BenchmarkPoints * FMath::Max(1, Out->MinQuantity) * ConditionFactor(Condition);
		}
		// A missing case fan makes it throttle a little.
		const int32 Fan = Assembly->FindPart(TEXT("CaseFan"));
		if (Fan != INDEX_NONE && Device.IsPartRemoved(Fan))
		{
			Total *= 0.92f;
		}
		return FMath::RoundToInt(Total * Quality);
	}

	int32 StockScore(const UWEItemDefinition* DeviceDef)
	{
		FWEItemInstance Stock;
		Stock.Definition = const_cast<UWEItemDefinition*>(DeviceDef);
		return RawScore(Stock);
	}

	float ValueFactor(const FWEItemInstance& Device)
	{
		if (!Device.Definition || !Device.Definition->HasAssembly() || Device.PartVariants == 0)
		{
			return 1.f;
		}
		const int32 Stock = StockScore(Device.Definition);
		return Stock > 0 ? FMath::Clamp(RawScore(Device) / static_cast<float>(Stock), 0.6f, 3.f) : 1.f;
	}

	int32 StarsFor(int32 InScore, int32 Required)
	{
		if (Required <= 0)
		{
			return 4;
		}
		const float Ratio = InScore / static_cast<float>(Required);
		return Ratio < 1.f ? 1 : Ratio < 1.1f ? 3 : Ratio < 1.3f ? 4 : 5;
	}
}

FText WEBenchmark::PointsText(int32 Points)
{
	FString Digits = FString::FromInt(FMath::Abs(Points));
	for (int32 i = Digits.Len() - 3; i > 0; i -= 3)
	{
		Digits.InsertAt(i, TEXT('.'));
	}
	return FText::FromString(Points < 0 ? TEXT("-") + Digits : Digits);
}