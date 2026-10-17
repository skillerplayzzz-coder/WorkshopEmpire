#include "Misc/AutomationTest.h"

#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "Framework/WEGameState.h"
#include "Items/WEItemDefinition.h"
#include "Logistics/WEVehicle.h"
#include "Progression/WEMilestoneComponent.h"
#include "Progression/WEProgressionComponent.h"
#include "Save/WESaveGame.h"
#include "Save/WESaveSystem.h"
#include "Staff/WEStaffComponent.h"
#include "WETestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace WEMilestoneTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	struct FFixture
	{
		FWETestWorld TW;
		AWEGameState* GameState = nullptr;
		UWECompanyComponent* Company = nullptr;
		UWEMilestoneComponent* Milestones = nullptr;
		UWEProgressionComponent* Progression = nullptr;

		FFixture()
		{
			GameState = TW.SpawnGameState<AWEGameState>();
			GameState->GetContracts()->SetCatalog(nullptr);
			Company = GameState->GetCompany();
			Milestones = GameState->GetMilestones();
			Progression = GameState->GetProgression();
			Company->SetBalance(FWEMoney());
		}

		const FWEMilestoneDefinition* Find(const TCHAR* Id) const
		{
			return Milestones->GetCatalog() ? Milestones->GetCatalog()->Milestones.FindByPredicate([Id](const FWEMilestoneDefinition& M) { return M.Id == FName(Id); }) : nullptr;
		}
	};
}

using namespace WEMilestoneTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEMilestoneTest, "WorkshopEmpire.Progression.Milestones", Flags)
bool FWEMilestoneTest::RunTest(const FString& Parameters)
{
	FFixture F;
	const FWEMilestoneDefinition* Dis5 = F.Find(TEXT("Dis5"));
	const FWEMilestoneDefinition* Rare1 = F.Find(TEXT("Rare1"));
	const FWEMilestoneDefinition* Tier2 = F.Find(TEXT("Tier2"));
	if (!TestNotNull(TEXT("Catalog with milestones"), F.Milestones->GetCatalog()) || !TestNotNull(TEXT("Dis5"), Dis5) || !TestNotNull(TEXT("Rare1"), Rare1)
		|| !TestNotNull(TEXT("Tier2"), Tier2))
	{
		return false;
	}
	TestTrue(TEXT("Catalog covers the whole game (25+ goals)"), F.Milestones->GetCatalog()->Milestones.Num() >= 25);

	// Counting and paying exactly once.
	F.Milestones->AddCount(EWEMilestoneKind::DevicesDisassembled, 4);
	TestFalse(TEXT("4 of 5: not yet"), F.Milestones->IsCompleted(Dis5->Id));
	TestEqual(TEXT("Progress text"), F.Milestones->GetProgressText(*Dis5).ToString(), FString(TEXT("4 / 5")));
	F.Milestones->AddCount(EWEMilestoneKind::DevicesDisassembled);
	TestTrue(TEXT("5 of 5: reached"), F.Milestones->IsCompleted(Dis5->Id));
	TestEqual(TEXT("Reward paid"), F.Company->GetBalance(), Dis5->Reward);
	F.Milestones->AddCount(EWEMilestoneKind::DevicesDisassembled);
	F.Milestones->CheckMilestones();
	TestEqual(TEXT("...only once"), F.Company->GetBalance(), Dis5->Reward);
	TestEqual(TEXT("Reward booked as other income, not as sales"), F.Company->GetCategoryTotalCents(EWEBookingCategory::Sales), int64(0));

	// Collectors' pieces count once per kind; common items do not count.
	const FWEMoney Before = F.Company->GetBalance();
	F.Milestones->AddRareFind(UWEItemDefinition::FindById(TEXT("SteelScrap")));
	F.Milestones->AddRareFind(UWEItemDefinition::FindById(TEXT("RareGpu")));
	F.Milestones->AddRareFind(UWEItemDefinition::FindById(TEXT("RareGpu")));
	TestEqual(TEXT("One kind of rare find"), F.Milestones->GetStats().RareFinds.Num(), 1);
	TestTrue(TEXT("First rare find milestone"), F.Milestones->IsCompleted(Rare1->Id));
	TestEqual(TEXT("Its reward"), F.Company->GetBalance().GetCents(), Before.GetCents() + Rare1->Reward.GetCents());

	// Values read live: company tier from a building.
	F.Progression->GrantUpgrade(TEXT("WorkshopExtension"));
	F.Milestones->CheckMilestones();
	TestTrue(TEXT("Tier 2 milestone"), F.Milestones->IsCompleted(Tier2->Id));

	// Money milestones show money.
	if (const FWEMilestoneDefinition* Revenue = F.Find(TEXT("Rev10k")))
	{
		TestTrue(TEXT("Money progress in euros"), F.Milestones->GetProgressText(*Revenue).ToString().Contains(TEXT("€")));
	}

	// Saved and loaded with the company (save version 13).
	F.Milestones->AddCount(EWEMilestoneKind::ToursCompleted, 3);
	UWESaveGame* Save = FWESaveSystem::Capture(F.TW.World);
	TestEqual(TEXT("Save version 13"), Save->SaveVersion, UWESaveGame::CurrentVersion);
	FFixture G;
	FWESaveSystem::Apply(G.TW.World, Save);
	TestTrue(TEXT("Loaded: completed milestones"), G.Milestones->IsCompleted(Dis5->Id) && G.Milestones->IsCompleted(Rare1->Id));
	TestEqual(TEXT("Loaded: statistics"), G.Milestones->GetStats().DevicesDisassembled, 6);
	TestEqual(TEXT("Loaded: tours"), G.Milestones->GetStats().ToursCompleted, 3);
	const FWEMoney Loaded = G.Company->GetBalance();
	G.Milestones->CheckMilestones();
	TestEqual(TEXT("Loading does not pay again"), G.Company->GetBalance(), Loaded);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWETierToursTest, "WorkshopEmpire.Logistics.TierToursAndTruck", Flags)
