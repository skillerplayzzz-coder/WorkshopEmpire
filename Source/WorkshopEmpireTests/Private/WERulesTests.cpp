#include "Misc/AutomationTest.h"

#include "Economy/WEContractComponent.h"
#include "Economy/WECourierComponent.h"
#include "Economy/WEMarketComponent.h"
#include "Economy/WETraderTerminal.h"
#include "EngineUtils.h"
#include "Framework/WEGameRulesComponent.h"
#include "Framework/WEGameState.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "World/WEDayNightComponent.h"
#include "WETestItems.h"
#include "WETestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace WERulesTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;
}

using namespace WERulesTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWERulesPresetsTest, "WorkshopEmpire.Rules.PresetsAndOptions", Flags)
bool FWERulesPresetsTest::RunTest(const FString& Parameters)
{
	// Phase 21: the defaults are the game as before; presets fill every option; the URL option survives a round trip.
	const FWEGameRules Old;
	TestTrue(TEXT("Default: diagnosis on, no mistakes, reviews on, no courier"), Old.bDiagnosisRequired && !Old.bRepairMistakes && Old.bCustomerReviews && !Old.bCourier);
	TestEqual(TEXT("Default: normal prices"), Old.BuyPriceFactor(), 1.f);
	const FWEGameRules Relaxed = FWEGameRules::FromPreset(EWEDifficultyPreset::Relaxed);
	const FWEGameRules Expert = FWEGameRules::FromPreset(EWEDifficultyPreset::Expert);
	TestTrue(TEXT("Relaxed: no diagnosis, no mistakes, no reviews, cheaper, simpler"), !Relaxed.bDiagnosisRequired && !Relaxed.bRepairMistakes && !Relaxed.bCustomerReviews
		&& Relaxed.BuyPriceFactor() < 1.f && Relaxed.SellPriceFactor() > 1.f && Relaxed.RepairTimeFactor() < 1.f);
	TestTrue(TEXT("Expert: dearer, longer, more mistakes"), Expert.BuyPriceFactor() > 1.f && Expert.SellPriceFactor() < 1.f && Expert.RepairTimeFactor() > 1.f
		&& Expert.MistakeChance() > FWEGameRules::FromPreset(EWEDifficultyPreset::Realistic).MistakeChance());
	for (const EWEDifficultyPreset P : { EWEDifficultyPreset::Relaxed, EWEDifficultyPreset::Realistic, EWEDifficultyPreset::Expert })
	{
		const FWEGameRules R = FWEGameRules::FromPreset(P);
		FWEGameRules Back;
		TestTrue(TEXT("Option parses"), FWEGameRules::FromOption(R.ToOption(), Back));
		TestTrue(TEXT("Round trip"), Back == R);
		TestEqual(TEXT("Preset detected"), R.DetectPreset(), P);
		TestTrue(TEXT("New companies get the courier"), R.bCourier);
	}
	FWEGameRules Custom = Expert;
	Custom.bCustomerReviews = false;
	Custom.Preset = Custom.DetectPreset();
	TestEqual(TEXT("A changed option: Individuell"), Custom.Preset, EWEDifficultyPreset::Custom);
	TestTrue(TEXT("Describe names the options"), Custom.Describe().ToString().Contains(TEXT("Bewertungen aus")));
	FWEGameRules Bad;
	TestFalse(TEXT("Garbage is refused"), FWEGameRules::FromOption(TEXT("12x"), Bad));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWERulesEconomyTest, "WorkshopEmpire.Rules.EconomyPrices", Flags)
bool FWERulesEconomyTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	UWEItemDefinition* Ram = UWEItemDefinition::FindById(TEXT("RamModule"));
	if (!TestNotNull(TEXT("Item data"), Ram))
	{
		return false;
	}
	const FWEItemInstance Item = FWEItemInstance::Make(Ram, 1, EWEItemCondition::Used, 60);
	const UWEMarketComponent* Market = GameState->GetMarket();
	const int64 Normal = UWEMarketComponent::GetBuyPrice(Item, 1.f, Market).GetCents();
	GameState->GetRules()->SetRules(FWEGameRules::FromPreset(EWEDifficultyPreset::Expert));
	const int64 Hard = UWEMarketComponent::GetBuyPrice(Item, 1.f, Market).GetCents();
	GameState->GetRules()->SetRules(FWEGameRules::FromPreset(EWEDifficultyPreset::Relaxed));
	const int64 Easy = UWEMarketComponent::GetBuyPrice(Item, 1.f, Market).GetCents();
	TestTrue(TEXT("Hard: about 15 % dearer"), FMath::Abs(Hard - FMath::RoundToInt64(Normal * 1.15)) <= 2);
	TestTrue(TEXT("Relaxed: about 15 % cheaper"), FMath::Abs(Easy - FMath::RoundToInt64(Normal * 0.85)) <= 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWERulesCourierTest, "WorkshopEmpire.Rules.CourierParcels", Flags)
