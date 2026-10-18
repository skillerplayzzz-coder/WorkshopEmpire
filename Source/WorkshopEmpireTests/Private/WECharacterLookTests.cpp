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

	// Women: brows only, no beard (stepping and clamping both keep it so).
	Look.Body = 0;
	Look.Face = 3;
	WELook::Step(Look, EWELookField::Body, 1);
	TestEqual(TEXT("Female"), static_cast<int32>(Look.Body), 1);
	TestTrue(TEXT("No beard after switching to female"), Look.Face < 2);
	for (int32 i = 0; i < 5; ++i)
	{
		WELook::Step(Look, EWELookField::Face, 1);
		TestTrue(TEXT("Female faces stay beardless"), Look.Face < 2);
	}
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
	Look.Body = 1;
	Preview->SetLook(Look);
	TestTrue(TEXT("Female body"), Avatar->GetBody() && Avatar->GetBody()->GetSkinnedAsset() && Avatar->GetBody()->GetSkinnedAsset()->GetName().Contains(TEXT("Female")));
	return true;
}

#endif
