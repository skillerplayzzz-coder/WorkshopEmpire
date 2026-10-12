#pragma once

#include "CoreMinimal.h"
#include "Framework/WEPlayerState.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "UObject/Package.h"
#include "WETestWorld.h"

/** Helpers that create transient item definitions and items for tests. */
namespace WETestItems
{
	/** Creates (or reuses) a transient definition. Ids must be unique per configuration. */
	inline UWEItemDefinition* MakeDefinition(const TCHAR* Id, float WeightKg, bool bStackable = false, int32 MaxStack = 1,
		bool bFitsInInventory = true, int64 ValueCents = 1000, EWEItemCategory Category = EWEItemCategory::SparePart)
	{
		UWEItemDefinition* Def = UWEItemDefinition::FindById(FName(Id));
		if (!Def)
		{
			Def = NewObject<UWEItemDefinition>(GetTransientPackage(), FName(FString(TEXT("TestDef_")) + Id));
			Def->AddToRoot(); // survives GC between tests; ids are unique
		}
		Def->ItemId = FName(Id);
		Def->DisplayName = FText::FromString(Id);
		Def->WeightKg = WeightKg;
		Def->bStackable = bStackable;
		Def->MaxStackSize = MaxStack;
		Def->bFitsInInventory = bFitsInInventory;
		Def->BaseValue = FWEMoney::FromCents(ValueCents);
		Def->Category = Category;
		Def->WorldScale = FVector(0.4f);
		return Def;
	}

	/** A bulky (carry-only) single item, like the Phase 1 test items. */
	inline AWEPickupItem* SpawnBulky(FWETestWorld& TW, const FVector& Location, const TCHAR* Name, float WeightKg)
	{
		UWEItemDefinition* Def = MakeDefinition(*FString::Printf(TEXT("Bulky_%s_%d"), Name, FMath::RoundToInt(WeightKg * 100.f)), WeightKg, false, 1, false);
		return AWEPickupItem::SpawnWorldItem(TW.World, FWEItemInstance::Make(Def), FTransform(Location));
	}

	inline AWEPickupItem* SpawnItem(FWETestWorld& TW, UWEItemDefinition* Def, int32 Quantity, const FVector& Location,
		EWEItemCondition Condition = EWEItemCondition::Used)
	{
		return AWEPickupItem::SpawnWorldItem(TW.World, FWEItemInstance::Make(Def, Quantity, Condition), FTransform(Location));
	}

	/** Character with a PlayerState (and therefore a player inventory). */
	inline AWECharacter* SpawnPlayer(FWETestWorld& TW, const FVector& Location)
	{
		AWECharacter* Character = TW.Spawn<AWECharacter>(Location);
		AWEPlayerState* PS = TW.Spawn<AWEPlayerState>(FVector::ZeroVector);
		Character->SetPlayerState(PS);
		return Character;
	}
}
