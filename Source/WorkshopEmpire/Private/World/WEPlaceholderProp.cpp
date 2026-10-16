#include "World/WEPlaceholderProp.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Framework/WEPlayerController.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "Progression/WEProgressionComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "World/WEPlaceholderVisuals.h"
#include "Net/UnrealNetwork.h"
#include "Workshop/WEFurnitureDefinition.h"
#include "Workshop/WEWorkshopLayoutComponent.h"

AWEPlaceholderProp::AWEPlaceholderProp()
{
	PrimaryActorTick.bCanEverTick = false;

	Boxes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Boxes"));
	// Movable: furniture can be moved in build mode (Phase 8).
	Boxes->SetMobility(EComponentMobility::Movable);
	Boxes->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RootComponent = Boxes;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Boxes->SetStaticMesh(CubeMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlaceholderMat(WEPlaceholder::MaterialPath);
	PlaceholderMaterial = PlaceholderMat.Object;

	// Replicated so the storage contents reach clients and can be addressed by RPCs.
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(5.f);
	Storage = CreateDefaultSubobject<UWEInventoryComponent>(TEXT("Storage"));
}

void AWEPlaceholderProp::AddBox(const FVector& Center, const FVector& BoxSize)
{
	Boxes->AddInstance(FTransform(FRotator::ZeroRotator, Center, BoxSize / 100.f));
}

float AWEPlaceholderProp::GetSurfaceHeight() const
{
	return PropType == EWEPlaceholderPropType::Workbench ? Size.Z : Size.Z * 0.2f;
}

void AWEPlaceholderProp::BuildBoxes(UInstancedStaticMeshComponent* ISM, EWEPlaceholderPropType Type, const FVector& InSize)
{
	ISM->ClearInstances();
	auto Box = [ISM](const FVector& Center, const FVector& BoxSize)
	{
		ISM->AddInstance(FTransform(FRotator::ZeroRotator, Center, BoxSize / 100.f));
	};
	const float D = InSize.X;
	const float W = InSize.Y;
	const float H = InSize.Z;

	switch (Type)
	{
	case EWEPlaceholderPropType::Workbench:
	{
		const float Top = 5.f;
		const float Leg = 7.f;
		Box(FVector(0.f, 0.f, H - Top * 0.5f), FVector(D, W, Top));
		for (const float SX : { -1.f, 1.f })
		{
			for (const float SY : { -1.f, 1.f })
			{
				Box(FVector(SX * (D * 0.5f - Leg), SY * (W * 0.5f - Leg), (H - Top) * 0.5f), FVector(Leg, Leg, H - Top));
			}
		}
		// Lower storage board.
		Box(FVector(0.f, 0.f, 18.f), FVector(D - 2.f * Leg, W - 2.f * Leg, 3.f));
		break;
	}
	case EWEPlaceholderPropType::Shelf:
	{
		const float Post = 5.f;
		for (const float SX : { -1.f, 1.f })
		{
			for (const float SY : { -1.f, 1.f })
			{
				Box(FVector(SX * (D * 0.5f - Post * 0.5f), SY * (W * 0.5f - Post * 0.5f), H * 0.5f), FVector(Post, Post, H));
			}
		}
		for (const float Level : { 0.2f, 0.55f, 0.9f })
		{
			Box(FVector(0.f, 0.f, H * Level), FVector(D, W, 3.f));
		}
		break;
	}
	case EWEPlaceholderPropType::Machine:
	{
		// Body, hopper frame on top, control box at the front right.
		const float BodyH = H * 0.72f;
		Box(FVector(0.f, 0.f, BodyH * 0.5f), FVector(D, W, BodyH));
		const float Wall = 4.f;
		const float HopperH = H - BodyH;
		for (const float S : { -1.f, 1.f })
		{
			Box(FVector(S * (D * 0.35f), 0.f, BodyH + HopperH * 0.5f), FVector(Wall, W * 0.7f, HopperH));
			Box(FVector(0.f, S * (W * 0.35f), BodyH + HopperH * 0.5f), FVector(D * 0.7f, Wall, HopperH));
		}
		Box(FVector(D * 0.5f + 6.f, W * 0.3f, BodyH * 0.65f), FVector(12.f, 22.f, 30.f));
		break;
	}
	case EWEPlaceholderPropType::Conveyor:
	{
		// Side rails, belt, legs and a chevron on the belt pointing along +X (the running direction).
		const float Rail = 6.f;
		for (const float S : { -1.f, 1.f })
		{
			Box(FVector(0.f, S * (W * 0.5f - Rail * 0.5f), H - 6.f), FVector(D, Rail, 12.f));
			for (const float SX : { -1.f, 1.f })
			{
				Box(FVector(SX * (D * 0.5f - 6.f), S * (W * 0.5f - Rail * 0.5f), (H - 12.f) * 0.5f), FVector(6.f, Rail, H - 12.f));
			}
		}
		Box(FVector(0.f, 0.f, H - 3.f), FVector(D, W - 2.f * Rail, 4.f));
		for (const float S : { -1.f, 1.f })
		{
			ISM->AddInstance(FTransform(FRotator(0.f, S * 40.f, 0.f), FVector(D * 0.25f, S * (W * 0.12f), H - 0.5f), FVector(0.22f, 0.04f, 0.02f)));
		}
		break;
	}
	case EWEPlaceholderPropType::Vehicle:
	{
		// Wheels, cargo box (back), cab (front) with a windscreen strip.
		const float Wheel = 34.f;
		for (const float SX : { -0.32f, 0.32f })
		{
			for (const float SY : { -1.f, 1.f })
			{
				Box(FVector(SX * D, SY * (W * 0.5f - 12.f), Wheel * 0.5f), FVector(Wheel * 1.6f, 22.f, Wheel));
			}
		}
		const float CargoLength = D * 0.68f;
		Box(FVector(-D * 0.5f + CargoLength * 0.5f, 0.f, Wheel + (H - Wheel) * 0.5f), FVector(CargoLength, W, H - Wheel));
		const float CabLength = D - CargoLength - 4.f;
		Box(FVector(D * 0.5f - CabLength * 0.5f, 0.f, Wheel + (H * 0.72f - Wheel) * 0.5f), FVector(CabLength, W - 6.f, H * 0.72f - Wheel));
		Box(FVector(D * 0.5f - 2.f, 0.f, H * 0.58f), FVector(4.f, W - 30.f, H * 0.18f));
		break;
	}
	case EWEPlaceholderPropType::Forklift:
	{
		// Body with counterweight at the back, overhead guard, mast and forks at the front.
		Box(FVector(-D * 0.12f, 0.f, 35.f), FVector(D * 0.6f, W, 50.f));
		Box(FVector(-D * 0.38f, 0.f, 75.f), FVector(D * 0.16f, W, 40.f));
		for (const float SY : { -1.f, 1.f })
		{
			Box(FVector(-D * 0.05f, SY * (W * 0.5f - 4.f), H * 0.6f), FVector(6.f, 6.f, H * 0.8f));
			Box(FVector(D * 0.2f, SY * 22.f, H * 0.5f), FVector(8.f, 8.f, H));
			Box(FVector(D * 0.36f, SY * 22.f, 6.f), FVector(D * 0.3f, 10.f, 4.f));
		}
		Box(FVector(-D * 0.05f, 0.f, H - 3.f), FVector(D * 0.4f, W, 4.f));
		break;
	}
	case EWEPlaceholderPropType::Crate:
	{
		const float Wall = 3.f;
		Box(FVector(0.f, 0.f, Wall * 0.5f), FVector(D, W, Wall));
		for (const float S : { -1.f, 1.f })
		{
			Box(FVector(S * (D * 0.5f - Wall * 0.5f), 0.f, H * 0.5f), FVector(Wall, W, H));
			Box(FVector(0.f, S * (W * 0.5f - Wall * 0.5f), H * 0.5f), FVector(D, Wall, H));
		}
		break;
	}
	}
}

void AWEPlaceholderProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildVisuals();
}

