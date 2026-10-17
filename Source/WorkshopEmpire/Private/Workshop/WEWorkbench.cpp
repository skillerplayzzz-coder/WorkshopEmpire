#include "Workshop/WEWorkbench.h"
#include "Fx/WEFx.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Framework/WEPlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Interaction/WEInteractionComponent.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Player/WECharacter.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "WELog.h"
#include "Workshop/WEDisassemblyDefinition.h"
#include "Workshop/WERepairDefinition.h"
#include "Workshop/WEFurnitureDefinition.h"
#include "Progression/WEProgressionComponent.h"
#include "World/WEPlaceholderVisuals.h"

#define LOCTEXT_NAMESPACE "WEWorkbench"

namespace WEWorkbenchConst
{
	constexpr float IndicatorInterval = 1.f / 30.f;
	constexpr float FlashSeconds = 0.6f;
	const FLinearColor Idle(0.12f, 0.12f, 0.12f);
	const FLinearColor Working(1.0f, 0.55f, 0.1f);
	const FLinearColor Done(0.2f, 0.9f, 0.3f);
}

AWEWorkbench::AWEWorkbench()
{
	PropType = EWEPlaceholderPropType::Workbench;
	Size = FVector(80.f, 200.f, 90.f);
	bHasStorage = true;
	StorageName = LOCTEXT("StorageName", "Werkbank");
	StorageSlots = 12;
	StorageMaxWeightKg = 60.f;

	WorkPoint = CreateDefaultSubobject<USceneComponent>(TEXT("WorkPoint"));
	WorkPoint->SetupAttachment(RootComponent);
	WorkPoint->SetMobility(EComponentMobility::Movable);

	WorkIndicator = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorkIndicator"));
	WorkIndicator->SetupAttachment(RootComponent);
	WorkIndicator->SetMobility(EComponentMobility::Movable);
	WorkIndicator->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WorkIndicator->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		WorkIndicator->SetStaticMesh(CubeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> Place(TEXT("/Game/WorkshopEmpire/Audio/SFX_PlaceItem.SFX_PlaceItem"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Start(TEXT("/Game/WorkshopEmpire/Audio/SFX_ScrewLoosen.SFX_ScrewLoosen"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Done(TEXT("/Game/WorkshopEmpire/Audio/SFX_PartRemoved.SFX_PartRemoved"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Finished(TEXT("/Game/WorkshopEmpire/Audio/SFX_DisassemblyDone.SFX_DisassemblyDone"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Cancelled(TEXT("/Game/WorkshopEmpire/Audio/SFX_Denied.SFX_Denied"));
	SoundPlace = Place.Object;
	SoundStepStart = Start.Object;
	SoundStepDone = Done.Object;
	SoundFinished = Finished.Object;
	SoundCancelled = Cancelled.Object;

	Random.Initialize(12345);
}

void AWEWorkbench::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWEWorkbench, PlacedItem);
	DOREPLIFETIME(AWEWorkbench, Work);
}

void AWEWorkbench::ApplyFurnitureExtras(const FWEFurnitureDefinition& Definition)
{
	BenchSpeedMultiplier = Definition.WorkSpeedMultiplier;
}

void AWEWorkbench::RebuildVisuals()
{
	Super::RebuildVisuals();
	// Work spot: left half of the top (right half stays free for small parts and tools).
	WorkPoint->SetRelativeLocation(FVector(0.f, -Size.Y * 0.22f, Size.Z));
	WorkIndicator->SetRelativeLocation(FVector(Size.X * 0.5f + 0.6f, 0.f, Size.Z - 2.5f));
	WorkIndicator->SetRelativeScale3D(FVector(0.01f, Size.Y / 100.f, 0.02f));
	WEPlaceholder::ApplyTint(WorkIndicator, PlaceholderMaterial, WEWorkbenchConst::Idle);
}

void AWEWorkbench::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		Random.Initialize(FMath::Rand());
	}
	WEPlaceholder::ApplyTint(WorkIndicator, PlaceholderMaterial, WEWorkbenchConst::Idle);
	// Benches placed later also get the bench-cabinet upgrade.
	if (const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this))
	{
		SetStorageBonusSlots(Progression->GetBenchExtraSlots());
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		GetWorldTimerManager().SetTimer(IndicatorTimer, this, &AWEWorkbench::UpdateIndicator, WEWorkbenchConst::IndicatorInterval, true);
	}
}

void AWEWorkbench::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(WorkTimer);
	GetWorldTimerManager().ClearTimer(IndicatorTimer);
	Super::EndPlay(EndPlayReason);
}

float AWEWorkbench::GetServerTime() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}
	const AGameStateBase* GameState = World->GetGameState();
	return GameState ? static_cast<float>(GameState->GetServerWorldTimeSeconds()) : World->GetTimeSeconds();
}

// --- Placement ---------------------------------------------------------------------------------------

bool AWEWorkbench::CanPlaceItem(const AWEPickupItem* Item, FText& OutReason) const
{
	if (!Item || !Item->GetItem().Definition)
	{
		return false;
	}
	const UWEItemDefinition* Def = Item->GetItem().Definition;
	if (!Def->CanBeDisassembled() && !Def->CanBeRepaired())
	{
		OutReason = LOCTEXT("NotDisassemblable", "Lässt sich nicht zerlegen");
		return false;
	}
	if (PlacedItem && PlacedItem != Item)
	{
		OutReason = LOCTEXT("Occupied", "Werkbank ist belegt");
		return false;
	}
	if (!Def->CanBeRepaired() && Item->GetItem().DisassemblyStep >= Def->Disassembly->NumSteps())
	{
		OutReason = LOCTEXT("AlreadyDone", "Bereits vollständig zerlegt");
		return false;
	}
	return true;
}

bool AWEWorkbench::PlaceItem(AWEPickupItem* Item)
{
	FText Reason;
	if (!HasAuthority() || !CanPlaceItem(Item, Reason))
	{
		return false;
	}
	PlacedItem = Item;
	Item->HandlePlacedOnWorkbench(this, WorkPoint);
	RaiseEvent(EWEWorkbenchEvent::Placed);
	if (UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this)) { Progression->NotifyEvent(EWEProgressEvent::ItemPlacedOnBench); }
	UE_LOG(LogWENet, Log, TEXT("%s: %s placed (step %d/%d)"), *GetName(), *Item->GetDisplayName().ToString(), GetNextStepIndex(), GetNumSteps());
	return true;
}

