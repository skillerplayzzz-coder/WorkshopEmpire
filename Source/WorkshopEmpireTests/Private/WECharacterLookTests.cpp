#include "Misc/AutomationTest.h"

#include "Components/PoseableMeshComponent.h"
#include "Player/WEAvatarComponent.h"
#include "Player/WEAvatarPreview.h"
#include "Player/WECharacter.h"
#include "Player/WECharacterLook.h"
#include "Save/WESaveGame.h"
#include "WETestItems.h"
#include "WETestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace WECharacterLookTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;
}

using namespace WECharacterLookTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWELookOptionsTest, "WorkshopEmpire.Character.LookOptions", Flags)
bool FWELookOptionsTest::RunTest(const FString& Parameters)
{
	// Every part of the look has choices with names; stepping wraps around and stays valid.
	for (int32 f = 0; f < static_cast<int32>(EWELookField::Num); ++f)
	{
		const EWELookField Field = static_cast<EWELookField>(f);
		TestTrue(*FString::Printf(TEXT("Field %d has at least two options"), f), WELook::NumOptions(Field) >= 2);
		TestFalse(*FString::Printf(TEXT("Field %d has a name"), f), WELook::GetFieldName(Field).IsEmpty());
		for (int32 o = 0; o < WELook::NumOptions(Field); ++o)
		{
			TestFalse(*FString::Printf(TEXT("Option %d/%d named"), f, o), WELook::GetOptionName(Field, o).IsEmpty());
		}
	}
	TestTrue(TEXT("Male and female"), WELook::NumOptions(EWELookField::Body) == 2);
	TestTrue(TEXT("At least 5 outfits (work clothes, casual, hoodie, t-shirt + work trousers, jacket)"), WELook::NumOptions(EWELookField::Outfit) >= 5);
	TestTrue(TEXT("Sneakers, work shoes, boots"), WELook::NumOptions(EWELookField::Shoes) >= 3);
	TestTrue(TEXT("Cap and beanie"), WELook::NumOptions(EWELookField::Headwear) >= 3);

	FWECharacterLook Look;
	Look.Outfit = static_cast<uint8>(WELook::NumOptions(EWELookField::Outfit) - 1);
	WELook::Step(Look, EWELookField::Outfit, 1);
	TestEqual(TEXT("Wraps to the first outfit"), static_cast<int32>(Look.Outfit), 0);
	WELook::Step(Look, EWELookField::Outfit, -1);
	TestEqual(TEXT("Wraps back"), static_cast<int32>(Look.Outfit), WELook::NumOptions(EWELookField::Outfit) - 1);

	// Phase 19: eight male faces (beards, stubble, freckles), five female ones (no beard; freckles, make-up).
	TestTrue(TEXT("At least 8 male faces"), WELook::NumOptionsFor(EWELookField::Face, 0) >= 8);
	TestTrue(TEXT("At least 5 female faces"), WELook::NumOptionsFor(EWELookField::Face, 1) >= 5);
	for (int32 o = 0; o < WELook::NumOptionsFor(EWELookField::Face, 1); ++o)
	{
		TestFalse(TEXT("Female face named"), WELook::GetOptionName(EWELookField::Face, o, 1).IsEmpty());
	}
	Look.Body = 0;
	Look.Face = 5;
	TestTrue(TEXT("Moustache"), WELook::GetFace(Look).Beard && FString(WELook::GetFace(Look).Beard) == TEXT("Hair_Moustache"));
	Look.Face = 7;
	TestTrue(TEXT("Freckles"), WELook::GetFace(Look).bFreckles && !WELook::GetFace(Look).Beard);
	Look.Face = 3;
	WELook::Step(Look, EWELookField::Body, 1);
	TestEqual(TEXT("Female"), static_cast<int32>(Look.Body), 1);
	TestTrue(TEXT("No beard after switching to female"), WELook::GetFace(Look).Beard == nullptr);
	for (int32 i = 0; i < 7; ++i)
	{
		WELook::Step(Look, EWELookField::Face, 1);
		TestTrue(TEXT("Female faces stay beardless and valid"), WELook::GetFace(Look).Beard == nullptr && Look.Face < WELook::NumOptionsFor(EWELookField::Face, 1));
	}
	Look.Face = 7;
	Look.Clamp();
	TestTrue(TEXT("Clamp maps a male face into the female list"), Look.Face < WELook::NumOptionsFor(EWELookField::Face, 1));
	Look.Face = 3;
	TestTrue(TEXT("Make-up"), WELook::GetFace(Look).bMakeup);
	FWECharacterLook Bad;
	Bad.Hair = 200;
	Bad.Skin = 99;
	Bad.Clamp();
	TestTrue(TEXT("Clamp keeps hair valid"), Bad.Hair < WELook::NumOptions(EWELookField::Hair));
	TestTrue(TEXT("Clamp keeps skin valid"), Bad.Skin < WELook::NumOptions(EWELookField::Skin));
	FWECharacterLook A = WELook::Random(7), B = WELook::Random(7);
	TestTrue(TEXT("Random is reproducible per seed"), A == B);
	B.Revision = 5;
	TestTrue(TEXT("The revision counter is not part of the look"), A == B);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWELookCharacterTest, "WorkshopEmpire.Character.LookOnCharacterAndSave", Flags)
