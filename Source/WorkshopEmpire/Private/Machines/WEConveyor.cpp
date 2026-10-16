#include "Machines/WEConveyor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Framework/WEPlayerController.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Machines/WEMachine.h"
#include "Machines/WEPowerGridComponent.h"
#include "Machines/WEProductionStorage.h"
#include "Net/UnrealNetwork.h"
#include "Player/WECharacter.h"
#include "TimerManager.h"
#include "WELog.h"
#include "World/WEPlaceholderVisuals.h"

#define LOCTEXT_NAMESPACE "WEConveyor"

AWEConveyor::AWEConveyor()
{
	PropType = EWEPlaceholderPropType::Conveyor;
	Size = FVector(100.f, 50.f, 45.f);
	Tint = FLinearColor(0.18f, 0.19f, 0.21f);
	bHasStorage = false;
}

void AWEConveyor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWEConveyor, Items);
	DOREPLIFETIME(AWEConveyor, bOn);
}

void AWEConveyor::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// Drawn items: a small pool of mesh components, moved locally between server updates.
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (int32 i = 0; i < MaxVisibleItems; ++i)
	{
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("BeltItem%d"), i));
		Mesh->SetupAttachment(RootComponent);
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetStaticMesh(Cube);
		Mesh->SetVisibility(false);
		Mesh->RegisterComponent();
		ItemMeshes.Add(Mesh);
	}
	GetWorld()->GetTimerManager().SetTimer(VisualTimer, this, &AWEConveyor::UpdateItemVisuals, 0.05f, true);
	OnRep_Items();
}

void AWEConveyor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(VisualTimer);
	}
	Super::EndPlay(EndPlayReason);
}

// --- Geometry ------------------------------------------------------------------------------------

FVector AWEConveyor::GetInputPortPoint() const
{
	return GetActorTransform().TransformPosition(FVector(-GetLength() * 0.5f - PortReach, 0.f, GetBeltHeight() * 0.6f));
}

FVector AWEConveyor::GetOutputPortPoint() const
{
	return GetActorTransform().TransformPosition(FVector(GetLength() * 0.5f + PortReach, 0.f, GetBeltHeight() * 0.6f));
}

float AWEConveyor::ProjectToBelt(const FVector& WorldPoint) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldPoint);
	return FMath::Clamp(Local.X + GetLength() * 0.5f, 0.f, GetLength());
}

bool AWEConveyor::CanAcceptAt(float Position) const
{
	if (Position < 0.f || Position > GetLength())
	{
		return false;
	}
	for (const FWEConveyorItem& Entry : Items)
	{
		if (FMath::Abs(Entry.Position - Position) < ItemSpacing * 0.9f)
		{
			return false;
		}
	}
	return true;
}

AWEPlaceholderProp* AWEConveyor::FindLineDestination() const
{
	const AWEConveyor* Belt = this;
	TSet<const AWEConveyor*> Visited;
	for (int32 Step = 0; Belt && Step < 32; ++Step)
	{
		Visited.Add(Belt);
		AWEPlaceholderProp* Next = UWEPowerGridComponent::FindPortActor(GetWorld(), Belt->GetOutputPortPoint(), Belt);
		const AWEConveyor* NextBelt = Cast<AWEConveyor>(Next);
		if (!NextBelt)
		{
			return Next;
		}
		if (Visited.Contains(NextBelt))
		{
			return nullptr; // a loop
		}
		Belt = NextBelt;
	}
	return nullptr;
}

int32 AWEConveyor::CountUnits(const UWEItemDefinition* Definition) const
{
	int32 Count = 0;
	for (const FWEConveyorItem& Entry : Items)
	{
		Count += !Definition || Entry.Item.Definition == Definition ? Entry.Item.Quantity : 0;
	}
	return Count;
}

// --- Server ----------------------------------------------------------------------------------------

