#include "World/WEAtmosphere.h"

#include "AIController.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Economy/WECompanyComponent.h"
#include "Engine/StaticMesh.h"
#include "Framework/WEPlayerController.h"
#include "Fx/WEFx.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "Player/WECharacter.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "WELog.h"

#define LOCTEXT_NAMESPACE "WEAtmosphere"

namespace WEAtmo
{
	template <typename T>
	T* Load(const FString& Path)
	{
		return LoadObject<T>(nullptr, *Path, nullptr, LOAD_Quiet | LOAD_NoWarn);
	}

	UStaticMesh* Mesh(const TCHAR* Name)
	{
		return Load<UStaticMesh>(FString::Printf(TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_%s.SM_Gen_%s"), Name, Name));
	}

	USoundBase* Sound(const TCHAR* Name)
	{
		return Load<USoundBase>(FString::Printf(TEXT("/Game/WorkshopEmpire/Audio/P20/%s.%s"), Name, Name));
	}

	float ServerTime(const UObject* Context)
	{
		const UWorld* World = Context ? Context->GetWorld() : nullptr;
		const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
		return GameState ? static_cast<float>(GameState->GetServerWorldTimeSeconds()) : (World ? World->GetTimeSeconds() : 0.f);
	}

	FString PlayerName(const AWECharacter* Character)
	{
		const APlayerState* PS = Character ? Character->GetPlayerState() : nullptr;
		return PS ? PS->GetPlayerName() : FString();
	}

	void Message(AWECharacter* Character, const FText& Text)
	{
		if (AWEPlayerController* PC = Character ? Cast<AWEPlayerController>(Character->GetController()) : nullptr)
		{
			PC->ClientShowMessage(Text);
		}
	}

	UStaticMeshComponent* MakePart(AActor* Owner, const FName& Name, USceneComponent* Parent)
	{
		UStaticMeshComponent* C = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Parent);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
		return C;
	}
}

// --- Radio -----------------------------------------------------------------------------------------------

AWEWorkshopRadio::AWEWorkshopRadio()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(2.f);
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(Mesh);
	Audio->SetRelativeLocation(FVector(0.f, 0.f, 10.f));
	Audio->bAutoActivate = false;
	Audio->bOverrideAttenuation = true;
	Audio->AttenuationOverrides.bAttenuate = true;
	Audio->AttenuationOverrides.bSpatialize = true;
	Audio->AttenuationOverrides.AttenuationShape = EAttenuationShape::Sphere;
	Audio->AttenuationOverrides.AttenuationShapeExtents = FVector(250.f, 0.f, 0.f);
	Audio->AttenuationOverrides.FalloffDistance = 1500.f;
	Audio->AttenuationOverrides.bAttenuateWithLPF = true;
	Audio->AttenuationOverrides.LPFRadiusMin = 400.f;
	Audio->AttenuationOverrides.LPFRadiusMax = 1700.f;
	Audio->SetVolumeMultiplier(0.55f);
}

void AWEWorkshopRadio::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWEWorkshopRadio, bOn);
	DOREPLIFETIME(AWEWorkshopRadio, Track);
	DOREPLIFETIME(AWEWorkshopRadio, TrackStart);
}

USoundBase* AWEWorkshopRadio::TrackSound(int32 Index)
{
	return WEAtmo::Sound(*FString::Printf(TEXT("SFX_Radio%d"), FMath::Clamp(Index, 0, NumTracks - 1) + 1));
}

void AWEWorkshopRadio::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!Mesh->GetStaticMesh())
	{
		Mesh->SetStaticMesh(WEAtmo::Mesh(TEXT("PortableRadio")));
	}
}