bool FWELookCharacterTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWECharacter* Player = WETestItems::SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	if (!TestNotNull(TEXT("Player"), Player))
	{
		return false;
	}
	FWECharacterLook Chosen;
	Chosen.Body = 1;
	Chosen.Hair = 4;
	Chosen.Outfit = 2;
	Chosen.Headwear = 2;
	Player->SetLookAuthority(Chosen);
	TestTrue(TEXT("Look set"), Player->HasLook() && Player->GetLook() == Chosen);
	// A loaded game wins over what the client sends from its profile afterwards.
	FWECharacterLook Saved;
	Saved.Body = 0;
	Saved.Outfit = 4;
	Saved.Glasses = 1;
	Player->ApplySavedLook(Saved);
	TestTrue(TEXT("Saved look applied"), Player->GetLook() == Saved);
	Player->SetLookAuthority(Chosen);
	TestTrue(TEXT("Profile look does not overwrite the saved one"), Player->GetLook() == Saved);
	FWESavedPlayer Record;
	Record.bHasLook = Player->HasLook();
	Record.Look = Player->GetLook();
	TestTrue(TEXT("Save record carries the look"), Record.bHasLook && Record.Look == Saved);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEAvatarBuildTest, "WorkshopEmpire.Character.AvatarFollowsMannequin", Flags)
bool FWEAvatarBuildTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEAvatarPreview* Preview = TW.Spawn<AWEAvatarPreview>(FVector::ZeroVector);
	if (!TestNotNull(TEXT("Preview"), Preview))
	{
		return false;
	}
	FWECharacterLook Look;
	Look.Hair = 1;
	Look.Headwear = 1;
	Look.Glasses = 1;
	Preview->SetLook(Look);
	UWEAvatarComponent* Avatar = Preview->GetAvatar();
	if (!TestTrue(TEXT("Avatar body built (Quaternius models imported)"), Avatar && Avatar->HasBody()))
	{
		return false;
	}
	TestTrue(TEXT("Most bones are driven by the mannequin"), Avatar->GetMappedBoneCount() >= 40);
	TestTrue(TEXT("Body mesh set"), Avatar->GetBody()->GetSkinnedAsset() != nullptr);
	// Phase 19: real clothes as follower meshes - the outfit and the shoes.
	Look.Outfit = 2;
	Look.Shoes = 2;
	Preview->SetLook(Look);
	const TArray<FName> Garments = Avatar->GetGarments();
	TestTrue(TEXT("Hoodie, jeans and boots worn"), Garments.Contains(TEXT("Hoodie")) && Garments.Contains(TEXT("Jeans")) && Garments.Contains(TEXT("Boots")));
	Look.Outfit = 0;
	Preview->SetLook(Look);
	TestTrue(TEXT("Overall is one piece + shoes"), Avatar->GetGarments().Num() == 2 && Avatar->GetGarments().Contains(TEXT("Overall")));
	Look.Body = 1;
	Preview->SetLook(Look);
	TestTrue(TEXT("Female clothes too"), Avatar->GetGarments().Num() == 2);
	TestTrue(TEXT("Female body"), Avatar->GetBody() && Avatar->GetBody()->GetSkinnedAsset() && Avatar->GetBody()->GetSkinnedAsset()->GetName().Contains(TEXT("Female")));
	return true;
}

#endif
