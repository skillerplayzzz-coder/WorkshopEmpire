#include "Misc/AutomationTest.h"
#include "WEMoney.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace WEMoneyTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEMoneyConstructionTest, "WorkshopEmpire.Core.Money.Construction", WEMoneyTests::Flags)
bool FWEMoneyConstructionTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Default is zero"), FWEMoney().GetCents(), int64(0));
	TestTrue(TEXT("Default IsZero"), FWEMoney().IsZero());
	TestEqual(TEXT("Start capital 250 EUR = 25000 cents"), FWEMoney::FromEuros(250).GetCents(), int64(25000));
	TestEqual(TEXT("FromEuros with cents"), FWEMoney::FromEuros(12, 34).GetCents(), int64(1234));
	TestTrue(TEXT("Negative detected"), FWEMoney::FromCents(-1).IsNegative());
	TestTrue(TEXT("Ordering"), FWEMoney::FromCents(99) < FWEMoney::FromEuros(1));
	TestTrue(TEXT("Equality"), FWEMoney::FromEuros(1) == FWEMoney::FromCents(100));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEMoneyArithmeticTest, "WorkshopEmpire.Core.Money.Arithmetic", WEMoneyTests::Flags)
bool FWEMoneyArithmeticTest::RunTest(const FString& Parameters)
{
	FWEMoney Result;

	TestTrue(TEXT("Add succeeds"), FWEMoney::TryAdd(FWEMoney::FromEuros(250), FWEMoney::FromCents(1), Result));
	TestEqual(TEXT("Add result"), Result.GetCents(), int64(25001));

	TestTrue(TEXT("Subtract succeeds"), FWEMoney::TrySubtract(FWEMoney::FromEuros(250), FWEMoney::FromEuros(300), Result));
	TestEqual(TEXT("Subtract can go negative (caller decides if allowed)"), Result.GetCents(), int64(-5000));

	// 0.10 added 1000 times must be exactly 100.00 (the reason we do not use float).
	FWEMoney Sum;
	for (int32 i = 0; i < 1000; ++i)
	{
		FWEMoney::TryAdd(Sum, FWEMoney::FromCents(10), Sum);
	}
	TestEqual(TEXT("No rounding drift"), Sum, FWEMoney::FromEuros(100));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEMoneyOverflowTest, "WorkshopEmpire.Core.Money.Overflow", WEMoneyTests::Flags)
bool FWEMoneyOverflowTest::RunTest(const FString& Parameters)
{
	const FWEMoney Max = FWEMoney::FromCents(MAX_int64);
	const FWEMoney Min = FWEMoney::FromCents(MIN_int64);
	FWEMoney Result = FWEMoney::FromCents(42);

	TestFalse(TEXT("Max + 1 overflows"), FWEMoney::TryAdd(Max, FWEMoney::FromCents(1), Result));
	TestEqual(TEXT("Result untouched after failed add"), Result.GetCents(), int64(42));
	TestFalse(TEXT("Min + -1 overflows"), FWEMoney::TryAdd(Min, FWEMoney::FromCents(-1), Result));
	TestFalse(TEXT("Min - 1 overflows"), FWEMoney::TrySubtract(Min, FWEMoney::FromCents(1), Result));
	TestFalse(TEXT("Max - -1 overflows"), FWEMoney::TrySubtract(Max, FWEMoney::FromCents(-1), Result));
	TestEqual(TEXT("Result still untouched"), Result.GetCents(), int64(42));

	TestTrue(TEXT("Max + -1 is fine"), FWEMoney::TryAdd(Max, FWEMoney::FromCents(-1), Result));
	TestEqual(TEXT("Max - 1"), Result.GetCents(), MAX_int64 - 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEMoneyDisplayTest, "WorkshopEmpire.Core.Money.DisplayString", WEMoneyTests::Flags)
bool FWEMoneyDisplayTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Zero"), FWEMoney().ToDisplayString(), FString(TEXT("0,00 €")));
	TestEqual(TEXT("Start capital"), FWEMoney::FromEuros(250).ToDisplayString(), FString(TEXT("250,00 €")));
	TestEqual(TEXT("Thousands"), FWEMoney::FromCents(123456).ToDisplayString(), FString(TEXT("1.234,56 €")));
	TestEqual(TEXT("Millions"), FWEMoney::FromEuros(1000000).ToDisplayString(), FString(TEXT("1.000.000,00 €")));
	TestEqual(TEXT("Negative cents"), FWEMoney::FromCents(-5).ToDisplayString(), FString(TEXT("-0,05 €")));
	TestEqual(TEXT("Min int64 does not crash"), FWEMoney::FromCents(MIN_int64).ToDisplayString(), FString(TEXT("-92.233.720.368.547.758,08 €")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
