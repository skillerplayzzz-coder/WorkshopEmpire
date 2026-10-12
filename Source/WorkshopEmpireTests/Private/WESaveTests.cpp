#include "Misc/AutomationTest.h"

#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEPickupItem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Save/WESaveGame.h"
#include "Save/WESaveSystem.h"
#include "WETestItems.h"
#include "WETestWorld.h"
#include "World/WEGarageDoor.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace WESaveTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	FString TestPath(const TCHAR* Name)
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"), TEXT("WESaveTests"), FString(Name) + TEXT(".wesave"));
	}

	void DeleteSaveFiles(const FString& Path)
	{
		IFileManager::Get().Delete(*Path, false, true, true);
		IFileManager::Get().Delete(*(Path + TEXT(".bak")), false, true, true);
		IFileManager::Get().Delete(*(Path + TEXT(".tmp")), false, true, true);
	}

	UWEInventoryComponent* SpawnContainer(FWETestWorld& TW, const FName& Name, const FVector& Location)
	{
		FActorSpawnParameters Params;
		Params.Name = Name;
		AActor* Actor = TW.World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(Location), Params);
		UWEInventoryComponent* Inventory = NewObject<UWEInventoryComponent>(Actor, TEXT("Storage"));
		Inventory->Configure(FText::FromString(TEXT("Testregal")), 6, 100.f);
		Inventory->RegisterComponent();
		return Inventory;
	}

	int32 CountWorldItems(UWorld* World, const UWEItemDefinition* Def)
	{
		int32 Count = 0;
		for (TActorIterator<AWEPickupItem> It(World); It; ++It)
		{
			if (IsValid(*It) && It->GetItem().Definition == Def)
			{
				Count += It->GetItem().Quantity;
			}
		}
		return Count;
	}
}

using namespace WESaveTests;
using namespace WETestItems;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWESaveRoundTripTest, "WorkshopEmpire.Save.RoundTripWorldContainersDoors", Flags)
bool FWESaveRoundTripTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	UWEItemDefinition* Screws = MakeDefinition(TEXT("S_Screws"), 0.01f, true, 100);
	UWEItemDefinition* Pc = MakeDefinition(TEXT("S_Pc"), 9.f, false, 1, false);

	AWEPickupItem* Computer = SpawnItem(TW, Pc, 1, FVector(100.f, 0.f, 50.f), EWEItemCondition::Defective);
	const FGuid ComputerId = Computer->GetItem().InstanceId;
	SpawnItem(TW, Screws, 7, FVector(200.f, 0.f, 50.f));
	UWEInventoryComponent* Shelf = SpawnContainer(TW, TEXT("TestShelf"), FVector(0.f, 300.f, 0.f));
	int32 Added = 0;
	Shelf->TryAddItem(FWEItemInstance::Make(Screws, 42, EWEItemCondition::New, 80), Added);
	AWEGarageDoor* Door = TW.World->SpawnActor<AWEGarageDoor>(AWEGarageDoor::StaticClass(), FTransform(FVector(500.f, 0.f, 0.f)));
	Door->SetOpen(true);

	const FString Path = TestPath(TEXT("RoundTrip"));
	DeleteSaveFiles(Path);
	FText Error;
	UWESaveGame* Captured = FWESaveSystem::Capture(TW.World);
	TestEqual(TEXT("Two world items captured"), Captured->WorldItems.Num(), 2);
	TestTrue(TEXT("Written"), FWESaveSystem::WriteToFile(Captured, Path, Error));

	// Change everything.
	for (TActorIterator<AWEPickupItem> It(TW.World); It; ++It)
	{
		It->Destroy();
	}
	Shelf->ClearAll();
	Door->SetOpen(false);
	SpawnItem(TW, Screws, 99, FVector(0.f, 0.f, 50.f)); // must disappear on load

	UWESaveGame* Loaded = FWESaveSystem::ReadFromFile(Path, Error);
	if (!TestNotNull(TEXT("Read back"), Loaded))
	{
		return false;
	}
	TestEqual(TEXT("Version"), Loaded->SaveVersion, UWESaveGame::CurrentVersion);
	FWESaveSystem::Apply(TW.World, Loaded);

	TestEqual(TEXT("Computer restored"), CountWorldItems(TW.World, Pc), 1);
	TestEqual(TEXT("World screws restored (7, not 99)"), CountWorldItems(TW.World, Screws), 7);
	bool bIdKept = false;
	for (TActorIterator<AWEPickupItem> It(TW.World); It; ++It)
	{
		if (IsValid(*It) && It->GetItem().Definition == Pc)
		{
			bIdKept = It->GetItem().InstanceId == ComputerId && It->GetItem().Condition == EWEItemCondition::Defective;
		}
	}
	TestTrue(TEXT("Computer id and condition kept"), bIdKept);
	TestEqual(TEXT("Shelf content restored"), Shelf->CountItems(Screws), 42);
	TestEqual(TEXT("Shelf item quality"), static_cast<int32>(Shelf->GetItemInSlot(0)->Quality), 80);
	TestTrue(TEXT("Door state restored"), Door->IsOpen());

	DeleteSaveFiles(Path);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWESaveCorruptionTest, "WorkshopEmpire.Save.CorruptFileFallsBackToBackup", Flags)
