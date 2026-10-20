#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Interaction/WEInteractable.h"
#include "WEAtmosphere.generated.h"

class UAudioComponent;
class UStaticMeshComponent;
class USoundBase;

/**
 * Phase 20: the workshop radio. Plays funk music (6 CC0 tracks) as a 3D sound at the radio; [E] switches it on and off,
 * [Q] skips to the next track. The server owns the playlist (track and its start time replicate), so every player hears
 * the same song at the same spot; the next track starts when one ends.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEWorkshopRadio : public AActor, public IWEInteractable
{
	GENERATED_BODY()

public:
	AWEWorkshopRadio();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IWEInteractable
	virtual FText GetInteractionText(const class AWECharacter* Interactor) const override;
	virtual bool CanInteract(const class AWECharacter* Interactor, FText& OutReason) const override;
	virtual void Interact(class AWECharacter* Interactor) override;
	virtual bool GetAlternativeInteraction(const class AWECharacter* Interactor, FText& OutText, FText& OutReason) const override;
	virtual void InteractAlternative(class AWECharacter* Interactor) override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;

	bool IsOn() const { return bOn; }
	int32 GetTrack() const { return Track; }
	/** This machine plays the music right now (tests). */
	bool IsPlayingLocally() const;
	static constexpr int32 NumTracks = 6;

	/** Server: switch on or off. */
	void SetOn(bool bInOn);
	/** Server: play a track from its start. */
	void StartTrack(int32 InTrack);

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	UFUNCTION() void OnRep_Playback();
	void ApplyPlayback();
	static USoundBase* TrackSound(int32 Index);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UAudioComponent> Audio;

	UPROPERTY(ReplicatedUsing = OnRep_Playback) bool bOn = true;
	UPROPERTY(ReplicatedUsing = OnRep_Playback) int32 Track = 0;
	/** Server world time when the current track started. */
	UPROPERTY(ReplicatedUsing = OnRep_Playback) float TrackStart = 0.f;

	FTimerHandle NextTrackTimer;
	int32 PlayingTrack = INDEX_NONE;
	float PlayingStart = -1.f;
};

/**
 * Phase 20: the coffee machine. [E] buys a coffee for 0,50 EUR from the company account: for three minutes the player walks
 * 15 % and works 20 % faster (see AWECharacter::GiveCoffeeBoost). Steam and a gurgle play on every machine.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWECoffeeMachine : public AActor, public IWEInteractable
{
	GENERATED_BODY()

public:
	AWECoffeeMachine();

	// IWEInteractable
	virtual FText GetInteractionText(const class AWECharacter* Interactor) const override;
	virtual bool CanInteract(const class AWECharacter* Interactor, FText& OutReason) const override;
	virtual void Interact(class AWECharacter* Interactor) override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;

	int32 GetCupsServed() const { return CupsServed; }
	static constexpr int64 PriceCents = 50;
	static constexpr float BoostSeconds = 180.f;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	UFUNCTION(NetMulticast, Unreliable) void MulticastBrew();

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	int32 CupsServed = 0;
};

UENUM()
enum class EWECatState : uint8
{
	Walk,
	Sit,
	Sleep,
	Petted
};

/**
 * Phase 20: "Schraube", the workshop cat. Wanders around the workshop on the navigation mesh, sits down now and then,
 * sleeps for a while (purring) and meows. [E] pets it. The server decides where it goes and what it does (state
 * replicates, movement like any character); every machine animates the procedural model (legs, tail, head) itself.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEWorkshopCat : public ACharacter, public IWEInteractable
{
	GENERATED_BODY()

public:
	AWEWorkshopCat();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	// IWEInteractable
	virtual FText GetInteractionText(const class AWECharacter* Interactor) const override;
	virtual bool CanInteract(const class AWECharacter* Interactor, FText& OutReason) const override;
	virtual void Interact(class AWECharacter* Interactor) override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;

	EWECatState GetCatState() const { return State; }
	int32 GetPetCount() const { return PetCount; }
	/** Server / shots: switch the state now (Seconds = how long it lasts). */
	void SetCatState(EWECatState InState, float Seconds);

	/** Wander area around the spawn point (half size, cm). */
	UPROPERTY(EditAnywhere, Category = "Cat") FVector2D WanderHalfExtent = FVector2D(260.f, 220.f);

protected:
	virtual void BeginPlay() override;
	void ServerThink(float DeltaSeconds);
	bool FollowPath(float DeltaSeconds);
	void PickGoal();
	void Animate(float DeltaSeconds);
	UFUNCTION() void OnRep_State();
	UFUNCTION(NetMulticast, Unreliable) void MulticastMeow(uint8 Kind);
	void PlayMeow(uint8 Kind);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Model;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Head;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Tail;
	UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> Legs;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Purr;

	UPROPERTY(ReplicatedUsing = OnRep_State) EWECatState State = EWECatState::Sit;
	UPROPERTY(Replicated) int32 PetCount = 0;

	FVector Home = FVector::ZeroVector;
	float StateTimer = 3.f;
	TArray<FVector> Path;
	int32 PathIndex = 0;
	float StuckTimer = 0.f;
	FVector LastCheck = FVector::ZeroVector;
	float MeowTimer = 30.f;

	// Animation (local).
	float WalkPhase = 0.f;
	float SitBlend = 1.f;
	float SleepBlend = 0.f;
	float AnimTime = 0.f;
};