bool FWERulesCourierTest::RunTest(const FString& Parameters)
{
	// Ordered things travel as parcels; the parcels keep exactly what was ordered (also through saving); opening spawns it.
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	TW.Spawn<AWEDeliveryZone>(FVector(200.f, 0.f, 6.f));
	UWECourierComponent* Courier = GameState->GetCourier();
	UWEItemDefinition* Tool = UWEItemDefinition::FindById(TEXT("Screwdriver"));
	UWEItemDefinition* Heavy = UWEItemDefinition::FindById(TEXT("IndustrialMotorLarge"));
	if (!TestNotNull(TEXT("Courier"), Courier) || !TestNotNull(TEXT("Item data"), Tool) || !TestNotNull(TEXT("Parcel item"), UWEItemDefinition::FindById(TEXT("Parcel"))))
	{
		return false;
	}
	const FWEItemInstance Screwdriver = FWEItemInstance::Make(Tool, 2, EWEItemCondition::New, 100);
	TestFalse(TEXT("Old rules: no courier"), Courier->ShouldShip(Screwdriver));
	GameState->GetRules()->SetRules(FWEGameRules::FromPreset(EWEDifficultyPreset::Realistic));
	TestTrue(TEXT("Courier rule: ships"), Courier->ShouldShip(Screwdriver));
	TestFalse(TEXT("Too heavy for a parcel"), Heavy && Courier->ShouldShip(FWEItemInstance::Make(Heavy, 1, EWEItemCondition::Defective, 40)));
	Courier->Order(Screwdriver, TEXT("Test-Versand"), 30.f);
	TestEqual(TEXT("On its way"), Courier->GetPendingCount(), 1);
	TestTrue(TEXT("A visit is planned"), Courier->GetNextVisitIn() > 0.f);

	// Saved while on its way, loaded again.
	TArray<FWECourierOrder> Orders;
	TArray<FWEParcelContents> Parcels;
	Courier->GetSaveState(Orders, Parcels);
	TestTrue(TEXT("Saved with the remaining time"), Orders.Num() == 1 && Orders[0].DueAt > 25.f && Orders[0].DueAt <= 30.f);
	Courier->SetSaveState({}, {});
	TestEqual(TEXT("Cleared"), Courier->GetPendingCount(), 0);
	Courier->SetSaveState(Orders, Parcels);
	TestEqual(TEXT("Back after loading"), Courier->GetPendingCount(), 1);

	// The courier puts it down (here directly): one parcel with the screwdrivers inside.
	Courier->GetSaveState(Orders, Parcels);
	Courier->SetSaveState({}, {});
	Courier->DropParcels(Orders, FVector(200.f, 0.f, 0.f));
	AWEPickupItem* Parcel = nullptr;
	int32 ScrewdriversInWorld = 0;
	for (TActorIterator<AWEPickupItem> It(TW.World); It; ++It)
	{
		Parcel = UWECourierComponent::IsParcel(It->GetItem()) ? *It : Parcel;
		ScrewdriversInWorld += It->GetItem().Definition == Tool ? 1 : 0;
	}
	if (!TestNotNull(TEXT("A parcel on the pallet"), Parcel))
	{
		return false;
	}
	TestEqual(TEXT("Not yet unpacked"), ScrewdriversInWorld, 0);
	const FWEParcelContents* Contents = Courier->FindContents(Parcel->GetItem().InstanceId);
	TestTrue(TEXT("Contents known"), Contents && Contents->Items.Num() == 1 && Contents->Items[0].Quantity == 2 && Contents->Sender == TEXT("Test-Versand"));
	Courier->GetSaveState(Orders, Parcels);
	TestEqual(TEXT("Parcel contents are saved"), Parcels.Num(), 1);

	const FGuid ParcelId = Parcel->GetItem().InstanceId;
	const TArray<FWEItemInstance> Found = Courier->OpenParcel(Parcel, nullptr);
	TestTrue(TEXT("Exactly what was ordered"), Found.Num() == 1 && Found[0].Definition == Tool && Found[0].Quantity == 2 && Found[0].Condition == EWEItemCondition::New);
	TestTrue(TEXT("Parcel gone"), !IsValid(Parcel) || Parcel->IsActorBeingDestroyed());
	TestNull(TEXT("Contents forgotten"), Courier->FindContents(ParcelId));
	ScrewdriversInWorld = 0;
	for (TActorIterator<AWEPickupItem> It(TW.World); It; ++It)
	{
		ScrewdriversInWorld += It->GetItem().Definition == Tool && !It->IsActorBeingDestroyed() ? It->GetItem().Quantity : 0;
	}
	TestEqual(TEXT("The screwdrivers lie there"), ScrewdriversInWorld, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWERulesReviewsTest, "WorkshopEmpire.Rules.ReviewsAndReputation", Flags)
bool FWERulesReviewsTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	GameState->GetContracts()->SetCatalog(nullptr);
	UWEContractComponent* Contracts = GameState->GetContracts();
	TestEqual(TEXT("Unknown at first"), Contracts->GetReputation(), 0.f);
	TestEqual(TEXT("Base offers"), Contracts->GetMaxOffers(), UWEContractComponent::MaxOffers);
	for (int32 i = 0; i < 3; ++i)
	{
		Contracts->AddReview(5, TEXT("Kunde"), UWEContractComponent::MakeReviewText(5, true, false, i));
	}
	TestEqual(TEXT("Three reviews kept"), Contracts->GetReviews().Num(), 3);
	TestEqual(TEXT("Reputation 5"), Contracts->GetReputation(), 5.f);
	TestEqual(TEXT("Good name: one offer more"), Contracts->GetMaxOffers(), UWEContractComponent::MaxOffers + 1);
	TestTrue(TEXT("Good name: better pay"), Contracts->GetReputationRewardFactor() > 1.f);
	TestFalse(TEXT("Review text"), Contracts->GetReviews()[0].Text.IsEmpty());
	for (int32 i = 0; i < 20; ++i)
	{
		Contracts->AddReview(1, TEXT("Kunde"), UWEContractComponent::MakeReviewText(1, false, true, i));
	}
	TestEqual(TEXT("Only the last 12 kept"), Contracts->GetReviews().Num(), UWEContractComponent::MaxReviews);
	TestTrue(TEXT("Bad name: fewer offers, less pay"), Contracts->GetMaxOffers() < UWEContractComponent::MaxOffers && Contracts->GetReputationRewardFactor() < 1.f);
	// Without the rule: no reputation, no effect.
	GameState->GetRules()->SetRules(FWEGameRules::FromPreset(EWEDifficultyPreset::Relaxed));
	TestEqual(TEXT("Reviews off: no reputation"), Contracts->GetReputation(), 0.f);
	TestEqual(TEXT("Reviews off: base offers"), Contracts->GetMaxOffers(), UWEContractComponent::MaxOffers);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWERulesCourierDetailsTest, "WorkshopEmpire.Rules.CourierDetails", Flags)
