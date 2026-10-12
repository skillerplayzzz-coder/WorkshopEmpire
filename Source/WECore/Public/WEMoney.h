#pragma once

#include "CoreMinimal.h"
#include "WEMoney.generated.h"

/**
 * Money amount stored as whole euro cents (int64).
 *
 * All prices, balances and transactions use this type instead of float so that
 * repeated buying/selling never accumulates rounding errors (spec section 10.6).
 * Arithmetic that could overflow goes through TryAdd/TrySubtract; the server is
 * the only place that is allowed to mutate company balances.
 */
USTRUCT(BlueprintType)
struct WECORE_API FWEMoney
{
	GENERATED_BODY()

	FWEMoney() = default;

	static FWEMoney FromCents(int64 InCents) { return FWEMoney(InCents); }
	static FWEMoney FromEuros(int64 Euros, int32 ExtraCents = 0);

	int64 GetCents() const { return Cents; }
	bool IsZero() const { return Cents == 0; }
	bool IsNegative() const { return Cents < 0; }

	/** Adds A and B into OutResult. Returns false (and leaves OutResult untouched) on int64 overflow. */
	static bool TryAdd(const FWEMoney& A, const FWEMoney& B, FWEMoney& OutResult);

	/** Subtracts B from A into OutResult. Returns false (and leaves OutResult untouched) on int64 overflow. */
	static bool TrySubtract(const FWEMoney& A, const FWEMoney& B, FWEMoney& OutResult);

	/**
	 * Fixed German-style display, e.g. "1.234,56 €" or "-0,05 €".
	 * Deterministic on purpose (used in tests); localized FText formatting comes with the UI system.
	 */
	FString ToDisplayString() const;

	bool operator==(const FWEMoney& Other) const { return Cents == Other.Cents; }
	bool operator!=(const FWEMoney& Other) const { return Cents != Other.Cents; }
	bool operator<(const FWEMoney& Other) const { return Cents < Other.Cents; }
	bool operator<=(const FWEMoney& Other) const { return Cents <= Other.Cents; }
	bool operator>(const FWEMoney& Other) const { return Cents > Other.Cents; }
	bool operator>=(const FWEMoney& Other) const { return Cents >= Other.Cents; }

private:
	explicit FWEMoney(int64 InCents) : Cents(InCents) {}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Money", meta = (AllowPrivateAccess = "true"))
	int64 Cents = 0;
};

FORCEINLINE uint32 GetTypeHash(const FWEMoney& Money)
{
	return GetTypeHash(Money.GetCents());
}