void AWEPlaceholderProp::RebuildVisuals()
{
	BuildBoxes(Boxes, PropType, Size);
	ApplyTint();
	Storage->Configure(StorageName, StorageSlots + StorageBonusSlots, StorageMaxWeightKg);
}

void AWEPlaceholderProp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWEPlaceholderProp, FurnitureId);
}

void AWEPlaceholderProp::ApplyFurniture(const FWEFurnitureDefinition& Definition)
{
	FurnitureId = Definition.Id;
	PropType = Definition.PropType;
	Size = Definition.Size;
	Tint = Definition.Tint;
	bHasStorage = Definition.bHasStorage;
	StorageName = Definition.StorageName.IsEmpty() ? Definition.DisplayName : Definition.StorageName;
	StorageSlots = Definition.StorageSlots;
	StorageMaxWeightKg = Definition.StorageMaxWeightKg;
	ApplyFurnitureExtras(Definition);
	RebuildVisuals();
}

void AWEPlaceholderProp::OnRep_FurnitureId()
{
	const UWEWorkshopLayoutComponent* Layout = UWEWorkshopLayoutComponent::Get(this);
	if (const FWEFurnitureDefinition* Definition = Layout ? Layout->FindDefinition(FurnitureId) : nullptr)
	{
		ApplyFurniture(*Definition);
	}
}

