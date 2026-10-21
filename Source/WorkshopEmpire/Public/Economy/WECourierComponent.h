#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "Items/WEItemInstance.h"
#include "Player/WECharacterLook.h"
#include "WECourierComponent.generated.h"

class AWEPickupItem;
class AWECharacter;
class UWEAvatarComponent;
class UStaticMeshComponent;

/** Phase 21: one ordered thing on its way (one parcel). */
USTRUCT()
struct WORKSHOPEMPIRE_API FWECourierOrder
{
	GENERATED_BODY()

	UPROPERTY() FWEItemInstance Item;
	/** More things in the same parcel (several pieces of one purchase). */
	UPROPERTY() TArray<FWEItemInstance> More;
	/** World time (server) from which the courier may bring it. */
	UPROPERTY() float DueAt = 0.f;
	UPROPERTY() FString Sender;
};

/** Phase 21: what is inside one parcel lying around (by the parcel's instance id). */
USTRUCT()
struct WORKSHOPEMPIRE_API FWEParcelContents
{
	GENERATED_BODY()

	UPROPERTY() FGuid ParcelId;
	UPROPERTY() TArray<FWEItemInstance> Items;
	UPROPERTY() FString Sender;
};

/**
 * Phase 21: the parcel courier. With the rule "Paketbote" on, things bought from the mail-order traders (and the
 * classifieds) are not there at once: they are packed into parcels, and now and then (not more often than every
 * few minutes) a courier walks from the street to the delivery area, sets the parcels down and leaves. A parcel is
 * picked up like any device and [Q] opens it - inside is exactly what was ordered. Server logic on the game state;
 * orders and parcel contents are saved.
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWECourierComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWECourierComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	static UWECourierComponent* Get(const UObject* WorldContext);
	static bool IsParcel(const FWEItemInstance& Item);
	/** Couriers carry what one person can lift (like real parcel services). */
	static constexpr float MaxParcelKg = 31.5f;
	/** At least this long between two visits. */
	static constexpr float MinVisitGap = 150.f;
	static constexpr int32 MaxParcelsPerTrip = 4;

	/** Whether this item would go by courier under the current rules. */
	bool ShouldShip(const FWEItemInstance& Item) const;
	/** Server: orders Item; it arrives with a later visit. Delay < 0 = the usual delivery time. */
	void Order(const FWEItemInstance& Item, const FString& Sender, float DelaySeconds = -1.f);
	/** Server: several things in one parcel (e.g. 3 screwdrivers of one purchase). */
	void Order(const TArray<FWEItemInstance>& Items, const FString& Sender, float DelaySeconds = -1.f);
	/** Couriers only come in the daytime (07:00-19:00 game time). */
	bool IsDeliveryTime() const;

	/** Orders still on their way (replicated). */
	int32 GetPendingCount() const { return PendingCount; }
	/** Seconds until the next visit is planned (replicated; < 0 = none). */
	float GetNextVisitIn() const;

	/** Server, called by the courier at the delivery area: the parcels appear there. */
	/** bAtPallet false: set down at Near (in front of a closed door). */
	void DropParcels(const TArray<FWECourierOrder>& Orders, const FVector& Near, bool bAnnounce = true, bool bAtPallet = true);
	void AnnounceDelivered(int32 Count, bool bAtPallet = true);
	/** Server: opens a parcel - its contents appear around it and the parcel is gone. */
	TArray<FWEItemInstance> OpenParcel(AWEPickupItem* Parcel, AWECharacter* By);
	/** What is in a parcel (server; empty if unknown). */
	const FWEParcelContents* FindContents(const FGuid& ParcelId) const;

	/** Save / load. Remaining seconds instead of world times. */
	void GetSaveState(TArray<FWECourierOrder>& OutOrders, TArray<FWEParcelContents>& OutParcels) const;
	void SetSaveState(const TArray<FWECourierOrder>& InOrders, const TArray<FWEParcelContents>& InParcels);

	/** Dev/tests: the next visit comes now (ignores the gap and the due times). */
	void DevSendCourierNow();
	class AWECourier* GetActiveCourier() const { return ActiveCourier.Get(); }

protected:
	void Update();
	void SendCourier(bool bIgnoreTimes);
	void RefreshReplicated();

	TArray<FWECourierOrder> Pending;
	UPROPERTY() TArray<FWEParcelContents> Parcels;
	float LastVisit = -1000.f;
	TWeakObjectPtr<class AWECourier> ActiveCourier;
	FTimerHandle UpdateTimer;

	UPROPERTY(Replicated) int32 PendingCount = 0;
	/** World time of the next planned visit (replicated; < 0 = none). */
	UPROPERTY(Replicated) float NextVisitAt = -1.f;
};

UENUM()
enum class EWECourierState : uint8
{
	Arriving,
	Dropping,
	Leaving
};

/** Phase 21: the parcel courier on foot - comes from the street with a stack of parcels, puts them down, leaves. */
UCLASS()
class WORKSHOPEMPIRE_API AWECourier : public ACharacter
{
	GENERATED_BODY()

public:
	AWECourier();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	/** Server: what to bring, where to put it down and where to go afterwards. */
	void Setup(const TArray<FWECourierOrder>& InOrders, const FVector& InDrop, const FVector& InExit, int32 LookSeed, const FVector& InDoor = FVector::ZeroVector);
	bool IsDroppingOutside() const { return bOutside; }
	EWECourierState GetCourierState() const { return State; }
	int32 GetParcelCount() const { return Carrying; }
	/** What he still carries (saved as pending orders when the game is saved meanwhile). */
	TArray<FWECourierOrder> GetOrdersForSave() const;

protected:
	virtual void BeginPlay() override;
	UFUNCTION() void OnRep_Look();
	UFUNCTION() void OnRep_Carrying();
	bool MoveTo(const FVector& Goal, float DeltaSeconds);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UWEAvatarComponent> Avatar;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Stack;

	UPROPERTY(ReplicatedUsing = OnRep_Look) FWECharacterLook Look;
	UPROPERTY(Replicated) EWECourierState State = EWECourierState::Arriving;
	/** Parcels in the arms (replicated for the stack in the hands). */
	UPROPERTY(ReplicatedUsing = OnRep_Carrying) int32 Carrying = 0;

private:
	TArray<FWECourierOrder> Orders;
	FVector Drop = FVector::ZeroVector;
	FVector Exit = FVector::ZeroVector;
	/** In front of the door (outside): where he puts the parcels down when the door is shut. */
	FVector Outside = FVector::ZeroVector;
	FVector DoorAt = FVector::ZeroVector;
	bool bOutside = false;
	TArray<FVector> Path;
	int32 PathIndex = 0;
	FVector PathGoal = FVector(FLT_MAX);
	float StateTime = 0.f;
	float StuckTimer = 0.f;
	float LifeTime = 0.f;
	FVector LastCheck = FVector::ZeroVector;
};