void AWEWorkshopRadio::BeginPlay()
{
	Super::BeginPlay();
	if (!Mesh->GetStaticMesh())
	{
		Mesh->SetStaticMesh(WEAtmo::Mesh(TEXT("PortableRadio")));
	}
	if (HasAuthority())
	{
		// Scripted runs stay quiet unless they ask for the radio; a normal game starts with music.
		bOn = !FParse::Param(FCommandLine::Get(), TEXT("WENoRadio"));
		StartTrack(FMath::RandRange(0, NumTracks - 1));
	}
	ApplyPlayback();
}

void AWEWorkshopRadio::StartTrack(int32 InTrack)
{
	if (!HasAuthority())
	{
		return;
	}
	Track = ((InTrack % NumTracks) + NumTracks) % NumTracks;
	TrackStart = WEAtmo::ServerTime(this);
	GetWorldTimerManager().ClearTimer(NextTrackTimer);
	if (bOn)
	{
		const USoundBase* S = TrackSound(Track);
		const float Duration = S && S->GetDuration() > 5.f && S->GetDuration() < 3600.f ? S->GetDuration() : 150.f;
		FTimerDelegate Next = FTimerDelegate::CreateWeakLambda(this, [this]() { StartTrack(Track + 1); });
		GetWorldTimerManager().SetTimer(NextTrackTimer, Next, Duration + 0.8f, false);
	}
	ForceNetUpdate();
	ApplyPlayback();
}

void AWEWorkshopRadio::SetOn(bool bInOn)
{
	if (!HasAuthority() || bOn == bInOn)
	{
		return;
	}
	bOn = bInOn;
	UE_LOG(LogWorkshopEmpire, Log, TEXT("WE_RADIO %s track %d"), bOn ? TEXT("on") : TEXT("off"), Track + 1);
	if (bOn)
	{
		StartTrack(Track);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(NextTrackTimer);
		ForceNetUpdate();
		ApplyPlayback();
	}
}

void AWEWorkshopRadio::OnRep_Playback()
{
	ApplyPlayback();
}

void AWEWorkshopRadio::ApplyPlayback()
{
	if (GetNetMode() == NM_DedicatedServer || !Audio)
	{
		return;
	}
	if (!bOn)
	{
		Audio->Stop();
		PlayingTrack = INDEX_NONE;
		return;
	}
	if (PlayingTrack == Track && FMath::IsNearlyEqual(PlayingStart, TrackStart) && Audio->IsPlaying())
	{
		return;
	}
	USoundBase* S = TrackSound(Track);
	if (!S)
	{
		return;
	}
	PlayingTrack = Track;
	PlayingStart = TrackStart;
	Audio->SetSound(S);
	// Joining in the middle of a song: start where the others are.
	const float Offset = FMath::Clamp(WEAtmo::ServerTime(this) - TrackStart, 0.f, FMath::Max(0.f, S->GetDuration() - 1.f));
	Audio->Play(Offset);
}

bool AWEWorkshopRadio::IsPlayingLocally() const
{
	return Audio && Audio->IsPlaying();
}

FText AWEWorkshopRadio::GetInteractionText(const AWECharacter* Interactor) const
{
	return bOn ? LOCTEXT("RadioOff", "Radio ausschalten") : LOCTEXT("RadioOn", "Radio einschalten");
}

bool AWEWorkshopRadio::CanInteract(const AWECharacter* Interactor, FText& OutReason) const
{
	return Interactor != nullptr;
}

void AWEWorkshopRadio::Interact(AWECharacter* Interactor)
{
	SetOn(!bOn);
}

bool AWEWorkshopRadio::GetAlternativeInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const
{
	if (!bOn)
	{
		return false;
	}
	OutText = LOCTEXT("RadioNext", "Nächster Titel");
	return true;
}

void AWEWorkshopRadio::InteractAlternative(AWECharacter* Interactor)
{
	if (bOn)
	{
		StartTrack(Track + 1);
		UE_LOG(LogWorkshopEmpire, Log, TEXT("WE_RADIO next track %d"), Track + 1);
	}
}

