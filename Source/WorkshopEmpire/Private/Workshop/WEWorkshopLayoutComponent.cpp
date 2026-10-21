#include "Workshop/WEWorkshopLayoutComponent.h"
#include "World/WEToolWall.h"

#include "Economy/WECompanyComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Framework/WEGameState.h"
#include "Framework/WEPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEPickupItem.h"
#include "Player/WECharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "WELog.h"
#include "Workshop/WEFurnitureDefinition.h"
#include "Progression/WEUpgradeDefinition.h"
#include "Progression/WEProgressionComponent.h"
#include "Workshop/WEWorkbench.h"
#include "Machines/WEMachine.h"
#include "Machines/WEConveyor.h"
#include "Machines/WEProductionStorage.h"
#include "World/WEBuildArea.h"

#define LOCTEXT_NAMESPACE "WEBuild"

namespace WEBuild
{
	/** Placement must be confirmed near the player (anti-cheat sanity check, generous). */
	constexpr float MaxPlaceDistance = 1200.f;
	constexpr float RemoveRefundFactor = 0.5f;
}

UWEWorkshopLayoutComponent::UWEWorkshopLayoutComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UWEFurnitureCatalog> DefaultCatalog(TEXT("/Game/WorkshopEmpire/Data/Workshop/DA_FurnitureCatalog.DA_FurnitureCatalog"));
	Catalog = DefaultCatalog.Object;
}

UWEWorkshopLayoutComponent* UWEWorkshopLayoutComponent::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AWEGameState* GameState = World ? World->GetGameState<AWEGameState>() : nullptr;
	return GameState ? GameState->GetLayout() : nullptr;
}

const FWEFurnitureDefinition* UWEWorkshopLayoutComponent::FindDefinition(FName Id) const
{
	return Catalog && !Id.IsNone() ? Catalog->Find(Id) : nullptr;
}

FText UWEWorkshopLayoutComponent::GetLockReason(const FWEFurnitureDefinition& Definition) const
{
	if (Definition.RequiredUpgrade.IsNone())
	{
		return FText::GetEmpty();
	}
	const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	if (Progression && Progression->HasUpgrade(Definition.RequiredUpgrade))
	{
		return FText::GetEmpty();
	}
	const FWEUpgradeDefinition* Upgrade = Progression && Progression->GetCatalog() ? Progression->GetCatalog()->Find(Definition.RequiredUpgrade) : nullptr;
	return FText::Format(LOCTEXT("Locked", "Benötigt: {0}"), Upgrade ? Upgrade->DisplayName : FText::FromName(Definition.RequiredUpgrade));
}

FVector UWEWorkshopLayoutComponent::GetFootprint(const AWEPlaceholderProp* Prop)
{
	return Prop ? Prop->GetSize() : FVector::ZeroVector;
}

FTransform UWEWorkshopLayoutComponent::Snap(const FVector& Location, float Yaw)
{
	const FVector Snapped(FMath::GridSnap(Location.X, GridSize), FMath::GridSnap(Location.Y, GridSize), 0.f);
	return FTransform(FRotator(0.f, FMath::GridSnap(FRotator::NormalizeAxis(Yaw), 15.f), 0.f), Snapped);
}

bool UWEWorkshopLayoutComponent::CanPlaceAt(const FVector& Size, const FTransform& Transform, const AActor* Ignore, FText& OutReason) const
{
	const UWorld* World = GetWorld();
	if (!World || Size.IsNearlyZero())
	{
		return false;
	}
	if (!AWEBuildArea::ContainsFootprint(World, Transform, FVector2D(Size.X * 0.5f, Size.Y * 0.5f)))
	{
		OutReason = LOCTEXT("Outside", "Nur innerhalb der Werkstatt");
		return false;
	}
	// A box slightly above the floor, slightly smaller than the furniture: touching is fine, overlapping is not.
	const FVector Half(FMath::Max(1.f, Size.X * 0.5f - 1.f), FMath::Max(1.f, Size.Y * 0.5f - 1.f), FMath::Max(1.f, Size.Z * 0.5f - 2.f));
	const FVector Center = Transform.TransformPosition(FVector(0.f, 0.f, Size.Z * 0.5f + 3.f));
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WEBuildOverlap), false, Ignore);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Center, Transform.GetRotation(), Objects, FCollisionShape::MakeBox(Half), Params);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		const AActor* Other = Overlap.GetActor();
		if (!Other || Other == Ignore || Other->IsHidden() || Other->IsA<AWEBuildArea>())
		{
			continue;
		}
		// Items lying on the furniture being moved travel along conceptually only if attached; anything else blocks.
		if (Ignore && Other->GetAttachParentActor() == Ignore)
		{
			continue;
		}
		OutReason = Other->IsA<APawn>() ? LOCTEXT("BlockedPawn", "Jemand steht im Weg")
			: Other->IsA<AWEPickupItem>() ? LOCTEXT("BlockedItem", "Ein Gegenstand liegt im Weg")
			: LOCTEXT("Blocked", "Kein Platz – hier steht schon etwas");
		return false;
	}
	return true;
}

