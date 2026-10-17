#include "Machines/WEMachine.h"
#include "Progression/WEMilestoneComponent.h"

#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Fx/WEFx.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Framework/WEPlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Machines/WEMachineDefinition.h"
#include "Machines/WEPowerGridComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/WECharacter.h"
#include "WELog.h"
#include "Workshop/WEFurnitureDefinition.h"

#define LOCTEXT_NAMESPACE "WEMachine"

namespace WEMachineConst
{
	constexpr float WornEfficiency = 0.7f;
}

AWEMachine::AWEMachine()
{
	PropType = EWEPlaceholderPropType::Machine;
	Size = FVector(110.f, 110.f, 140.f);
	Tint = FLinearColor(0.55f, 0.45f, 0.12f);
	bHasStorage = true;
	StorageSlots = 8;
	OutputStorage = CreateDefaultSubobject<UWEInventoryComponent>(TEXT("Output"));
	Random.Initialize(2468);
}

void AWEMachine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWEMachine, MachineDefinition);
	DOREPLIFETIME(AWEMachine, Status);
	DOREPLIFETIME(AWEMachine, Processing);
}

void AWEMachine::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		Random.Initialize(FMath::Rand());
	}
	UpdateFeedback();
}

void AWEMachine::ApplyFurnitureExtras(const FWEFurnitureDefinition& Definition)
{
	SetMachineDefinition(Definition.Machine);
}

void AWEMachine::SetMachineDefinition(UWEMachineDefinition* InDefinition)
{
	MachineDefinition = InDefinition;
	if (!MachineDefinition)
	{
		return;
	}
	bHasStorage = true;
	StorageName = FText::Format(LOCTEXT("InputName", "{0} – Eingang"), MachineDefinition->DisplayName);
	StorageSlots = MachineDefinition->InputSlots;
	StorageMaxWeightKg = MachineDefinition->InputMaxWeightKg;
	RebuildVisuals();
}

void AWEMachine::RebuildVisuals()
{
	Super::RebuildVisuals();
	if (MachineDefinition)
	{
		GetStorage()->SetAcceptsBulky(MachineDefinition->bAcceptsBulky);
		OutputStorage->Configure(FText::Format(LOCTEXT("OutputName", "{0} – Ausgabe"), MachineDefinition->DisplayName),
			MachineDefinition->OutputSlots, MachineDefinition->OutputMaxWeightKg);
		// Bulky products (bales) wait in the output when a conveyor carries them away (Phase 10).
		OutputStorage->SetAcceptsBulky(true);
	}
}

float AWEMachine::GetServerTime() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.f;
}

// --- State ---------------------------------------------------------------------------------------

FText AWEMachine::GetStateText(EWEMachineState State)
{
	switch (State)
	{
	case EWEMachineState::Off:              return LOCTEXT("StOff", "Aus");
	case EWEMachineState::Idle:             return LOCTEXT("StIdle", "Bereit – wartet auf Material");
	case EWEMachineState::Running:          return LOCTEXT("StRunning", "Läuft");
	case EWEMachineState::OutputFull:       return LOCTEXT("StFull", "Ausgabe voll – Produkte entnehmen");
	case EWEMachineState::NoPower:          return LOCTEXT("StNoPower", "Stromanschluss überlastet – wartet");
	case EWEMachineState::NeedsMaintenance: return LOCTEXT("StMaint", "Wartung nötig");
	}
	return FText::GetEmpty();
}

FText AWEMachine::GetStatusText() const
{
	const FText State = GetStateText(Status.State);
	const bool bHasRecipe = MachineDefinition && MachineDefinition->Recipes.IsValidIndex(Status.RecipeIndex);
	if (bHasRecipe && (Status.State == EWEMachineState::Running || Processing.Num() > 0))
	{
		return FText::Format(LOCTEXT("StatusRecipe", "{0} – {1}"), State, MachineDefinition->Recipes[Status.RecipeIndex].DisplayName);
	}
	return State;
}

void AWEMachine::SetState(EWEMachineState NewState)
{
	if (Status.State == NewState)
	{
		return;
	}
	Status.State = NewState;
	Status.ProgressAtStart = Progress;
	++Status.EventCounter;
	ForceNetUpdate();
	OnRep_Status();
}

