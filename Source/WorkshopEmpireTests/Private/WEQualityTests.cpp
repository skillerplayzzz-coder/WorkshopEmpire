#include "Misc/AutomationTest.h"

#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WETraderTerminal.h"
#include "GameFramework/DamageType.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Framework/WEGameState.h"
#include "HAL/FileManager.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Progression/WEMilestoneComponent.h"
#include "Progression/WEProgressionComponent.h"
#include "Save/WESaveGame.h"
#include "Save/WESaveSystem.h"
#include "WETestWorld.h"
#include "Workshop/WEWorkshopLayoutComponent.h"
#include "World/WEPlaceholderProp.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Phase 17 (quality assurance): save files under abuse - damaged in every way, saved and loaded again and
 * again, written by older versions, and with a big late-game world.
 */
namespace WEQualityTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	FString TestPath(const TCHAR* Name)
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"), TEXT("WEQualityTests"), FString(Name) + TEXT(".wesave"));
	}

	void DeleteSaveFiles(const FString& Path)
	{
		for (const TCHAR* Suffix : { TEXT(""), TEXT(".bak"), TEXT(".tmp") })
		{
			IFileManager::Get().Delete(*(Path + Suffix), false, true, true);
		}
	}

	/** A company with a bit of everything: money, buildings, furniture, items, milestone statistics. */
	struct FCompanyWorld
	{
		FWETestWorld TW;
		AWEGameState* GameState = nullptr;

		explicit FCompanyWorld(int32 NumItems)
		{
			GameState = TW.SpawnGameState<AWEGameState>();
			GameState->GetContracts()->SetCatalog(nullptr);
			GameState->GetMilestones()->SetCatalog(nullptr); // statistics only, no rewards changing the balance
			GameState->GetCompany()->SetBalance(FWEMoney::FromEuros(12345));
			GameState->GetProgression()->GrantUpgrade(TEXT("WorkshopExtension"));
			GameState->GetMilestones()->AddCount(EWEMilestoneKind::DevicesRepaired, 7);
			UWEWorkshopLayoutComponent* Layout = GameState->GetLayout();
			Layout->SpawnFurniture(TEXT("Shredder"), FTransform(FVector(0.f, 0.f, 0.f)));
			Layout->SpawnFurniture(TEXT("HeavyShelf"), FTransform(FVector(300.f, 0.f, 0.f)));
			const TCHAR* Ids[] = { TEXT("SteelScrap"), TEXT("OldComputer"), TEXT("CopperCable"), TEXT("CableBox"), TEXT("RamModule") };
			for (int32 i = 0; i < NumItems; ++i)
			{
				UWEItemDefinition* Def = UWEItemDefinition::FindById(FName(Ids[i % UE_ARRAY_COUNT(Ids)]));
				// Stacks only for stackable items (a world item never holds 3 computers).
				AWEPickupItem::SpawnWorldItem(TW.World, FWEItemInstance::Make(Def, Def->bStackable ? 1 + i % 3 : 1, EWEItemCondition::Used, static_cast<uint8>(i % 100)),
					FTransform(FVector(500.f + (i % 40) * 30.f, (i / 40) * 30.f, 20.f)));
			}
		}

		int32 CountItems() const
		{
			int32 Count = 0;
			for (TActorIterator<AWEPickupItem> It(TW.World); It; ++It)
			{
				Count += IsValid(*It) ? It->GetItem().Quantity : 0;
			}
			return Count;
		}

		int32 CountFurniture() const
		{
			int32 Count = 0;
			for (TActorIterator<AWEPlaceholderProp> It(TW.World); It; ++It)
			{
				Count += IsValid(*It) && !It->GetFurnitureId().IsNone() ? 1 : 0;
			}
			return Count;
		}

		FString Summary() const
		{
			return FString::Printf(TEXT("items=%d furniture=%d balance=%lld tier=%d repaired=%d"), CountItems(), CountFurniture(),
				GameState->GetCompany()->GetBalance().GetCents(), GameState->GetProgression()->GetCompanyTier(), GameState->GetMilestones()->GetStats().DevicesRepaired);
		}
	};
}