bool AWEWorkbench::TakeOffItem(AWECharacter* By)
{
	if (!HasAuthority() || !PlacedItem || IsBusy() || !By || By->GetHeldItem())
	{
		return false;
	}
	AWEPickupItem* Item = PlacedItem;
	PlacedItem = nullptr;
	Item->HandleRemovedFromWorkbench();
	RaiseEvent(EWEWorkbenchEvent::Removed);
	if (!By->TryPickUpItem(Item))
	{
		// Could not carry it (should not happen for placeable items): leave it on the bench.
		PlacedItem = Item;
		Item->HandlePlacedOnWorkbench(this, WorkPoint);
		return false;
	}
	return true;
}

FVector AWEWorkbench::GetWorkSpotLocation() const
{
	return WorkPoint->GetComponentLocation();
}

// --- Disassembly ------------------------------------------------------------------------------------

int32 AWEWorkbench::GetNumSteps() const
{
	const UWEItemDefinition* Def = PlacedItem ? PlacedItem->GetItem().Definition.Get() : nullptr;
	return Def && Def->Disassembly ? Def->Disassembly->NumSteps() : 0;
}

int32 AWEWorkbench::GetNextStepIndex() const
{
	return PlacedItem ? PlacedItem->GetItem().DisassemblyStep : INDEX_NONE;
}

const FWEDisassemblyStep* AWEWorkbench::GetNextStep() const
{
	const UWEItemDefinition* Def = PlacedItem ? PlacedItem->GetItem().Definition.Get() : nullptr;
	return Def && Def->Disassembly ? Def->Disassembly->GetStep(GetNextStepIndex()) : nullptr;
}

FText AWEWorkbench::GetNextStepPrompt() const
{
	const FWEDisassemblyStep* Step = GetNextStep();
	if (!Step)
	{
		return FText::GetEmpty();
	}
	if (Step->RequiredTool == EWEToolType::None)
	{
		return Step->ActionText;
	}
	return FText::Format(LOCTEXT("StepWithTool", "{0} ({1})"), Step->ActionText, WEItems::GetToolText(Step->RequiredTool));
}

float AWEWorkbench::FindToolSpeed(const AWECharacter* Character, EWEToolType Tool)
{
	if (Tool == EWEToolType::None)
	{
		return 1.f;
	}
	const UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr;
	float Best = 0.f;
	if (Inventory)
	{
		for (const FWEInventorySlotData& Slot : Inventory->GetAllSlots())
		{
			const UWEItemDefinition* Def = Slot.Item.Definition;
			if (Def && Def->ToolType == Tool)
			{
				Best = FMath::Max(Best, Def->ToolSpeedMultiplier);
			}
		}
	}
	return Best;
}

bool AWEWorkbench::CanStartStep(const AWECharacter* Character, FText& OutReason) const
{
	const FWEDisassemblyStep* Step = GetNextStep();
	if (!Character || !Step)
	{
		return false;
	}
	if (IsBusy())
	{
		OutReason = Work.Worker == Character ? LOCTEXT("Working", "Arbeite …") : LOCTEXT("BusyOther", "Wird gerade bearbeitet");
		return false;
	}
	if (Character->GetHeldItem())
	{
		OutReason = LOCTEXT("HandsFull", "Hände sind voll");
		return false;
	}
	if (PlacedItem->GetItem().IsCustomerItem())
	{
		OutReason = LOCTEXT("CustomerNoDisassembly", "Kundengerät – nicht zerlegen");
		return false;
	}
	if (FindToolSpeed(Character, Step->RequiredTool) <= 0.f)
	{
		OutReason = FText::Format(LOCTEXT("ToolMissing", "Werkzeug fehlt: {0}"), WEItems::GetToolText(Step->RequiredTool));
		return false;
	}
	return true;
}