void AWEMachine::OnRep_Status()
{
	if (Status.EventCounter != LastHandledEvent)
	{
		LastHandledEvent = Status.EventCounter;
		LocalSegmentStart = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	}
	UpdateFeedback();
}

void AWEMachine::UpdateFeedback()
{
	UWorld* World = GetWorld();
	if (!World || GetNetMode() == NM_DedicatedServer || !MachineDefinition || !HasActorBegunPlay())
	{
		return;
	}
	const bool bRunning = Status.State == EWEMachineState::Running;
	const FTransform& Xf = GetActorTransform();
	if (!bFeedbackInitialised)
	{
		// First look at the state (joined or loaded): no start sound, no old cycle effect.
		bFeedbackInitialised = true;
		LastHandledCycle = Status.CycleCounter;
	}
	else if (Status.CycleCounter != LastHandledCycle)
	{
		LastHandledCycle = Status.CycleCounter;
		const FVector At = Xf.TransformPosition(MachineDefinition->CycleFxOffset);
		AWEFx::Burst(this, MachineDefinition->CycleFx, At, Xf.TransformVector(FVector(1.f, 0.f, 0.6f)).GetSafeNormal());
		if (MachineDefinition->CycleSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, MachineDefinition->CycleSound, At);
		}
	}
	if (bRunning)
	{
		World->GetTimerManager().ClearTimer(StopFeedbackTimer);
		if (!bFeedbackRunning)
		{
			SetFeedbackRunning(true);
		}
	}
	else if (bFeedbackRunning && !World->GetTimerManager().IsTimerActive(StopFeedbackTimer))
	{
		// Between two cycles the machine is idle for a moment: only stop if it stays that way.
		World->GetTimerManager().SetTimer(StopFeedbackTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (Status.State != EWEMachineState::Running)
			{
				SetFeedbackRunning(false);
			}
		}), 0.75f, false);
	}
}

void AWEMachine::SetFeedbackRunning(bool bRunning)
{
	UWorld* World = GetWorld();
	if (!World || !MachineDefinition)
	{
		return;
	}
	bFeedbackRunning = bRunning;
	if (USoundBase* Edge = bRunning ? MachineDefinition->StartSound.Get() : MachineDefinition->StopSound.Get())
	{
		UGameplayStatics::PlaySoundAtLocation(this, Edge, GetActorLocation() + FVector(0.f, 0.f, 80.f));
	}
	if (MachineDefinition->RunningSound && !RunningAudio && bRunning)
	{
		RunningAudio = UGameplayStatics::SpawnSoundAttached(MachineDefinition->RunningSound, GetRootComponent(), NAME_None, FVector(0.f, 0.f, 80.f),
			EAttachLocation::KeepRelativeOffset, true, 1.f, 1.f, 0.f, nullptr, nullptr, false);
	}
	if (RunningAudio)
	{
		if (bRunning) { RunningAudio->FadeIn(0.4f); } else { RunningAudio->FadeOut(0.6f, 0.f); }
	}
	if (bRunning)
	{
		if (!RunningFxActor && MachineDefinition->RunningFx != EWEFxKind::None)
		{
			RunningFxActor = AWEFx::Loop(GetRootComponent(), MachineDefinition->RunningFx, MachineDefinition->RunningFxOffset);
		}
		if (!SecondFxActor && MachineDefinition->SecondRunningFx != EWEFxKind::None)
		{
			SecondFxActor = AWEFx::Loop(GetRootComponent(), MachineDefinition->SecondRunningFx, MachineDefinition->SecondRunningFxOffset);
		}
	}
	if (RunningFxActor) { RunningFxActor->SetEmitting(bRunning); }
	if (SecondFxActor) { SecondFxActor->SetEmitting(bRunning); }
	// The machine body trembles while it works (visual meshes only, not the collision).
	if (MachineDefinition->RunningShake > 0.f)
	{
		UInstancedStaticMeshComponent* Visual = FindComponentByClass<UInstancedStaticMeshComponent>();
		if (bRunning && Visual)
		{
			ShakeBase = Visual->GetRelativeLocation();
			World->GetTimerManager().SetTimer(ShakeTimer, this, &AWEMachine::ShakeTick, 1.f / 30.f, true);
		}
		else if (!bRunning)
		{
			World->GetTimerManager().ClearTimer(ShakeTimer);
			if (Visual) { Visual->SetRelativeLocation(ShakeBase); }
		}
	}
}
void AWEMachine::ShakeTick()
{
	UInstancedStaticMeshComponent* Visual = FindComponentByClass<UInstancedStaticMeshComponent>();
	if (!Visual || !MachineDefinition || !GetWorld())
	{
		return;
	}
	const float T = GetWorld()->GetTimeSeconds();
	const float A = MachineDefinition->RunningShake;
	Visual->SetRelativeLocation(ShakeBase + FVector(FMath::Sin(T * 61.f) * A, FMath::Sin(T * 47.f + 1.3f) * A, FMath::Sin(T * 83.f) * A * 0.5f));
}

