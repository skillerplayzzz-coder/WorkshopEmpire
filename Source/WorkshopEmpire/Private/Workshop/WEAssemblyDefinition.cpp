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

FText UWEAssemblyDefinition::GetInstallBlocker(int32 Part, int32 RemovedMask) const
{
	if (!Parts.IsValidIndex(Part) || Part >= MaxParts || (RemovedMask & (1 << Part)) == 0)
	{
		return NSLOCTEXT("WEAssembly", "AlreadyIn", "Ist schon eingebaut");
	}
	// The way in: everything in front of the part must be out.
	TArray<FText> InFront = GetBlockers(Part, RemovedMask);
	if (InFront.Num() > 0)
	{
		return FText::Format(NSLOCTEXT("WEAssembly", "InstallFrontFirst", "Erst ausbauen: {0}"), FText::Join(FText::FromString(TEXT(", ")), InFront));
	}
	if (Parts[Part].bServiceAccess)
	{
		return FText::GetEmpty();
	}
	// What it sits on: every (non-body) part that needs this one out first must be in.
	TArray<FText> Base;
	for (int32 i = 0; i < Parts.Num() && i < MaxParts; ++i)
	{
		if (i != Part && !Parts[i].bShell && (RemovedMask & (1 << i)) != 0 && Parts[i].Requires.Contains(Parts[Part].Id))
		{
			Base.Add(Parts[i].DisplayName);
		}
	}
	if (Base.Num() > 0)
	{
		return FText::Format(NSLOCTEXT("WEAssembly", "InstallBaseFirst", "Erst einbauen: {0}"), FText::Join(FText::FromString(TEXT(", ")), Base));
	}
	return FText::GetEmpty();
}

bool UWEAssemblyDefinition::CanInstall(int32 Part, int32 RemovedMask) const
{
	return Parts.IsValidIndex(Part) && !Parts[Part].bShell && GetInstallBlocker(Part, RemovedMask).IsEmpty();
}

TArray<FText> UWEAssemblyDefinition::GetMissingForTest(int32 RemovedMask) const
{
	TArray<FText> Result;
	for (int32 i = 0; i < Parts.Num() && i < MaxParts; ++i)
	{
		const FWEAssemblyPart& P = Parts[i];
		if ((RemovedMask & (1 << i)) != 0 && !P.bOptional && !P.bServiceAccess && !P.bShell)
		{
			Result.Add(P.DisplayName);
		}
	}
	return Result;
}

uint8 UWEAssemblyDefinition::CountTakenApart(int32 RemovedMask) const
{
	int32 Count = 0;
	for (int32 i = 0; i < Parts.Num() && i < MaxParts; ++i)
	{
		Count += (RemovedMask & (1 << i)) != 0 && !Parts[i].bOptional ? 1 : 0;
	}
	return static_cast<uint8>(Count);
}

int32 UWEAssemblyDefinition::CountMissingOptional(int32 RemovedMask) const
{
	int32 Count = 0;
	for (int32 i = 0; i < Parts.Num() && i < MaxParts; ++i)
	{
		Count += (RemovedMask & (1 << i)) != 0 && Parts[i].bOptional ? 1 : 0;
	}
	return Count;
}

bool UWEAssemblyDefinition::IsComplete(int32 RemovedMask) const
{
	const int32 Count = FMath::Min(Parts.Num(), MaxParts);
	return Count > 0 && (RemovedMask & ((1 << Count) - 1)) == ((1 << Count) - 1);
}
