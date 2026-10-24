#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Player/WECharacterLook.h"
#include "WECharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UWEInputConfig;
class UWEInteractionComponent;
class AWEPickupItem;
struct FInputActionValue;

/**
 * First-person player character.
 *
 * Movement (walk, sprint, crouch, jump) uses CharacterMovementComponent replication. Carrying is
 * server-authoritative: only the server sets HeldItem; clients request through UWEInteractionComponent.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWECharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AWECharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }
	UWEInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }
	const UWEInputConfig* GetInputConfig() const { return InputConfig; }
	/** Phase 20: key rebinding changes the live mapping context. */
	UWEInputConfig* GetMutableInputConfig() const { return InputConfig; }

	// --- Carrying ---------------------------------------------------------------------------

	AWEPickupItem* GetHeldItem() const { return HeldItem; }
	/** Carry limit in the hands; a hand truck in the inventory adds to it (Phase 8). */
	float GetMaxCarryWeightKg() const;

	class UWEBuildComponent* GetBuildComponent() const { return BuildComponent; }

	/** Server only. Validates and takes Item into the hands. Returns false if rejected (already held, too heavy, hands full). */
	bool TryPickUpItem(AWEPickupItem* Item);

	/** Server only. Puts the held item down in front of the character (or before the first obstacle). */
	void DropHeldItem();

	/** World transform where DropHeldItem would place Item. Uses the server-side view point. */
	FTransform ComputeDropTransform(const AWEPickupItem* Item) const;

	/** Same, for an object of the given radius (e.g. an item dropped from the inventory). */
	FTransform ComputeDropTransform(float Radius, const AActor* IgnoredActor) const;

	/** Workbench under the crosshair within reach (server-side view), or nullptr. */
	class AWEWorkbench* FindAimedWorkbench() const;
	/** Machine under the crosshair (to put a carried device in). */
	class AWEMachine* FindAimedMachine() const;
	class AWEPlaceholderProp* FindAimedProp() const;

	/** Server only. Releases the held item without placing it (used by loading). */
	AWEPickupItem* ReleaseHeldItem();

	// --- Inventory ----------------------------------------------------------------------------

	/** The player's inventory (lives on the PlayerState so it survives respawns). */
	class UWEInventoryComponent* GetPlayerInventory() const;

	/** Server only. Moves a small world item into the player inventory (whole or partial stack). */
	bool TryStoreWorldItem(AWEPickupItem* Item);

	// --- Movement -----------------------------------------------------------------------------

	bool IsSprinting() const { return bIsSprinting; }
	void SetSprinting(bool bNewSprinting);

	// --- Forklift (Phase 11) ------------------------------------------------------------------

	class AWEForklift* GetForklift() const { return Forklift; }
	/** Server: set by AWEForklift::Mount / Dismount. */
	void SetForklift(class AWEForklift* InForklift);
	/** Speed and carry limit follow the forklift (any machine). */
	void OnForkliftChanged();
	/** [X] while driving: get off (asks the server). */
	void RequestExitForklift();

	// --- Phase 18c: character look -------------------------------------------------------------------------
	const FWECharacterLook& GetLook() const { return Look; }
	bool HasLook() const { return Look.Revision > 0; }
	/** Server: the look from the save game (wins over what the client sends from its profile). */
	void ApplySavedLook(const FWECharacterLook& Saved);
	/** Server: set the look (the local player sends its profile look when it takes control). */
	void SetLookAuthority(const FWECharacterLook& NewLook);
	class UWEAvatarComponent* GetAvatar() const { return Avatar; }
	/** Phase 19 mirror: server - a change made in game (wins over the profile, stays in the save). */
	void ChangeLookAuthority(const FWECharacterLook& NewLook);
	/** Phase 19 mirror: owning client - change the look now. */
	void RequestChangeLook(const FWECharacterLook& NewLook);

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_PlayerState() override;
	virtual void PossessedBy(AController* NewController) override;
	void ApplyPlayerLook();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_SprintStarted();
	void Input_SprintCompleted();
	void Input_CrouchStarted();
	void Input_CrouchCompleted();
	void Input_Interact();
	void Input_Examine();
	void Input_Secondary();
	void Input_Alternative();
	void Input_Build();
	void Input_Remove();
	void Input_Setting();
	void Input_Cycle(const struct FInputActionValue& Value);
	void Input_Inventory();
	void Input_QuickSave();
	void Input_QuickLoad();
	void Input_Pause();
	void Input_View();