void AWEMachine::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Attached effects are separate actors: take them along.
	if (RunningFxActor) { RunningFxActor->Destroy(); }
	if (SecondFxActor) { SecondFxActor->Destroy(); }
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(ShakeTimer); GetWorld()->GetTimerManager().ClearTimer(StopFeedbackTimer); }
	Super::EndPlay(EndPlayReason);
}

float AWEMachine::GetEfficiency() const
{
	return MachineDefinition && Status.Maintenance < MachineDefinition->WornThreshold ? WEMachineConst::WornEfficiency : 1.f;
}

float AWEMachine::GetProgress() const
{
	if (HasAuthority())
	{
		return Progress;
	}
	if (Status.State != EWEMachineState::Running || Status.CycleSeconds <= 0.f)
	{
		return Status.ProgressAtStart;
	}
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	return FMath::Clamp(Status.ProgressAtStart + (Now - LocalSegmentStart) / Status.CycleSeconds, 0.f, 1.f);
}

float AWEMachine::GetPowerDrawKw() const
{
	if (!MachineDefinition || !Status.bOn)
	{
		return 0.f;
	}
	return Status.State == EWEMachineState::Running ? MachineDefinition->PowerKw : MachineDefinition->IdlePowerKw;
}

float AWEMachine::GetProcessingWeightKg() const
{
	float Kg = 0.f;
	for (const FWEItemInstance& Item : Processing)
	{
		Kg += Item.GetTotalWeightKg();
	}
	return Kg;
}

// --- Simulation (server) ---------------------------------------------------------------------------

int32 AWEMachine::CountIngredient(const FWEMachineRecipe& Recipe, bool bSecond, int32& OutSlot) const
{
	OutSlot = INDEX_NONE;
	int32 Count = 0;
	for (const FWEInventorySlotData& Slot : GetStorage()->GetAllSlots())
	{
		const FWEItemInstance& Item = Slot.Item;
		if (Item.IsCustomerItem() || !Item.Definition)
		{
			continue;
		}
		const bool bMatch = bSecond ? Item.Definition == Recipe.SecondInputItem
			: Recipe.InputItem ? Item.Definition == Recipe.InputItem : Recipe.InputCategories.Contains(Item.Definition->Category);
		if (bMatch)
		{
			OutSlot = OutSlot == INDEX_NONE ? Slot.SlotIndex : OutSlot;
			Count += Item.Quantity;
		}
	}
	return Count;
}

bool AWEMachine::MatchesRecipe(const FWEMachineRecipe& Recipe, int32& OutSlot) const
{
	const int32 Count = CountIngredient(Recipe, false, OutSlot);
	if (OutSlot == INDEX_NONE || Count < (Recipe.InputItem ? Recipe.InputQuantity : 1))
	{
		return false;
	}
	int32 SecondSlot = INDEX_NONE;
	return !Recipe.SecondInputItem || CountIngredient(Recipe, true, SecondSlot) >= Recipe.SecondInputQuantity;
}

bool AWEMachine::TakeIngredient(const FWEMachineRecipe& Recipe, bool bSecond, int32 Quantity)
{
	int32 Needed = Quantity;
	while (Needed > 0)
	{
		int32 Slot = INDEX_NONE;
		if (CountIngredient(Recipe, bSecond, Slot) <= 0)
		{
			break;
		}
		const FWEItemInstance* InSlot = GetStorage()->GetItemInSlot(Slot);
		FWEItemInstance Taken;
		if (!InSlot || GetStorage()->RemoveFromSlot(Slot, FMath::Min(Needed, InSlot->Quantity), Taken) != EWEInventoryResult::Success)
		{
			break;
		}
		Needed -= Taken.Quantity;
		Processing.Add(Taken);
	}
	return Needed <= 0;
}

