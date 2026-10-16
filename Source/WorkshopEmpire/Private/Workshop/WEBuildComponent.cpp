#include "Workshop/WEBuildComponent.h"

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
}

void AWEBuildGhost::SetShape(EWEPlaceholderPropType Type, const FVector& Size)
{
	if (Type == ShapeType && Size.Equals(ShapeSize))
	{
		return;
	}
	ShapeType = Type;
	ShapeSize = Size;
	AWEPlaceholderProp::BuildBoxes(Boxes, Type, Size);
	ValidState = -1;
}

void AWEBuildGhost::SetValid(bool bValid)
{
	if (ValidState != (bValid ? 1 : 0))
	{
		ValidState = bValid ? 1 : 0;
		WEPlaceholder::ApplyTint(Boxes, Material, bValid ? FLinearColor(0.2f, 0.85f, 0.3f) : FLinearColor(0.9f, 0.15f, 0.1f));
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

void UWEBuildComponent::CycleSelection(int32 Direction)
{
	const UWEWorkshopLayoutComponent* Layout = UWEWorkshopLayoutComponent::Get(this);
	const int32 Count = Layout && Layout->GetCatalog() ? Layout->GetCatalog()->Items.Num() : 0;
	// -1 = "Bearbeiten", 0..Count-1 = catalog.
	SelectIndex(((SelectedIndex + 1 + (Direction >= 0 ? 1 : -1) + Count + 1) % (Count + 1)) - 1);
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
		if (Company && !Company->CanAfford(Definition->Cost))
		{
			bPreviewValid = false;
			Reason = LOCTEXT("NoMoney", "Nicht genug Geld auf dem Firmenkonto");
		}
	}
	if (Ghost)
	{
		Ghost->SetShape(Type, Size);
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
			Lines.Add(FString::Printf(TEXT("%s %s – %s"), i == SelectedIndex ? TEXT("▶") : TEXT("   "), *D.DisplayName.ToString(), *D.Cost.ToDisplayString()));
		}
	}
	return Lines;
}

#undef LOCTEXT_NAMESPACE