bool UWEWorkshopLayoutComponent::CanPickUpFurniture(const AWEPlaceholderProp* Prop, FText& OutReason)
{
	if (!Prop || Prop->IsHidden())
	{
		return false;
	}
	if (const AWEWorkbench* Bench = Cast<AWEWorkbench>(Prop))
	{
		if (Bench->IsBusy() || Bench->GetPlacedItem())
		{
			OutReason = LOCTEXT("BenchInUse", "Erst den Gegenstand von der Werkbank nehmen");
			return false;
		}
	}
	if (const AWEMachine* Machine = Cast<AWEMachine>(Prop))
	{
		if (Machine->IsOn() || Machine->GetProcessing().Num() > 0)
		{
			OutReason = LOCTEXT("MachineOn", "Erst die Maschine ausschalten und leerlaufen lassen");
			return false;
		}
	}
	if (const AWEConveyor* Belt = Cast<AWEConveyor>(Prop); Belt && Belt->GetItems().Num() > 0)
	{
		OutReason = LOCTEXT("BeltNotEmpty", "Erst das Band leeren");
		return false;
	}
	// Loose items lying on boards or the top.
	const FVector Size = Prop->GetSize();
	const FVector Half(Size.X * 0.5f, Size.Y * 0.5f, Size.Z * 0.5f + 20.f);
	const FVector Center = Prop->GetActorTransform().TransformPosition(FVector(0.f, 0.f, Size.Z * 0.5f + 20.f));
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WEBuildItemsOn), false, Prop);
	Prop->GetWorld()->OverlapMultiByObjectType(Overlaps, Center, Prop->GetActorQuat(), FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects),
		FCollisionShape::MakeBox(Half), Params);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		const AWEPickupItem* Item = Cast<AWEPickupItem>(Overlap.GetActor());
		if (Item && !Item->IsHeld() && !Item->IsActorBeingDestroyed())
		{
			OutReason = LOCTEXT("ItemsOnIt", "Erst die Gegenstände herunternehmen");
			return false;
		}
	}
	return true;
}

void UWEWorkshopLayoutComponent::Notify(AWECharacter* Character, const FText& Message) const
{
	if (AWEPlayerController* PC = Character ? Cast<AWEPlayerController>(Character->GetController()) : nullptr)
	{
		PC->ClientShowMessage(Message);
	}
}

FText UWEWorkshopLayoutComponent::ResultText(EWEBuildResult Result)
{
	switch (Result)
	{
	case EWEBuildResult::Success:          return LOCTEXT("ResOk", "Erledigt");
	case EWEBuildResult::InvalidFurniture: return LOCTEXT("ResInvalid", "Nicht möglich");
	case EWEBuildResult::OutsideArea:      return LOCTEXT("ResOutside", "Nur innerhalb der Werkstatt");
	case EWEBuildResult::Blocked:          return LOCTEXT("ResBlocked", "Kein Platz");
	case EWEBuildResult::NotEnoughMoney:   return LOCTEXT("ResMoney", "Nicht genug Geld auf dem Firmenkonto");
	case EWEBuildResult::NotMovable:       return LOCTEXT("ResNotMovable", "Grundausstattung kann nur verschoben werden");
	case EWEBuildResult::NotEmpty:         return LOCTEXT("ResNotEmpty", "Erst leer räumen");
	case EWEBuildResult::Duplicate:        return LOCTEXT("ResDup", "Anfrage wurde bereits bearbeitet");
	case EWEBuildResult::Locked:           return LOCTEXT("ResLocked", "Noch nicht verfügbar");
	}
	return FText::GetEmpty();
}