int32 AWEMachine::FindStartableRecipe()
{
	if (!HasAuthority() || !MachineDefinition)
	{
		return INDEX_NONE;
	}
	if (!Status.bOn)
	{
		SetState(EWEMachineState::Off);
		return INDEX_NONE;
	}
	if (Status.Maintenance == 0)
	{
		SetState(EWEMachineState::NeedsMaintenance);
		return INDEX_NONE;
	}
	if (Processing.Num() > 0)
	{
		return Status.RecipeIndex; // a paused cycle continues
	}
	const int32 FreeSlots = OutputStorage->GetNumSlots() - OutputStorage->GetAllSlots().Num();
	for (int32 i = 0; i < MachineDefinition->Recipes.Num(); ++i)
	{
		if (MachineDefinition->Recipes.IsValidIndex(Status.SelectedRecipe) && i != Status.SelectedRecipe)
		{
			continue; // a chosen recipe: only that one
		}
		int32 Slot = INDEX_NONE;
		const FWEMachineRecipe& Recipe = MachineDefinition->Recipes[i];
		if (MatchesRecipe(Recipe, Slot))
		{
			if (FreeSlots < FMath::Max(1, Recipe.Outputs.Num()))
			{
				SetState(EWEMachineState::OutputFull);
				return INDEX_NONE;
			}
			return i;
		}
	}
	SetState(EWEMachineState::Idle);
	return INDEX_NONE;
}

void AWEMachine::SetWaitingForPower()
{
	SetState(EWEMachineState::NoPower);
}

void AWEMachine::StartCycle(int32 RecipeIndex)
{
	if (!HasAuthority() || !MachineDefinition || !MachineDefinition->Recipes.IsValidIndex(RecipeIndex))
	{
		return;
	}
	const FWEMachineRecipe& Recipe = MachineDefinition->Recipes[RecipeIndex];
	if (Processing.Num() == 0)
	{
		// Take the inputs now: they cannot be taken out of a running machine.
		int32 Slot = INDEX_NONE;
		if (!MatchesRecipe(Recipe, Slot))
		{
			return;
		}
		TakeIngredient(Recipe, false, Recipe.InputItem ? Recipe.InputQuantity : 1);
		if (Recipe.SecondInputItem)
		{
			TakeIngredient(Recipe, true, Recipe.SecondInputQuantity);
		}
		if (Processing.Num() == 0)
		{
			return;
		}
		Progress = 0.f;
		Status.RecipeIndex = static_cast<int8>(RecipeIndex);
		Status.CycleSeconds = FMath::Max(0.5f, (Recipe.BaseSeconds + Recipe.SecondsPerKg * GetProcessingWeightKg()) / (GetEfficiency() * OperatorBonus));
		UE_LOG(LogWENet, Log, TEXT("%s: start '%s' (%.1f kg, %.1fs)"), *GetName(), *Recipe.DisplayName.ToString(), GetProcessingWeightKg(), Status.CycleSeconds);
	}
	SetState(EWEMachineState::Running);
}

void AWEMachine::AdvanceCycle(float DeltaSeconds)
{
	if (!HasAuthority() || Status.State != EWEMachineState::Running || Status.CycleSeconds <= 0.f)
	{
		return;
	}
	Progress += DeltaSeconds / Status.CycleSeconds;
	if (Progress >= 1.f)
	{
		CompleteCycle();
	}
}

