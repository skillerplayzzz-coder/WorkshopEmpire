#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "Items/WEItemTypes.h"
#include "WEMoney.h"
#include "WEContractComponent.generated.h"

class AWECharacter;
class UWEItemDefinition;

/** Blueprint for generated customer orders. */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEContractTemplate
{
	GENERATED_BODY()

	/** E.g. "Bastler sucht Arbeitsspeicher". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract")
	TObjectPtr<UWEItemDefinition> Item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract", meta = (ClampMin = "1"))
	int32 MinQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract", meta = (ClampMin = "1"))
	int32 MaxQuantity = 1;

	/** Worst condition the customer still accepts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract")
	EWEItemCondition MinCondition = EWEItemCondition::Damaged;

	/** Reward = market value of the requested items x this factor (orders pay better than traders). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract", meta = (ClampMin = "0.1"))
	float RewardFactor = 1.4f;

	/** Time to deliver after accepting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract", meta = (ClampMin = "1", Units = "Minutes"))
	float DurationMinutes = 20.f;
};

/** All order templates (one Data Asset). */
UCLASS(BlueprintType)
class WORKSHOPEMPIRE_API UWEContractCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract")
	TArray<FWEContractTemplate> Templates;
};

UENUM(BlueprintType)
enum class EWEContractState : uint8
{
	Offered,
	Accepted
};

USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEContract
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Contract") FGuid Id;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") FText Title;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") TObjectPtr<UWEItemDefinition> Item;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") int32 Quantity = 1;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") EWEItemCondition MinCondition = EWEItemCondition::Damaged;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") FWEMoney Reward;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") EWEContractState State = EWEContractState::Offered;
	/** Offered: offer disappears then. Accepted: delivery deadline. Server world time. */
	UPROPERTY(BlueprintReadOnly, Category = "Contract") float ExpiresAt = 0.f;
	/** Accepted: duration after accepting, in seconds (copied from the template). */
	UPROPERTY(BlueprintReadOnly, Category = "Contract") float DeliverySeconds = 1200.f;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") FString AcceptedBy;
};

UENUM()
enum class EWEContractResult : uint8
{
	Success,
	NotFound,
	WrongState,
	TooManyAccepted,
	MissingItems
};

DECLARE_MULTICAST_DELEGATE(FWEOnContractsChanged);

/**
 * Customer orders (spec §5.1, §10.2): the server keeps a few offers, players accept them for the
 * company and deliver matching items (right item, condition at least MinCondition, not partially
 * disassembled) before the deadline. Reward goes to the company account. Lives on AWEGameState.
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWEContractComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEContractComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	static UWEContractComponent* Get(const UObject* WorldContext);

	const TArray<FWEContract>& GetContracts() const { return Contracts; }
	const FWEContract* FindContract(const FGuid& Id) const;

	/** Server time (or local time in tests without game state). */
	float GetServerTime() const;

	/** Items in Character's inventory that would count for Contract. */
	static int32 CountDeliverable(const FWEContract& Contract, const AWECharacter* Character);

	// --- Server only ------------------------------------------------------------------------------

	void SetCatalog(UWEContractCatalog* InCatalog) { Catalog = InCatalog; }

	/** Removes expired contracts and tops the offers up. Runs on a timer; public for tests. */
	void UpdateContracts();

	/** Creates one offer from a random template (tests may pass a fixed index). */
	bool AddOffer(int32 TemplateIndex = INDEX_NONE);

	EWEContractResult Accept(const FGuid& Id, const FString& PlayerName);
	EWEContractResult Deliver(const FGuid& Id, AWECharacter* Character);

	/** Loading / tests. */
	void SetContracts(const TArray<FWEContract>& InContracts);

	FWEOnContractsChanged OnContractsChanged;

	static constexpr int32 MaxOffers = 3;
	static constexpr int32 MaxAccepted = 3;
	/** How long an unaccepted offer stays on the board. */
	static constexpr float OfferLifetimeSeconds = 600.f;

protected:
	UFUNCTION()
	void OnRep_Contracts();

	UPROPERTY(EditAnywhere, Category = "Contracts")
	TObjectPtr<UWEContractCatalog> Catalog;

	UPROPERTY(ReplicatedUsing = OnRep_Contracts)
	TArray<FWEContract> Contracts;

private:
	FTimerHandle UpdateTimer;
};