using namespace WEQualityTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWESaveFuzzTest, "WorkshopEmpire.Save.DamagedFilesNeverCrash", Flags)
bool FWESaveFuzzTest::RunTest(const FString& Parameters)
{
	// Every damaged variant of a real save must be refused cleanly (no crash, no half-loaded world).
	FCompanyWorld W(30);
	const FString Path = TestPath(TEXT("Fuzz"));
	DeleteSaveFiles(Path);
	FText Error;
	if (!TestTrue(TEXT("Written"), FWESaveSystem::WriteToFile(FWESaveSystem::Capture(W.TW.World), Path, Error)))
	{
		return false;
	}
	TArray<uint8> Good;
	FFileHelper::LoadFileToArray(Good, *Path);
	const FString Damaged = TestPath(TEXT("FuzzDamaged"));
	FRandomStream Random(1717);
	int32 Refused = 0, Variants = 0;
	auto TryRead = [&](const TArray<uint8>& Bytes)
	{
		DeleteSaveFiles(Damaged);
		FFileHelper::SaveArrayToFile(Bytes, *Damaged);
		++Variants;
		Refused += FWESaveSystem::ReadFromFile(Damaged, Error) ? 0 : 1;
	};
	TryRead({});                                                           // empty file
	for (int32 Length : { 1, 4, 8, 15, 16, 32, Good.Num() / 2, Good.Num() - 1 })  // truncated
	{
		TryRead(TArray<uint8>(Good.GetData(), FMath::Clamp(Length, 0, Good.Num())));
	}
	for (int32 i = 0; i < 120; ++i)                                         // random bit flips anywhere
	{
		TArray<uint8> Bytes = Good;
		const int32 Flips = Random.RandRange(1, 8);
		for (int32 f = 0; f < Flips; ++f)
		{
			Bytes[Random.RandRange(0, Bytes.Num() - 1)] ^= static_cast<uint8>(1 << Random.RandRange(0, 7));
		}
		TryRead(Bytes);
	}
	TArray<uint8> Junk;                                                    // random junk with a valid magic number
	Junk.SetNumUninitialized(4096);
	for (uint8& B : Junk) { B = static_cast<uint8>(Random.RandRange(0, 255)); }
	FMemory::Memcpy(Junk.GetData(), Good.GetData(), 4);
	TryRead(Junk);
	TArray<uint8> Longer = Good;                                           // trailing garbage
	Longer.Append(Junk);
	TryRead(Longer);
	TestEqual(TEXT("Every damaged variant refused"), Refused, Variants);
	AddInfo(FString::Printf(TEXT("%d damaged variants of a %d-byte save, %d refused"), Variants, Good.Num(), Refused));
	TestNotNull(TEXT("The good file still reads"), FWESaveSystem::ReadFromFile(Path, Error));
	DeleteSaveFiles(Path);
	DeleteSaveFiles(Damaged);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWESaveRepeatTest, "WorkshopEmpire.Save.RepeatedSaveLoadIsStable", Flags)
bool FWESaveRepeatTest::RunTest(const FString& Parameters)
{
	// Saving and loading 10 times in a row must not lose, duplicate or change anything.
	FCompanyWorld W(60);
	const FString Path = TestPath(TEXT("Repeat"));
	const FString Start = W.Summary();
	FText Error;
	for (int32 Round = 0; Round < 10; ++Round)
	{
		DeleteSaveFiles(Path);
		if (!FWESaveSystem::WriteToFile(FWESaveSystem::Capture(W.TW.World), Path, Error))
		{
			AddError(FString::Printf(TEXT("Round %d: write failed: %s"), Round, *Error.ToString()));
			return false;
		}
		UWESaveGame* Read = FWESaveSystem::ReadFromFile(Path, Error);
		if (!Read)
		{
			AddError(FString::Printf(TEXT("Round %d: read failed: %s"), Round, *Error.ToString()));
			return false;
		}
		FWESaveSystem::Apply(W.TW.World, Read);
	}
	TestEqual(TEXT("Same company after 10 save/load cycles"), W.Summary(), Start);
	AddInfo(Start);
	DeleteSaveFiles(Path);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWESaveOldVersionTest, "WorkshopEmpire.Save.OlderVersionsLoad", Flags)
bool FWESaveOldVersionTest::RunTest(const FString& Parameters)
{
	// A save of an older version (fields added later are missing) loads; new systems start from their defaults.
	FCompanyWorld W(10);
	const FString Path = TestPath(TEXT("Old"));
	FText Error;
	for (const int32 Version : { 12, 11, 8, 4 })
	{
		UWESaveGame* Old = FWESaveSystem::Capture(W.TW.World);
		Old->SaveVersion = Version;
		Old->bHasMilestones = false;          // added in 13
		if (Version < 11) { Old->BookTotals.Reset(); }
		if (Version < 10) { Old->bHasStaff = false; Old->Employees.Reset(); }
		if (Version < 7) { Old->bHasFurniture = false; Old->Furniture.Reset(); }
		DeleteSaveFiles(Path);
		FWESaveSystem::WriteToFile(Old, Path, Error);
		UWESaveGame* Read = FWESaveSystem::ReadFromFile(Path, Error);
		if (!TestNotNull(*FString::Printf(TEXT("Version %d reads"), Version), Read))
		{
			continue;
		}
		FCompanyWorld Fresh(0);
		FWESaveSystem::Apply(Fresh.TW.World, Read);
		TestEqual(*FString::Printf(TEXT("Version %d: balance"), Version), Fresh.GameState->GetCompany()->GetBalance().GetCents(), W.GameState->GetCompany()->GetBalance().GetCents());
		TestEqual(*FString::Printf(TEXT("Version %d: world items"), Version), Fresh.CountItems(), W.CountItems());
		TestEqual(*FString::Printf(TEXT("Version %d: milestone statistics keep their own value (no milestones in old saves)"), Version),
			Fresh.GameState->GetMilestones()->GetStats().DevicesRepaired, 7);
	}
	DeleteSaveFiles(Path);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWESaveLargeWorldTest, "WorkshopEmpire.Save.LargeWorldIsFast", Flags)