FText AWEPlaceholderProp::GetFurnitureName() const
{
	if (const UWEWorkshopLayoutComponent* Layout = UWEWorkshopLayoutComponent::Get(this))
	{
		if (const FWEFurnitureDefinition* Definition = Layout->FindDefinition(FurnitureId))
		{
			return Definition->DisplayName;
		}
	}
	if (!StorageName.IsEmpty())
	{
		return StorageName;
	}
	return PropType == EWEPlaceholderPropType::Workbench ? NSLOCTEXT("WEPlaceholderProp", "Table", "Tisch") : NSLOCTEXT("WEPlaceholderProp", "Furniture", "Möbel");
}

void AWEPlaceholderProp::BeginPlay()
{
	// Same values on server and clients (map data, or the replicated furniture id), so slot counts match.
	if (!FurnitureId.IsNone())
	{
		OnRep_FurnitureId();
	}
	Storage->Configure(StorageName, StorageSlots + StorageBonusSlots, StorageMaxWeightKg);
	Super::BeginPlay();
	ApplyTint();
	if (!RequiredUpgrade.IsNone())
	{
		// Locked until the company owns the upgrade (also on clients before the game state arrived).
		const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
		SetUnlocked(Progression && Progression->HasUpgrade(RequiredUpgrade));
	}
}

FText AWEPlaceholderProp::GetInteractionText(const AWECharacter* Interactor) const
{
	return FText::Format(NSLOCTEXT("WEPlaceholderProp", "Open", "{0} öffnen"), StorageName);
}

bool AWEPlaceholderProp::CanInteract(const AWECharacter* Interactor, FText& OutReason) const
{
	return HasStorage() && Interactor != nullptr;
}

void AWEPlaceholderProp::Interact(AWECharacter* Interactor)
{
	if (bHasStorage && Interactor)
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(Interactor->GetController()))
		{
			PC->ClientOpenContainer(Storage);
		}
	}
}

bool AWEPlaceholderProp::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	if (!bHasStorage)
	{
		return false;
	}
	OutInfo.Title = StorageName;
	OutInfo.Description = NSLOCTEXT("WEPlaceholderProp", "StorageDesc", "Lagerplatz für Kleinteile und Material.");
	OutInfo.Details = {
		FText::Format(NSLOCTEXT("WEPlaceholderProp", "Slots", "Belegt: {0} / {1} Plätze"),
			FText::AsNumber(Storage->GetAllSlots().Num()), FText::AsNumber(Storage->GetNumSlots())),
		FText::Format(NSLOCTEXT("WEPlaceholderProp", "Weight", "Gewicht: {0} / {1}"),
			AWEPickupItem::FormatWeight(Storage->GetTotalWeightKg()), AWEPickupItem::FormatWeight(Storage->GetMaxWeightKg()))
	};
	return true;
}

void AWEPlaceholderProp::ApplyTint()
{
	WEPlaceholder::ApplyTint(Boxes, PlaceholderMaterial, Tint);
}

void AWEPlaceholderProp::SetUnlocked(bool bInUnlocked)
{
	bUnlocked = bInUnlocked;
	SetActorHiddenInGame(!bUnlocked);
	SetActorEnableCollision(bUnlocked);
}

void AWEPlaceholderProp::SetStorageBonusSlots(int32 Bonus)
{
	StorageBonusSlots = FMath::Max(0, Bonus);
	Storage->Configure(StorageName, StorageSlots + StorageBonusSlots, StorageMaxWeightKg);
	Storage->NotifyChanged();
}