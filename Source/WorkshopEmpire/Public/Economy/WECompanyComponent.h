#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WEMoney.h"
#include "WECompanyComponent.generated.h"

/** One booking on the company account (shown in the trade UI, newest first). */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWETransactionRecord
{
	GENERATED_BODY()

	/** Positive = income, negative = expense. */
	UPROPERTY(BlueprintReadOnly, Category = "Economy")
	FWEMoney Amount;

	UPROPERTY(BlueprintReadOnly, Category = "Economy")
	FString Description;

	UPROPERTY(BlueprintReadOnly, Category = "Economy")
	FString PlayerName;
};

/** What a booking was for (Unternehmensverwaltung, Phase 12). Derived from the booking text. */
UENUM(BlueprintType)
enum class EWEBookingCategory : uint8
{
	Sales,
	Orders,
	Purchases,
	Equipment,
	Expansion,
	Wages,
	Power,
	Logistics,
	Other,
	Count UMETA(Hidden)
};

DECLARE_MULTICAST_DELEGATE(FWEOnCompanyChanged);

/**
 * The shared company account (spec §10, §17.1). Lives on AWEGameState, so every player sees the
 * same balance. Only the server changes it, always in whole cents (FWEMoney), never below zero.
 *
 * Double-booking protection (§10.6): every client request carries a GUID; the server remembers the
 * last few hundred and ignores a repeated one (network resend, double click, modified client).
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWECompanyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWECompanyComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Finds the company of a world (on its game state), or nullptr. */
	static UWECompanyComponent* Get(const UObject* WorldContext);

	FWEMoney GetBalance() const { return Balance; }
	bool CanAfford(const FWEMoney& Amount) const { return !Amount.IsNegative() && Balance >= Amount; }
	const TArray<FWETransactionRecord>& GetRecentTransactions() const { return RecentTransactions; }

	// --- Books (Phase 12) ---------------------------------------------------------------------------

	static EWEBookingCategory Classify(const FString& Description);
	static FText GetCategoryName(EWEBookingCategory Category);
	/** Net amount of a category since the company started (income positive, expenses negative). */
	int64 GetCategoryTotalCents(EWEBookingCategory Category) const;
	/** The same for the last hour of play. */
	int64 GetCategoryLastHourCents(EWEBookingCategory Category) const;
	const TArray<int64>& GetCategoryTotals() const { return CategoryTotals; }
	/** Loading. */
	void SetCategoryTotals(const TArray<int64>& InTotals);

	// --- Server only ------------------------------------------------------------------------------

	/** Adds income. Returns false (and changes nothing) for negative amounts or overflow. */
	bool Deposit(const FWEMoney& Amount, const FString& Description, const FString& PlayerName = FString());

	/** Pays an expense. Returns false (and changes nothing) if the company cannot afford it. */
	bool TryWithdraw(const FWEMoney& Amount, const FString& Description, const FString& PlayerName = FString());

	/** Sets the balance directly (new game, loading). */
	void SetBalance(const FWEMoney& NewBalance);

	/**
	 * Registers a client request id. Returns false if this id was already processed, in which case
	 * the request must be ignored. Invalid ids are rejected.
	 */
	bool RegisterRequest(const FGuid& RequestId);

	FWEOnCompanyChanged OnCompanyChanged;

	static constexpr int32 MaxRecentTransactions = 12;
	static constexpr int32 MaxRememberedRequests = 512;

protected:
	UFUNCTION()
	void OnRep_Company();

	void AddRecord(const FWEMoney& Amount, const FString& Description, const FString& PlayerName);

	UPROPERTY(ReplicatedUsing = OnRep_Company)
	FWEMoney Balance;

	UPROPERTY(ReplicatedUsing = OnRep_Company)
	TArray<FWETransactionRecord> RecentTransactions;

	UPROPERTY(Replicated)
	TArray<int64> CategoryTotals;

	UPROPERTY(Replicated)
	TArray<int64> CategoryLastHour;

	/** Server: bookings of the last hour (time, category, cents). */
	struct FHourEntry { float Time; uint8 Category; int64 Cents; };
	TArray<FHourEntry> HourEntries;
	void UpdateLastHour();

private:
	TSet<FGuid> ProcessedRequests;
	TArray<FGuid> ProcessedOrder;
};
