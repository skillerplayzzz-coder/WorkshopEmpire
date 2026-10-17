#include "Items/WEPickupItem.h"
#include "WELog.h"
#include "Framework/WEPlayerController.h"
#include "Items/WELootTable.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "World/WESurface.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "Player/WECharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "World/WEPlaceholderVisuals.h"
#include "Workshop/WEDisassemblyDefinition.h"
#include "Workshop/WEWorkbench.h"

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
		Mesh->SetStaticMesh(CubeMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlaceholderMat(WEPlaceholder::MaterialPath);
	PlaceholderMaterial = PlaceholderMat.Object;
}

AWEPickupItem* AWEPickupItem::SpawnWorldItem(UWorld* World, const FWEItemInstance& InItem, const FTransform& Transform)
{
	if (!World || !InItem.IsValid())
	{
		return nullptr;
	}
	AWEPickupItem* Actor = World->SpawnActorDeferred<AWEPickupItem>(AWEPickupItem::StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Actor)
	{
		Actor->Item = InItem;
		Actor->FinishSpawning(Transform);
	}
	return Actor;
}

void AWEPickupItem::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyDefinitionVisuals();
}

void AWEPickupItem::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority() && Item.Definition && !Item.InstanceId.IsValid())
	{
		Item.InstanceId = FGuid::NewGuid();
	}
	ApplyDefinitionVisuals();
}

void AWEPickupItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWEPickupItem, Item);
	DOREPLIFETIME(AWEPickupItem, Holder);
	DOREPLIFETIME(AWEPickupItem, PlacedOn);
}

void AWEPickupItem::ApplyDefinitionVisuals()
{
	const UWEItemDefinition* Def = Item.Definition;
	if (!Def)
	{
		return;
	}
	if (Def->WorldMesh && Mesh->GetStaticMesh() != Def->WorldMesh)
	{
		Mesh->SetStaticMesh(Def->WorldMesh);
	}
	Mesh->SetRelativeScale3D(Def->WorldScale);
	Mesh->SetMassOverrideInKg(NAME_None, FMath::Max(0.05f, Item.GetTotalWeightKg()), true);
	// A modelled mesh brings its own textures; the basic placeholder shapes get a surface or the tint.
	const bool bModelled = Def->WorldMesh && !Def->WorldMesh->GetPathName().StartsWith(TEXT("/Engine/BasicShapes"));
	if (bModelled)
	{
		for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
		{
			Mesh->SetMaterial(i, nullptr); // back to the mesh's own materials
		}
	}
	else if (Def->Surface != EWESurface::Placeholder)
	{
		WESurface::Apply(Mesh, Def->Surface, FLinearColor::LerpUsingHSV(Def->Tint, FLinearColor::White, 0.5f), 0.f);
	}
	else
	{
		WEPlaceholder::ApplyTint(Mesh, PlaceholderMaterial, Def->Tint);
	}
}

void AWEPickupItem::SetItem(const FWEItemInstance& NewItem)
{
	Item = NewItem;
	ApplyDefinitionVisuals();
	ForceNetUpdate();
}

void AWEPickupItem::OnRep_Item()
{
	ApplyDefinitionVisuals();
}