bool AWEConveyor::AddItemAt(const FWEItemInstance& Item, float Position)
{
	if (!HasAuthority() || !Item.IsValid() || !CanAcceptAt(Position))
	{
		return false;
	}
	// Kept sorted front first (largest position first).
	int32 Index = 0;
	while (Index < Items.Num() && Items[Index].Position > Position)
	{
		++Index;
	}
	FWEConveyorItem Entry;
	Entry.Item = Item;
	Entry.Position = Position;
	Items.Insert(Entry, Index);
	ItemsChanged();
	return true;
}

bool AWEConveyor::SetPower(bool bInOn)
{
	if (!HasAuthority())
	{
		return false;
	}
	bOn = bInOn;
	ItemsChanged();
	return true;
}

void AWEConveyor::StepFlow(float DeltaSeconds)
{
	if (!HasAuthority() || !bOn)
	{
		return;
	}
	bool bChanged = false;
	// 1) Move, front first; every item keeps its distance to the one ahead.
	float Limit = GetLength();
	for (FWEConveyorItem& Entry : Items)
	{
		const float NewPosition = FMath::Max(Entry.Position, FMath::Min(Entry.Position + SpeedCmPerSecond * DeltaSeconds, Limit));
		bChanged |= !FMath::IsNearlyEqual(NewPosition, Entry.Position);
		Entry.Position = NewPosition;
		Limit = Entry.Position - ItemSpacing;
	}
	// 2) The front item leaves at the end if someone takes it.
	if (Items.Num() > 0 && Items[0].Position >= GetLength() - KINDA_SMALL_NUMBER && HandOver(Items[0].Item))
	{
		Items.RemoveAt(0);
		bChanged = true;
	}
	// 3) Room at the start: take the next product from the machine / store behind.
	if ((Items.Num() == 0 || Items.Last().Position >= ItemSpacing) && PullFromSource())
	{
		bChanged = true;
	}
	if (bChanged)
	{
		ItemsChanged();
	}
}

bool AWEConveyor::HandOver(const FWEItemInstance& Item)
{
	const FVector Port = GetOutputPortPoint();
	AWEPlaceholderProp* Target = UWEPowerGridComponent::FindPortActor(GetWorld(), Port, this);
	if (AWEConveyor* Next = Cast<AWEConveyor>(Target))
	{
		return Next->AddItemAt(Item, Next->ProjectToBelt(Port));
	}
	if (AWEMachine* Machine = Cast<AWEMachine>(Target))
	{
		return Machine->AcceptFromBelt(Item);
	}
	if (Target && Target->HasStorage() && Target->GetStorage()->GetAddableQuantity(Item) >= Item.Quantity)
	{
		int32 Added = 0;
		Target->GetStorage()->TryAddItem(Item, Added);
		return Added == Item.Quantity;
	}
	return false;
}

bool AWEConveyor::PullFromSource()
{
	AWEPlaceholderProp* Source = UWEPowerGridComponent::FindPortActor(GetWorld(), GetInputPortPoint(), this);
	FWEItemInstance Taken;
	if (AWEMachine* Machine = Cast<AWEMachine>(Source))
	{
		if (Machine->TakeOutputForBelt(Taken))
		{
			return AddItemAt(Taken, 0.f);
		}
	}
	else if (AWEProductionStorage* Store = Cast<AWEProductionStorage>(Source))
	{
		if (Store->TakeForBelt(Taken, FindLineDestination()))
		{
			return AddItemAt(Taken, 0.f);
		}
	}
	return false;
}

bool AWEConveyor::CanPutOnBelt(const AWEPickupItem* Item, const AWECharacter* By, FText& OutReason) const
{
	if (!Item || !Item->GetItem().IsValid())
	{
		return false;
	}
	if (Item->GetItem().IsCustomerItem())
	{
		OutReason = LOCTEXT("Customer", "Kundengerät – nicht aufs Band");
		return false;
	}
	for (float Position = 0.f; Position <= GetLength(); Position += ItemSpacing * 0.5f)
	{
		if (CanAcceptAt(Position))
		{
			return true;
		}
	}
	OutReason = LOCTEXT("BeltFull", "Band voll");
	return false;
}