bool AWEWorkbench::TryStartStep(AWECharacter* Character)
{
	FText Reason;
	if (!HasAuthority() || !CanStartStep(Character, Reason))
	{
		UE_LOG(LogWENet, Log, TEXT("%s: step refused for %s: %s"), *GetName(), *GetNameSafe(Character), *Reason.ToString());
		return false;
	}
	const FWEDisassemblyStep* Step = GetNextStep();
	const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	const float Speed = FindToolSpeed(Character, Step->RequiredTool) * (Progression ? Progression->GetWorkSpeedMultiplier() : 1.f) * BenchSpeedMultiplier;

	// A broken device has broken parts: decide which ones now, so they come out "Defekt".
	FWEItemInstance Item = PlacedItem->GetItem();
	if (!Item.Repair.HasFlag(FWERepairState::FaultsRolled))
	{
		EnsureFaultsRolled(Item);
		PlacedItem->SetItem(Item);
	}

	Work.RepairKind = EWERepairStepKind::None;
	Work.RepairFault = INDEX_NONE;
	Work.StepIndex = GetNextStepIndex();
	Work.Worker = Character;
	Work.Duration = FMath::Max(0.1f, Step->BaseSeconds / Speed);
	Work.StartServerTime = GetServerTime();
	GetWorldTimerManager().SetTimer(WorkTimer, this, &AWEWorkbench::CompleteActiveStep, Work.Duration, false);
	RaiseEvent(EWEWorkbenchEvent::StepStarted);
	UE_LOG(LogWENet, Log, TEXT("%s: %s started step %d '%s' (%.1fs)"), *GetName(), *Character->GetName(), Work.StepIndex,
		*Step->ActionText.ToString(), Work.Duration);
	return true;
}

void AWEWorkbench::CancelActiveStep()
{
	if (!HasAuthority() || !IsBusy())
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(WorkTimer);
	Work.StepIndex = INDEX_NONE;
	Work.Worker = nullptr;
	Work.RepairKind = EWERepairStepKind::None;
	Work.RepairFault = INDEX_NONE;
	RaiseEvent(EWEWorkbenchEvent::StepCancelled);
}

void AWEWorkbench::CompleteActiveStep()
{
	if (!HasAuthority() || !IsBusy())
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(WorkTimer);

	AWECharacter* Worker = Work.Worker;
	const bool bRepair = Work.RepairKind != EWERepairStepKind::None;
	const FWEDisassemblyStep* Step = GetNextStep();
	if (!PlacedItem || (!bRepair && !Step) || !IsValid(Worker) || !UWEInteractionComponent::IsWithinReach(Worker, this, WorkReach))
	{
		UE_LOG(LogWENet, Log, TEXT("%s: step cancelled (worker gone or out of reach)"), *GetName());
		if (AWEPlayerController* PC = Worker ? Cast<AWEPlayerController>(Worker->GetController()) : nullptr)
		{
			PC->ClientShowMessage(LOCTEXT("Cancelled", "Arbeitsschritt abgebrochen – zu weit weg"));
		}
		CancelActiveStep();
		return;
	}
	if (bRepair)
	{
		CompleteRepairStep(Worker);
		return;
	}

	FWEItemInstance Source = PlacedItem->GetItem();
	const TArray<FWEItemInstance> Outputs = GenerateOutputs(*Step, Source);
	const int32 Total = GetNumSteps();
	Source.DisassemblyStep = static_cast<uint8>(FMath::Min(Work.StepIndex + 1, 255));
	UE_LOG(LogWENet, Log, TEXT("%s: step %d/%d done ('%s'), %d output stacks"), *GetName(), Source.DisassemblyStep, Total,
		*Step->ActionText.ToString(), Outputs.Num());

	Work.StepIndex = INDEX_NONE;
	Work.Worker = nullptr;

	if (Source.DisassemblyStep >= Total)
	{
		// Fully disassembled: the item is used up.
		AWEPickupItem* Done = PlacedItem;
		PlacedItem = nullptr;
		const FText Name = Done->GetItem().Definition->DisplayName;
		Done->Destroy();
		DeliverOutputs(Outputs, Worker);
		RaiseEvent(EWEWorkbenchEvent::Finished);
		if (UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this)) { Progression->NotifyEvent(EWEProgressEvent::DisassemblyFinished); }
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(Worker->GetController()))
		{
			PC->ClientShowMessage(FText::Format(LOCTEXT("FinishedMsg", "Vollständig zerlegt: {0}"), Name));
		}
		return;
	}

	PlacedItem->SetItem(Source);
	DeliverOutputs(Outputs, Worker);
	RaiseEvent(EWEWorkbenchEvent::StepCompleted);
}

TArray<FWEItemInstance> AWEWorkbench::GenerateOutputs(const FWEDisassemblyStep& Step, const FWEItemInstance& Source)
{
	TArray<FWEItemInstance> Result;
	for (const FWEDisassemblyOutput& Output : Step.Outputs)
	{
		if (!Output.Item || Random.FRand() > Output.Chance)
		{
			continue;
		}
		int32 Quantity = Random.RandRange(FMath::Min(Output.MinQuantity, Output.MaxQuantity), FMath::Max(Output.MinQuantity, Output.MaxQuantity));
		// Known faults: a part taken out for repair is missing, a broken one comes out "Defekt".
		int32 BrokenUnits = 0;
		const UWERepairDefinition* Repair = Source.Definition->CanBeRepaired() ? Source.Definition->Repair.Get() : nullptr;
		if (Repair && Output.bInheritCondition && Source.Repair.HasFlag(FWERepairState::FaultsRolled))
		{
			for (int32 FaultIndex = 0; FaultIndex < Repair->Faults.Num(); ++FaultIndex)
			{
				if (Repair->Faults[FaultIndex].Part != Output.Item)
				{
					continue;
				}
				const bool bRemoved = FWERepairState::HasBit(Source.Repair.Removed, FaultIndex);
				const bool bReplaced = FWERepairState::HasBit(Source.Repair.Replaced, FaultIndex);
				if (bRemoved && !bReplaced)
				{
					--Quantity;
				}
				else if (FWERepairState::HasBit(Source.Repair.Faults, FaultIndex))
				{
					++BrokenUnits;
				}
			}
		}
		if (Quantity <= 0)
		{
			continue;
		}
		BrokenUnits = FMath::Min(BrokenUnits, Quantity);
		EWEItemCondition Condition = EWEItemCondition::Used;
		uint8 Quality = 50;
		if (Output.bInheritCondition)
		{
			// A defective device usually has one broken part: the others come out the same or up to two
			// condition levels better than the device (never worse). Keeps "buy broken, part out" profitable.
			const int32 Index = FMath::Clamp(static_cast<int32>(Source.Condition) - Random.RandRange(0, 2), 0, static_cast<int32>(EWEItemCondition::Scrap));
			Condition = static_cast<EWEItemCondition>(Index);
			Quality = static_cast<uint8>(FMath::Clamp(static_cast<int32>(Source.Quality) + Random.RandRange(-15, 15), 0, 100));
		}
		for (int32 i = 0; i < BrokenUnits; ++i)
		{
			Result.Add(FWEItemInstance::Make(Output.Item, 1, EWEItemCondition::Defective, static_cast<uint8>(FMath::Min<int32>(Quality, 25))));
		}
		// Stackable outputs may exceed one stack; split so every stack is valid.
		int32 Remaining = Quantity - BrokenUnits;
		const int32 MaxStack = Output.Item->GetMaxStackSize();
		while (Remaining > 0)
		{
			const int32 Count = FMath::Min(Remaining, MaxStack);
			Result.Add(FWEItemInstance::Make(Output.Item, Count, Condition, Quality));
			Remaining -= Count;
		}
	}
	return Result;
}

