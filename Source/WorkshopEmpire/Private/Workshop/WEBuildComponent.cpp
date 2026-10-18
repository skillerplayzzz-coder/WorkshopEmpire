#include "Workshop/WEBuildComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Economy/WECompanyComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Framework/WEPlayerController.h"
#include "Player/WECharacter.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Workshop/WEFurnitureDefinition.h"
#include "Workshop/WEWorkshopLayoutComponent.h"
#include "World/WEPlaceholderVisuals.h"

#define LOCTEXT_NAMESPACE "WEBuildMode"

namespace WEBuildMode
{
	constexpr float AimRange = 600.f;
	constexpr float PreviewInterval = 1.f / 30.f;
}

// --- Ghost -------------------------------------------------------------------------------------------

AWEBuildGhost::AWEBuildGhost()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	Boxes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Boxes"));
	Boxes->SetMobility(EComponentMobility::Movable);
	Boxes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Boxes->SetCastShadow(false);
	RootComponent = Boxes;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlaceholderMat(WEPlaceholder::MaterialPath);
	if (CubeMesh.Succeeded())
	{
		Boxes->SetStaticMesh(CubeMesh.Object);
	}
	Material = PlaceholderMat.Object;
	Footprint = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Footprint"));
	Footprint->SetupAttachment(RootComponent);
	Footprint->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Footprint->SetCastShadow(false);
	if (CubeMesh.Succeeded())
	{
		Footprint->SetStaticMesh(CubeMesh.Object);
	}
}

void AWEBuildGhost::SetShape(EWEPlaceholderPropType Type, const FVector& Size, const TArray<TPair<UStaticMesh*, FTransform>>& Parts)
{
	TArray<UStaticMesh*> Meshes;
	for (const TPair<UStaticMesh*, FTransform>& P : Parts) { Meshes.Add(P.Key); }
	if (Type == ShapeType && Size.Equals(ShapeSize) && Meshes == ShapeMeshes)
	{
		return;
	}
	ShapeType = Type;
	ShapeSize = Size;
	ShapeMeshes = Meshes;
	AWEPlaceholderProp::BuildBoxes(Boxes, Type, Size);
	for (UStaticMeshComponent* Part : ModelParts) { if (Part) { Part->DestroyComponent(); } }
	ModelParts.Reset();
	for (const TPair<UStaticMesh*, FTransform>& P : Parts)
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		Part->SetStaticMesh(P.Key);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetupAttachment(RootComponent);
		Part->SetRelativeTransform(P.Value);
		Part->RegisterComponent();
		ModelParts.Add(Part);
	}
	// With a model the boxes only stand in for collision: hidden; the footprint shows the space it takes.
	Boxes->SetVisibility(ModelParts.Num() == 0);
	Footprint->SetRelativeTransform(FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 0.6f), FVector(Size.X / 100.f, Size.Y / 100.f, 0.01f)));
	ValidState = -1;
}

void AWEBuildGhost::ApplyGhostMaterial()
{
	if (!GhostMaterial)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/WorkshopEmpire/Materials/FX/M_WEGhost.M_WEGhost"), nullptr, LOAD_Quiet | LOAD_NoWarn);
		GhostMaterial = UMaterialInstanceDynamic::Create(Base ? Base : Material.Get(), this);
	}
	auto Paint = [this](UMeshComponent* Mesh)
	{
		for (int32 i = 0; Mesh && i < FMath::Max(1, Mesh->GetNumMaterials()); ++i)
		{
			Mesh->SetMaterial(i, GhostMaterial);
		}
	};
	Paint(Boxes);
	Paint(Footprint);
	for (UStaticMeshComponent* Part : ModelParts) { Paint(Part); }
}

void AWEBuildGhost::SetValid(bool bValid)
{
	if (ValidState != (bValid ? 1 : 0))
	{
		ValidState = bValid ? 1 : 0;
		ApplyGhostMaterial();
		const FLinearColor Color = bValid ? FLinearColor(0.15f, 0.9f, 0.3f) : FLinearColor(0.95f, 0.12f, 0.08f);
		if (GhostMaterial)
		{
			GhostMaterial->SetVectorParameterValue(TEXT("Color"), Color);
			GhostMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.38f);
		}
	}
}

// --- Build mode ----------------------------------------------------------------------------------------

UWEBuildComponent::UWEBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWEBuildComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyGhost();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PreviewTimer);
	}
	Super::EndPlay(EndPlayReason);
}

AWECharacter* UWEBuildComponent::GetCharacter() const
{
	return Cast<AWECharacter>(GetOwner());
}