bool AWEConveyor::PutOnBelt(AWEPickupItem* Item, const AWECharacter* By)
{
	FText Reason;
	if (!HasAuthority() || !CanPutOnBelt(Item, By, Reason))
	{
		return false;
	}
	// The free spot closest to where the player stands.
	const float Wanted = By ? ProjectToBelt(By->GetActorLocation()) : 0.f;
	float Best = -1.f;
	for (float Position = 0.f; Position <= GetLength(); Position += ItemSpacing * 0.25f)
	{
		if (CanAcceptAt(Position) && (Best < 0.f || FMath::Abs(Position - Wanted) < FMath::Abs(Best - Wanted)))
		{
			Best = Position;
		}
	}
	const FWEItemInstance Instance = Item->GetItem();
	if (Best < 0.f || !AddItemAt(Instance, Best))
	{
		return false;
	}
	if (AWECharacter* Holder = Item->GetHolder())
	{
		Holder->ReleaseHeldItem();
	}
	UE_LOG(LogWENet, Log, TEXT("%s: %s put on the belt at %.0f cm"), *GetName(), *Item->GetDisplayName().ToString(), Best);
	Item->Destroy();
	return true;
}

bool AWEConveyor::TakeFrontItem(AWECharacter* Character)
{
	UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr;
	if (!HasAuthority() || !Inventory || Items.Num() == 0 || Character->GetHeldItem())
	{
		return false;
	}
	FWEItemInstance& Front = Items[0].Item;
	if (Front.Definition->bFitsInInventory)
	{
		int32 Added = 0;
		Inventory->TryAddItem(Front, Added);
		if (Added <= 0)
		{
			if (AWEPlayerController* PC = Cast<AWEPlayerController>(Character->GetController()))
			{
				PC->ClientShowMessage(LOCTEXT("InvFull", "Inventar voll"));
			}
			return false;
		}
		Front.Quantity -= Added;
		if (Front.Quantity <= 0)
		{
			Items.RemoveAt(0);
		}
		ItemsChanged();
		return true;
	}
	// Bulky: into the hands.
	const FVector At = GetActorTransform().TransformPosition(FVector(Items[0].Position - GetLength() * 0.5f, 0.f, GetBeltHeight() + 30.f));
	AWEPickupItem* Spawned = AWEPickupItem::SpawnWorldItem(GetWorld(), Front, FTransform(GetActorRotation(), At));
	if (!Spawned || !Character->TryPickUpItem(Spawned))
	{
		if (Spawned)
		{
			Spawned->Destroy();
		}
		return false;
	}
	Items.RemoveAt(0);
	ItemsChanged();
	return true;
}

void AWEConveyor::RestoreState(bool bInOn, const TArray<FWEConveyorItem>& InItems)
{
	if (!HasAuthority())
	{
		return;
	}
	bOn = bInOn;
	Items = InItems;
	Items.Sort([](const FWEConveyorItem& A, const FWEConveyorItem& B) { return A.Position > B.Position; });
	ItemsChanged();
}

void AWEConveyor::ItemsChanged()
{
	LastUpdateTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	ForceNetUpdate();
	if (GetNetMode() != NM_DedicatedServer)
	{
		OnRep_Items();
	}
}

// --- Visuals (local) ---------------------------------------------------------------------------------

void AWEConveyor::OnRep_Items()
{
	LastUpdateTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	for (int32 i = 0; i < ItemMeshes.Num(); ++i)
	{
		UStaticMeshComponent* Mesh = ItemMeshes[i];
		const UWEItemDefinition* Def = Items.IsValidIndex(i) ? Items[i].Item.Definition.Get() : nullptr;
		if (!Mesh)
		{
			continue;
		}
		Mesh->SetVisibility(Def != nullptr);
		if (!Def)
		{
			continue;
		}
		if (Def->WorldMesh && Mesh->GetStaticMesh() != Def->WorldMesh)
		{
			Mesh->SetStaticMesh(Def->WorldMesh);
		}
		// Shown at most 40 cm big, so bulky devices still fit on the belt picture.
		FVector Scale = Def->WorldScale;
		const float Largest = Scale.GetMax();
		Scale *= Largest > 0.4f ? 0.4f / Largest : 1.f;
		Mesh->SetRelativeScale3D(Scale);
		if (Mesh->ComponentTags.Num() == 0 || Mesh->ComponentTags[0] != Def->ItemId)
		{
			Mesh->ComponentTags = { Def->ItemId };
			WEPlaceholder::ApplyTint(Mesh, PlaceholderMaterial, Def->Tint);
		}
	}
	UpdateItemVisuals();
}