void AWEMachine::CompleteCycle()
{
	if (!HasAuthority() || !MachineDefinition || !MachineDefinition->Recipes.IsValidIndex(Status.RecipeIndex))
	{
		return;
	}
	const FWEMachineRecipe& Recipe = MachineDefinition->Recipes[Status.RecipeIndex];
	const float InputKg = GetProcessingWeightKg();
	TArray<FWEItemInstance> Products;
	for (const FWEMachineOutput& Output : Recipe.Outputs)
	{
		if (!Output.Item || Random.FRand() > Output.Chance)
		{
			continue;
		}
		const int32 Quantity = Output.PerInputKg > 0.f ? FMath::RoundToInt32(InputKg * Output.PerInputKg)
			: Random.RandRange(FMath::Min(Output.MinQuantity, Output.MaxQuantity), FMath::Max(Output.MinQuantity, Output.MaxQuantity));
		for (int32 Remaining = Quantity; Remaining > 0;)
		{
			const int32 Count = FMath::Min(Remaining, Output.Item->GetMaxStackSize());
			// Materials: same condition/quality as workbench materials, so they stack together.
			Products.Add(FWEItemInstance::Make(Output.Item, Count, Output.Condition, static_cast<uint8>(FMath::Clamp(Output.Quality, 0, 100))));
			Remaining -= Count;
		}
	}
	UE_LOG(LogWENet, Log, TEXT("%s: done '%s' from %.1f kg -> %d product stacks"), *GetName(), *Recipe.DisplayName.ToString(), InputKg, Products.Num());
	DeliverProducts(Products);

	Processing.Reset();
	Progress = 0.f;
	Status.RecipeIndex = INDEX_NONE;
	++Status.CycleCounter;
	if (UWEMilestoneComponent* Milestones = UWEMilestoneComponent::Get(this)) { Milestones->AddCount(EWEMilestoneKind::MachineCycles); }
	Status.Maintenance = static_cast<uint8>(FMath::Max(0, FMath::RoundToInt32(Status.Maintenance - MachineDefinition->WearPerCycle)));
	SetState(Status.Maintenance == 0 ? EWEMachineState::NeedsMaintenance : EWEMachineState::Idle);
	ForceNetUpdate();
}

void AWEMachine::DeliverProducts(const TArray<FWEItemInstance>& Products)
{
	int32 Index = 0;
	for (const FWEItemInstance& Product : Products)
	{
		int32 Added = 0;
		if (Product.Definition->bFitsInInventory || bOutputConnected)
		{
			OutputStorage->TryAddItem(Product, Added);
		}
		if (Added < Product.Quantity)
		{
			// Output full or bulky (bales): put it down in front of the machine.
			FWEItemInstance Rest = Product;
			Rest.Quantity -= Added;
			const FVector Front = GetActorTransform().TransformPosition(FVector(Size.X * 0.5f + 60.f, -Size.Y * 0.3f + Index * 40.f, 40.f));
			AWEPickupItem::SpawnWorldItem(GetWorld(), Rest, FTransform(GetActorRotation(), Front));
			++Index;
		}
	}
}

void AWEMachine::RestoreState(bool bOn, uint8 Maintenance, int32 RecipeIndex, float InProgress, float CycleSeconds, const TArray<FWEItemInstance>& InProcessing, int32 SelectedRecipe)
{
	if (!HasAuthority())
	{
		return;
	}
	Status.bOn = bOn;
	Status.Maintenance = Maintenance;
	Status.RecipeIndex = static_cast<int8>(RecipeIndex);
	Status.CycleSeconds = CycleSeconds;
	Status.SelectedRecipe = MachineDefinition && MachineDefinition->Recipes.IsValidIndex(SelectedRecipe) ? static_cast<int8>(SelectedRecipe) : INDEX_NONE;
	Processing = InProcessing;
	Progress = InProgress;
	// The grid decides next step whether the machine runs (power).
	SetState(bOn ? EWEMachineState::Idle : EWEMachineState::Off);
	Status.ProgressAtStart = Progress;
	ForceNetUpdate();
}

// --- Operation (server) -----------------------------------------------------------------------------

bool AWEMachine::CanInsertItem(const AWEPickupItem* Item, FText& OutReason) const
{
	const FWEItemInstance* Instance = Item ? &Item->GetItem() : nullptr;
	if (!Instance || !Instance->Definition || !MachineDefinition)
	{
		return false;
	}
	if (Instance->IsCustomerItem())
	{
		OutReason = LOCTEXT("Customer", "Kundengerät – nicht verarbeiten");
		return false;
	}
	if (!MachineDefinition->AcceptsItem(Instance->Definition))
	{
		OutReason = FText::Format(LOCTEXT("NotAccepted", "{0} verarbeitet das nicht"), MachineDefinition->DisplayName);
		return false;
	}
	if (GetStorage()->GetAddableQuantity(*Instance) < Instance->Quantity)
	{
		OutReason = LOCTEXT("InputFull", "Eingang voll");
		return false;
	}
	return true;
}

