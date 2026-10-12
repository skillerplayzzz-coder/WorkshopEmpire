#include "Items/WEPickupItem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Player/WECharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "World/WEPlaceholderVisuals.h"

#define LOCTEXT_NAMESPACE "WEPickupItem"

AWEPickupItem::AWEPickupItem()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(30.f);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	Mesh->SetSimulatePhysics(true);
	Mesh->SetLinearDamping(0.2f);
	Mesh->SetAngularDamping(0.5f);
	RootComponent = Mesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		ItemMesh = CubeMesh.Object;
		Mesh->SetStaticMesh(ItemMesh);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlaceholderMat(WEPlaceholder::MaterialPath);
	PlaceholderMaterial = PlaceholderMat.Object;

	DisplayName = LOCTEXT("DefaultName", "Gegenstand");
}

void AWEPickupItem::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (ItemMesh && Mesh->GetStaticMesh() != ItemMesh)
	{
		Mesh->SetStaticMesh(ItemMesh);
	}
	Mesh->SetMassOverrideInKg(NAME_None, WeightKg, true);
	ApplyTint();
}

void AWEPickupItem::BeginPlay()
{
	Super::BeginPlay();
	ApplyTint();
}

void AWEPickupItem::ApplyTint()
{
	WEPlaceholder::ApplyTint(Mesh, PlaceholderMaterial, Tint);
}

void AWEPickupItem::OnRep_AttachmentReplication()
{
	// Physics must be off before the client attaches the root to the holder's hand,
	// otherwise the simulated body would fight the attachment.
	if (GetAttachmentReplication().AttachParent)
	{
		ApplyHeldState(true);
	}
	Super::OnRep_AttachmentReplication();
}

void AWEPickupItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWEPickupItem, Holder);
}

void AWEPickupItem::SetItemData(const FText& InName, const FText& InDescription, float InWeightKg)
{
	DisplayName = InName;
	Description = InDescription;
	WeightKg = FMath::Max(0.01f, InWeightKg);
	Mesh->SetMassOverrideInKg(NAME_None, WeightKg, true);
}

FText AWEPickupItem::FormatWeight(float Kg)
{
	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 1;
	Options.MaximumFractionalDigits = 1;
	return FText::Format(LOCTEXT("WeightFmt", "{0} kg"), FText::AsNumber(Kg, &Options));
}

float AWEPickupItem::GetCarryRadius() const
{
	return Mesh ? Mesh->Bounds.BoxExtent.GetMax() : 20.f;
}

// --- Interaction -------------------------------------------------------------------------------

FText AWEPickupItem::GetInteractionText(const AWECharacter* Interactor) const
{
	return LOCTEXT("PickUp", "Aufheben");
}

bool AWEPickupItem::CanInteract(const AWECharacter* Interactor, FText& OutReason) const
{
	return CanBePickedUpBy(Interactor, OutReason);
}

void AWEPickupItem::Interact(AWECharacter* Interactor)
{
	if (Interactor)
	{
		Interactor->TryPickUpItem(this);
	}
}

bool AWEPickupItem::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	OutInfo.Title = DisplayName;
	OutInfo.Description = Description;
	OutInfo.Details.Reset();
	OutInfo.Details.Add(FText::Format(LOCTEXT("WeightLine", "Gewicht: {0}"), FormatWeight(WeightKg)));
	if (Holder)
	{
		const APlayerState* PS = Holder->GetPlayerState();
		OutInfo.Details.Add(FText::Format(LOCTEXT("HeldByLine", "Getragen von: {0}"),
			FText::FromString(PS ? PS->GetPlayerName() : Holder->GetName())));
	}
	return true;
}

bool AWEPickupItem::CanBePickedUpBy(const AWECharacter* Character, FText& OutReason) const
{
	if (!Character || !IsValid(this))
	{
		OutReason = FText::GetEmpty();
		return false;
	}
	if (Holder)
	{
		OutReason = Holder == Character
			? LOCTEXT("AlreadyHolding", "Bereits in der Hand")
			: LOCTEXT("HeldByOther", "Wird gerade getragen");
		return false;
	}
	if (Character->GetHeldItem())
	{
		OutReason = LOCTEXT("HandsFull", "Hände sind voll");
		return false;
	}
	if (WeightKg > Character->GetMaxCarryWeightKg())
	{
		OutReason = FText::Format(LOCTEXT("TooHeavy", "Zu schwer zum Tragen ({0})"), FormatWeight(WeightKg));
		return false;
	}
	return true;
}

void AWEPickupItem::HandlePickedUp(AWECharacter* NewHolder, USceneComponent* AttachTo)
{
	check(HasAuthority());
	const float Radius = GetCarryRadius();
	Holder = NewHolder;
	ApplyHeldState(true);
	if (AttachTo)
	{
		AttachToComponent(AttachTo, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		// Push bigger objects forward and down so they do not cover the view.
		SetActorRelativeLocation(FVector(Radius * 0.9f, 0.f, -Radius * 0.8f));
	}
	ForceNetUpdate();
}

void AWEPickupItem::HandleDropped(const FTransform& DropTransform)
{
	check(HasAuthority());
	Holder = nullptr;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorLocationAndRotation(DropTransform.GetLocation(), DropTransform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
	ApplyHeldState(false);
	ForceNetUpdate();
}

void AWEPickupItem::OnRep_Holder()
{
	ApplyHeldState(Holder != nullptr);
}

void AWEPickupItem::ApplyHeldState(bool bHeld)
{
	if (bHeld)
	{
		Mesh->SetSimulatePhysics(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetSimulatePhysics(true);
		Mesh->WakeAllRigidBodies();
	}
}

#undef LOCTEXT_NAMESPACE