const FWEFurnitureDefinition* UWEBuildComponent::GetSelectedDefinition() const
{
	const UWEWorkshopLayoutComponent* Layout = UWEWorkshopLayoutComponent::Get(this);
	const UWEFurnitureCatalog* Catalog = Layout ? Layout->GetCatalog() : nullptr;
	return Catalog && Catalog->Items.IsValidIndex(SelectedIndex) ? &Catalog->Items[SelectedIndex] : nullptr;
}

void UWEBuildComponent::Toggle()
{
	SetBuildMode(!bBuildMode);
}

void UWEBuildComponent::SetBuildMode(bool bOn)
{
	AWECharacter* Character = GetCharacter();
	if (bOn && (!Character || Character->GetHeldItem()))
	{
		if (AWEPlayerController* PC = Character ? Cast<AWEPlayerController>(Character->GetController()) : nullptr)
		{
			PC->ClientShowMessage(LOCTEXT("HandsFull", "Erst die Hände frei machen"));
		}
		return;
	}
	bBuildMode = bOn;
	MovingProp.Reset();
	HoveredProp.Reset();
	bHasPreview = false;
	if (bOn)
	{
		if (!Ghost)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags |= RF_Transient;
			Ghost = GetWorld()->SpawnActor<AWEBuildGhost>(AWEBuildGhost::StaticClass(), FTransform::Identity, Params);
		}
		GetWorld()->GetTimerManager().SetTimer(PreviewTimer, this, &UWEBuildComponent::UpdatePreview, WEBuildMode::PreviewInterval, true);
		UpdatePreview();
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(PreviewTimer);
		DestroyGhost();
	}
}

void UWEBuildComponent::DestroyGhost()
{
	if (Ghost)
	{
		Ghost->Destroy();
		Ghost = nullptr;
	}
}

TArray<int32> UWEBuildComponent::GetMenuOrder() const
{
	// Catalog indices grouped by build-menu category (catalog order inside a category).
	TArray<int32> Order;
	const UWEWorkshopLayoutComponent* Layout = UWEWorkshopLayoutComponent::Get(this);
	if (const UWEFurnitureCatalog* Catalog = Layout ? Layout->GetCatalog() : nullptr)
	{
		for (int32 c = 0; c < WEFurniture::NumCategories; ++c)
		{
			for (int32 i = 0; i < Catalog->Items.Num(); ++i)
			{
				if (static_cast<int32>(WEFurniture::GetCategory(Catalog->Items[i])) == c)
				{
					Order.Add(i);
				}
			}
		}
	}
	return Order;
}

int32 UWEBuildComponent::GetSelectedCategory() const
{
	const FWEFurnitureDefinition* Definition = GetSelectedDefinition();
	return Definition ? static_cast<int32>(WEFurniture::GetCategory(*Definition)) : INDEX_NONE;
}

void UWEBuildComponent::CycleSelection(int32 Direction)
{
	// "Bearbeiten" (-1) first, then the catalog in menu order.
	TArray<int32> Order = GetMenuOrder();
	Order.Insert(INDEX_NONE, 0);
	const int32 At = FMath::Max(0, Order.IndexOfByKey(SelectedIndex));
	SelectIndex(Order[(At + (Direction >= 0 ? 1 : -1) + Order.Num()) % Order.Num()]);
}

void UWEBuildComponent::CycleCategory(int32 Direction)
{
	const UWEWorkshopLayoutComponent* Layout = UWEWorkshopLayoutComponent::Get(this);
	const UWEFurnitureCatalog* Catalog = Layout ? Layout->GetCatalog() : nullptr;
	if (!Catalog)
	{
		return;
	}
	// First piece of the next category that has any (from "Bearbeiten" the first category).
	const int32 Current = GetSelectedCategory();
	for (int32 Step = 1; Step <= WEFurniture::NumCategories; ++Step)
	{
		const int32 Wanted = ((Current == INDEX_NONE ? (Direction >= 0 ? -1 : 0) : Current) + Step * (Direction >= 0 ? 1 : -1) + WEFurniture::NumCategories * 2) % WEFurniture::NumCategories;
		for (const int32 i : GetMenuOrder())
		{
			if (static_cast<int32>(WEFurniture::GetCategory(Catalog->Items[i])) == Wanted)
			{
				SelectIndex(i);
				return;
			}
		}
	}
}

void UWEBuildComponent::SelectIndex(int32 Index)
{
	SelectedIndex = Index;
	MovingProp.Reset();
	UpdatePreview();
}