void AWEWorkbench::DeliverOutputs(const TArray<FWEItemInstance>& Outputs, AWECharacter* Worker, bool bAnnounce)
{
	TMap<FString, int32> Counts; // same item name listed once with a total
	TArray<FString> Order;
	int32 Index = 0;
	for (const FWEItemInstance& Output : Outputs)
	{
		int32 Added = 0;
		if (Storage && Output.Definition->bFitsInInventory)
		{
			Storage->TryAddItem(Output, Added);
		}
		if (Added < Output.Quantity)
		{
			// Bench storage full (or bulky part): put the rest in front of the bench.
			FWEItemInstance Rest = Output;
			Rest.Quantity -= Added;
			const FVector Front = GetActorTransform().TransformPosition(FVector(Size.X * 0.5f + 35.f, -Size.Y * 0.3f + Index * 25.f, Size.Z + 20.f));
			AWEPickupItem::SpawnWorldItem(GetWorld(), Rest, FTransform(GetActorRotation(), Front));
		}
		const FString Name = Output.GetDisplayName().ToString();
		if (!Counts.Contains(Name))
		{
			Order.Add(Name);
		}
		Counts.FindOrAdd(Name) += Output.Quantity;
		++Index;
	}

	TArray<FString> Parts;
	for (const FString& Name : Order)
	{
		const int32 Count = Counts[Name];
		Parts.Add(Count > 1 ? FString::Printf(TEXT("%d× %s"), Count, *Name) : Name);
	}
	if (AWEPlayerController* PC = bAnnounce && Worker ? Cast<AWEPlayerController>(Worker->GetController()) : nullptr)
	{
		PC->ClientShowMessage(Parts.Num() > 0
			? FText::FromString(TEXT("+ ") + FString::Join(Parts, TEXT(", ")))
			: LOCTEXT("NothingUsable", "Nichts Brauchbares dabei"));
	}
}

void AWEWorkbench::ResetForLoad()
{
	if (!HasAuthority())
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(WorkTimer);
	Work.StepIndex = INDEX_NONE;
	Work.Worker = nullptr;
	Work.RepairKind = EWERepairStepKind::None;
	Work.RepairFault = INDEX_NONE;
	if (PlacedItem)
	{
		PlacedItem->HandleRemovedFromWorkbench();
		PlacedItem = nullptr;
	}
	ForceNetUpdate();
}

float AWEWorkbench::GetProgress() const
{
	if (!IsBusy() || Work.Duration <= 0.f)
	{
		return 0.f;
	}
	// Measured on this machine's own clock from when the step start arrived. The client's estimate of
	// the server clock can be off by seconds, which made bars jump to 100 % (seen in the network test).
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	return FMath::Clamp((Now - LocalStepStartTime) / Work.Duration, 0.f, 1.f);
}

// --- Feedback (every machine) ----------------------------------------------------------------------

void AWEWorkbench::RaiseEvent(EWEWorkbenchEvent Event)
{
	Work.LastEvent = Event;
	++Work.EventCounter;
	ForceNetUpdate();
	OnRep_Work(); // server / listen host plays the feedback too
}

void AWEWorkbench::OnRep_PlacedItem()
{
}

void AWEWorkbench::OnRep_Work()
{
	if (Work.EventCounter == LastHandledEvent)
	{
		return;
	}
	LastHandledEvent = Work.EventCounter;
	if (Work.LastEvent == EWEWorkbenchEvent::StepStarted && GetWorld())
	{
		LocalStepStartTime = GetWorld()->GetTimeSeconds();
	}
	if (Work.LastEvent == EWEWorkbenchEvent::StepCompleted || Work.LastEvent == EWEWorkbenchEvent::Finished)
	{
		FlashUntil = GetWorld() ? GetWorld()->GetTimeSeconds() + WEWorkbenchConst::FlashSeconds : 0.f;
		// A little dust and a few bits fly when a part comes off (more at the end).
		const FVector At = WorkPoint->GetComponentLocation() + FVector(0.f, 0.f, 15.f);
		const float FxScale = Work.LastEvent == EWEWorkbenchEvent::Finished ? 1.f : 0.5f;
		AWEFx::Burst(this, EWEFxKind::Dust, At, FVector::UpVector, FLinearColor::White, FxScale);
		AWEFx::Burst(this, EWEFxKind::Chips, At, FVector::UpVector, FLinearColor(0.35f, 0.36f, 0.38f), 0.35f * FxScale);
	}
	PlayEventSound(Work.LastEvent);
}