bool AWEMachine::InsertItem(AWEPickupItem* Item)
{
	FText Reason;
	if (!HasAuthority() || !CanInsertItem(Item, Reason))
	{
		return false;
	}
	int32 Added = 0;
	GetStorage()->TryAddItem(Item->GetItem(), Added);
	if (Added <= 0)
	{
		return false;
	}
	if (AWECharacter* Holder = Item->GetHolder())
	{
		Holder->ReleaseHeldItem();
	}
	UE_LOG(LogWENet, Log, TEXT("%s: %s put in"), *GetName(), *Item->GetDisplayName().ToString());
	Item->Destroy();
	return true;
}

bool AWEMachine::SetPower(bool bOn)
{
	if (!HasAuthority() || !MachineDefinition)
	{
		return false;
	}
	Status.bOn = bOn;
	if (!bOn)
	{
		SetState(EWEMachineState::Off); // a running cycle pauses and keeps its progress
	}
	else
	{
		SetState(Status.Maintenance == 0 ? EWEMachineState::NeedsMaintenance : EWEMachineState::Idle);
	}
	ForceNetUpdate();
	if (UWEPowerGridComponent* Grid = UWEPowerGridComponent::Get(this))
	{
		Grid->StepMachines(0.f); // react at once (start or wait for power)
	}
	return true;
}

bool AWEMachine::Maintain(AWECharacter* Character)
{
	UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr;
	if (!HasAuthority() || !MachineDefinition || !MachineDefinition->MaintenanceItem || !Inventory || Status.Maintenance >= 100)
	{
		return false;
	}
	for (const FWEInventorySlotData& Slot : Inventory->GetAllSlots())
	{
		if (Slot.Item.Definition == MachineDefinition->MaintenanceItem)
		{
			FWEItemInstance Used;
			if (Inventory->RemoveFromSlot(Slot.SlotIndex, 1, Used) != EWEInventoryResult::Success)
			{
				return false;
			}
			Status.Maintenance = 100;
			if (Status.State == EWEMachineState::NeedsMaintenance)
			{
				SetState(Status.bOn ? EWEMachineState::Idle : EWEMachineState::Off);
			}
			ForceNetUpdate();
			if (AWEPlayerController* PC = Cast<AWEPlayerController>(Character->GetController()))
			{
				PC->ClientShowMessage(FText::Format(LOCTEXT("Maintained", "{0} gewartet – wieder 100 %"), MachineDefinition->DisplayName));
			}
			UE_LOG(LogWENet, Log, TEXT("%s maintained %s"), *Character->GetName(), *GetName());
			return true;
		}
	}
	return false;
}

void AWEMachine::MaintainWithKit()
{
	if (!HasAuthority() || !MachineDefinition)
	{
		return;
	}
	Status.Maintenance = 100;
	if (Status.State == EWEMachineState::NeedsMaintenance)
	{
		SetState(Status.bOn ? EWEMachineState::Idle : EWEMachineState::Off);
	}
	ForceNetUpdate();
	UE_LOG(LogWENet, Log, TEXT("%s maintained by staff"), *GetName());
}

int32 AWEMachine::TakeOutputs(AWECharacter* Character)
{
	UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr;
	if (!HasAuthority() || !Inventory)
	{
		return 0;
	}
	const int32 Moved = OutputStorage->TransferAllTo(Inventory);
	// Bulky products (only there while a conveyor was connected) go down in front of the machine.
	const TArray<FWEInventorySlotData> Left = OutputStorage->GetAllSlots();
	for (const FWEInventorySlotData& Slot : Left)
	{
		FWEItemInstance Bulky;
		if (Slot.Item.Definition && !Slot.Item.Definition->bFitsInInventory && OutputStorage->RemoveFromSlot(Slot.SlotIndex, Slot.Item.Quantity, Bulky) == EWEInventoryResult::Success)
		{
			DeliverProducts({ Bulky });
		}
	}
	if (OutputStorage->GetAllSlots().Num() > 0)
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(Character->GetController()))
		{
			PC->ClientShowMessage(LOCTEXT("NotAll", "Nicht alles passt ins Inventar"));
		}
	}
	if (Status.State == EWEMachineState::OutputFull)
	{
		SetState(Status.bOn ? EWEMachineState::Idle : EWEMachineState::Off);
	}
	return Moved;
}