void UWEBuildComponent::Rotate(float Degrees)
{
	Yaw = FRotator::NormalizeAxis(Yaw + Degrees);
	UpdatePreview();
}

void UWEBuildComponent::SetRotation(float Degrees)
{
	Yaw = FRotator::NormalizeAxis(Degrees);
	UpdatePreview();
}

bool UWEBuildComponent::TraceAim(FVector& OutLocation, AActor*& OutHitActor) const
{
	OutHitActor = nullptr;
	if (bUseAimOverride)
	{
		OutLocation = AimOverride;
		return true;
	}
	const AWECharacter* Character = GetCharacter();
	if (!Character)
	{
		return false;
	}
	FVector Eye;
	FRotator View;
	Character->GetActorEyesViewPoint(Eye, View);
	if (const UCameraComponent* Camera = Character->GetFirstPersonCamera())
	{
		Eye = Camera->GetComponentLocation();
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WEBuildAim), false, Character);
	if (Ghost)
	{
		Params.AddIgnoredActor(Ghost);
	}
	if (MovingProp.IsValid())
	{
		Params.AddIgnoredActor(MovingProp.Get());
	}
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Eye, Eye + View.Vector() * WEBuildMode::AimRange, ECC_Visibility, Params))
	{
		return false;
	}
	OutHitActor = Hit.GetActor();
	OutLocation = Hit.ImpactPoint;
	return true;
}

void UWEBuildComponent::UpdatePreview()
{
	const UWEWorkshopLayoutComponent* Layout = UWEWorkshopLayoutComponent::Get(this);
	bHasPreview = false;
	bPreviewValid = false;
	Reason = FText::GetEmpty();
	HoveredProp.Reset();
	if (!bBuildMode || !Layout)
	{
		return;
	}
	FVector Aim;
	AActor* HitActor = nullptr;
	const bool bHit = TraceAim(Aim, HitActor);

	const FWEFurnitureDefinition* Definition = GetSelectedDefinition();
	AWEPlaceholderProp* Moving = MovingProp.Get();
	if (!Definition && !Moving)
	{
		// Edit mode: which furniture is aimed at.
		HoveredProp = Cast<AWEPlaceholderProp>(HitActor);
		if (Ghost)
		{
			Ghost->SetActorHiddenInGame(true);
		}
		return;
	}

	const FVector Size = Moving ? Moving->GetSize() : Definition->Size;
	const EWEPlaceholderPropType Type = Moving ? Moving->GetPropType() : Definition->PropType;
	if (!bHit)
	{
		Reason = LOCTEXT("AimFloor", "Auf den Boden zielen");
		if (Ghost)
		{
			Ghost->SetActorHiddenInGame(true);
		}
		return;
	}
	// Stand on the floor where the crosshair hits (the floor is at height 0).
	PreviewTransform = UWEWorkshopLayoutComponent::Snap(FVector(Aim.X, Aim.Y, 0.f), Yaw);
	bHasPreview = true;
	bPreviewValid = Layout->CanPlaceAt(Size, PreviewTransform, Moving, Reason);
	if (bPreviewValid && Definition && !Moving)
	{
		const UWECompanyComponent* Company = UWECompanyComponent::Get(this);
		const UWEWorkshopLayoutComponent* LockLayout = UWEWorkshopLayoutComponent::Get(this);
		const FText Lock = LockLayout ? LockLayout->GetLockReason(*Definition) : FText::GetEmpty();
		if (!Lock.IsEmpty())
		{
			bPreviewValid = false;
			Reason = Lock;
		}
		else if (Company && !Company->CanAfford(Definition->Cost))
		{
			bPreviewValid = false;
			Reason = LOCTEXT("NoMoney", "Nicht genug Geld auf dem Firmenkonto");
		}
	}
	if (Ghost)
	{
		TArray<TPair<UStaticMesh*, FTransform>> Parts;
		if (Moving)
		{
			Moving->GetVisualParts(Parts);
		}
		else
		{
			AWEPlaceholderProp::GetVisualLayout(Definition->VisualMesh, Definition->VisualYaw, Definition->bTileVisual, Definition->Size,
				Definition->DecoMeshes, Definition->DecoTransforms, Parts);
		}
		Ghost->SetShape(Type, Size, Parts);
		Ghost->SetValid(bPreviewValid);
		Ghost->SetActorTransform(PreviewTransform + FTransform(FVector(0.f, 0.f, 0.5f)));
		Ghost->SetActorHiddenInGame(false);
	}
}

