#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "WEMoney.h"
#include "WEMilestoneComponent.generated.h"

/** What a milestone counts (Phase 16). */
UENUM(BlueprintType)
enum class EWEMilestoneKind : uint8
{
	DevicesDisassembled UMETA(DisplayName = "Geräte zerlegt"),
	DevicesRepaired UMETA(DisplayName = "Geräte repariert"),
	OrdersCompleted UMETA(DisplayName = "Aufträge erledigt"),
	IndustrialOrders UMETA(DisplayName = "Industrieaufträge erledigt"),
	MachineCycles UMETA(DisplayName = "Maschinenvorgänge"),
	ToursCompleted UMETA(DisplayName = "Touren gefahren"),
	Revenue UMETA(DisplayName = "Umsatz (Cent)"),
	CompanyValue UMETA(DisplayName = "Firmenwert (Cent)"),
	CompanyTier UMETA(DisplayName = "Unternehmensstufe"),
	Employees UMETA(DisplayName = "Mitarbeiter"),
	RareFinds UMETA(DisplayName = "Sammlerstücke (Sorten)"),
};

USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEMilestoneDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Milestone") FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Milestone") FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Milestone") FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Milestone") EWEMilestoneKind Kind = EWEMilestoneKind::DevicesDisassembled;
	/** Count to reach (money in cents). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Milestone", meta = (ClampMin = "1")) int64 Target = 1;
	/** Paid into the company account when reached. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Milestone") FWEMoney Reward;
};

/** All milestones (Data Asset /Game/WorkshopEmpire/Data/Progression/DA_MilestoneCatalog, Tools/Maps/build_progression.py). */
UCLASS(BlueprintType)
class WORKSHOPEMPIRE_API UWEMilestoneCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Milestone") TArray<FWEMilestoneDefinition> Milestones;
};

/** Lifetime statistics of the company (saved since version 13). */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWECompanyStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Milestone") int32 DevicesDisassembled = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Milestone") int32 DevicesRepaired = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Milestone") int32 OrdersCompleted = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Milestone") int32 IndustrialOrders = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Milestone") int32 MachineCycles = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Milestone") int32 ToursCompleted = 0;
	/** Item ids of the different collectibles/rare finds sold or delivered. */
	UPROPERTY(BlueprintReadOnly, Category = "Milestone") TArray<FName> RareFinds;
};

DECLARE_MULTICAST_DELEGATE(FWEOnMilestonesChanged);

/**
 * Long-term goals (Phase 16) on the GameState: counts what the company does and pays a reward for each
 * reached milestone - from the first disassembled device to a company worth a million. Server only
 * counts and completes; statistics and completed ids replicate for the "Unternehmen" page.
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWEMilestoneComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEMilestoneComponent();

	static UWEMilestoneComponent* Get(const UObject* WorldContext);

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server: counts an event and checks the milestones. */
	void AddCount(EWEMilestoneKind Kind, int32 Amount = 1);
	/** Server: a rare item was sold or delivered (counts each kind once). */
	void AddRareFind(const class UWEItemDefinition* Item);
	/** Server: checks all milestones now (values like company value are read live). */
	void CheckMilestones();

	int64 GetProgress(EWEMilestoneKind Kind) const;
	bool IsCompleted(FName Id) const { return Completed.Contains(Id); }
	const FWECompanyStats& GetStats() const { return Stats; }
	const TArray<FName>& GetCompleted() const { return Completed; }
	const UWEMilestoneCatalog* GetCatalog() const { return Catalog; }
	void SetCatalog(UWEMilestoneCatalog* InCatalog) { Catalog = InCatalog; }
	/** Total of all rare kinds that exist in the catalogs (for "x of y"). */
	static int32 NumRareKinds();

	/** Save/load. */
	void SetState(const FWECompanyStats& InStats, const TArray<FName>& InCompleted);
	/** Text for the progress of one milestone, e.g. "37 / 50" or "12.300 € / 100.000 €". */
	FText GetProgressText(const FWEMilestoneDefinition& Milestone) const;

	FWEOnMilestonesChanged OnMilestonesChanged;

	static constexpr float CheckIntervalSeconds = 5.f;

protected:
	UFUNCTION()
	void OnRep_Milestones();

	UPROPERTY(ReplicatedUsing = OnRep_Milestones)
	FWECompanyStats Stats;

	UPROPERTY(ReplicatedUsing = OnRep_Milestones)
	TArray<FName> Completed;

	UPROPERTY(EditAnywhere, Category = "Milestone")
	TObjectPtr<UWEMilestoneCatalog> Catalog;

private:
	void CheckMilestonesOf(TOptional<EWEMilestoneKind> OnlyKind);

	FTimerHandle CheckTimer;
};