AWEPlaceholderProp* UWEWorkshopLayoutComponent::SpawnFurniture(FName FurnitureId, const FTransform& Transform, FName ActorName)
{
	const FWEFurnitureDefinition* Definition = FindDefinition(FurnitureId);
	UWorld* World = GetWorld();
	if (!Definition || !World || !GetOwner()->HasAuthority())
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.bDeferConstruction = true;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// A stable name: container contents in saves are matched by actor name.
	Params.Name = ActorName.IsNone() ? FName(*FString::Printf(TEXT("Furniture_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Short))) : ActorName;
	Params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	// A just removed piece (destroyed, not yet garbage collected) still holds the name: free it, so a
	// loaded save gets its furniture back under the same name (container contents match by name).
	if (!ActorName.IsNone())
	{
		if (UObject* Old = StaticFindObjectFast(nullptr, World->PersistentLevel, ActorName); Old && (!IsValid(Old) || Cast<AActor>(Old)->IsActorBeingDestroyed()))
		{
			Old->Rename(*MakeUniqueObjectName(World->PersistentLevel, Old->GetClass(), TEXT("RemovedFurniture")).ToString(), nullptr,
				REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty);
		}
	}
	UClass* Class = Definition->Machine ? AWEMachine::StaticClass()
		: Definition->PropType == EWEPlaceholderPropType::Conveyor ? AWEConveyor::StaticClass()
		: Definition->bProductionStorage ? AWEProductionStorage::StaticClass()
		: Definition->bWorkbench ? AWEWorkbench::StaticClass()
		: Definition->Id == FName(TEXT("ToolWall")) ? AWEToolWall::StaticClass() : AWEPlaceholderProp::StaticClass();
	AWEPlaceholderProp* Prop = World->SpawnActor<AWEPlaceholderProp>(Class, Transform, Params);
	if (!Prop)
	{
		return nullptr;
	}
	Prop->ApplyFurniture(*Definition);
	Prop->FinishSpawning(Transform);
	return Prop;
}

EWEBuildResult UWEWorkshopLayoutComponent::PlaceFurniture(AWECharacter* By, FName FurnitureId, const FTransform& InTransform, const FGuid& RequestId, AWEPlaceholderProp** OutProp)
{
	const FWEFurnitureDefinition* Definition = FindDefinition(FurnitureId);
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!GetOwner()->HasAuthority() || !Definition || !Company || !By)
	{
		return EWEBuildResult::InvalidFurniture;
	}
	const FTransform Transform = Snap(InTransform.GetLocation(), InTransform.Rotator().Yaw);
	if (FVector::Dist2D(By->GetActorLocation(), Transform.GetLocation()) > WEBuild::MaxPlaceDistance)
	{
		return EWEBuildResult::OutsideArea;
	}
	FText Reason = GetLockReason(*Definition);
	if (!Reason.IsEmpty())
	{
		Notify(By, Reason);
		return EWEBuildResult::Locked;
	}
	if (!CanPlaceAt(Definition->Size, Transform, nullptr, Reason))
	{
		Notify(By, Reason);
		return Reason.EqualTo(ResultText(EWEBuildResult::OutsideArea)) ? EWEBuildResult::OutsideArea : EWEBuildResult::Blocked;
	}
	if (!Company->CanAfford(Definition->Cost))
	{
		Notify(By, ResultText(EWEBuildResult::NotEnoughMoney));
		return EWEBuildResult::NotEnoughMoney;
	}
	if (!Company->RegisterRequest(RequestId))
	{
		return EWEBuildResult::Duplicate;
	}
	const APlayerState* PS = By->GetPlayerState();
	if (!Company->TryWithdraw(Definition->Cost, FString::Printf(TEXT("Einrichtung: %s"), *Definition->DisplayName.ToString()), PS ? PS->GetPlayerName() : FString()))
	{
		return EWEBuildResult::NotEnoughMoney;
	}
	AWEPlaceholderProp* Prop = SpawnFurniture(FurnitureId, Transform);
	if (!Prop)
	{
		Company->Deposit(Definition->Cost, TEXT("Rückbuchung Einrichtung"), PS ? PS->GetPlayerName() : FString());
		return EWEBuildResult::InvalidFurniture;
	}
	if (OutProp)
	{
		*OutProp = Prop;
	}
	Notify(By, FText::Format(LOCTEXT("Placed", "{0} aufgestellt (−{1})"), Definition->DisplayName, FText::FromString(Definition->Cost.ToDisplayString())));
	UE_LOG(LogWENet, Log, TEXT("%s placed %s at %s for %s"), *By->GetName(), *FurnitureId.ToString(), *Transform.GetLocation().ToCompactString(), *Definition->Cost.ToDisplayString());
	return EWEBuildResult::Success;
}

EWEBuildResult UWEWorkshopLayoutComponent::MoveFurniture(AWECharacter* By, AWEPlaceholderProp* Prop, const FTransform& InTransform)
{
	if (!GetOwner()->HasAuthority() || !IsValid(Prop) || !By)
	{
		return EWEBuildResult::InvalidFurniture;
	}
	FText Reason;
	if (!CanPickUpFurniture(Prop, Reason))
	{
		Notify(By, Reason);
		return EWEBuildResult::NotEmpty;
	}
	const FTransform Transform = Snap(InTransform.GetLocation(), InTransform.Rotator().Yaw);
	if (FVector::Dist2D(By->GetActorLocation(), Transform.GetLocation()) > WEBuild::MaxPlaceDistance)
	{
		return EWEBuildResult::OutsideArea;
	}
	if (!CanPlaceAt(Prop->GetSize(), Transform, Prop, Reason))
	{
		Notify(By, Reason);
		return EWEBuildResult::Blocked;
	}
	Prop->SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
	Prop->ForceNetUpdate();
	Notify(By, FText::Format(LOCTEXT("Moved", "{0} verschoben"), Prop->GetFurnitureName()));
	UE_LOG(LogWENet, Log, TEXT("%s moved %s to %s"), *By->GetName(), *Prop->GetName(), *Transform.GetLocation().ToCompactString());
	return EWEBuildResult::Success;
}

EWEBuildResult UWEWorkshopLayoutComponent::RemoveFurniture(AWECharacter* By, AWEPlaceholderProp* Prop, const FGuid& RequestId)
{
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!GetOwner()->HasAuthority() || !IsValid(Prop) || !By || !Company)
	{
		return EWEBuildResult::InvalidFurniture;
	}
	const FWEFurnitureDefinition* Definition = FindDefinition(Prop->GetFurnitureId());
	if (!Definition)
	{
		Notify(By, ResultText(EWEBuildResult::NotMovable));
		return EWEBuildResult::NotMovable;
	}
	FText Reason;
	const AWEMachine* Machine = Cast<AWEMachine>(Prop);
	if ((Prop->GetStorage() && Prop->GetStorage()->GetAllSlots().Num() > 0) || (Machine && Machine->GetOutputStorage()->GetAllSlots().Num() > 0)
		|| !CanPickUpFurniture(Prop, Reason))
	{
		Notify(By, Reason.IsEmpty() ? ResultText(EWEBuildResult::NotEmpty) : Reason);
		return EWEBuildResult::NotEmpty;
	}
	if (!Company->RegisterRequest(RequestId))
	{
		return EWEBuildResult::Duplicate;
	}
	const FWEMoney Refund = FWEMoney::FromCents(FMath::RoundToInt64(Definition->Cost.GetCents() * WEBuild::RemoveRefundFactor));
	const APlayerState* PS = By->GetPlayerState();
	Company->Deposit(Refund, FString::Printf(TEXT("Einrichtung verkauft: %s"), *Definition->DisplayName.ToString()), PS ? PS->GetPlayerName() : FString());
	Notify(By, FText::Format(LOCTEXT("Removed", "{0} abgebaut (+{1})"), Definition->DisplayName, FText::FromString(Refund.ToDisplayString())));
	UE_LOG(LogWENet, Log, TEXT("%s removed %s (+%s)"), *By->GetName(), *Prop->GetName(), *Refund.ToDisplayString());
	Prop->Destroy();
	return EWEBuildResult::Success;
}

#undef LOCTEXT_NAMESPACE