void AWEWorkbench::PlayEventSound(EWEWorkbenchEvent Event) const
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	USoundBase* Sound = nullptr;
	switch (Event)
	{
	case EWEWorkbenchEvent::Placed:
	case EWEWorkbenchEvent::Removed:       Sound = SoundPlace; break;
	case EWEWorkbenchEvent::StepStarted:   Sound = SoundStepStart; break;
	case EWEWorkbenchEvent::StepCompleted: Sound = SoundStepDone; break;
	case EWEWorkbenchEvent::Finished:      Sound = SoundFinished; break;
	case EWEWorkbenchEvent::StepCancelled: Sound = SoundCancelled; break;
	default: break;
	}
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, WorkPoint->GetComponentLocation());
	}
}

void AWEWorkbench::UpdateIndicator()
{
	UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(WorkIndicator->GetMaterial(0));
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	float Fill = 1.f;
	FLinearColor Color = WEWorkbenchConst::Idle;
	if (Now < FlashUntil)
	{
		Color = WEWorkbenchConst::Done;
	}
	else if (IsBusy())
	{
		Fill = FMath::Max(0.02f, GetProgress());
		Color = WEWorkbenchConst::Working;
	}
	WorkIndicator->SetRelativeScale3D(FVector(0.01f, Size.Y / 100.f * Fill, 0.02f));
	if (MID)
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

bool AWEWorkbench::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	Super::GetExamineInfo(OutInfo);
	OutInfo.Description = LOCTEXT("BenchDesc", "Hier werden Gegenstände Schritt für Schritt zerlegt. Unten ist Platz für Teile und Material.");
	if (PlacedItem)
	{
		OutInfo.Details.Add(FText::Format(LOCTEXT("OnBench", "Auf der Werkbank: {0} (Schritt {1} / {2})"),
			PlacedItem->GetItem().Definition->DisplayName, FText::AsNumber(GetNextStepIndex()), FText::AsNumber(GetNumSteps())));
		const FText Status = GetRepairStatusText(PlacedItem->GetItem());
		if (!Status.IsEmpty())
		{
			OutInfo.Details.Add(FText::Format(LOCTEXT("RepairLine", "Reparatur: {0}"), Status));
		}
	}
	return true;
}

// --- Repair (§9.5) -----------------------------------------------------------------------------------

void AWEWorkbench::ShowMessage(AWECharacter* Character, const FText& Message) const
{
	if (AWEPlayerController* PC = Character ? Cast<AWEPlayerController>(Character->GetController()) : nullptr)
	{
		PC->ClientShowMessage(Message);
	}
}

void AWEWorkbench::EnsureFaultsRolled(FWEItemInstance& Item)
{
	const UWEItemDefinition* Def = Item.Definition;
	if (!HasAuthority() || !Def || !Def->CanBeRepaired() || Item.Repair.HasFlag(FWERepairState::FaultsRolled))
	{
		return;
	}
	Item.Repair.SetFlag(FWERepairState::FaultsRolled);
	Item.Repair.Faults = 0;
	if (!Item.IsBroken())
	{
		return;
	}
	const UWERepairDefinition* Repair = Def->Repair;
	const int32 NumFaults = FMath::Min(Repair->Faults.Num(), UWERepairDefinition::MaxFaults);
	int32 Count = 1;
	if (Item.Condition >= EWEItemCondition::Defective && Random.FRand() < Repair->SecondFaultChance)
	{
		Count = 2;
	}
	// Weighted pick without repetition.
	for (int32 n = 0; n < Count; ++n)
	{
		float Total = 0.f;
		for (int32 i = 0; i < NumFaults; ++i)
		{
			Total += FWERepairState::HasBit(Item.Repair.Faults, i) ? 0.f : FMath::Max(0.f, Repair->Faults[i].Weight);
		}
		if (Total <= 0.f)
		{
			break;
		}
		float Pick = Random.FRand() * Total;
		for (int32 i = 0; i < NumFaults; ++i)
		{
			const float Weight = FWERepairState::HasBit(Item.Repair.Faults, i) ? 0.f : FMath::Max(0.f, Repair->Faults[i].Weight);
			if (Weight > 0.f && (Pick -= Weight) <= 0.f)
			{
				Item.Repair.Faults |= static_cast<uint8>(1 << i);
				break;
			}
		}
	}
}

