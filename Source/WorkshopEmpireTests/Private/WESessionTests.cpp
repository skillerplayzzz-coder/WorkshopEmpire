#include "Misc/AutomationTest.h"

#include "Framework/WEGameMode.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEItemInstance.h"
#include "Online/WESessionSubsystem.h"
#include "Save/WESaveGame.h"
#include "Save/WESaveSystem.h"
#include "WETestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace WESessionTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	FWESavedPlayer MakePlayer(const TCHAR* Key, const TCHAR* Name, int32 Copper)
	{
		FWESavedPlayer P;
		P.PlayerKey = Key;
		P.PlayerName = Name;
		P.PawnTransform = FTransform(FVector(100.f, 50.f, 90.f));
		if (Copper > 0)
		{
			FWEItemInstance Item = FWEItemInstance::Make(UWEItemDefinition::FindById(TEXT("CopperCable")), Copper, EWEItemCondition::Used, 50);
			FWEInventorySlotData Slot;
			Slot.SlotIndex = 0;
			Slot.Item = Item;
			P.Inventory = FWESaveSystem::ToSavedSlots({ Slot });
		}
		return P;
	}
}

using namespace WESessionTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWESessionHelpersTest, "WorkshopEmpire.Session.VersionAddressAndOptions", Flags)
bool FWESessionHelpersTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Patch differences are fine"), UWESessionSubsystem::IsCompatibleVersion(TEXT("0.13.0"), TEXT("0.13.4")));
	TestFalse(TEXT("Other minor version"), UWESessionSubsystem::IsCompatibleVersion(TEXT("0.13.0"), TEXT("0.12.0")));
	TestFalse(TEXT("Unparsable"), UWESessionSubsystem::IsCompatibleVersion(TEXT("0.13.0"), TEXT("")));
	TestFalse(TEXT("Version is set"), UWESessionSubsystem::GetGameVersion().IsEmpty());

	TestEqual(TEXT("Default port added"), UWESessionSubsystem::NormalizeAddress(TEXT(" 192.168.0.5 ")), FString(TEXT("192.168.0.5:7777")));
	TestEqual(TEXT("Own port kept"), UWESessionSubsystem::NormalizeAddress(TEXT("10.0.0.2:7790")), FString(TEXT("10.0.0.2:7790")));
	TestTrue(TEXT("No URL tricks"), UWESessionSubsystem::NormalizeAddress(TEXT("1.2.3.4?listen")).IsEmpty());
	TestTrue(TEXT("Port range"), UWESessionSubsystem::NormalizeAddress(TEXT("1.2.3.4:99999")).IsEmpty());
	TestTrue(TEXT("Missing host"), UWESessionSubsystem::NormalizeAddress(TEXT(":7777")).IsEmpty());
	TestTrue(TEXT("Empty"), UWESessionSubsystem::NormalizeAddress(TEXT("")).IsEmpty());

	const FString Options = UWESessionSubsystem::MakeJoinOptions(TEXT("Anna Maria?Admin=1"), /*bLAN*/ true);
	TestTrue(TEXT("Name without URL characters"), Options.Contains(TEXT("?WEName=Anna_MariaAdmin1")));
	TestTrue(TEXT("Version sent"), Options.Contains(FString::Printf(TEXT("?WEVersion=%s"), *UWESessionSubsystem::GetGameVersion())));
	TestTrue(TEXT("LAN flag for IP with Steam"), Options.EndsWith(TEXT("?bIsLanMatch")));
	TestFalse(TEXT("No LAN flag otherwise"), UWESessionSubsystem::MakeJoinOptions(TEXT("Anna"), false).Contains(TEXT("bIsLanMatch")));
	TestFalse(TEXT("Steam: no own name (the Steam name is used)"), UWESessionSubsystem::MakeJoinOptions(FString(), false).Contains(TEXT("WEName")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWESessionMessagesTest, "WorkshopEmpire.Session.ErrorMessages", Flags)
bool FWESessionMessagesTest::RunTest(const FString& Parameters)
{
	// The server's refusal reason reaches the player unchanged.
	const FString Full = AWEGameMode::CheckLogin(4, 4, TEXT("?WEVersion=0.0.1"));
	TestTrue(TEXT("Full first"), Full.Contains(TEXT("voll (4/4")));
	TestTrue(TEXT("Shown in the menu"), UWESessionSubsystem::DescribeNetworkFailure(ENetworkFailure::PendingConnectionFailure, Full).ToString().Contains(TEXT("Das Spiel ist voll")));
	TestTrue(TEXT("Engine's own full check"), UWESessionSubsystem::DescribeNetworkFailure(ENetworkFailure::PendingConnectionFailure, TEXT("Server full.")).ToString().Contains(TEXT("voll")));
	TestTrue(TEXT("Host did not answer"), UWESessionSubsystem::DescribeNetworkFailure(ENetworkFailure::PendingConnectionFailure, FString()).ToString().Contains(TEXT("nicht geantwortet")));
	TestTrue(TEXT("Lost"), UWESessionSubsystem::DescribeNetworkFailure(ENetworkFailure::ConnectionLost, TEXT("x")).ToString().Contains(TEXT("verloren")));
	TestTrue(TEXT("Timeout"), UWESessionSubsystem::DescribeNetworkFailure(ENetworkFailure::ConnectionTimeout, TEXT("x")).ToString().Contains(TEXT("Zeitüberschreitung")));
	TestTrue(TEXT("Version"), UWESessionSubsystem::DescribeNetworkFailure(ENetworkFailure::OutdatedClient, TEXT("x")).ToString().Contains(TEXT("Spielversion")));
	TestTrue(TEXT("Port"), UWESessionSubsystem::DescribeNetworkFailure(ENetworkFailure::NetDriverListenFailure, TEXT("x")).ToString().Contains(TEXT("Port")));
	TestTrue(TEXT("Address"), UWESessionSubsystem::DescribeTravelFailure(ETravelFailure::PendingNetGameCreateFailure, TEXT("x")).ToString().Contains(TEXT("Adresse")));
	TestTrue(TEXT("Steam: full"), UWESessionSubsystem::DescribeJoinResult(EOnJoinSessionCompleteResult::SessionIsFull).ToString().Contains(TEXT("voll")));
	TestTrue(TEXT("Steam: gone"), UWESessionSubsystem::DescribeJoinResult(EOnJoinSessionCompleteResult::SessionDoesNotExist).ToString().Contains(TEXT("nicht mehr")));
	TestTrue(TEXT("Steam: address"), UWESessionSubsystem::DescribeJoinResult(EOnJoinSessionCompleteResult::CouldNotRetrieveAddress).ToString().Contains(TEXT("nicht erreichbar")));

	// Login rules on the server.
	const FString Version = UWESessionSubsystem::GetGameVersion();
	TestTrue(TEXT("Room for a 4th"), AWEGameMode::CheckLogin(3, 4, FString::Printf(TEXT("?WEVersion=%s"), *Version)).IsEmpty());
	TestTrue(TEXT("Other version refused"), AWEGameMode::CheckLogin(1, 4, TEXT("?WEVersion=0.1.0")).Contains(TEXT("Andere Spielversion")));
	TestTrue(TEXT("Console join without version allowed"), AWEGameMode::CheckLogin(1, 4, TEXT("?Name=x")).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWESessionRejoinTest, "WorkshopEmpire.Session.LeaveAndRejoin", Flags)
bool FWESessionRejoinTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEGameMode* GameMode = TW.Spawn<AWEGameMode>(FVector::ZeroVector);
	if (!TestNotNull(TEXT("Game mode"), GameMode))
	{
		return false;
	}

	// Steam: the same id comes back.
	GameMode->RememberDepartedPlayer(MakePlayer(TEXT("steam:111"), TEXT("Anna"), 6));
	GameMode->RememberDepartedPlayer(MakePlayer(TEXT("steam:111"), TEXT("Anna"), 8)); // left twice: newest wins
	TestEqual(TEXT("One entry per player"), GameMode->GetDepartedPlayers().Num(), 1);

	// A save while Anna is away keeps her inventory.
	UWESaveGame* Save = FWESaveSystem::Capture(TW.World);
	FWESaveSystem::AddDepartedPlayers(Save, GameMode->GetDepartedPlayers()); // what Capture does with the session's game mode
	const FWESavedPlayer* InSave = FWESaveSystem::FindPlayer(Save->Players, TEXT("steam:111"), FString());
	TestTrue(TEXT("Departed player in the save"), InSave && InSave->Inventory.Num() == 1 && InSave->Inventory[0].Item.Quantity == 8);

	FWESavedPlayer Back;
	TestTrue(TEXT("Rejoin by id"), GameMode->TakeDepartedPlayer(TEXT("steam:111"), TEXT("Anna"), Back));
	TestEqual(TEXT("Inventory back"), Back.Inventory.Num() > 0 ? Back.Inventory[0].Item.Quantity : 0, 8);
	TestEqual(TEXT("Taken out of the list"), GameMode->GetDepartedPlayers().Num(), 0);

	// LAN: a new id each start, recognised by the (unique) name.
	GameMode->RememberDepartedPlayer(MakePlayer(TEXT("lan:aaa"), TEXT("Ben"), 3));
	TestTrue(TEXT("Rejoin by name"), GameMode->TakeDepartedPlayer(TEXT("lan:bbb"), TEXT("Ben"), Back) && Back.PlayerKey == TEXT("lan:aaa"));
	GameMode->RememberDepartedPlayer(MakePlayer(TEXT("lan:1"), TEXT("Spieler"), 1));
	GameMode->RememberDepartedPlayer(MakePlayer(TEXT("lan:2"), TEXT("Spieler"), 2));
	TestFalse(TEXT("Two with the same name: nobody gets the wrong inventory"), GameMode->TakeDepartedPlayer(TEXT("lan:3"), TEXT("Spieler"), Back));
	TestTrue(TEXT("The host is never matched by name"), FWESaveSystem::FindPlayer({ MakePlayer(TEXT("lan:9"), TEXT("Mike"), 1) }, TEXT("Host"), TEXT("Mike")) == nullptr);

	// Version 12 keeps names in the file.
	const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"), TEXT("SessionRejoin.wesave"));
	FText Error;
	TestTrue(TEXT("Write"), FWESaveSystem::WriteToFile(Save, Path, Error));
	const UWESaveGame* Read = FWESaveSystem::ReadFromFile(Path, Error);
	TestTrue(TEXT("Version 12"), Read && Read->SaveVersion == UWESaveGame::CurrentVersion && UWESaveGame::CurrentVersion >= 12);
	TestTrue(TEXT("Name saved"), Read && FWESaveSystem::FindPlayer(Read->Players, TEXT("other-id"), TEXT("Anna")) != nullptr);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