bool FWETierToursTest::RunTest(const FString& Parameters)
{
	FFixture F;
	const UWETourCatalog* Tours = F.GameState->GetStaff()->GetTourCatalog();
	if (!TestNotNull(TEXT("Tour catalog"), Tours))
	{
		return false;
	}
	// Data: bigger lots need a bigger company; chance entries are only for rare finds.
	int32 TierTours = 0;
	for (const FWETourDefinition& Tour : Tours->Tours)
	{
		TierTours += Tour.MinTier > 1 ? 1 : 0;
		for (const FWETourItem& Item : Tour.PickupItems)
		{
			if (Item.Chance < 1.f)
			{
				TestTrue(*FString::Printf(TEXT("%s: only rare finds are left to chance"), *Tour.Id.ToString()), Item.Item && Item.Item->Rarity >= EWEItemRarity::Rare);
			}
		}
	}
	TestTrue(TEXT("Tours for tiers 2-4"), TierTours >= 3);

	AWEVehicle* Van = F.TW.Spawn<AWEVehicle>(FVector(500.f, 0.f, 0.f));
	F.Progression->GrantUpgrade(TEXT("OldVan"));
	F.Progression->ApplyEffects();
	for (int32 i = 0; i < Tours->Tours.Num() && Van->GetSelectedTour() != FName(TEXT("PickupOfficeClearance")); ++i)
	{
		Van->CycleTour();
	}
	TestEqual(TEXT("Office clearance selected"), Van->GetSelectedTour(), FName(TEXT("PickupOfficeClearance")));
	TestTrue(TEXT("Needs tier 3"), Van->GetStartBlockReason().ToString().Contains(TEXT("Stufe 3")));
	TestFalse(TEXT("Cannot start"), Van->StartTour(nullptr));

	// Truck: third vehicle level after the commercial hall.
	const float VanSpeed = Van->GetStats().TourSpeed;
	F.Progression->GrantUpgrade(TEXT("WorkshopExtension"));
	F.Progression->GrantUpgrade(TEXT("CommercialHall"));
	TestFalse(TEXT("Tier 3 reached: no tier block any more"), Van->GetStartBlockReason().ToString().Contains(TEXT("Stufe")));
	F.Progression->GrantUpgrade(TEXT("Truck"));
	F.Progression->ApplyEffects();
	TestTrue(TEXT("Truck is faster"), Van->GetStats().TourSpeed > VanSpeed * 2.f);
	TestEqual(TEXT("Truck cargo"), Van->GetStats().CargoSlots, 40);
	// Regression (Phase 15 drive animation): the van stays where it is parked when it becomes visible.
	TestTrue(TEXT("Van still at its parking place"), Van->GetActorLocation().Equals(FVector(500.f, 0.f, 0.f), 1.f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