void AWEConveyor::UpdateItemVisuals()
{
	if (ItemMeshes.Num() == 0)
	{
		return;
	}
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	// Continue the motion since the last update, but never further than one server step (a blocked belt stops).
	const float Elapsed = bOn ? FMath::Clamp(Now - LastUpdateTime, 0.f, UWEPowerGridComponent::StepSeconds) : 0.f;
	float Limit = GetLength();
	for (int32 i = 0; i < Items.Num() && i < ItemMeshes.Num(); ++i)
	{
		const float Position = FMath::Max(Items[i].Position, FMath::Min(Items[i].Position + SpeedCmPerSecond * Elapsed, Limit));
		Limit = Position - ItemSpacing;
		if (UStaticMeshComponent* Mesh = ItemMeshes[i])
		{
			const float HalfHeight = Mesh->GetRelativeScale3D().Z * 50.f;
			Mesh->SetRelativeLocation(FVector(Position - GetLength() * 0.5f, 0.f, GetBeltHeight() + HalfHeight));
		}
	}
}

// --- Interaction ---------------------------------------------------------------------------------------

FText AWEConveyor::GetInteractionText(const AWECharacter* Interactor) const
{
	return Items.Num() > 0 ? FText::Format(LOCTEXT("TakeFront", "Vom Band nehmen ({0})"), Items[0].Item.GetDisplayName()) : LOCTEXT("Belt", "Förderband");
}

bool AWEConveyor::CanInteract(const AWECharacter* Interactor, FText& OutReason) const
{
	return Interactor && Items.Num() > 0 && !Interactor->GetHeldItem();
}

void AWEConveyor::Interact(AWECharacter* Interactor)
{
	TakeFrontItem(Interactor);
}

bool AWEConveyor::GetAlternativeInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const
{
	if (!Interactor)
	{
		return false;
	}
	OutText = bOn ? LOCTEXT("Stop", "Band anhalten") : LOCTEXT("Start", "Band starten");
	return true;
}

void AWEConveyor::InteractAlternative(AWECharacter* Interactor)
{
	SetPower(!bOn);
}

bool AWEConveyor::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	OutInfo.Title = GetFurnitureName();
	OutInfo.Description = LOCTEXT("Desc", "Transportiert Material von hinten nach vorn: nimmt Produkte aus der Maschine oder dem Materiallager dahinter und gibt sie an das ab, was davor steht. [E] Gegenstand auflegen bzw. herunternehmen, [Q] an/aus.");
	OutInfo.Details.Reset();
	OutInfo.Details.Add(bOn ? LOCTEXT("On", "Läuft (0,15 kW)") : LOCTEXT("Off", "Steht"));
	OutInfo.Details.Add(FText::Format(LOCTEXT("Count", "Auf dem Band: {0} Stapel, {1} Stück"), FText::AsNumber(Items.Num()), FText::AsNumber(CountUnits())));
	const AWEPlaceholderProp* From = UWEPowerGridComponent::FindPortActor(GetWorld(), GetInputPortPoint(), this);
	const AWEPlaceholderProp* To = UWEPowerGridComponent::FindPortActor(GetWorld(), GetOutputPortPoint(), this);
	OutInfo.Details.Add(FText::Format(LOCTEXT("From", "Nimmt von: {0}"), From ? From->GetFurnitureName() : LOCTEXT("Nothing", "–")));
	OutInfo.Details.Add(FText::Format(LOCTEXT("To", "Gibt ab an: {0}"), To ? To->GetFurnitureName() : LOCTEXT("NothingTo", "– (Band stoppt am Ende)")));
	return true;
}

#undef LOCTEXT_NAMESPACE
