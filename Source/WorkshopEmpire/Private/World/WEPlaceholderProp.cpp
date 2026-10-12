#include "World/WEPlaceholderProp.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Framework/WEPlayerController.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "World/WEPlaceholderVisuals.h"

AWEPlaceholderProp::AWEPlaceholderProp()
{
	PrimaryActorTick.bCanEverTick = false;

	Boxes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Boxes"));
	Boxes->SetMobility(EComponentMobility::Static);
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

void AWEPlaceholderProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Boxes->ClearInstances();

	const float D = Size.X;
	const float W = Size.Y;
	const float H = Size.Z;

	switch (PropType)
	{
	case EWEPlaceholderPropType::Workbench:
	{
		const float Top = 5.f;
		const float Leg = 7.f;
		AddBox(FVector(0.f, 0.f, H - Top * 0.5f), FVector(D, W, Top));
		for (const float SX : { -1.f, 1.f })
		{
			for (const float SY : { -1.f, 1.f })
			{
				AddBox(FVector(SX * (D * 0.5f - Leg), SY * (W * 0.5f - Leg), (H - Top) * 0.5f), FVector(Leg, Leg, H - Top));
			}
		}
		// Lower storage board.
		AddBox(FVector(0.f, 0.f, 18.f), FVector(D - 2.f * Leg, W - 2.f * Leg, 3.f));
		break;
	}
	case EWEPlaceholderPropType::Shelf:
	{
		const float Post = 5.f;
		for (const float SX : { -1.f, 1.f })
		{
			for (const float SY : { -1.f, 1.f })
			{
				AddBox(FVector(SX * (D * 0.5f - Post * 0.5f), SY * (W * 0.5f - Post * 0.5f), H * 0.5f), FVector(Post, Post, H));
			}
		}
		for (const float Level : { 0.2f, 0.55f, 0.9f })
		{
			AddBox(FVector(0.f, 0.f, H * Level), FVector(D, W, 3.f));
		}
		break;
	}
	}

	ApplyTint();
	Storage->Configure(StorageName, StorageSlots, StorageMaxWeightKg);
}

void AWEPlaceholderProp::BeginPlay()
{
	// Same values on server and clients (they come from the map), so slot counts match without replication.
	Storage->Configure(StorageName, StorageSlots, StorageMaxWeightKg);
	Super::BeginPlay();
	ApplyTint();
}

FText AWEPlaceholderProp::GetInteractionText(const AWECharacter* Interactor) const
{
	return FText::Format(NSLOCTEXT("WEPlaceholderProp", "Open", "{0} öffnen"), StorageName);
}

bool AWEPlaceholderProp::CanInteract(const AWECharacter* Interactor, FText& OutReason) const
{
	return bHasStorage && Interactor != nullptr;
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
