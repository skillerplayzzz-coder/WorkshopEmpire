#include "WEMoney.h"

FWEMoney FWEMoney::FromEuros(int64 Euros, int32 ExtraCents)
{
	return FWEMoney(Euros * 100 + ExtraCents);
}

bool FWEMoney::TryAdd(const FWEMoney& A, const FWEMoney& B, FWEMoney& OutResult)
{
	if ((B.Cents > 0 && A.Cents > MAX_int64 - B.Cents) ||
		(B.Cents < 0 && A.Cents < MIN_int64 - B.Cents))
	{
		return false;
	}
	OutResult = FWEMoney(A.Cents + B.Cents);
	return true;
}

bool FWEMoney::TrySubtract(const FWEMoney& A, const FWEMoney& B, FWEMoney& OutResult)
{
	if ((B.Cents < 0 && A.Cents > MAX_int64 + B.Cents) ||
		(B.Cents > 0 && A.Cents < MIN_int64 + B.Cents))
	{
		return false;
	}
	OutResult = FWEMoney(A.Cents - B.Cents);
	return true;
}

FString FWEMoney::ToDisplayString() const
{
	// Work on the unsigned magnitude so MIN_int64 does not overflow on negation.
	const bool bNegative = Cents < 0;
	const uint64 Magnitude = bNegative ? static_cast<uint64>(-(Cents + 1)) + 1 : static_cast<uint64>(Cents);

	const uint64 Euros = Magnitude / 100;
	const uint64 Rest = Magnitude % 100;

	// Group euros in thousands with '.'.
	FString EuroDigits = FString::Printf(TEXT("%llu"), Euros);
	FString Grouped;
	const int32 Len = EuroDigits.Len();
	for (int32 i = 0; i < Len; ++i)
	{
		if (i > 0 && (Len - i) % 3 == 0)
		{
			Grouped.AppendChar(TEXT('.'));
		}
		Grouped.AppendChar(EuroDigits[i]);
	}

	return FString::Printf(TEXT("%s%s,%02llu €"), bNegative ? TEXT("-") : TEXT(""), *Grouped, Rest);
}