FText AWEMachine::GetSelectedRecipeText() const
{
	return MachineDefinition && MachineDefinition->Recipes.IsValidIndex(Status.SelectedRecipe)
		? MachineDefinition->Recipes[Status.SelectedRecipe].DisplayName : LOCTEXT("AutoRecipe", "Automatisch");
}

bool AWEMachine::SelectRecipe(int32 RecipeIndex)
{
	if (!HasAuthority() || !MachineDefinition || (RecipeIndex != INDEX_NONE && !MachineDefinition->Recipes.IsValidIndex(RecipeIndex)))
	{
		return false;
	}
	Status.SelectedRecipe = static_cast<int8>(RecipeIndex);
	++Status.EventCounter;
	ForceNetUpdate();
	UE_LOG(LogWENet, Log, TEXT("%s: recipe %s"), *GetName(), *GetSelectedRecipeText().ToString());
	return true;
}

void AWEMachine::CycleRecipe()
{
	if (!MachineDefinition || MachineDefinition->Recipes.Num() < 2)
	{
		return;
	}
	const int32 Next = Status.SelectedRecipe + 1;
	SelectRecipe(MachineDefinition->Recipes.IsValidIndex(Next) ? Next : INDEX_NONE);
}

bool AWEMachine::CanAcceptFromBelt(const FWEItemInstance& Item) const
{
	if (!MachineDefinition || !Item.Definition || Item.IsCustomerItem())
	{
		return false;
	}
	const bool bWanted = MachineDefinition->Recipes.IsValidIndex(Status.SelectedRecipe)
		? MachineDefinition->Recipes[Status.SelectedRecipe].UsesItem(Item.Definition) : MachineDefinition->AcceptsItem(Item.Definition);
	return bWanted && GetStorage()->GetAddableQuantity(Item) >= Item.Quantity;
}

bool AWEMachine::AcceptFromBelt(const FWEItemInstance& Item)
{
	if (!HasAuthority() || !CanAcceptFromBelt(Item))
	{
		return false;
	}
	int32 Added = 0;
	GetStorage()->TryAddItem(Item, Added);
	return Added == Item.Quantity;
}

bool AWEMachine::TakeOutputForBelt(FWEItemInstance& OutItem)
{
	const TArray<FWEInventorySlotData>& Slots = OutputStorage->GetAllSlots();
	if (!HasAuthority() || Slots.Num() == 0)
	{
		return false;
	}
	const FWEInventorySlotData First = Slots[0];
	if (OutputStorage->RemoveFromSlot(First.SlotIndex, First.Item.Quantity, OutItem) != EWEInventoryResult::Success)
	{
		return false;
	}
	if (Status.State == EWEMachineState::OutputFull)
	{
		SetState(Status.bOn ? EWEMachineState::Idle : EWEMachineState::Off);
	}
	return true;
}

// --- Interaction ---------------------------------------------------------------------------------------

FText AWEMachine::GetInteractionText(const AWECharacter* Interactor) const
{
	return FText::Format(LOCTEXT("OpenInput", "Eingang öffnen ({0})"), MachineDefinition ? MachineDefinition->DisplayName : FText::GetEmpty());
}

bool AWEMachine::GetSecondaryInteraction(const AWECharacter* Interactor, FText& OutText) const
{
	int32 Count = 0;
	for (const FWEInventorySlotData& Slot : OutputStorage->GetAllSlots())
	{
		Count += Slot.Item.Quantity;
	}
	if (Count == 0 || !Interactor || Interactor->GetHeldItem())
	{
		return false;
	}
	OutText = FText::Format(LOCTEXT("TakeOut", "Produkte entnehmen ({0})"), FText::AsNumber(Count));
	return true;
}

void AWEMachine::InteractSecondary(AWECharacter* Interactor)
{
	TakeOutputs(Interactor);
}

bool AWEMachine::GetAlternativeInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const
{
	if (!MachineDefinition || !Interactor)
	{
		return false;
	}
	OutText = Status.bOn ? LOCTEXT("SwitchOff", "Ausschalten")
		: FText::Format(LOCTEXT("SwitchOn", "Einschalten ({0} kW)"), FText::AsNumber(MachineDefinition->PowerKw));
	return true;
}

void AWEMachine::InteractAlternative(AWECharacter* Interactor)
{
	SetPower(!Status.bOn);
}