FText AWEPickupItem::GetDisplayName() const
{
	if (Item.Quantity > 1)
	{
		return FText::Format(LOCTEXT("StackName", "{0} ({1})"), Item.GetDisplayName(), FText::AsNumber(Item.Quantity));
	}
	return Item.GetDisplayName();
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

bool AWEPickupItem::GoesIntoInventory() const
{
	return Item.Definition && Item.Definition->bFitsInInventory;
}

// --- Interaction -------------------------------------------------------------------------------

FText AWEPickupItem::GetInteractionText(const AWECharacter* Interactor) const
{
	if (PlacedOn)
	{
		return PlacedOn->GetNextStepPrompt();
	}
	return GoesIntoInventory() ? LOCTEXT("PickUp", "Aufheben") : LOCTEXT("Carry", "Tragen");
}

bool AWEPickupItem::CanInteract(const AWECharacter* Interactor, FText& OutReason) const
{
	if (!Interactor || !Item.IsValid())
	{
		return false;
	}
	if (PlacedOn)
	{
		return PlacedOn->CanStartStep(Interactor, OutReason);
	}
	if (GoesIntoInventory())
	{
		if (Holder)
		{
			OutReason = LOCTEXT("HeldByOther", "Wird gerade getragen");
			return false;
		}
		const UWEInventoryComponent* Inventory = Interactor->GetPlayerInventory();
		if (!Inventory)
		{
			return false;
		}
		if (Inventory->GetAddableQuantity(Item) <= 0)
		{
			const bool bWeight = Item.GetUnitWeightKg() > Inventory->GetMaxWeightKg() - Inventory->GetTotalWeightKg();
			OutReason = bWeight ? LOCTEXT("InvTooHeavy", "Inventar zu schwer") : LOCTEXT("InvFull", "Inventar voll");
			return false;
		}
		return true;
	}
	return CanBePickedUpBy(Interactor, OutReason);
}

void AWEPickupItem::Interact(AWECharacter* Interactor)
{
	if (!Interactor)
	{
		return;
	}
	if (PlacedOn)
	{
		PlacedOn->TryStartStep(Interactor);
		return;
	}
	if (GoesIntoInventory())
	{
		Interactor->TryStoreWorldItem(this);
	}
	else
	{
		Interactor->TryPickUpItem(this);
	}
}

bool AWEPickupItem::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	const UWEItemDefinition* Def = Item.Definition;
	if (!Def)
	{
		return false;
	}
	OutInfo.Title = GetDisplayName();
	OutInfo.Description = Def->Description;
	OutInfo.Details.Reset();
	OutInfo.Details.Add(FText::Format(LOCTEXT("CategoryLine", "Kategorie: {0}"), WEItems::GetCategoryText(Def->Category)));
	OutInfo.Details.Add(FText::Format(LOCTEXT("ConditionLine", "Zustand: {0} (Qualität {1} %)"), WEItems::GetConditionText(Item.Condition), FText::AsNumber(Item.Quality)));
	OutInfo.Details.Add(FText::Format(LOCTEXT("WeightLine", "Gewicht: {0}"), FormatWeight(Item.GetTotalWeightKg())));
	OutInfo.Details.Add(FText::Format(LOCTEXT("ValueLine", "Schätzwert: {0}"), FText::FromString(Item.GetEstimatedValue().ToDisplayString())));
	if (Def->CanBeDisassembled())
	{
		OutInfo.Details.Add(FText::Format(LOCTEXT("DisassemblyLine", "Zerlegbar: Schritt {0} von {1} erledigt"),
			FText::AsNumber(Item.DisassemblyStep), FText::AsNumber(Def->Disassembly->NumSteps())));
	}
	if (Def->Rarity >= EWEItemRarity::Uncommon)
	{
		const TCHAR* Names[] = { TEXT("Gewöhnlich"), TEXT("Ungewöhnlich"), TEXT("Selten"), TEXT("Sehr selten") };
		OutInfo.Details.Add(FText::Format(LOCTEXT("RarityLine", "Seltenheit: {0}"), FText::FromString(Names[FMath::Clamp(static_cast<int32>(Def->Rarity), 0, 3)])));
	}
	if (Def->IsOpenable())
	{
		OutInfo.Details.Add(LOCTEXT("OpenableLine", "Inhalt unbekannt – mit [Q] öffnen"));
	}
	const FText RepairStatus = AWEWorkbench::GetRepairStatusText(Item);
	if (!RepairStatus.IsEmpty())
	{
		OutInfo.Details.Add(FText::Format(LOCTEXT("RepairStatusLine", "Reparatur: {0}"), RepairStatus));
	}
	if (Item.IsCustomerItem())
	{
		OutInfo.Details.Add(LOCTEXT("CustomerLine", "Kundengerät (Reparaturauftrag) – nicht verkaufen oder zerlegen"));
	}
	if (Def->ToolType != EWEToolType::None)
	{
		OutInfo.Details.Add(FText::Format(LOCTEXT("ToolLine", "Werkzeug: {0} (Tempo ×{1})"),
			WEItems::GetToolText(Def->ToolType), FText::AsNumber(Def->ToolSpeedMultiplier)));
	}
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
	if (!Character || !IsValid(this) || !Item.IsValid())
	{
		OutReason = FText::GetEmpty();
		return false;
	}
	if (Holder)
	{
		OutReason = Holder == Character
			? LOCTEXT("AlreadyHolding", "Bereits in der Hand")
			: LOCTEXT("HeldByOtherCarry", "Wird gerade getragen");
		return false;
	}
	if (PlacedOn)
	{
		OutReason = LOCTEXT("OnBench", "Liegt auf der Werkbank");
		return false;
	}
	if (Character->GetHeldItem())
	{
		OutReason = LOCTEXT("HandsFull", "Hände sind voll");
		return false;
	}
	if (GetWeightKg() > Character->GetMaxCarryWeightKg())
	{
		OutReason = FText::Format(LOCTEXT("TooHeavy", "Zu schwer zum Tragen ({0})"), FormatWeight(GetWeightKg()));
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
	ApplyHeldState(Holder != nullptr || PlacedOn != nullptr);
}

bool AWEPickupItem::GetSecondaryInteraction(const AWECharacter* Interactor, FText& OutText) const
{
	if (PlacedOn && Interactor && !PlacedOn->IsBusy() && !Interactor->GetHeldItem())
	{
		OutText = LOCTEXT("TakeOff", "Von der Werkbank nehmen");
		return true;
	}
	return false;
}

void AWEPickupItem::InteractSecondary(AWECharacter* Interactor)
{
	if (PlacedOn)
	{
		PlacedOn->TakeOffItem(Interactor);
	}
}

bool AWEPickupItem::GetAlternativeInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const
{
	if (!PlacedOn)
	{
		if (Item.Definition && Item.Definition->IsOpenable() && !Holder && Interactor && !Interactor->GetHeldItem())
		{
			OutText = LOCTEXT("OpenContainer", "Öffnen");
			return true;
		}
		return false;
	}
	if (PlacedOn->IsBusy())
	{
		return false; // the [E] line already says "Arbeite …" / "Wird gerade bearbeitet"
	}
	FText Reason;
	if (PlacedOn->CanStartRepairStep(Interactor, Reason))
	{
		OutText = FText::Format(LOCTEXT("RepairPrompt", "Reparieren: {0}"), PlacedOn->GetNextRepairPrompt());
		return true;
	}
	if (!Reason.IsEmpty())
	{
		OutReason = FText::Format(LOCTEXT("RepairBlocked", "Reparieren: {0}"), Reason);
	}
	return false;
}

void AWEPickupItem::InteractAlternative(AWECharacter* Interactor)
{
	if (PlacedOn)
	{
		PlacedOn->TryStartRepairStep(Interactor);
	}
	else if (Item.Definition && Item.Definition->IsOpenable())
	{
		OpenContainer(Interactor);
	}
}

TArray<FWEItemInstance> AWEPickupItem::OpenContainer(AWECharacter* By)
{
	TArray<FWEItemInstance> Found;
	const UWEItemDefinition* Def = Item.Definition;
	if (!HasAuthority() || !Def || !Def->IsOpenable() || Holder || PlacedOn || IsActorBeingDestroyed())
	{
		return Found;
	}
	// Seeded by the pallet's id: the contents are fixed when it is bought (no re-rolling by loading).
	FRandomStream Random(static_cast<int32>(GetTypeHash(Item.InstanceId)));
	Found = Def->Contents->Roll(Random);

	TArray<FString> Names;
	TArray<FString> Rare;
	const FVector Base = GetActorLocation();
	for (int32 i = 0; i < Found.Num(); ++i)
	{
		const FVector Offset((i % 3 - 1) * 45.f, (i / 3 % 3 - 1) * 45.f, 25.f + (i / 9) * 35.f);
		SpawnWorldItem(GetWorld(), Found[i], FTransform(GetActorRotation(), Base + Offset));
		const FString Name = Found[i].Quantity > 1 ? FString::Printf(TEXT("%d× %s"), Found[i].Quantity, *Found[i].GetDisplayName().ToString())
			: Found[i].GetDisplayName().ToString();
		Names.Add(Name);
		if (Found[i].Definition->Rarity >= EWEItemRarity::Rare)
		{
			Rare.Add(Found[i].GetDisplayName().ToString());
		}
	}
	UE_LOG(LogWENet, Log, TEXT("%s opened %s: %s"), *GetNameSafe(By), *GetDisplayName().ToString(), *FString::Join(Names, TEXT(", ")));
	if (AWEPlayerController* PC = By ? Cast<AWEPlayerController>(By->GetController()) : nullptr)
	{
		PC->ClientShowMessage(Rare.Num() > 0
			? FText::Format(LOCTEXT("OpenedRare", "Seltener Fund: {0}!  ({1})"), FText::FromString(FString::Join(Rare, TEXT(", "))), FText::FromString(FString::Join(Names, TEXT(", "))))
			: Names.Num() > 0 ? FText::Format(LOCTEXT("Opened", "Geöffnet: {0}"), FText::FromString(FString::Join(Names, TEXT(", "))))
			: LOCTEXT("OpenedEmpty", "Leer – Pech gehabt"));
	}
	Destroy();
	return Found;
}

void AWEPickupItem::HandlePlacedOnWorkbench(AWEWorkbench* Bench, USceneComponent* AttachTo)
{
	check(HasAuthority());
	Holder = nullptr;
	PlacedOn = Bench;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	ApplyHeldState(true);
	if (AttachTo)
	{
		AttachToComponent(AttachTo, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		// Rest the item on the surface: lift by half its height.
		const float HalfHeight = Mesh->Bounds.BoxExtent.Z;
		SetActorRelativeLocation(FVector(0.f, 0.f, HalfHeight + 0.5f));
	}
	ForceNetUpdate();
}

void AWEPickupItem::HandleRemovedFromWorkbench()
{
	check(HasAuthority());
	PlacedOn = nullptr;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	ApplyHeldState(false);
	ForceNetUpdate();
}

void AWEPickupItem::OnRep_AttachmentReplication()
{
	// Physics must be off before the client attaches the root to the holder's hand,
	// otherwise the simulated body would fight the attachment.
	if (GetAttachmentReplication().AttachParent)
	{
		Mesh->SetSimulatePhysics(false); // collision mode is set by OnRep_Holder (hands vs. workbench)
	}
	Super::OnRep_AttachmentReplication();
}

void AWEPickupItem::ApplyHeldState(bool bHeld)
{
	if (bHeld)
	{
		Mesh->SetSimulatePhysics(false);
		// In the hands: no collision at all. On a workbench: still traceable so it can be focused for [E]/[R].
		Mesh->SetCollisionEnabled(PlacedOn ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
	else
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetSimulatePhysics(true);
		Mesh->WakeAllRigidBodies();
	}
}

#undef LOCTEXT_NAMESPACE
