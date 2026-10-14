#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "Items/WEItemTypes.h"
#include "WEMoney.h"
#include "WEContractComponent.generated.h"

class AWECharacter;
class UWEItemDefinition;
struct FWEItemInstance;

UENUM(BlueprintType)
enum class EWEContractKind : uint8
{
	/** Customer wants items: deliver them from the inventory. */
	Supply,
	/** Customer brings a broken device (on the delivery pallet): repair it and hand it back. */
	Repair
};

/** Blueprint for generated customer orders. */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEContractTemplate
{
	GENERATED_BODY()

	/** E.g. "Bastler sucht Arbeitsspeicher". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract")
	EWEContractKind Kind = EWEContractKind::Supply;

	/** Repair orders: condition of the customer's device when it arrives. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract", meta = (EditCondition = "Kind == EWEContractKind::Repair"))
	EWEItemCondition CustomerCondition = EWEItemCondition::Defective;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract")
	TObjectPtr<UWEItemDefinition> Item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract", meta = (ClampMin = "1"))
	int32 MinQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract", meta = (ClampMin = "1"))
	int32 MaxQuantity = 1;

	/** Lowest quality (0-100) the customer accepts (Phase 7). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contract", meta = (ClampMin = "0", ClampMax = "100"))
	uint8 MinQuality = 0;

	/** Worst condition the customer still accepts (repair orders: condition after the repair). */
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
	UPROPERTY(BlueprintReadOnly, Category = "Contract") uint8 MinQuality = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") FWEMoney Reward;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") EWEContractState State = EWEContractState::Offered;
	/** Offered: offer disappears then. Accepted: delivery deadline. Server world time. */
	UPROPERTY(BlueprintReadOnly, Category = "Contract") float ExpiresAt = 0.f;
	/** Accepted: duration after accepting, in seconds (copied from the template). */
	UPROPERTY(BlueprintReadOnly, Category = "Contract") float DeliverySeconds = 1200.f;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") FString AcceptedBy;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") EWEContractKind Kind = EWEContractKind::Supply;
	UPROPERTY(BlueprintReadOnly, Category = "Contract") EWEItemCondition CustomerCondition = EWEItemCondition::Defective;
	/** Repair orders: instance id of the customer's device (spawned when the order is accepted). */
	UPROPERTY(BlueprintReadOnly, Category = "Contract") FGuid CustomerItemId;
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

	/** Items in Character's inventory that would count for Contract (repair orders: 1 if the repaired device is in the hands). */
	static int32 CountDeliverable(const FWEContract& Contract, const AWECharacter* Character);

	/** Repair orders: the customer's device is repaired (working, not taken apart). */
	static bool IsRepairedCustomerItem(const FWEContract& Contract, const FWEItemInstance& Item);

	/** Supply orders: Item counts (right item, condition, quality, intact, not a customer's). */
	static bool Matches(const FWEContract& Contract, const FWEItemInstance& Item);

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

	/** Repair orders: removes the customer's device from the world (order expired: the customer took it back). */
	void RemoveCustomerItem(const FGuid& ItemId);

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