bool AWEWorkshopRadio::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	OutInfo.Title = LOCTEXT("RadioTitle", "Werkstattradio");
	OutInfo.Description = LOCTEXT("RadioDesc", "Ein alter Kassettenrekorder mit Radio. Läuft den ganzen Tag Funk. [E] an und aus, [Q] nächster Titel.");
	OutInfo.Details.Add(bOn ? FText::Format(LOCTEXT("RadioTrack", "Titel: {0} von {1}"), FText::AsNumber(Track + 1), FText::AsNumber(NumTracks))
		: LOCTEXT("RadioIsOff", "Ausgeschaltet"));
	OutInfo.Details.Add(LOCTEXT("RadioMusic", "Musik: Holizna, „Funk“ (CC0)"));
	return true;
}

// --- Coffee machine --------------------------------------------------------------------------------------

AWECoffeeMachine::AWECoffeeMachine()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(1.f);
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void AWECoffeeMachine::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!Mesh->GetStaticMesh())
	{
		Mesh->SetStaticMesh(WEAtmo::Mesh(TEXT("CoffeeMachine")));
	}
}

void AWECoffeeMachine::BeginPlay()
{
	Super::BeginPlay();
	if (!Mesh->GetStaticMesh())
	{
		Mesh->SetStaticMesh(WEAtmo::Mesh(TEXT("CoffeeMachine")));
	}
}

FText AWECoffeeMachine::GetInteractionText(const AWECharacter* Interactor) const
{
	return FText::Format(LOCTEXT("CoffeeDrink", "Kaffee trinken ({0})"), FText::FromString(FWEMoney::FromCents(PriceCents).ToDisplayString()));
}

bool AWECoffeeMachine::CanInteract(const AWECharacter* Interactor, FText& OutReason) const
{
	if (!Interactor)
	{
		return false;
	}
	if (const float Left = Interactor->GetCoffeeBoostRemaining(); Left > 20.f)
	{
		const int32 Seconds = FMath::CeilToInt(Left);
		OutReason = FText::Format(LOCTEXT("CoffeeStillAwake", "Noch hellwach ({0}:{1})"), FText::AsNumber(Seconds / 60),
			FText::FromString(FString::Printf(TEXT("%02d"), Seconds % 60)));
		return false;
	}
	const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!Company || !Company->CanAfford(FWEMoney::FromCents(PriceCents)))
	{
		OutReason = LOCTEXT("CoffeeNoMoney", "Kein Geld für Kaffee");
		return false;
	}
	return true;
}

void AWECoffeeMachine::Interact(AWECharacter* Interactor)
{
	FText Reason;
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!HasAuthority() || !CanInteract(Interactor, Reason) || !Company
		|| !Company->TryWithdraw(FWEMoney::FromCents(PriceCents), TEXT("Kaffee"), WEAtmo::PlayerName(Interactor)))
	{
		return;
	}
	Interactor->GiveCoffeeBoost(BoostSeconds);
	++CupsServed;
	MulticastBrew();
	WEAtmo::Message(Interactor, LOCTEXT("CoffeeDone", "Kaffee! 3 Minuten lang läufst du 15 % und arbeitest 20 % schneller."));
	UE_LOG(LogWorkshopEmpire, Log, TEXT("WE_COFFEE %s boost %.0fs"), *WEAtmo::PlayerName(Interactor), BoostSeconds);
}

void AWECoffeeMachine::MulticastBrew_Implementation()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	const FBox Bounds = Mesh->Bounds.GetBox();
	AWEFx::Burst(this, EWEFxKind::Steam, FVector(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Max.Z - 6.f), FVector::UpVector, FLinearColor::White, 0.5f);
	if (USoundBase* Gurgle = WEAtmo::Load<USoundBase>(TEXT("/Game/WorkshopEmpire/Audio/SFX_PourSizzle.SFX_PourSizzle")))
	{
		UGameplayStatics::PlaySoundAtLocation(this, Gurgle, GetActorLocation(), 0.3f, 1.5f, 0.f,
			WEAtmo::Load<USoundAttenuation>(TEXT("/Game/WorkshopEmpire/Audio/SA_WESmall.SA_WESmall")));
	}
}

bool AWECoffeeMachine::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	OutInfo.Title = LOCTEXT("CoffeeTitle", "Kaffeemaschine");
	OutInfo.Description = LOCTEXT("CoffeeDesc", "Filterkaffee, stark. Eine Tasse macht 3 Minuten lang wach: 15 % schneller laufen, 20 % schneller arbeiten.");
	OutInfo.Details.Add(FText::Format(LOCTEXT("CoffeePrice", "Preis: {0} pro Tasse (Firmenkasse)"), FText::FromString(FWEMoney::FromCents(PriceCents).ToDisplayString())));
	return true;
}

// --- Cat -------------------------------------------------------------------------------------------------

namespace WECatConst
{
	constexpr float Radius = 18.f;
	constexpr float HalfHeight = 19.f;
	constexpr float WalkSpeed = 65.f;
	constexpr float BodyHeight = 21.f;
	const FVector Neck(16.f, 0.f, 4.f);
	const FVector TailBase(-18.f, 0.f, 2.f);
	const FVector Hips[4] = { FVector(12.f, 5.f, -5.f), FVector(12.f, -5.f, -5.f), FVector(-12.f, 5.f, -5.f), FVector(-12.f, -5.f, -5.f) };
	// Diagonal gait: front left with rear right, front right with rear left.
	constexpr float GaitOffset[4] = { 0.f, PI, PI, 0.f };
}

AWEWorkshopCat::AWEWorkshopCat()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;
	GetCapsuleComponent()->InitCapsuleSize(WECatConst::Radius, WECatConst::HalfHeight);
	// [E] aims at the cat (interaction traces on Visibility).
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	// Players and employees walk past (a cat in the way should never block anyone).
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 260.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = WECatConst::WalkSpeed;
	GetCharacterMovement()->MaxStepHeight = 15.f;
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetVisibility(false);

	Model = CreateDefaultSubobject<USceneComponent>(TEXT("Model"));
	Model->SetupAttachment(GetCapsuleComponent());
	Model->SetRelativeLocation(FVector(0.f, 0.f, -WECatConst::HalfHeight));
	Body = WEAtmo::MakePart(this, TEXT("Body"), Model);
	Body->SetRelativeLocation(FVector(0.f, 0.f, WECatConst::BodyHeight));
	Head = WEAtmo::MakePart(this, TEXT("Head"), Body);
	Head->SetRelativeLocation(WECatConst::Neck);
	Tail = WEAtmo::MakePart(this, TEXT("Tail"), Body);
	Tail->SetRelativeLocation(WECatConst::TailBase);
	static const TCHAR* LegNames[4] = { TEXT("LegFL"), TEXT("LegFR"), TEXT("LegRL"), TEXT("LegRR") };
	for (int32 i = 0; i < 4; ++i)
	{
		UStaticMeshComponent* Leg = WEAtmo::MakePart(this, LegNames[i], Body);
		Leg->SetRelativeLocation(WECatConst::Hips[i]);
		Legs.Add(Leg);
	}
}

void AWEWorkshopCat::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWEWorkshopCat, State);
	DOREPLIFETIME(AWEWorkshopCat, PetCount);
}

void AWEWorkshopCat::BeginPlay()
{
	Super::BeginPlay();
	Home = GetActorLocation();
	if (GetNetMode() != NM_DedicatedServer)
	{
		Body->SetStaticMesh(WEAtmo::Mesh(TEXT("CatBody")));
		Head->SetStaticMesh(WEAtmo::Mesh(TEXT("CatHead")));
		Tail->SetStaticMesh(WEAtmo::Mesh(TEXT("CatTail")));
		for (UStaticMeshComponent* Leg : Legs)
		{
			Leg->SetStaticMesh(WEAtmo::Mesh(TEXT("CatLeg")));
		}
	}
	if (HasAuthority())
	{
		SetCatState(EWECatState::Sit, FMath::FRandRange(3.f, 8.f));
		MeowTimer = FMath::FRandRange(20.f, 50.f);
	}
	OnRep_State();
}

void AWEWorkshopCat::SetCatState(EWECatState InState, float Seconds)
{
	if (!HasAuthority())
	{
		return;
	}
	State = InState;
	StateTimer = Seconds;
	Path.Reset();
	PathIndex = 0;
	if (State == EWECatState::Walk)
	{
		PickGoal();
		StateTimer = Seconds;
	}
	ForceNetUpdate();
	OnRep_State();
}

void AWEWorkshopCat::OnRep_State()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// Purring while asleep or petted.
	const bool bPurr = State == EWECatState::Sleep || State == EWECatState::Petted;
	if (bPurr && !Purr)
	{
		if (USoundBase* Loop = WEAtmo::Sound(TEXT("SFX_cat_purrsleepy_loop")))
		{
			Purr = UGameplayStatics::SpawnSoundAttached(Loop, Body, NAME_None, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, true,
				State == EWECatState::Petted ? 0.8f : 0.45f, 1.f, 0.f, WEAtmo::Load<USoundAttenuation>(TEXT("/Game/WorkshopEmpire/Audio/SA_WESmall.SA_WESmall")), nullptr, false);
		}
	}
	else if (!bPurr && Purr)
	{
		Purr->FadeOut(1.f, 0.f);
		Purr = nullptr;
	}
	else if (Purr)
	{
		Purr->SetVolumeMultiplier(State == EWECatState::Petted ? 0.8f : 0.45f);
	}
}

void AWEWorkshopCat::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		ServerThink(DeltaSeconds);
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		Animate(DeltaSeconds);
	}
}

void AWEWorkshopCat::ServerThink(float DeltaSeconds)
{
	StateTimer -= DeltaSeconds;
	switch (State)
	{
	case EWECatState::Walk:
		if (FollowPath(DeltaSeconds) || StateTimer <= 0.f)
		{
			// Arrived: sit down, curl up for a nap or stroll on.
			const float Roll = FMath::FRand();
			if (Roll < 0.45f)
			{
				SetCatState(EWECatState::Sit, FMath::FRandRange(6.f, 14.f));
			}
			else if (Roll < 0.75f)
			{
				SetCatState(EWECatState::Sleep, FMath::FRandRange(25.f, 60.f));
			}
			else
			{
				SetCatState(EWECatState::Walk, 25.f);
			}
		}
		break;
	case EWECatState::Petted:
		if (StateTimer <= 0.f)
		{
			SetCatState(EWECatState::Sit, 5.f);
		}
		break;
	default:
		if (StateTimer <= 0.f)
		{
			SetCatState(EWECatState::Walk, 25.f);
		}
		break;
	}
	// Now and then a meow (not in its sleep).
	MeowTimer -= DeltaSeconds;
	if (MeowTimer <= 0.f)
	{
		MeowTimer = FMath::FRandRange(35.f, 80.f);
		if (State != EWECatState::Sleep)
		{
			MulticastMeow(static_cast<uint8>(FMath::RandRange(0, 1)));
		}
	}
}

void AWEWorkshopCat::PickGoal()
{
	Path.Reset();
	PathIndex = 0;
	StateTimer = 25.f;
	StuckTimer = 0.f;
	LastCheck = GetActorLocation();
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	for (int32 Try = 0; Try < 8; ++Try)
	{
		const FVector Candidate = Home + FVector(FMath::FRandRange(-WanderHalfExtent.X, WanderHalfExtent.X), FMath::FRandRange(-WanderHalfExtent.Y, WanderHalfExtent.Y), 0.f);
		if (FVector::Dist2D(Candidate, GetActorLocation()) < 80.f)
		{
			continue;
		}
		if (!Nav)
		{
			Path.Add(Candidate);
			return;
		}
		FNavLocation OnNav;
		if (!Nav->ProjectPointToNavigation(Candidate, OnNav, FVector(60.f, 60.f, 200.f)))
		{
			continue;
		}
		if (const UNavigationPath* NavPath = Nav->FindPathToLocationSynchronously(GetWorld(), GetActorLocation(), OnNav.Location, this);
			NavPath && NavPath->IsValid() && !NavPath->IsPartial() && NavPath->GetPathLength() < 900.f)
		{
			Path.Append(NavPath->PathPoints);
			return;
		}
	}
}

bool AWEWorkshopCat::FollowPath(float DeltaSeconds)
{
	while (Path.IsValidIndex(PathIndex) && FVector::Dist2D(Path[PathIndex], GetActorLocation()) < 25.f)
	{
		++PathIndex;
	}
	if (!Path.IsValidIndex(PathIndex))
	{
		return true;
	}
	FVector To = Path[PathIndex] - GetActorLocation();
	To.Z = 0.f;
	AddMovementInput(To.GetSafeNormal());
	StuckTimer += DeltaSeconds;
	if (StuckTimer > 3.f)
	{
		// Something in the way: give up on this goal.
		if (FVector::Dist2D(LastCheck, GetActorLocation()) < 20.f)
		{
			Path.Reset();
			return true;
		}
		StuckTimer = 0.f;
		LastCheck = GetActorLocation();
	}
	return false;
}

void AWEWorkshopCat::Animate(float DeltaSeconds)
{
	using namespace WECatConst;
	AnimTime += DeltaSeconds;
	const float Walk = FMath::Clamp(GetVelocity().Size2D() / WalkSpeed, 0.f, 1.f);
	WalkPhase = FMath::Fmod(WalkPhase + DeltaSeconds * Walk * 2.f * PI * 1.7f, 2.f * PI);
	const bool bSits = State == EWECatState::Sit || State == EWECatState::Petted;
	SitBlend = FMath::FInterpConstantTo(SitBlend, bSits && Walk < 0.2f ? 1.f : 0.f, DeltaSeconds, 2.2f);
	SleepBlend = FMath::FInterpConstantTo(SleepBlend, State == EWECatState::Sleep && Walk < 0.2f ? 1.f : 0.f, DeltaSeconds, 1.2f);
	const float Sit = FMath::SmoothStep(0.f, 1.f, SitBlend);
	const float Sleep = FMath::SmoothStep(0.f, 1.f, SleepBlend);
	const float Stand = FMath::Clamp(1.f - Sit - Sleep, 0.f, 1.f);

	// Body: sitting pitches it up onto the haunches, sleeping lays it flat on the floor; a small bob while walking.
	const float Bob = FMath::Sin(WalkPhase * 2.f) * 0.6f * Walk;
	Body->SetRelativeLocationAndRotation(FVector(-3.f * Sit, 0.f, BodyHeight - 9.f * Sit - 12.f * Sleep + Bob), FRotator(30.f * Sit, 0.f, 0.f));
	for (int32 i = 0; i < 4; ++i)
	{
		const bool bFront = i < 2;
		const float Swing = FMath::Sin(WalkPhase + GaitOffset[i]) * 28.f * Walk * Stand;
		// Sitting: front legs straight down (against the body's pitch), hind legs folded forward. Sleeping: all tucked in.
		const float Pitch = Swing + (bFront ? -30.f * Sit : 60.f * Sit) + 90.f * Sleep;
		Legs[i]->SetRelativeRotation(FRotator(Pitch, 0.f, 0.f));
		Legs[i]->SetRelativeScale3D(FVector(1.f, 1.f, bFront ? 1.f - 0.15f * Sit : 1.f));
	}
	// Tail up while walking, swaying; on the floor when sitting; curled round the body asleep.
	const float Sway = FMath::Sin(AnimTime * (Walk > 0.1f ? 3.f : 1.3f)) * (10.f + 10.f * Walk);
	Tail->SetRelativeRotation(FRotator(Stand * FMath::Lerp(-25.f, -60.f, Walk) - 30.f * Sit + 22.f * Sleep, Sway * (1.f - Sleep) + 75.f * Sleep, 0.f));
	// Head: looks around when standing still, level when sitting, down on the paws asleep.
	const float Look = FMath::Sin(AnimTime * 0.37f) * 28.f * (1.f - Walk) * (1.f - Sleep);
	Head->SetRelativeLocationAndRotation(Neck + FVector(0.f, 0.f, -3.f * Sleep), FRotator(-22.f * Sit - 12.f * Sleep, Look + 30.f * Sleep, 0.f));
}

void AWEWorkshopCat::MulticastMeow_Implementation(uint8 Kind)
{
	PlayMeow(Kind);
}

void AWEWorkshopCat::PlayMeow(uint8 Kind)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	static const TCHAR* Names[3] = { TEXT("SFX_cat_softmew"), TEXT("SFX_cat_mewfood"), TEXT("SFX_cat_mewpurr") };
	if (USoundBase* S = WEAtmo::Sound(Names[FMath::Min<int32>(Kind, 2)]))
	{
		UGameplayStatics::PlaySoundAtLocation(this, S, Head->GetComponentLocation(), 0.7f, FMath::FRandRange(0.95f, 1.08f), 0.f,
			WEAtmo::Load<USoundAttenuation>(TEXT("/Game/WorkshopEmpire/Audio/SA_WESmall.SA_WESmall")));
	}
}

FText AWEWorkshopCat::GetInteractionText(const AWECharacter* Interactor) const
{
	return LOCTEXT("CatPet", "Schraube streicheln");
}

bool AWEWorkshopCat::CanInteract(const AWECharacter* Interactor, FText& OutReason) const
{
	if (State == EWECatState::Petted)
	{
		OutReason = LOCTEXT("CatPurrs", "Schraube schnurrt schon");
		return false;
	}
	return Interactor != nullptr;
}

void AWEWorkshopCat::Interact(AWECharacter* Interactor)
{
	if (!HasAuthority() || !Interactor || State == EWECatState::Petted)
	{
		return;
	}
	// Turns towards the hand and enjoys it.
	FVector ToPlayer = Interactor->GetActorLocation() - GetActorLocation();
	ToPlayer.Z = 0.f;
	if (!ToPlayer.IsNearlyZero())
	{
		SetActorRotation(ToPlayer.Rotation());
	}
	GetCharacterMovement()->StopMovementImmediately();
	++PetCount;
	SetCatState(EWECatState::Petted, 5.f);
	MulticastMeow(2);
	UE_LOG(LogWorkshopEmpire, Log, TEXT("WE_CAT petted by %s (%d)"), *WEAtmo::PlayerName(Interactor), PetCount);
}

bool AWEWorkshopCat::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	OutInfo.Title = LOCTEXT("CatTitle", "Schraube");
	OutInfo.Description = LOCTEXT("CatDesc", "Die Werkstattkatze. Kam eines Tages durchs offene Tor und blieb. Schläft gern zwischen den Werkbänken.");
	const FText Mood = State == EWECatState::Sleep ? LOCTEXT("CatSleeping", "schläft")
		: State == EWECatState::Walk ? LOCTEXT("CatStrolling", "streift umher")
		: State == EWECatState::Petted ? LOCTEXT("CatPurring", "schnurrt")
		: LOCTEXT("CatSitting", "sitzt und schaut zu");
	OutInfo.Details.Add(FText::Format(LOCTEXT("CatMood", "Gerade: {0}"), Mood));
	OutInfo.Details.Add(FText::Format(LOCTEXT("CatPets", "Gestreichelt: {0}-mal"), FText::AsNumber(PetCount)));
	return true;
}

#undef LOCTEXT_NAMESPACE
