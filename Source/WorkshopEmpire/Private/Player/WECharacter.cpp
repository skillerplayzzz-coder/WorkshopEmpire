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
#include "Interaction/WEInteractionComponent.h"
#include "InputActionValue.h"
#include "Items/WEPickupItem.h"
#include "Net/UnrealNetwork.h"
#include "Player/WEInputConfig.h"
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

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetOwnerNoSee(true);
	BodyMesh->SetRelativeScale3D(FVector(0.65f, 0.65f, 1.76f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CylinderMesh.Object);
	}

	InteractionComponent = CreateDefaultSubobject<UWEInteractionComponent>(TEXT("InteractionComponent"));

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
}

void AWECharacter::BeginPlay()
{
	Super::BeginPlay();
	CurrentCameraHeight = BaseEyeHeight;
	UpdateMovementSpeed();
}

void AWECharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// A player leaving (or dying later on) must never take an item out of the world.
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

	AddInputMappingContext();
}

void AWECharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	AddInputMappingContext();
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
	InteractionComponent->RequestPrimaryInteraction();
}

void AWECharacter::Input_Examine()
{
	InteractionComponent->ToggleExamine();
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
	GetCharacterMovement()->MaxWalkSpeed = (bIsSprinting && !bIsCrouched) ? SprintSpeed : WalkSpeed;
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
	Item->HandlePickedUp(this, HoldPoint);
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
	const FTransform DropTransform = ComputeDropTransform(Item);
	HeldItem = nullptr;
	Item->HandleDropped(DropTransform);
	UE_LOG(LogWENet, Log, TEXT("%s dropped %s at %s"), *GetName(), *Item->GetName(), *DropTransform.GetLocation().ToCompactString());
}

FTransform AWECharacter::ComputeDropTransform(const AWEPickupItem* Item) const
{
	FVector EyeLocation;
	FRotator EyeRotation;
	GetActorEyesViewPoint(EyeLocation, EyeRotation);

	// Place where the player looks (e.g. on the workbench or a shelf) if a surface is within reach,
	// otherwise put the item down a short distance in front and let physics settle it.
	const FVector Direction = EyeRotation.Vector();
	const float Radius = Item ? FMath::Clamp(Item->GetCarryRadius(), 5.f, 60.f) : 20.f;
	const FVector Start = EyeLocation;
	const FVector End = EyeLocation + Direction * PlaceReach;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(WEDropTrace), false, this);
	if (Item)
	{
		Params.AddIgnoredActor(Item);
	}

	FVector Location = EyeLocation + Direction * DropDistance;
	FHitResult Hit;
	if (GetWorld() && GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(Radius), Params))
	{
		Location = Hit.Location;
	}

	return FTransform(FRotator(0.f, EyeRotation.Yaw, 0.f), Location);
}
