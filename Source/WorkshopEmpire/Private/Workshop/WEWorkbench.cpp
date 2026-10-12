#include "Workshop/WEWorkbench.h"

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

void AWEWorkbench::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
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
	if (!Def->CanBeDisassembled())
	{
		OutReason = LOCTEXT("NotDisassemblable", "Lässt sich nicht zerlegen");
		return false;
	}
	if (PlacedItem && PlacedItem != Item)
	{
		OutReason = LOCTEXT("Occupied", "Werkbank ist belegt");
		return false;
	}
	if (Item->GetItem().DisassemblyStep >= Def->Disassembly->NumSteps())
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
	const float Speed = FindToolSpeed(Character, Step->RequiredTool);

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
	const FWEDisassemblyStep* Step = GetNextStep();
	if (!PlacedItem || !Step || !IsValid(Worker) || !UWEInteractionComponent::IsWithinReach(Worker, this, WorkReach))
	{
		UE_LOG(LogWENet, Log, TEXT("%s: step cancelled (worker gone or out of reach)"), *GetName());
		if (AWEPlayerController* PC = Worker ? Cast<AWEPlayerController>(Worker->GetController()) : nullptr)
		{
			PC->ClientShowMessage(LOCTEXT("Cancelled", "Arbeitsschritt abgebrochen – zu weit weg"));
		}
		CancelActiveStep();
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
		const int32 Quantity = Random.RandRange(FMath::Min(Output.MinQuantity, Output.MaxQuantity), FMath::Max(Output.MinQuantity, Output.MaxQuantity));
		if (Quantity <= 0)
		{
			continue;
		}
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
		// Stackable outputs may exceed one stack; split so every stack is valid.
		int32 Remaining = Quantity;
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

void AWEWorkbench::DeliverOutputs(const TArray<FWEItemInstance>& Outputs, AWECharacter* Worker)
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
	if (AWEPlayerController* PC = Worker ? Cast<AWEPlayerController>(Worker->GetController()) : nullptr)
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
	}
	return true;
}

#undef LOCTEXT_NAMESPACE