void UWEBuildComponent::Confirm()
{
	AWECharacter* Character = GetCharacter();
	AWEPlayerController* PC = Character ? Cast<AWEPlayerController>(Character->GetController()) : nullptr;
	if (!bBuildMode || !PC)
	{
		return;
	}
	UpdatePreview();
	if (AWEPlaceholderProp* Moving = MovingProp.Get())
	{
		if (bHasPreview && bPreviewValid)
		{
			PC->ServerMoveFurniture(Moving, PreviewTransform);
			MovingProp.Reset();
		}
		return;
	}
	if (const FWEFurnitureDefinition* Definition = GetSelectedDefinition())
	{
		if (bHasPreview && bPreviewValid)
		{
			PC->ServerPlaceFurniture(Definition->Id, PreviewTransform, FGuid::NewGuid());
		}
		return;
	}
	// Edit mode: pick up the aimed furniture to move it.
	if (AWEPlaceholderProp* Hovered = HoveredProp.Get())
	{
		FText Why;
		if (UWEWorkshopLayoutComponent::CanPickUpFurniture(Hovered, Why))
		{
			MovingProp = Hovered;
			Yaw = Hovered->GetActorRotation().Yaw;
			UpdatePreview();
		}
		else
		{
			PC->ClientShowMessage(Why);
		}
	}
}

void UWEBuildComponent::Remove()
{
	AWECharacter* Character = GetCharacter();
	AWEPlayerController* PC = Character ? Cast<AWEPlayerController>(Character->GetController()) : nullptr;
	if (!bBuildMode || !PC)
	{
		return;
	}
	if (MovingProp.IsValid())
	{
		MovingProp.Reset(); // cancel moving
		UpdatePreview();
		return;
	}
	UpdatePreview();
	if (AWEPlaceholderProp* Hovered = HoveredProp.Get(); Hovered && !GetSelectedDefinition())
	{
		PC->ServerRemoveFurniture(Hovered, FGuid::NewGuid());
	}
}

FText UWEBuildComponent::GetStatusText() const
{
	if (const AWEPlaceholderProp* Moving = MovingProp.Get())
	{
		return bPreviewValid ? FText::Format(LOCTEXT("MoveHere", "[E] {0} hier abstellen · [X] abbrechen"), Moving->GetFurnitureName())
			: FText::Format(LOCTEXT("MoveBlocked", "{0}: {1}"), Moving->GetFurnitureName(), Reason);
	}
	if (const FWEFurnitureDefinition* Definition = GetSelectedDefinition())
	{
		return bPreviewValid ? FText::Format(LOCTEXT("PlaceHere", "[E] {0} hier aufstellen ({1})"), Definition->DisplayName, FText::FromString(Definition->Cost.ToDisplayString()))
			: FText::Format(LOCTEXT("PlaceBlocked", "{0}: {1}"), Definition->DisplayName, Reason);
	}
	if (const AWEPlaceholderProp* Hovered = HoveredProp.Get())
	{
		return Hovered->IsPlayerPlaced()
			? FText::Format(LOCTEXT("EditBought", "{0}: [E] verschieben · [X] abbauen (½ Preis zurück)"), Hovered->GetFurnitureName())
			: FText::Format(LOCTEXT("EditMap", "{0}: [E] verschieben (Grundausstattung)"), Hovered->GetFurnitureName());
	}
	return LOCTEXT("EditNothing", "Auf ein Möbelstück zielen, um es zu verschieben oder abzubauen");
}

TArray<FString> UWEBuildComponent::GetMenuLines() const
{
	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("%s Bearbeiten (verschieben / abbauen)"), SelectedIndex == INDEX_NONE ? TEXT("▶") : TEXT("   ")));
	const UWEWorkshopLayoutComponent* Layout = UWEWorkshopLayoutComponent::Get(this);
	if (const UWEFurnitureCatalog* Catalog = Layout ? Layout->GetCatalog() : nullptr)
	{
		for (int32 i = 0; i < Catalog->Items.Num(); ++i)
		{
			const FWEFurnitureDefinition& D = Catalog->Items[i];
			const FText Lock = Layout->GetLockReason(D);
			Lines.Add(FString::Printf(TEXT("%s %s – %s%s"), i == SelectedIndex ? TEXT("▶") : TEXT("   "), *D.DisplayName.ToString(), *D.Cost.ToDisplayString(),
				Lock.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("  (%s)"), *Lock.ToString())));
		}
	}
	return Lines;
}

#undef LOCTEXT_NAMESPACE