bool FWESaveCorruptionTest::RunTest(const FString& Parameters)
{
	const FString Path = TestPath(TEXT("Corrupt"));
	DeleteSaveFiles(Path);

	FText Error;
	UWESaveGame* First = NewObject<UWESaveGame>();
	First->MapName = TEXT("First");
	UWESaveGame* Second = NewObject<UWESaveGame>();
	Second->MapName = TEXT("Second");
	TestTrue(TEXT("Write first"), FWESaveSystem::WriteToFile(First, Path, Error));
	TestTrue(TEXT("Write second (first becomes .bak)"), FWESaveSystem::WriteToFile(Second, Path, Error));
	TestTrue(TEXT("Backup exists"), IFileManager::Get().FileExists(*(Path + TEXT(".bak"))));
	TestFalse(TEXT("No temp file left"), IFileManager::Get().FileExists(*(Path + TEXT(".tmp"))));

	bool bUsedBackup = true;
	UWESaveGame* Read = FWESaveSystem::ReadFromFile(Path, Error, &bUsedBackup);
	TestTrue(TEXT("Intact file reads the newest save"), Read && Read->MapName == TEXT("Second") && !bUsedBackup);

	// Flip a byte in the payload: CRC must catch it, backup must be used (logged as a warning).
	AddExpectedMessage(TEXT("loaded backup instead"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
	TArray<uint8> Bytes;
	FFileHelper::LoadFileToArray(Bytes, *Path);
	Bytes[Bytes.Num() / 2] ^= 0xFF;
	FFileHelper::SaveArrayToFile(Bytes, *Path);
	Read = FWESaveSystem::ReadFromFile(Path, Error, &bUsedBackup);
	TestTrue(TEXT("Damaged file -> backup loaded"), Read && Read->MapName == TEXT("First") && bUsedBackup);

	// Truncated main file and garbage backup: clean failure, no crash.
	FFileHelper::SaveStringToFile(TEXT("not a save"), *Path);
	FFileHelper::SaveStringToFile(TEXT("also not a save"), *(Path + TEXT(".bak")));
	Read = FWESaveSystem::ReadFromFile(Path, Error);
	TestNull(TEXT("Both damaged -> no save"), Read);
	TestFalse(TEXT("Error message set"), Error.IsEmpty());

	DeleteSaveFiles(Path);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWESaveVersionTest, "WorkshopEmpire.Save.VersionAndValidation", Flags)
bool FWESaveVersionTest::RunTest(const FString& Parameters)
{
	const FString Path = TestPath(TEXT("Version"));
	DeleteSaveFiles(Path);
	FText Error;

	UWESaveGame* Future = NewObject<UWESaveGame>();
	Future->SaveVersion = UWESaveGame::CurrentVersion + 1;
	FWESaveSystem::WriteToFile(Future, Path, Error);
	TestNull(TEXT("Newer save version is refused"), FWESaveSystem::ReadFromFile(Path, Error));
	TestFalse(TEXT("Error message set"), Error.IsEmpty());

	TestTrue(TEXT("Valid slot name"), FWESaveSystem::IsValidSlotName(TEXT("Quicksave_1")));
	TestFalse(TEXT("Path traversal rejected"), FWESaveSystem::IsValidSlotName(TEXT("../evil")));
	TestFalse(TEXT("Empty rejected"), FWESaveSystem::IsValidSlotName(TEXT("")));

	AddExpectedMessage(TEXT("unknown or invalid item"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
	FWESavedItem Unknown;
	Unknown.ItemId = TEXT("NoSuchItem_42");
	FWEItemInstance Out;
	TestFalse(TEXT("Unknown item ids are skipped, not crashed on"), FWESaveSystem::FromSaved(Unknown, Out));

	DeleteSaveFiles(Path);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