FWERepairStepInfo AWEWorkbench::GetNextRepairStep(FText& OutReason) const
{
	FWERepairStepInfo Info;
	if (!PlacedItem)
	{
		return Info;
	}
	const FWEItemInstance& Item = PlacedItem->GetItem();
	const UWEItemDefinition* Def = Item.Definition;
	const UWERepairDefinition* Repair = Def && Def->CanBeRepaired() ? Def->Repair.Get() : nullptr;
	if (!Repair)
	{
		return Info; // not repairable at all: no repair prompt
	}
	if (Item.IsPartiallyDisassembled())
	{
		OutReason = LOCTEXT("RepairPartly", "Teilweise zerlegt – nicht mehr reparierbar");
		return Info;
	}
	if (Item.Condition == EWEItemCondition::Scrap)
	{
		OutReason = LOCTEXT("RepairScrap", "Schrott – nicht mehr zu retten");
		return Info;
	}
	if (!Item.IsBroken())
	{
		if (Repair->RefurbishMaterial && !Item.Repair.HasFlag(FWERepairState::Refurbished) && Item.Quality < 95)
		{
			Info.Kind = EWERepairStepKind::Refurbish;
			Info.Text = FText::Format(LOCTEXT("Refurbish", "Reinigen und aufbereiten ({0})"), Repair->RefurbishMaterial->DisplayName);
			Info.Seconds = Repair->RefurbishSeconds;
			Info.Material = Repair->RefurbishMaterial;
			return Info;
		}
		OutReason = Item.Repair.HasFlag(FWERepairState::Refurbished)
			? LOCTEXT("RepairDoneRefurbished", "Funktioniert und ist aufbereitet")
			: LOCTEXT("RepairWorks", "Funktioniert – nichts zu reparieren");
		return Info;
	}
	if (!Item.Repair.HasFlag(FWERepairState::Diagnosed))
	{
		Info.Kind = EWERepairStepKind::Diagnose;
		Info.Text = LOCTEXT("Diagnose", "Fehler suchen");
		Info.Tool = Repair->DiagnoseTool;
		Info.Seconds = Repair->DiagnoseSeconds;
		return Info;
	}
	for (int32 i = 0; i < FMath::Min(Repair->Faults.Num(), UWERepairDefinition::MaxFaults); ++i)
	{
		if (!FWERepairState::HasBit(Item.Repair.Faults, i) || FWERepairState::HasBit(Item.Repair.Replaced, i))
		{
			continue;
		}
		const FWERepairFault& Fault = Repair->Faults[i];
		Info.FaultIndex = i;
		Info.Tool = Fault.Tool;
		if (!FWERepairState::HasBit(Item.Repair.Removed, i))
		{
			Info.Kind = EWERepairStepKind::Remove;
			Info.Text = Fault.RemoveText;
			Info.Seconds = Fault.RemoveSeconds;
		}
		else
		{
			Info.Kind = EWERepairStepKind::Install;
			Info.Text = Fault.InstallText;
			Info.Seconds = Fault.InstallSeconds;
			Info.Material = Fault.Part;
		}
		return Info;
	}
	Info.Kind = EWERepairStepKind::Test;
	Info.Text = LOCTEXT("FunctionTest", "Funktionstest");
	Info.Tool = Repair->TestTool;
	Info.Seconds = Repair->TestSeconds;
	return Info;
}

FText AWEWorkbench::GetNextRepairPrompt() const
{
	FText Reason;
	const FWERepairStepInfo Info = GetNextRepairStep(Reason);
	if (Info.Kind == EWERepairStepKind::None)
	{
		return FText::GetEmpty();
	}
	if (Info.Tool == EWEToolType::None)
	{
		return Info.Text;
	}
	return FText::Format(LOCTEXT("RepairWithTool", "{0} ({1})"), Info.Text, WEItems::GetToolText(Info.Tool));
}

FText AWEWorkbench::GetActiveJobText() const
{
	// The item changes only when a step completes, so the "next" step is the one that is running.
	return Work.RepairKind != EWERepairStepKind::None ? GetNextRepairPrompt() : GetNextStepPrompt();
}

bool AWEWorkbench::FindSparePart(const AWECharacter* Character, const UWEInventoryComponent* InStorage, const UWEItemDefinition* Part,
	UWEInventoryComponent*& OutInventory, int32& OutSlot)
{
	OutInventory = nullptr;
	OutSlot = INDEX_NONE;
	if (!Part)
	{
		return false;
	}
	bool bFound = false;
	EWEItemCondition BestCondition = EWEItemCondition::Scrap;
	uint8 BestQuality = 0;
	const UWEInventoryComponent* Sources[] = { Character ? Character->GetPlayerInventory() : nullptr, InStorage };
	for (const UWEInventoryComponent* Source : Sources)
	{
		if (!Source)
		{
			continue;
		}
		for (const FWEInventorySlotData& Slot : Source->GetAllSlots())
		{
			const FWEItemInstance& Candidate = Slot.Item;
			if (Candidate.Definition != Part || Candidate.Condition > EWEItemCondition::Damaged || Candidate.IsPartiallyDisassembled()
				|| Candidate.IsCustomerItem())
			{
				continue;
			}
			// Best condition first, then quality (a repair should work).
			if (!bFound || Candidate.Condition < BestCondition || (Candidate.Condition == BestCondition && Candidate.Quality > BestQuality))
			{
				bFound = true;
				BestCondition = Candidate.Condition;
				BestQuality = Candidate.Quality;
				OutInventory = const_cast<UWEInventoryComponent*>(Source);
				OutSlot = Slot.SlotIndex;
			}
		}
	}
	return bFound;
}

