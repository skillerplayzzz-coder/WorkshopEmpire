#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/WEItemTypes.h"
#include "WEMoney.h"
#include "WEStaffTypes.generated.h"

class UWEItemDefinition;

/** Employee roles (§15, Phase 11). Repair technicians, production workers and a workshop manager follow later. */
UENUM(BlueprintType)
enum class EWEStaffRole : uint8
{
	/** Lagerarbeiter: carries material from A to B. */
	WarehouseWorker,
	/** Maschinenbediener: keeps one machine running (switches it on, maintains it, clears the output). */
	MachineOperator,
	/** Fahrer: drives the van on tours. */
	Driver
};

UENUM(BlueprintType)
enum class EWETaskKind : uint8
{
	None,
	/** Source (storage or machine output) -> target (storage, machine input or van). */
	Transport,
	/** Target = machine; Source (optional) = storage for products and maintenance kits. */
	OperateMachine,
	/** Target = van. */
	DriveVehicle
};

UENUM(BlueprintType)
enum class EWEEmployeeState : uint8
{
	Idle,
	Walking,
	Working,
	/** Something is missing (empty source, full target); tries again shortly. */
	Waiting,
	Paused,
	/** Wages not paid: works again after the next successful payroll. */
	Unpaid,
	/** Driver on a tour (not in the workshop). */
	Away
};

/** Who an applicant / employee is. */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEEmployeeProfile
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Staff") FString Name;
	UPROPERTY(BlueprintReadOnly, Category = "Staff") EWEStaffRole Role = EWEStaffRole::WarehouseWorker;
	/** 1-3: faster walking and working, higher wage. */
	UPROPERTY(BlueprintReadOnly, Category = "Staff") uint8 Skill = 1;
	UPROPERTY(BlueprintReadOnly, Category = "Staff") int32 WageCentsPerMinute = 60;

	bool IsValid() const { return !Name.IsEmpty(); }
	float GetSpeedFactor() const { return 1.f + 0.2f * (FMath::Clamp<int32>(Skill, 1, 3) - 1); }
};

/** What an employee does. Actors are referenced directly (replicated; saved by actor name). */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEEmployeeTask
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Staff") EWETaskKind Kind = EWETaskKind::None;
	UPROPERTY(BlueprintReadOnly, Category = "Staff") TObjectPtr<AActor> Source;
	UPROPERTY(BlueprintReadOnly, Category = "Staff") TObjectPtr<AActor> Target;
	/** Transport: only this material (None = everything the target takes). */
	UPROPERTY(BlueprintReadOnly, Category = "Staff") TObjectPtr<UWEItemDefinition> Filter;
};

USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEStaffRoleDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Staff") EWEStaffRole Role = EWEStaffRole::WarehouseWorker;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Staff") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Staff", meta = (MultiLine = "true")) FText Description;
	/** Wage at skill 1; skill 2 = +25 %, skill 3 = +50 %. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Staff", meta = (ClampMin = "1")) int32 WageCentsPerMinute = 60;
	/** Work clothes colour (placeholder body). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Staff") FLinearColor Tint = FLinearColor(0.2f, 0.35f, 0.6f);
};

/** Roles and applicant names (Data Asset DA_StaffCatalog). */
UCLASS(BlueprintType)
class WORKSHOPEMPIRE_API UWEStaffCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	const FWEStaffRoleDefinition* FindRole(EWEStaffRole Role) const
	{
		return Roles.FindByPredicate([Role](const FWEStaffRoleDefinition& R) { return R.Role == Role; });
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Staff") TArray<FWEStaffRoleDefinition> Roles;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Staff") TArray<FString> FirstNames;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Staff") TArray<FString> LastNames;
	/** At most this many employees. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Staff", meta = (ClampMin = "1")) int32 MaxEmployees = 6;
};

UENUM(BlueprintType)
enum class EWETourKind : uint8
{
	/** Sell the whole cargo directly at the trader (better price, no shipping). */
	Sell,
	/** Pick up a bulk purchase (paid at departure). */
	Pickup,
	/** Phase 12: deliver accepted orders and supply contracts from the cargo. */
	Deliver
};

USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWETourItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tour") TObjectPtr<UWEItemDefinition> Item;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tour", meta = (ClampMin = "1")) int32 Quantity = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tour") EWEItemCondition Condition = EWEItemCondition::Defective;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tour", meta = (ClampMin = "0", ClampMax = "100")) int32 Quality = 40;
};

/** A van tour (§16 Waren transportieren). */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWETourDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tour") FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tour") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tour", meta = (MultiLine = "true")) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tour") EWETourKind Kind = EWETourKind::Sell;
	/** Duration with the basic van (the better van is faster). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tour", meta = (ClampMin = "0.1")) float Minutes = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tour") FWEMoney FuelCost;
	/** Sell tours: factor on the best trader price for delivering directly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tour", meta = (ClampMin = "0")) float SellFactor = 1.15f;
	/** Pickup tours: price of the lot and what comes back. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tour") FWEMoney PickupPrice;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tour") TArray<FWETourItem> PickupItems;
};

UCLASS(BlueprintType)
class WORKSHOPEMPIRE_API UWETourCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	const FWETourDefinition* Find(FName Id) const
	{
		return Tours.FindByPredicate([Id](const FWETourDefinition& T) { return T.Id == Id; });
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tour") TArray<FWETourDefinition> Tours;
};