public:
	/** Phase 18c: [V] third person (see your own character) / first person. */
	void SetThirdPerson(bool bOn);
	bool IsThirdPerson() const { return bThirdPerson; }

	/** Phase 20 coffee machine. Server: walk 15 % faster and work 20 % faster for Seconds. */
	void GiveCoffeeBoost(float Seconds);
	/** Seconds of coffee boost left (every machine). */
	float GetCoffeeBoostRemaining() const;
	bool HasCoffeeBoost() const { return GetCoffeeBoostRemaining() > 0.f; }
	/** Work speed factor from the coffee (1 or 1.2). */
	float GetWorkBoost() const { return HasCoffeeBoost() ? CoffeeWorkFactor : 1.f; }
	static constexpr float CoffeeWalkFactor = 1.15f;
	static constexpr float CoffeeWorkFactor = 1.2f;
protected:
	/** Server world time when the coffee boost ends. */
	UPROPERTY(ReplicatedUsing = OnRep_CoffeeBoost)
	float CoffeeBoostEnd = 0.f;
	UFUNCTION()
	void OnRep_CoffeeBoost();
	FTimerHandle CoffeeTimer;
	bool bThirdPerson = false;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<class USpringArmComponent> ThirdPersonBoom;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> ThirdPersonCamera;

	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bNewSprinting);

	UFUNCTION(Server, Reliable)
	void ServerExitForklift();

	UFUNCTION(Server, Reliable)
	void ServerSetLook(const FWECharacterLook& NewLook);

	UFUNCTION(Server, Reliable)
	void ServerChangeLook(const FWECharacterLook& NewLook);

	UFUNCTION()
	void OnRep_Look();

	/** Owning client: sends the look of the local profile once. */
	void SendProfileLook();
	bool bProfileLookSent = false;
	bool bLookFromSave = false;

	UFUNCTION()
	void OnRep_Forklift();

	UFUNCTION()
	void OnRep_IsSprinting();

	UFUNCTION()
	void OnRep_HeldItem();

	void UpdateMovementSpeed();
	void AddInputMappingContext();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	/** Where carried items are attached (child of the camera). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> HoldPoint;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UWEInteractionComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<class UWEBuildComponent> BuildComponent;

	/** Optional authored input setup. If empty, UWEInputConfig::CreateDefault is used. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UWEInputConfig> InputConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SprintSpeed = 550.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float CrouchSpeed = 150.f;

	/** How fast the camera follows eye-height changes (crouch). */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraHeightInterpSpeed = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "Carry")
	float MaxCarryWeightKg = 30.f;

	/** Distance in front of the eyes where items are dropped when no surface is aimed at. */
	UPROPERTY(EditDefaultsOnly, Category = "Carry")
	float DropDistance = 110.f;

	/** Max distance of an aimed surface (workbench, shelf, floor) the held item is placed on. */
	UPROPERTY(EditDefaultsOnly, Category = "Carry")
	float PlaceReach = 250.f;

	UPROPERTY(ReplicatedUsing = OnRep_IsSprinting)
	bool bIsSprinting = false;

	UPROPERTY(ReplicatedUsing = OnRep_HeldItem)
	TObjectPtr<AWEPickupItem> HeldItem;

	/** Quality update: green/red outline where the carried device would lie on the aimed workbench (local only). */
	void UpdatePlacementPreview();
	UPROPERTY(Transient)
	TObjectPtr<class AWEBuildGhost> PlacementGhost;

	UPROPERTY(ReplicatedUsing = OnRep_Forklift)
	TObjectPtr<class AWEForklift> Forklift;

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	FWECharacterLook Look;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<class UWEAvatarComponent> Avatar;

private:
	float CurrentCameraHeight = 0.f;
};
