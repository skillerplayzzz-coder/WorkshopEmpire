#include "Items/WEItemDefinition.h"

#include "Engine/AssetManager.h"
#include "UObject/UObjectIterator.h"

const FPrimaryAssetType UWEItemDefinition::PrimaryAssetType(TEXT("WEItem"));

FPrimaryAssetId UWEItemDefinition::GetPrimaryAssetId() const
{
	// Asset name as primary id (stable for the Asset Manager); gameplay and saves use ItemId.
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}

UWEItemDefinition* UWEItemDefinition::FindById(FName ItemId)
{
	if (ItemId.IsNone())
	{
		return nullptr;
	}

	// Already loaded (map references, transient test definitions).
	for (TObjectIterator<UWEItemDefinition> It; It; ++It)
	{
		if (It->ItemId == ItemId && !It->HasAnyFlags(RF_ClassDefaultObject))
		{
			return *It;
		}
	}

	// Not loaded yet: load the registered item definitions (a few dozen small assets) and search them.
	if (UAssetManager* Manager = UAssetManager::GetIfInitialized())
	{
		TArray<FPrimaryAssetId> Ids;
		Manager->GetPrimaryAssetIdList(PrimaryAssetType, Ids);
		for (const FPrimaryAssetId& Id : Ids)
		{
			UWEItemDefinition* Def = Cast<UWEItemDefinition>(Manager->GetPrimaryAssetPath(Id).TryLoad());
			if (Def && Def->ItemId == ItemId)
			{
				return Def;
			}
		}
	}
	return nullptr;
}
