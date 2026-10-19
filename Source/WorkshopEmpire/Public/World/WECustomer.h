#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interaction/WEInteractable.h"
#include "Items/WEItemInstance.h"
#include "Player/WECharacterLook.h"
#include "WEMoney.h"
#include "WECustomer.generated.h"

class UWEAvatarComponent;

UENUM()
enum class EWECustomerWish : uint8
{
	/** Brings a broken device: a repair order if the workshop takes it. */
	Repair,
	/** Wants to sell a device to the workshop for a price. */
	SellToUs,
	/** Looks for a part and pays well for it. */
	BuyFromUs
};

UENUM()
enum class EWECustomerState : uint8
{
	Arriving,
	Waiting,
	Leaving
};

/**
 * Phase 19: a walk-in customer. During opening hours people come from the street to the garage door with a wish:
 * a repair, something to sell, or a part they want to buy. [E] takes the deal, [Q] sends them away; after a while
 * they leave on their own. Each one looks different (random character look). Server decides; movement replicates.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWECustomer : public ACharacter, public IWEInteractable
{
	GENERATED_BODY()

public:
	AWECustomer();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	// IWEInteractable
	virtual FText GetInteractionText(const class AWECharacter* Interactor) const override;
	virtual bool CanInteract(const class AWECharacter* Interactor, FText& OutReason) const override;
	virtual void Interact(class AWECharacter* Interactor) override;
	virtual bool GetAlternativeInteraction(const class AWECharacter* Interactor, FText& OutText, FText& OutReason) const override;
	virtual void InteractAlternative(class AWECharacter* Interactor) override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;

	/** Server: what they want and where they go. */
	void Setup(const FString& InName, EWECustomerWish InWish, const FWEItemInstance& InItem, int64 InPriceCents, const FVector& InCounter, const FVector& InExit, int32 LookSeed);

	EWECustomerWish GetWish() const { return Wish; }
	EWECustomerState GetCustomerState() const { return State; }
	const FWEItemInstance& GetItem() const { return Item; }
	FWEMoney GetPrice() const { return FWEMoney::FromCents(PriceCents); }
	/** "Bringt einen PC zur Reparatur (Lohn 34 €)". */
	FText GetWishText() const;

	/** Server: how long they wait at the door. */
	static constexpr float PatienceSeconds = 150.f;

	/** Server / tests: walk at once (no navigation). */
	void SetInstantTravel(bool bInstant) { bInstantTravel = bInstant; }

protected:
	virtual void BeginPlay() override;
	UFUNCTION() void OnRep_Look();
	void Leave(const FText& Message, class AWECharacter* Interactor);
	/** Server: steps along the path to Goal; true when there. */
	bool MoveTo(const FVector& Goal, float DeltaSeconds);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UWEAvatarComponent> Avatar;

	UPROPERTY(ReplicatedUsing = OnRep_Look) FWECharacterLook Look;
	UPROPERTY(Replicated) FString CustomerName;
	UPROPERTY(Replicated) EWECustomerWish Wish = EWECustomerWish::Repair;
	UPROPERTY(Replicated) EWECustomerState State = EWECustomerState::Arriving;
	UPROPERTY(Replicated) FWEItemInstance Item;
	UPROPERTY(Replicated) int64 PriceCents = 0;

private:
	FVector Counter = FVector::ZeroVector;
	FVector Exit = FVector::ZeroVector;
	TArray<FVector> Path;
	int32 PathIndex = 0;
	FVector PathGoal = FVector(FLT_MAX);
	float WaitTime = 0.f;
	float StuckTimer = 0.f;
	FVector LastCheck = FVector::ZeroVector;
	bool bInstantTravel = false;
};

/** Phase 19: brings walk-in customers during opening hours (server, called by the day/night clock). */
namespace WEWalkIn
{
	/** Spawns a customer if the workshop is open and not too many are around. Returns it (or nullptr). */
	WORKSHOPEMPIRE_API AWECustomer* TrySpawn(UWorld* World, bool bForce = false);
	constexpr int32 MaxCustomers = 2;
}