bool FWERulesCourierDetailsTest::RunTest(const FString& Parameters)
{
	// One purchase of several pieces = one parcel; no courier at night; destroyed parcels are not saved; loading drops a walking courier.
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	TW.Spawn<AWEDeliveryZone>(FVector(200.f, 0.f, 6.f));
	GameState->GetRules()->SetRules(FWEGameRules::FromPreset(EWEDifficultyPreset::Realistic));
	UWECourierComponent* Courier = GameState->GetCourier();
	UWEItemDefinition* Tool = UWEItemDefinition::FindById(TEXT("Screwdriver"));
	if (!TestNotNull(TEXT("Item data"), Tool))
	{
		return false;
	}
	TArray<FWEItemInstance> Three;
	for (int32 i = 0; i < 3; ++i)
	{
		Three.Add(FWEItemInstance::Make(Tool, 1, EWEItemCondition::New, 100));
	}
	Courier->Order(Three, TEXT("Werkzeug-Versand"), 0.f);
	TestEqual(TEXT("Three pieces, one order"), Courier->GetPendingCount(), 1);

	if (UWEDayNightComponent* Clock = GameState->GetDayNight())
	{
		Clock->SetTime(1, 22.f * 60.f);
		TestFalse(TEXT("22:00: no courier"), Courier->IsDeliveryTime());
		Clock->SetTime(2, 9.f * 60.f);
		TestTrue(TEXT("09:00: courier comes"), Courier->IsDeliveryTime());
	}

	// Loading while he walks: he is gone, his parcel is pending again (not delivered twice).
	Courier->DevSendCourierNow();
	TestNotNull(TEXT("Courier on the way"), Courier->GetActiveCourier());
	TArray<FWECourierOrder> Orders;
	TArray<FWEParcelContents> Parcels;
	Courier->GetSaveState(Orders, Parcels);
	TestEqual(TEXT("His parcel counts as pending in the save"), Orders.Num(), 1);
	Courier->SetSaveState(Orders, Parcels);
	TestNull(TEXT("Loading removes the walking courier"), Courier->GetActiveCourier());
	TestEqual(TEXT("Pending once"), Courier->GetPendingCount(), 1);

	Courier->GetSaveState(Orders, Parcels);
	Courier->SetSaveState({}, {});
	Courier->DropParcels(Orders, FVector(200.f, 0.f, 0.f));
	AWEPickupItem* Parcel = nullptr;
	for (TActorIterator<AWEPickupItem> It(TW.World); It; ++It)
	{
		Parcel = UWECourierComponent::IsParcel(It->GetItem()) ? *It : Parcel;
	}
	if (!TestNotNull(TEXT("Parcel"), Parcel))
	{
		return false;
	}
	const FWEParcelContents* Contents = Courier->FindContents(Parcel->GetItem().InstanceId);
	TestTrue(TEXT("All three screwdrivers in the one parcel"), Contents && Contents->Items.Num() == 3);
	Parcel->Destroy();
	Courier->GetSaveState(Orders, Parcels);
	TestEqual(TEXT("A destroyed parcel is not saved"), Parcels.Num(), 0);
	return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS
