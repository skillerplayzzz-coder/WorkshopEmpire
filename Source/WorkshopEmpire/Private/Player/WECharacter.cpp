#include "Player/WECharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Framework/WEPlayerController.h"
#include "Framework/WEPlayerState.h"
#include "Interaction/WEInteractionComponent.h"
#include "InputActionValue.h"
#include "Inventory/WEInventoryComponent.h"
#include "Workshop/WEWorkbench.h"
#include "Workshop/WEBuildComponent.h"
#include "Machines/WEMachine.h"
#include "Machines/WEConveyor.h"
#include "Logistics/WEForklift.h"
#include "Logistics/WEVehicle.h"
#include "Progression/WEProgressionComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Net/UnrealNetwork.h"
#include "Player/WEInputConfig.h"
#include "Player/WEMannequin.h"
#include "Player/WEAvatarComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/WEFootstepComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "WELog.h"

static TAutoConsoleVariable<float> CVarWEMouseSensitivity(
	TEXT("we.MouseSensitivity"), 1.0f,
	TEXT("Mouse look sensitivity multiplier (default 1.0)."),
	ECVF_Default);

static TAutoConsoleVariable<bool> CVarWEInvertY(
	TEXT("we.InvertY"), false,
	TEXT("Invert vertical mouse look."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarWEFieldOfView(
	TEXT("we.FieldOfView"), 90.f,
	TEXT("First-person field of view (settings page, Phase 17.5)."),
	ECVF_Default);

AWECharacter::AWECharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
	BaseEyeHeight = 70.f;
	CrouchedEyeHeight = 35.f;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->SetFieldOfView(90.f);

	HoldPoint = CreateDefaultSubobject<USceneComponent>(TEXT("HoldPoint"));
	HoldPoint->SetupAttachment(FirstPersonCamera);
	HoldPoint->SetRelativeLocation(FVector(75.f, 22.f, -28.f));

	// Animated body for the other players (Phase 15); the own player sees through the camera only.
	WEMannequin::Configure(GetMesh(), 88.f);
	CreateDefaultSubobject<UWEFootstepComponent>(TEXT("Footsteps"));
	GetMesh()->SetOwnerNoSee(true);
	// Phase 18c: the visible body from the character creator; the mannequin above only drives its animation.
	Avatar = CreateDefaultSubobject<UWEAvatarComponent>(TEXT("Avatar"));
	Avatar->SetupAttachment(GetCapsuleComponent());
	ThirdPersonBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonBoom"));
	ThirdPersonBoom->SetupAttachment(GetCapsuleComponent());
	ThirdPersonBoom->SetRelativeLocation(FVector(0.f, 0.f, 55.f));
	ThirdPersonBoom->TargetArmLength = 260.f;
	ThirdPersonBoom->SocketOffset = FVector(0.f, 55.f, 15.f);
	ThirdPersonBoom->bUsePawnControlRotation = true;
	ThirdPersonBoom->ProbeSize = 14.f;
	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(ThirdPersonBoom);
	ThirdPersonCamera->SetAutoActivate(false);

	InteractionComponent = CreateDefaultSubobject<UWEInteractionComponent>(TEXT("InteractionComponent"));
	BuildComponent = CreateDefaultSubobject<UWEBuildComponent>(TEXT("BuildComponent"));

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = WalkSpeed;
	Move->MaxWalkSpeedCrouched = CrouchSpeed;
	Move->JumpZVelocity = 420.f;
	Move->AirControl = 0.25f;
	Move->BrakingDecelerationWalking = 2000.f;
	Move->GetNavAgentPropertiesRef().bCanCrouch = true;
	Move->SetCrouchedHalfHeight(55.f);
	Move->bCanWalkOffLedgesWhenCrouching = true;
}

void AWECharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AWECharacter, bIsSprinting, COND_SkipOwner);
	DOREPLIFETIME(AWECharacter, HeldItem);
	DOREPLIFETIME(AWECharacter, Forklift);
	DOREPLIFETIME(AWECharacter, Look);
}

void AWECharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	ApplyPlayerLook();
}

void AWECharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	ApplyPlayerLook();
}

void AWECharacter::ApplyPlayerLook()
{
	// Player number decides body and clothes colour, the same on every machine.
	const int32 Index = GetPlayerState() ? GetPlayerState()->GetPlayerId() : 0;
	WEMannequin::ApplyBody(GetMesh(), HasLook() ? Look.Body == 1 : Index % 2 == 1);
	WEMannequin::Tint(GetMesh(), WEMannequin::PlayerColor(Index));
	// Phase 18c: with a chosen look the Quaternius body shows (driven by the mannequin), not on a dedicated server.
	if (HasLook() && Avatar && GetNetMode() != NM_DedicatedServer)
	{
		Avatar->SetDriver(GetMesh());
		Avatar->SetOwnerNoSee(!bThirdPerson);
		Avatar->ApplyLook(Look);
	}
	SendProfileLook();
}

void AWECharacter::SendProfileLook()
{
	if (bProfileLookSent || !IsLocallyControlled() || !Controller || !Controller->IsPlayerController())
	{
		return;
	}
	bProfileLookSent = true;
	bool bFound = false;
	FWECharacterLook Profile = WELook::LoadProfileLook(&bFound);
	if (!bFound)
	{
		// No character created on this computer yet: a random one (the creator can change it any time).
		Profile = WELook::Random(GetTypeHash(GetPlayerState() ? GetPlayerState()->GetPlayerName() : GetName()));
	}
	if (HasAuthority())
	{
		SetLookAuthority(Profile);
	}
	else
	{
		ServerSetLook(Profile);
	}
}

void AWECharacter::ServerSetLook_Implementation(const FWECharacterLook& NewLook)
{
	SetLookAuthority(NewLook);
}

void AWECharacter::SetLookAuthority(const FWECharacterLook& NewLook)
{
	if (!HasAuthority() || bLookFromSave)
	{
		return; // a loaded game keeps the look stored in it
	}
	const uint8 Revision = Look.Revision;
	Look = NewLook;
	Look.Clamp();
	Look.Revision = static_cast<uint8>(FMath::Max(1, (Revision + 1) % 256));
	OnRep_Look();
	ForceNetUpdate();
}

void AWECharacter::ApplySavedLook(const FWECharacterLook& Saved)
{
	if (!HasAuthority())
	{
		return;
	}
	bLookFromSave = false;
	SetLookAuthority(Saved);
	bLookFromSave = true;
}

void AWECharacter::OnRep_Look()
{
	ApplyPlayerLook();
}

void AWECharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyPlayerLook();
	CurrentCameraHeight = BaseEyeHeight;
	UpdateMovementSpeed();
	FirstPersonCamera->SetFieldOfView(FMath::Clamp(CVarWEFieldOfView.GetValueOnGameThread(), 70.f, 110.f));
}

void AWECharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// A player leaving (or dying later on) must never take an item out of the world.
	if (HasAuthority() && Forklift)
	{
		Forklift->Dismount();
	}
	if (HasAuthority() && HeldItem)
	{
		DropHeldItem();
	}
	Super::EndPlay(EndPlayReason);
}

void AWECharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled())
	{
		CurrentCameraHeight = FMath::FInterpTo(CurrentCameraHeight, BaseEyeHeight, DeltaSeconds, CameraHeightInterpSpeed);
		FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, CurrentCameraHeight));
	}
}

void AWECharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	// The capsule centre just moved down; keep the camera where it was and let Tick ease it to the new eye height.
	CurrentCameraHeight += ScaledHalfHeightAdjust;
	UpdateMovementSpeed();
}

void AWECharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	CurrentCameraHeight -= ScaledHalfHeightAdjust;
	UpdateMovementSpeed();
}

// --- Input -------------------------------------------------------------------------------------

void AWECharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!InputConfig)
	{
		InputConfig = UWEInputConfig::CreateDefault(this);
	}

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		UE_LOG(LogWorkshopEmpire, Error, TEXT("%s: EnhancedInputComponent missing, input disabled."), *GetName());
		return;
	}

	Input->BindAction(InputConfig->MoveAction, ETriggerEvent::Triggered, this, &AWECharacter::Input_Move);
	Input->BindAction(InputConfig->LookAction, ETriggerEvent::Triggered, this, &AWECharacter::Input_Look);
	Input->BindAction(InputConfig->JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	Input->BindAction(InputConfig->JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	Input->BindAction(InputConfig->SprintAction, ETriggerEvent::Started, this, &AWECharacter::Input_SprintStarted);
	Input->BindAction(InputConfig->SprintAction, ETriggerEvent::Completed, this, &AWECharacter::Input_SprintCompleted);
	Input->BindAction(InputConfig->CrouchAction, ETriggerEvent::Started, this, &AWECharacter::Input_CrouchStarted);
	Input->BindAction(InputConfig->CrouchAction, ETriggerEvent::Completed, this, &AWECharacter::Input_CrouchCompleted);
	Input->BindAction(InputConfig->InteractAction, ETriggerEvent::Started, this, &AWECharacter::Input_Interact);
	Input->BindAction(InputConfig->ExamineAction, ETriggerEvent::Started, this, &AWECharacter::Input_Examine);
	Input->BindAction(InputConfig->SecondaryAction, ETriggerEvent::Started, this, &AWECharacter::Input_Secondary);
	Input->BindAction(InputConfig->AlternativeAction, ETriggerEvent::Started, this, &AWECharacter::Input_Alternative);
	Input->BindAction(InputConfig->BuildAction, ETriggerEvent::Started, this, &AWECharacter::Input_Build);
	Input->BindAction(InputConfig->RemoveAction, ETriggerEvent::Started, this, &AWECharacter::Input_Remove);
	Input->BindAction(InputConfig->CycleAction, ETriggerEvent::Triggered, this, &AWECharacter::Input_Cycle);
	Input->BindAction(InputConfig->SettingAction, ETriggerEvent::Started, this, &AWECharacter::Input_Setting);
	Input->BindAction(InputConfig->InventoryAction, ETriggerEvent::Started, this, &AWECharacter::Input_Inventory);
	Input->BindAction(InputConfig->QuickSaveAction, ETriggerEvent::Started, this, &AWECharacter::Input_QuickSave);
	Input->BindAction(InputConfig->QuickLoadAction, ETriggerEvent::Started, this, &AWECharacter::Input_QuickLoad);
	Input->BindAction(InputConfig->PauseAction, ETriggerEvent::Started, this, &AWECharacter::Input_Pause);
	if (InputConfig->ViewAction)
	{
		Input->BindAction(InputConfig->ViewAction, ETriggerEvent::Started, this, &AWECharacter::Input_View);
	}

	AddInputMappingContext();
}

void AWECharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	AddInputMappingContext();
	SendProfileLook();
}

void AWECharacter::AddInputMappingContext()
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController() || !InputConfig || !InputConfig->MappingContext)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		if (!Subsystem->HasMappingContext(InputConfig->MappingContext))
		{
			Subsystem->AddMappingContext(InputConfig->MappingContext, 0);
		}
	}
}

void AWECharacter::Input_Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), Axis.Y);
	AddMovementInput(GetActorRightVector(), Axis.X);
}

void AWECharacter::Input_Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>() * CVarWEMouseSensitivity.GetValueOnGameThread();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(CVarWEInvertY.GetValueOnGameThread() ? Axis.Y : -Axis.Y);
}

void AWECharacter::Input_SprintStarted()
{
	SetSprinting(true);
}

void AWECharacter::Input_SprintCompleted()
{
	SetSprinting(false);
}

void AWECharacter::Input_CrouchStarted()
{
	Crouch();
}

void AWECharacter::Input_CrouchCompleted()
{
	UnCrouch();
}

void AWECharacter::Input_Interact()
{
	if (BuildComponent->IsActive())
	{
		BuildComponent->Confirm();
		return;
	}
	InteractionComponent->RequestPrimaryInteraction();
}

void AWECharacter::Input_Build()
{
	if (Forklift)
	{
		return; // no building from the forklift
	}
	BuildComponent->Toggle();
}

void AWECharacter::Input_Remove()
{
	if (BuildComponent->IsActive())
	{
		BuildComponent->Remove();
		return;
	}
	if (Forklift)
	{
		RequestExitForklift();
		return;
	}
	InteractionComponent->RequestExtraInteraction();
}

void AWECharacter::Input_Setting()
{
	if (BuildComponent->IsActive())
	{
		BuildComponent->CycleCategory(1); // [T] in build mode: next category of the build menu
		return;
	}
	InteractionComponent->RequestSettingInteraction();
}

void AWECharacter::Input_Cycle(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();
	if (BuildComponent->IsActive() && !FMath::IsNearlyZero(Axis))
	{
		BuildComponent->CycleSelection(Axis > 0.f ? -1 : 1);
	}
}

float AWECharacter::GetMaxCarryWeightKg() const
{
	// The best carrying aid in the inventory counts (e.g. a folding hand truck).
	float Bonus = 0.f;
	if (const UWEInventoryComponent* Inventory = GetPlayerInventory())
	{
		for (const FWEInventorySlotData& Slot : Inventory->GetAllSlots())
		{
			Bonus = FMath::Max(Bonus, Slot.Item.Definition ? Slot.Item.Definition->CarryBonusKg : 0.f);
		}
	}
	return Forklift ? AWEForklift::LiftKg : MaxCarryWeightKg + Bonus;
}

void AWECharacter::SetForklift(AWEForklift* InForklift)
{
	Forklift = InForklift;
	OnForkliftChanged();
	ForceNetUpdate();
}

void AWECharacter::OnRep_Forklift()
{
	OnForkliftChanged();
}

void AWECharacter::OnForkliftChanged()
{
	if (Forklift)
	{
		SetSprinting(false);
		if (BuildComponent->IsActive())
		{
			BuildComponent->SetBuildMode(false);
		}
	}
	UpdateMovementSpeed();
}

void AWECharacter::RequestExitForklift()
{
	if (HasAuthority())
	{
		ServerExitForklift_Implementation();
	}
	else
	{
		ServerExitForklift();
	}
}

void AWECharacter::ServerExitForklift_Implementation()
{
	if (Forklift)
	{
		Forklift->Dismount();
	}
}

void AWECharacter::Input_Examine()
{
	InteractionComponent->ToggleExamine();
}

void AWECharacter::Input_Secondary()
{
	if (BuildComponent->IsActive())
	{
		BuildComponent->Rotate(90.f);
		return;
	}
	InteractionComponent->RequestSecondaryInteraction();
}

void AWECharacter::Input_Alternative()
{
	if (BuildComponent->IsActive())
	{
		BuildComponent->Rotate(15.f);
		return;
	}
	InteractionComponent->RequestAlternativeInteraction();
}

void AWECharacter::Input_Inventory()
{
	if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetController()))
	{
		PC->ToggleInventory();
	}
}

void AWECharacter::Input_View()
{
	SetThirdPerson(!bThirdPerson);
}

void AWECharacter::SetThirdPerson(bool bOn)
{
	bThirdPerson = bOn;
	FirstPersonCamera->SetActive(!bOn);
	ThirdPersonCamera->SetFieldOfView(FirstPersonCamera->FieldOfView);
	ThirdPersonCamera->SetActive(bOn);
	// The own body: hidden in first person (the camera sits in the head), visible from behind.
	if (Avatar && Avatar->HasBody())
	{
		Avatar->SetOwnerNoSee(!bOn);
	}
	else
	{
		GetMesh()->SetOwnerNoSee(!bOn);
	}
}

void AWECharacter::Input_Pause()
{
	if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetController()))
	{
		PC->TogglePauseMenu();
	}
}

void AWECharacter::Input_QuickSave()
{
	if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetController()))
	{
		PC->RequestSave(TEXT("Quicksave"));
	}
}

void AWECharacter::Input_QuickLoad()
{
	if (AWEPlayerController* PC = Cast<AWEPlayerController>(GetController()))
	{
		PC->RequestLoad(TEXT("Quicksave"));
	}
}

// --- Sprint ------------------------------------------------------------------------------------

void AWECharacter::SetSprinting(bool bNewSprinting)
{
	if (bIsSprinting == bNewSprinting)
	{
		return;
	}
	// Apply locally right away for responsiveness; the server copy keeps movement corrections consistent.
	bIsSprinting = bNewSprinting;
	UpdateMovementSpeed();
	if (!HasAuthority())
	{
		ServerSetSprinting(bNewSprinting);
	}
}

void AWECharacter::ServerSetSprinting_Implementation(bool bNewSprinting)
{
	bIsSprinting = bNewSprinting;
	UpdateMovementSpeed();
}

void AWECharacter::OnRep_IsSprinting()
{
	UpdateMovementSpeed();
}

void AWECharacter::UpdateMovementSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = Forklift ? AWEForklift::DriveSpeed : (bIsSprinting && !bIsCrouched) ? SprintSpeed : WalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}

// --- Carrying ----------------------------------------------------------------------------------

void AWECharacter::OnRep_HeldItem()
{
	InteractionComponent->UpdateFocus();
}

bool AWECharacter::TryPickUpItem(AWEPickupItem* Item)
{
	if (!HasAuthority() || !Item)
	{
		return false;
	}

	FText Reason;
	if (!Item->CanBePickedUpBy(this, Reason))
	{
		UE_LOG(LogWENet, Log, TEXT("%s pickup of %s rejected: %s"), *GetName(), *Item->GetName(), *Reason.ToString());
		return false;
	}

	HeldItem = Item;
	if (Forklift)
	{
		// On the forks, a little lifted.
		Item->HandlePickedUp(this, Forklift->GetForks());
		Item->SetActorRelativeLocation(FVector(Item->GetCarryRadius() * 0.6f, 0.f, Item->GetCarryRadius() + 15.f));
	}
	else
	{
		Item->HandlePickedUp(this, HoldPoint);
	}
	UE_LOG(LogWENet, Log, TEXT("%s picked up %s"), *GetName(), *Item->GetName());
	return true;
}

void AWECharacter::DropHeldItem()
{
	if (!HasAuthority() || !HeldItem)
	{
		return;
	}

	AWEPickupItem* Item = HeldItem;

	// Aiming at a free workbench with a disassemblable item: put it on the work spot instead.
	if (AWEWorkbench* Bench = FindAimedWorkbench())
	{
		FText Reason;
		if (Bench->CanPlaceItem(Item, Reason))
		{
			HeldItem = nullptr;
			Bench->PlaceItem(Item);
			return;
		}
	}
	// Aiming at a machine that takes it (e.g. a device into the shredder).
	if (AWEMachine* Machine = FindAimedMachine())
	{
		FText Reason;
		if (Machine->CanInsertItem(Item, Reason) && Machine->InsertItem(Item))
		{
			return;
		}
	}
	// Aiming at the van: into the cargo.
	if (AWEVehicle* Van = Cast<AWEVehicle>(FindAimedProp()))
	{
		if (Van->LoadItem(Item))
		{
			return;
		}
	}
	// Aiming at a conveyor: onto the belt.
	if (AWEConveyor* Belt = Cast<AWEConveyor>(FindAimedProp()))
	{
		if (Belt->PutOnBelt(Item, this))
		{
			return;
		}
	}

	const FTransform DropTransform = ComputeDropTransform(Item);
	HeldItem = nullptr;
	Item->HandleDropped(DropTransform);
	UE_LOG(LogWENet, Log, TEXT("%s dropped %s at %s"), *GetName(), *Item->GetName(), *DropTransform.GetLocation().ToCompactString());
}

AWEWorkbench* AWECharacter::FindAimedWorkbench() const
{
	return Cast<AWEWorkbench>(FindAimedProp());
}

AWEMachine* AWECharacter::FindAimedMachine() const
{
	return Cast<AWEMachine>(FindAimedProp());
}

AWEPlaceholderProp* AWECharacter::FindAimedProp() const
{
	FVector EyeLocation;
	FRotator EyeRotation;
	GetActorEyesViewPoint(EyeLocation, EyeRotation);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WEBenchTrace), false, this);
	if (Forklift)
	{
		Params.AddIgnoredActor(Forklift);
	}
	if (HeldItem)
	{
		Params.AddIgnoredActor(HeldItem);
	}
	// Loose items lying on the bench must not hide it: skip them and trace again.
	const FVector End = EyeLocation + EyeRotation.Vector() * PlaceReach;
	for (int32 Attempt = 0; Attempt < 4 && GetWorld(); ++Attempt)
	{
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, End, ECC_Visibility, Params))
		{
			return nullptr;
		}
		if (AWEPickupItem* Item = Cast<AWEPickupItem>(Hit.GetActor()); Item && !Item->GetPlacedOn())
		{
			Params.AddIgnoredActor(Item);
			continue;
		}
		return Cast<AWEPlaceholderProp>(Hit.GetActor());
	}
	return nullptr;
}

AWEPickupItem* AWECharacter::ReleaseHeldItem()
{
	if (!HasAuthority() || !HeldItem)
	{
		return nullptr;
	}
	AWEPickupItem* Item = HeldItem;
	HeldItem = nullptr;
	Item->HandleDropped(Item->GetActorTransform());
	return Item;
}

UWEInventoryComponent* AWECharacter::GetPlayerInventory() const
{
	const AWEPlayerState* PS = GetPlayerState<AWEPlayerState>();
	return PS ? PS->GetInventory() : nullptr;
}

bool AWECharacter::TryStoreWorldItem(AWEPickupItem* Item)
{
	UWEInventoryComponent* Inventory = GetPlayerInventory();
	if (!HasAuthority() || !IsValid(Item) || Item->IsHeld() || !Inventory)
	{
		return false;
	}

	const FWEItemInstance WorldItem = Item->GetItem();
	int32 Added = 0;
	const EWEInventoryResult Result = Inventory->TryAddItem(WorldItem, Added);
	if (Added <= 0)
	{
		UE_LOG(LogWENet, Log, TEXT("%s could not store %s (result %d)"), *GetName(), *Item->GetName(), static_cast<int32>(Result));
		return false;
	}

	if (Added >= WorldItem.Quantity)
	{
		Item->Destroy();
	}
	else
	{
		FWEItemInstance Rest = WorldItem;
		Rest.Quantity -= Added;
		Item->SetItem(Rest);
	}
	UE_LOG(LogWENet, Log, TEXT("%s stored %d x %s"), *GetName(), Added, *WorldItem.GetDisplayName().ToString());
	if (WorldItem.Definition->ToolType != EWEToolType::None)
	{
		if (UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this)) { Progression->NotifyEvent(EWEProgressEvent::ToolStored); }
	}
	return true;
}

FTransform AWECharacter::ComputeDropTransform(const AWEPickupItem* Item) const
{
	return ComputeDropTransform(Item ? Item->GetCarryRadius() : 20.f, Item);
}

FTransform AWECharacter::ComputeDropTransform(float InRadius, const AActor* IgnoredActor) const
{
	FVector EyeLocation;
	FRotator EyeRotation;
	GetActorEyesViewPoint(EyeLocation, EyeRotation);

	// Place where the player looks (e.g. on the workbench or a shelf) if a surface is within reach,
	// otherwise put the item down a short distance in front and let physics settle it.
	const FVector Direction = EyeRotation.Vector();
	const float Radius = FMath::Clamp(InRadius, 5.f, 60.f);
	const FVector Start = EyeLocation;
	const FVector End = EyeLocation + Direction * PlaceReach;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(WEDropTrace), false, this);
	if (IgnoredActor)
	{
		Params.AddIgnoredActor(IgnoredActor);
	}
	if (Forklift)
	{
		Params.AddIgnoredActor(Forklift);
	}

	FVector Location = EyeLocation + Direction * DropDistance;
	FHitResult Hit;
	if (GetWorld() && GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(Radius), Params))
	{
		Location = Hit.Location;
	}

	return FTransform(FRotator(0.f, EyeRotation.Yaw, 0.f), Location);
}