bool AWEMachine::GetExtraInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const
{
	if (!MachineDefinition || !MachineDefinition->MaintenanceItem || !Interactor || Status.Maintenance >= 100)
	{
		return false;
	}
	const UWEInventoryComponent* Inventory = Interactor->GetPlayerInventory();
	if (Inventory && Inventory->CountItems(MachineDefinition->MaintenanceItem) > 0)
	{
		OutText = FText::Format(LOCTEXT("Maintain", "Warten (Wartung {0} %)"), FText::AsNumber(Status.Maintenance));
		return true;
	}
	if (Status.Maintenance < MachineDefinition->WornThreshold + 20.f)
	{
		OutReason = FText::Format(LOCTEXT("MaintainMissing", "Wartung {0} %: {1} fehlt"), FText::AsNumber(Status.Maintenance), MachineDefinition->MaintenanceItem->DisplayName);
	}
	return false;
}

void AWEMachine::InteractExtra(AWECharacter* Interactor)
{
	Maintain(Interactor);
}

bool AWEMachine::GetSettingInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const
{
	if (!MachineDefinition || !Interactor || MachineDefinition->Recipes.Num() < 2)
	{
		return false;
	}
	OutText = FText::Format(LOCTEXT("RecipeSetting", "Rezept: {0}"), GetSelectedRecipeText());
	return true;
}

void AWEMachine::InteractSetting(AWECharacter* Interactor)
{
	CycleRecipe();
	if (AWEPlayerController* PC = Interactor ? Cast<AWEPlayerController>(Interactor->GetController()) : nullptr)
	{
		PC->ClientShowMessage(FText::Format(LOCTEXT("RecipeChosen", "{0}: Rezept {1}"), MachineDefinition->DisplayName, GetSelectedRecipeText()));
	}
}

bool AWEMachine::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	if (!MachineDefinition)
	{
		return Super::GetExamineInfo(OutInfo);
	}
	OutInfo.Title = MachineDefinition->DisplayName;
	OutInfo.Description = LOCTEXT("MachineDesc", "Industriemaschine. [E] Eingang füllen, [Q] ein/aus, [R] Produkte entnehmen, [X] warten.");
	OutInfo.Details.Reset();
	OutInfo.Details.Add(FText::Format(LOCTEXT("ExState", "Zustand: {0}"), GetStatusText()));
	if (Status.State == EWEMachineState::Running || Processing.Num() > 0)
	{
		OutInfo.Details.Add(FText::Format(LOCTEXT("ExProgress", "Fortschritt: {0} %"), FText::AsNumber(FMath::RoundToInt(GetProgress() * 100.f))));
	}
	OutInfo.Details.Add(FText::Format(LOCTEXT("ExPower", "Strom: {0} kW im Betrieb, {1} kW bereit"), FText::AsNumber(MachineDefinition->PowerKw), FText::AsNumber(MachineDefinition->IdlePowerKw)));
	OutInfo.Details.Add(FText::Format(LOCTEXT("ExMaint", "Wartung: {0} %{1}"), FText::AsNumber(Status.Maintenance),
		GetEfficiency() < 1.f ? LOCTEXT("ExWorn", " (verschlissen: langsamer)") : FText::GetEmpty()));
	OutInfo.Details.Add(FText::Format(LOCTEXT("ExStorage", "Eingang: {0} / {1} Plätze · Ausgabe: {2} / {3} Plätze"),
		FText::AsNumber(GetStorage()->GetAllSlots().Num()), FText::AsNumber(GetStorage()->GetNumSlots()),
		FText::AsNumber(OutputStorage->GetAllSlots().Num()), FText::AsNumber(OutputStorage->GetNumSlots())));
	TArray<FString> Recipes;
	for (const FWEMachineRecipe& Recipe : MachineDefinition->Recipes)
	{
		Recipes.Add(Recipe.DisplayName.ToString());
	}
	OutInfo.Details.Add(FText::Format(LOCTEXT("ExRecipes", "Kann: {0}"), FText::FromString(FString::Join(Recipes, TEXT(", ")))));
	if (MachineDefinition->Recipes.Num() > 1)
	{
		OutInfo.Details.Add(FText::Format(LOCTEXT("ExSelected", "Rezept: {0} ([T] wechseln)"), GetSelectedRecipeText()));
	}
	return true;
}

#undef LOCTEXT_NAMESPACE