bool AWEWorkbench::CanStartRepairStep(const AWECharacter* Character, FText& OutReason) const
{
	const FWERepairStepInfo Info = GetNextRepairStep(OutReason);
	if (!Character || Info.Kind == EWERepairStepKind::None)
	{
		return false;
	}
	if (IsBusy())
	{
		OutReason = Work.Worker == Character ? LOCTEXT("RepairWorking", "Arbeite …") : LOCTEXT("RepairBusyOther", "Wird gerade bearbeitet");
		return false;
	}
	if (Character->GetHeldItem())
	{
		OutReason = LOCTEXT("RepairHandsFull", "Hände sind voll");
		return false;
	}
	if (FindToolSpeed(Character, Info.Tool) <= 0.f)
	{
		OutReason = FText::Format(LOCTEXT("RepairToolMissing", "Werkzeug fehlt: {0}"), WEItems::GetToolText(Info.Tool));
		return false;
	}
	UWEInventoryComponent* Inventory = nullptr;
	int32 Slot = INDEX_NONE;
	if (Info.Material && !FindSparePart(Character, Storage, Info.Material, Inventory, Slot))
	{
		OutReason = Info.Kind == EWERepairStepKind::Install
			? FText::Format(LOCTEXT("PartMissing", "Ersatzteil fehlt: {0}"), Info.Material->DisplayName)
			: FText::Format(LOCTEXT("MaterialMissing", "Material fehlt: {0}"), Info.Material->DisplayName);
		return false;
	}
	return true;
}

bool AWEWorkbench::TryStartRepairStep(AWECharacter* Character)
{
	FText Reason;
	if (!HasAuthority() || !CanStartRepairStep(Character, Reason))
	{
		UE_LOG(LogWENet, Log, TEXT("%s: repair step refused for %s: %s"), *GetName(), *GetNameSafe(Character), *Reason.ToString());
		return false;
	}
	const FWERepairStepInfo Info = GetNextRepairStep(Reason);
	const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	const float Speed = FindToolSpeed(Character, Info.Tool) * (Progression ? Progression->GetWorkSpeedMultiplier() : 1.f) * BenchSpeedMultiplier;

	Work.StepIndex = 0; // marks the bench busy; the repair kind says what is done
	Work.RepairKind = Info.Kind;
	Work.RepairFault = static_cast<int8>(Info.FaultIndex);
	Work.Worker = Character;
	Work.Duration = FMath::Max(0.1f, Info.Seconds / Speed);
	Work.StartServerTime = GetServerTime();
	GetWorldTimerManager().SetTimer(WorkTimer, this, &AWEWorkbench::CompleteActiveStep, Work.Duration, false);
	RaiseEvent(EWEWorkbenchEvent::StepStarted);
	UE_LOG(LogWENet, Log, TEXT("%s: %s started repair '%s' (%.1fs)"), *GetName(), *Character->GetName(), *Info.Text.ToString(), Work.Duration);
	return true;
}