bool FWESaveLargeWorldTest::RunTest(const FString& Parameters)
{
	// 2,000 loose items (far more than a tidy late-game company has) save and load quickly and completely.
	FCompanyWorld W(2000);
	const FString Path = TestPath(TEXT("Large"));
	DeleteSaveFiles(Path);
	FText Error;
	double Start = FPlatformTime::Seconds();
	UWESaveGame* Save = FWESaveSystem::Capture(W.TW.World);
	const bool bWritten = FWESaveSystem::WriteToFile(Save, Path, Error);
	const double SaveMs = (FPlatformTime::Seconds() - Start) * 1000.0;
	Start = FPlatformTime::Seconds();
	UWESaveGame* Read = FWESaveSystem::ReadFromFile(Path, Error);
	FCompanyWorld Fresh(0);
	if (Read)
	{
		FWESaveSystem::Apply(Fresh.TW.World, Read);
	}
	const double LoadMs = (FPlatformTime::Seconds() - Start) * 1000.0;
	const int64 Bytes = IFileManager::Get().FileSize(*Path);
	AddInfo(FString::Printf(TEXT("2000 items: save %.0f ms, load %.0f ms, %lld bytes"), SaveMs, LoadMs, Bytes));
	TestTrue(TEXT("Written"), bWritten);
	TestEqual(TEXT("All items back"), Fresh.CountItems(), W.CountItems());
	TestTrue(TEXT("Save under 1 s"), SaveMs < 1000.0);
	TestTrue(TEXT("Load under 3 s"), LoadMs < 3000.0);
	TestTrue(TEXT("File under 5 MB"), Bytes > 0 && Bytes < 5 * 1024 * 1024);
	DeleteSaveFiles(Path);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEItemNetAndFallTest, "WorkshopEmpire.Items.RestingItemsSleepAndFallenItemsReturn", Flags)
bool FWEItemNetAndFallTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	GameState->GetContracts()->SetCatalog(nullptr);
	AWEDeliveryZone* Pallet = TW.Spawn<AWEDeliveryZone>(FVector(320.f, -220.f, 6.f));
	UWEItemDefinition* Scrap = UWEItemDefinition::FindById(TEXT("SteelScrap"));
	AWEPickupItem* Item = AWEPickupItem::SpawnWorldItem(TW.World, FWEItemInstance::Make(Scrap, 3), FTransform(FVector(600.f, 0.f, 50.f)));
	if (!TestNotNull(TEXT("Item"), Item) || !TestNotNull(TEXT("Pallet"), Pallet))
	{
		return false;
	}
	// Network: a resting item stops replicating (dormant), any change wakes it first.
	TestEqual(TEXT("New item replicates"), Item->NetDormancy, DORM_Awake);
	UStaticMeshComponent* Body = Item->FindComponentByClass<UStaticMeshComponent>();
	Body->PutAllRigidBodiesToSleep();
	Body->OnComponentSleep.Broadcast(Body, NAME_None);
	TestEqual(TEXT("Resting item is dormant"), Item->NetDormancy, DORM_DormantAll);
	FWEItemInstance Changed = Item->GetItem();
	Changed.Quantity = 2;
	Item->SetItem(Changed);
	TestEqual(TEXT("A change wakes it"), Item->NetDormancy, DORM_Awake);
	// Fell through the world: back on the delivery pallet, not lost.
	Item->SetActorLocation(FVector(600.f, 0.f, -5000.f));
	Item->FellOutOfWorld(*GetDefault<UDamageType>());
	TestTrue(TEXT("Rescued to the pallet"), IsValid(Item) && FVector::Dist2D(Item->GetActorLocation(), Pallet->GetActorLocation()) < 10.f && Item->GetActorLocation().Z > 0.f);
	TestEqual(TEXT("Nothing lost"), Item->GetItem().Quantity, 2);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
