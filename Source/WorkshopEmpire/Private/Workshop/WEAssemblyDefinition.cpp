#include "Workshop/WEAssemblyDefinition.h"

bool UWEAssemblyDefinition::IsAvailable(int32 Part, int32 RemovedMask) const
{
	if (!Parts.IsValidIndex(Part) || Part >= MaxParts || (RemovedMask & (1 << Part)) != 0)
	{
		return false;
	}
	for (const FName& Required : Parts[Part].Requires)
	{
		const int32 Index = FindPart(Required);
		if (Index != INDEX_NONE && (RemovedMask & (1 << Index)) == 0)
		{
			return false;
		}
	}
	return true;
}

TArray<FText> UWEAssemblyDefinition::GetBlockers(int32 Part, int32 RemovedMask) const
{
	TArray<FText> Result;
	if (!Parts.IsValidIndex(Part))
	{
		return Result;
	}
	for (const FName& Required : Parts[Part].Requires)
	{
		const int32 Index = FindPart(Required);
		if (Index != INDEX_NONE && (RemovedMask & (1 << Index)) == 0)
		{
			Result.Add(Parts[Index].DisplayName);
		}
	}
	return Result;
}

int32 UWEAssemblyDefinition::GetDefaultPart(int32 RemovedMask) const
{
	for (int32 i = 0; i < Parts.Num() && i < MaxParts; ++i)
	{
		if (IsAvailable(i, RemovedMask))
		{
			return i;
		}
	}
	return INDEX_NONE;
}

bool UWEAssemblyDefinition::IsComplete(int32 RemovedMask) const
{
	const int32 Count = FMath::Min(Parts.Num(), MaxParts);
	return Count > 0 && (RemovedMask & ((1 << Count) - 1)) == ((1 << Count) - 1);
}