void AWEWorkbench::CompleteRepairStep(AWECharacter* Worker)
{
	FWEItemInstance Item = PlacedItem->GetItem();
	const UWERepairDefinition* Repair = Item.Definition && Item.Definition->CanBeRepaired() ? Item.Definition->Repair.Get() : nullptr;
	const EWERepairStepKind Kind = Work.RepairKind;
	const int32 FaultIndex = Work.RepairFault;
	Work.StepIndex = INDEX_NONE;
	Work.Worker = nullptr;
	Work.RepairKind = EWERepairStepKind::None;
	Work.RepairFault = INDEX_NONE;
	if (!Repair)
	{
		RaiseEvent(EWEWorkbenchEvent::StepCancelled);
		return;
	}
	const FWERepairFault* Fault = Repair->GetFault(FaultIndex);
	const uint8 Bit = FaultIndex >= 0 && FaultIndex < 8 ? static_cast<uint8>(1 << FaultIndex) : 0;
	const FText Name = Item.Definition->DisplayName;
	EWEWorkbenchEvent Event = EWEWorkbenchEvent::StepCompleted;
	FText Message;

	switch (Kind)
	{
	case EWERepairStepKind::Diagnose:
	{
		EnsureFaultsRolled(Item);
		Item.Repair.SetFlag(FWERepairState::Diagnosed);
		Message = FText::Format(LOCTEXT("DiagnoseResult", "Diagnose: {0}"), GetRepairStatusText(Item));
		break;
	}
	case EWERepairStepKind::Remove:
	{
		if (!Fault)
		{
			break;
		}
		Item.Repair.Removed |= Bit;
		if (Fault->bReturnsBrokenPart && Fault->Part)
		{
			const uint8 BrokenQuality = static_cast<uint8>(Random.RandRange(5, 25));
			DeliverOutputs({ FWEItemInstance::Make(Fault->Part, 1, EWEItemCondition::Defective, BrokenQuality) }, Worker, /*bAnnounce*/ false);
			Message = FText::Format(LOCTEXT("RemovedPart", "Ausgebaut: {0} (defekt) – liegt im Werkbanklager"), Fault->Part->DisplayName);
		}
		else
		{
			Message = FText::Format(LOCTEXT("RemovedNoPart", "Erledigt: {0}"), Fault->RemoveText);
		}
		break;
	}
	case EWERepairStepKind::Install:
	{
		UWEInventoryComponent* Source = nullptr;
		int32 Slot = INDEX_NONE;
		FWEItemInstance Used;
		if (!Fault || !FindSparePart(Worker, Storage, Fault->Part, Source, Slot) || Source->RemoveFromSlot(Slot, 1, Used) != EWEInventoryResult::Success)
		{
			// The part was moved away while working.
			Message = FText::Format(LOCTEXT("PartGone", "Ersatzteil fehlt: {0}"), Fault ? Fault->Part->DisplayName : FText::GetEmpty());
			Event = EWEWorkbenchEvent::StepCancelled;
			break;
		}
		const bool bDud = Used.Condition >= EWEItemCondition::Damaged && Random.FRand() < Repair->DamagedPartFailChance;
		Item.Repair.Replaced |= Bit;
		if (!bDud)
		{
			Item.Repair.Faults &= ~Bit;
		}
		UE_LOG(LogWENet, Log, TEXT("%s: installed %s (%s)%s"), *GetName(), *Used.GetDisplayName().ToString(),
			*WEItems::GetConditionText(Used.Condition).ToString(), bDud ? TEXT(" - does not work") : TEXT(""));
		Message = FText::Format(LOCTEXT("Installed", "Eingebaut: {0} ({1})"), Used.GetDisplayName(), WEItems::GetConditionText(Used.Condition));
		break;
	}
	case EWERepairStepKind::Test:
	{
		if (Item.Repair.Faults == 0)
		{
			Item.Condition = FMath::Min(Item.Condition, EWEItemCondition::Used);
			Item.Quality = static_cast<uint8>(FMath::Min<int32>(90, Item.Quality + Repair->RepairQualityGain));
			Item.Repair.Removed = 0;
			Item.Repair.Replaced = 0;
			Event = EWEWorkbenchEvent::Finished;
			Message = FText::Format(LOCTEXT("TestPassed", "Funktionstest bestanden: {0} läuft wieder ({1}, Qualität {2} %)"),
				Name, WEItems::GetConditionText(Item.Condition), FText::AsNumber(Item.Quality));
			UE_LOG(LogWENet, Log, TEXT("%s: %s repaired"), *GetName(), *Name.ToString());
			if (UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this)) { Progression->NotifyEvent(EWEProgressEvent::DisassemblyFinished); }
		}
		else
		{
			TArray<FString> Failed;
			for (int32 i = 0; i < Repair->Faults.Num() && i < UWERepairDefinition::MaxFaults; ++i)
			{
				if (FWERepairState::HasBit(Item.Repair.Faults, i))
				{
					Failed.Add(Repair->Faults[i].DisplayName.ToString());
				}
			}
			// The bad spare part has to come out again.
			Item.Repair.Replaced &= ~Item.Repair.Faults;
			Item.Repair.Removed &= ~Item.Repair.Faults;
			Event = EWEWorkbenchEvent::StepCancelled;
			Message = FText::Format(LOCTEXT("TestFailed", "Funktionstest fehlgeschlagen: {0} – das Ersatzteil taugt nichts"),
				FText::FromString(FString::Join(Failed, TEXT(", "))));
		}
		break;
	}
	case EWERepairStepKind::Refurbish:
	{
		UWEInventoryComponent* Source = nullptr;
		int32 Slot = INDEX_NONE;
		FWEItemInstance Used;
		if (!FindSparePart(Worker, Storage, Repair->RefurbishMaterial, Source, Slot) || Source->RemoveFromSlot(Slot, 1, Used) != EWEInventoryResult::Success)
		{
			Message = FText::Format(LOCTEXT("MaterialGone", "Material fehlt: {0}"),
				Repair->RefurbishMaterial ? Repair->RefurbishMaterial->DisplayName : FText::GetEmpty());
			Event = EWEWorkbenchEvent::StepCancelled;
			break;
		}
		Item.Quality = static_cast<uint8>(FMath::Min<int32>(95, Item.Quality + Repair->RefurbishQualityGain));
		if (Item.Quality >= 80 && Item.Condition == EWEItemCondition::Used)
		{
			Item.Condition = EWEItemCondition::VeryGood;
		}
		Item.Repair.SetFlag(FWERepairState::Refurbished);
		Event = EWEWorkbenchEvent::Finished;
		Message = FText::Format(LOCTEXT("Refurbished", "Aufbereitet: {0} ({1}, Qualität {2} %)"),
			Name, WEItems::GetConditionText(Item.Condition), FText::AsNumber(Item.Quality));
		break;
	}
	default:
		break;
	}

	PlacedItem->SetItem(Item);
	RaiseEvent(Event);
	if (!Message.IsEmpty())
	{
		ShowMessage(Worker, Message);
	}
}

FText AWEWorkbench::GetRepairStatusText(const FWEItemInstance& Item)
{
	const UWEItemDefinition* Def = Item.Definition;
	const UWERepairDefinition* Repair = Def && Def->CanBeRepaired() ? Def->Repair.Get() : nullptr;
	if (!Repair)
	{
		return FText::GetEmpty();
	}
	if (!Item.IsBroken())
	{
		return Item.Repair.HasFlag(FWERepairState::Refurbished) ? LOCTEXT("StatusRefurbished", "funktioniert, aufbereitet") : LOCTEXT("StatusWorks", "funktioniert");
	}
	if (!Item.Repair.HasFlag(FWERepairState::Diagnosed))
	{
		return LOCTEXT("StatusUnknown", "Fehler unbekannt – erst Fehler suchen");
	}
	TArray<FString> Lines;
	for (int32 i = 0; i < Repair->Faults.Num() && i < UWERepairDefinition::MaxFaults; ++i)
	{
		const FString FaultName = Repair->Faults[i].DisplayName.ToString();
		if (FWERepairState::HasBit(Item.Repair.Replaced, i))
		{
			Lines.Add(FString::Printf(TEXT("%s – ersetzt, Test ausstehend"), *FaultName));
		}
		else if (FWERepairState::HasBit(Item.Repair.Faults, i))
		{
			Lines.Add(FWERepairState::HasBit(Item.Repair.Removed, i) ? FaultName + TEXT(" (ausgebaut)") : FaultName);
		}
	}
	if (Lines.Num() == 0)
	{
		return LOCTEXT("StatusReadyForTest", "bereit für den Funktionstest");
	}
	return FText::FromString(FString::Join(Lines, TEXT(", ")));
}

#undef LOCTEXT_NAMESPACE
