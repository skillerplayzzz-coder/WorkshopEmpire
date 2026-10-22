#include "Workshop/WEWorkbench.h"
#include "Framework/WEGameRulesComponent.h"
#include "World/WEToolWall.h"
#include "Fx/WEFx.h"

#include "Camera/CameraComponent.h"
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
#include "Progression/WEMilestoneComponent.h"
#include "Progression/WEProgressionComponent.h"
#include "Progression/WETutorialComponent.h"
#include "World/WEPlaceholderVisuals.h"
#include "Workshop/WEWorkPlan.h"
#include "Workshop/WEAssemblyDefinition.h"
#include "Workshop/WEAssemblyView.h"
#include "Workshop/WEBenchmark.h"

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
	// Room for a completely taken-apart PC (parts, screws, scrap) plus what was there before.
	StorageSlots = 24;
	StorageMaxWeightKg = 100.f;

	WorkPoint = CreateDefaultSubobject<USceneComponent>(TEXT("WorkPoint"));
	WorkPoint->SetupAttachment(RootComponent);
	WorkPoint->SetMobility(EComponentMobility::Movable);

	ScrewBowl = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScrewBowl"));
	ScrewBowl->SetupAttachment(RootComponent);
	ScrewBowl->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ScrewBowl->SetCanEverAffectNavigation(false);

	// Work view camera (Phase 17.5): aimed at the device when the view opens; only used as a view target.
	WorkCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("WorkCamera"));
	WorkCamera->SetupAttachment(RootComponent);
	WorkCamera->SetFieldOfView(52.f);
	WorkCamera->bConstrainAspectRatio = false;
	// Phase 18a: the camera orbits and zooms to parts - no motion blur smearing the small details.
	WorkCamera->PostProcessSettings.bOverride_MotionBlurAmount = true;
	WorkCamera->PostProcessSettings.MotionBlurAmount = 0.f;

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
	// Phase 21: magnetic screw bowl at the front edge, right of the work spot.
	if (!ScrewBowl->GetStaticMesh())
	{
		ScrewBowl->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_ScrewBowl.SM_Gen_ScrewBowl"), nullptr, LOAD_Quiet | LOAD_NoWarn));
	}
	ScrewBowl->SetRelativeLocation(FVector(Size.X * 0.5f - 11.f, Size.Y * 0.04f, Size.Z));
	ScrewBowl->SetWorldScale3D(FVector::OneVector);
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
	if (AssemblyView)
	{
		AssemblyView->Destroy();
		AssemblyView = nullptr;
	}
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
	if (const UWEAssemblyDefinition* Assembly = GetAssembly())
	{
		// Devices with a part layout lie the way one works on them (PC: on its side, open side up), resting on the top.
		if (const UStaticMesh* DeviceMesh = Item->GetMesh() ? Item->GetMesh()->GetStaticMesh() : nullptr)
		{
			const FBox Local = DeviceMesh->GetBoundingBox();
			const FBox Turned = Local.TransformBy(FTransform(Assembly->BenchRotation, FVector::ZeroVector, Item->GetMesh()->GetRelativeScale3D()));
			Item->SetActorRelativeRotation(Assembly->BenchRotation);
			Item->SetActorRelativeLocation(FVector(-Turned.GetCenter().X, -Turned.GetCenter().Y, -Turned.Min.Z + 0.3f));
		}
	}
	ApplyKnownFaults();
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

FVector AWEWorkbench::GetScrewBowlLocation() const
{
	return ScrewBowl ? ScrewBowl->GetComponentLocation() + FVector(0.f, 0.f, 0.4f) : GetWorkSpotLocation();
}

FVector AWEWorkbench::GetToolRestLocation(int32 Slot) const
{
	// Tools lie side by side along the front edge, right of the bowl.
	return GetActorTransform().TransformPosition(FVector(Size.X * 0.5f - 9.f, Size.Y * 0.04f + 14.f + Slot * 7.5f, Size.Z));
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
	if (!PlacedItem)
	{
		return INDEX_NONE;
	}
	const FWEItemInstance& Item = PlacedItem->GetItem();
	if (const UWEAssemblyDefinition* Assembly = GetAssembly())
	{
		// Parts come out in any order; the default (employees, "automatic") is the first one that is free.
		return Assembly->GetDefaultPart(Item.RemovedParts);
	}
	return Item.DisassemblyStep;
}

const FWEDisassemblyStep* AWEWorkbench::GetNextStep() const
{
	return GetStepAt(INDEX_NONE);
}

const UWEAssemblyDefinition* AWEWorkbench::GetAssembly() const
{
	const UWEItemDefinition* Def = PlacedItem ? PlacedItem->GetItem().Definition.Get() : nullptr;
	return Def && Def->HasAssembly() ? Def->Assembly.Get() : nullptr;
}

const FWEDisassemblyStep* AWEWorkbench::GetStepAt(int32 Part) const
{
	const UWEItemDefinition* Def = PlacedItem ? PlacedItem->GetItem().Definition.Get() : nullptr;
	return Def && Def->Disassembly ? Def->Disassembly->GetStep(Part == INDEX_NONE ? GetNextStepIndex() : Part) : nullptr;
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
	// Phase 21: the own inventory plus tool walls within reach.
	TArray<const UWEItemDefinition*> Tools;
	AWEToolWall::GatherTools(Character, Tools);
	float Best = 0.f;
	for (const UWEItemDefinition* Def : Tools)
	{
		if (Def->ToolType == Tool)
		{
			Best = FMath::Max(Best, Def->ToolSpeedMultiplier);
		}
	}
	return Best;
}

bool AWEWorkbench::CanStartStep(const AWECharacter* Character, FText& OutReason, int32 Part) const
{
	const FWEDisassemblyStep* Step = GetStepAt(Part);
	if (!Character || !Step)
	{
		return false;
	}
	if (const UWEAssemblyDefinition* Assembly = GetAssembly(); Assembly && Part != INDEX_NONE)
	{
		const int32 Removed = PlacedItem->GetItem().RemovedParts;
		if ((Removed & (1 << Part)) != 0)
		{
			OutReason = LOCTEXT("PartOut", "Schon ausgebaut");
			return false;
		}
		const TArray<FText> Blockers = Assembly->GetBlockers(Part, Removed);
		if (Blockers.Num() > 0)
		{
			OutReason = FText::Format(LOCTEXT("PartBlocked", "Erst ausbauen: {0}"), FText::Join(FText::FromString(TEXT(", ")), Blockers));
			return false;
		}
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
	// Customer devices are not taken apart - except a PC brought in for an upgrade (its parts may be swapped, not the case).
	const bool bUpgradeOrder = PlacedItem->GetItem().Repair.HasFlag(FWERepairState::CustomerUpgrade);
	const UWEAssemblyDefinition* PartLayout = GetAssembly();
	const int32 PartIndex = Part != INDEX_NONE ? Part : GetNextStepIndex();
	if (PlacedItem->GetItem().IsCustomerItem() && (!bUpgradeOrder || !PartLayout || !PartLayout->Parts.IsValidIndex(PartIndex) || PartLayout->Parts[PartIndex].bShell))
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

bool AWEWorkbench::TryStartStep(AWECharacter* Character, int32 Part)
{
	FText Reason;
	if (!HasAuthority() || !CanStartStep(Character, Reason, Part))
	{
		UE_LOG(LogWENet, Log, TEXT("%s: step refused for %s: %s"), *GetName(), *GetNameSafe(Character), *Reason.ToString());
		return false;
	}
	const int32 StepIndex = Part != INDEX_NONE ? Part : GetNextStepIndex();
	const FWEDisassemblyStep* Step = GetStepAt(StepIndex);
	const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	const float Speed = FindToolSpeed(Character, Step->RequiredTool) * (Progression ? Progression->GetWorkSpeedMultiplier() : 1.f) * BenchSpeedMultiplier
		* (Character ? Character->GetWorkBoost() : 1.f);

	// A broken device has broken parts: decide which ones now, so they come out "Defekt".
	FWEItemInstance Item = PlacedItem->GetItem();
	if (!Item.Repair.HasFlag(FWERepairState::FaultsRolled))
	{
		EnsureFaultsRolled(Item);
		PlacedItem->SetItem(Item);
	}

	Work.RepairKind = EWERepairStepKind::None;
	Work.RepairFault = INDEX_NONE;
	Work.bInstall = false;
	Work.StepIndex = StepIndex;
	Work.Worker = Character;
	Work.Duration = FMath::Max(0.1f, Step->BaseSeconds * UWEGameRulesComponent::GetRules(this).RepairTimeFactor() / Speed);
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
	ClearManualState();
	Work.StepIndex = INDEX_NONE;
	Work.Worker = nullptr;
	Work.RepairKind = EWERepairStepKind::None;
	Work.RepairFault = INDEX_NONE;
	Work.bInstall = false;
	RaiseEvent(EWEWorkbenchEvent::StepCancelled);
}

void AWEWorkbench::ClearManualState()
{
	Work.bManual = false;
	Work.TaskCount = 0;
	Work.TaskMask = 0;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ManualIdleTimer);
	}
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
	ClearManualState();
	// Tutorial: a player finished a step by hand or with the timer.
	if (UWETutorialComponent* Tutorial = IsValid(Worker) && Worker->IsPlayerControlled() ? UWETutorialComponent::Get(this) : nullptr)
	{
		Tutorial->NotifyEvent(bRepair ? EWETutorialEvent::RepairStep : EWETutorialEvent::DisassemblyStep);
	}
	const FWEDisassemblyStep* Step = bRepair ? nullptr : GetStepAt(Work.StepIndex);
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
	if (Work.bInstall)
	{
		CompleteInstall(Worker);
		return;
	}

	FWEItemInstance Source = PlacedItem->GetItem();
	const TArray<FWEItemInstance> Outputs = GenerateOutputs(*Step, Source, GetAssembly() ? Work.StepIndex : INDEX_NONE);
	const int32 Total = GetNumSteps();
	bool bFinished = false;
	if (const UWEAssemblyDefinition* Assembly = GetAssembly())
	{
		// Part layout: this part is out; the device is used up when the last one (the case) is.
		Source.RemovedParts |= (1 << Work.StepIndex);
		Source.DisassemblyStep = Assembly->CountTakenApart(Source.RemovedParts);
		bFinished = Assembly->IsComplete(Source.RemovedParts);
		// The part took its state with it: a fault it had (or a part that was put in later) is no longer in the device.
		Source.SetInstalledCondition(Work.StepIndex, INDEX_NONE);
		Source.SetPartVariant(Work.StepIndex, 0);
		if (const UWERepairDefinition* Repair = Source.Definition->CanBeRepaired() ? Source.Definition->Repair.Get() : nullptr)
		{
			for (const FWEDisassemblyOutput& Out : Step->Outputs)
			{
				for (int32 f = 0; f < Repair->Faults.Num() && f < UWERepairDefinition::MaxFaults; ++f)
				{
					if (Out.bInheritCondition && Out.Item && Repair->Faults[f].Part == Out.Item)
					{
						const uint8 Bit = static_cast<uint8>(1 << f);
						Source.Repair.Faults &= ~Bit;
						Source.Repair.Removed &= ~Bit;
						Source.Repair.Replaced &= ~Bit;
					}
				}
			}
		}
	}
	else
	{
		Source.DisassemblyStep = static_cast<uint8>(FMath::Min(Work.StepIndex + 1, 255));
		bFinished = Source.DisassemblyStep >= Total;
	}
	UE_LOG(LogWENet, Log, TEXT("%s: step %d/%d done ('%s'), %d output stacks"), *GetName(), Source.DisassemblyStep, Total,
		*Step->ActionText.ToString(), Outputs.Num());

	Work.StepIndex = INDEX_NONE;
	Work.Worker = nullptr;

	if (bFinished)
	{
		// Fully disassembled: the item is used up.
		AWEPickupItem* Done = PlacedItem;
		PlacedItem = nullptr;
		const FText Name = Done->GetItem().Definition->DisplayName;
		Done->Destroy();
		DeliverOutputs(Outputs, Worker);
		RaiseEvent(EWEWorkbenchEvent::Finished);
		if (UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this)) { Progression->NotifyEvent(EWEProgressEvent::DisassemblyFinished); }
		if (UWEMilestoneComponent* Milestones = UWEMilestoneComponent::Get(this)) { Milestones->AddCount(EWEMilestoneKind::DevicesDisassembled); }
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

TArray<FWEItemInstance> AWEWorkbench::GenerateOutputs(const FWEDisassemblyStep& Step, const FWEItemInstance& Source, int32 Part)
{
	TArray<FWEItemInstance> Result;
	for (const FWEDisassemblyOutput& Output : Step.Outputs)
	{
		if (!Output.Item || Random.FRand() > Output.Chance)
		{
			continue;
		}
		int32 Quantity = Random.RandRange(FMath::Min(Output.MinQuantity, Output.MaxQuantity), FMath::Max(Output.MinQuantity, Output.MaxQuantity));
		// A part that was put in during reassembly comes out the way it went in (the better variant too).
		const int32 InstalledCondition = Output.bInheritCondition ? Source.GetInstalledCondition(Part) : INDEX_NONE;
		if (InstalledCondition != INDEX_NONE)
		{
			UWEItemDefinition* OutItem = Output.Item;
			const UWEAssemblyDefinition* Assembly = Source.Definition->HasAssembly() ? Source.Definition->Assembly.Get() : nullptr;
			const int32 Variant = Source.GetPartVariant(Part);
			if (Assembly && Assembly->Parts.IsValidIndex(Part) && Variant > 0 && Assembly->Parts[Part].Alternatives.IsValidIndex(Variant - 1) && Assembly->Parts[Part].Alternatives[Variant - 1])
			{
				OutItem = Assembly->Parts[Part].Alternatives[Variant - 1];
			}
			const EWEItemCondition Condition = static_cast<EWEItemCondition>(FMath::Clamp(InstalledCondition, 0, static_cast<int32>(EWEItemCondition::Scrap)));
			const uint8 Quality = static_cast<uint8>(FMath::Clamp(static_cast<int32>(Source.Quality) + Random.RandRange(-5, 5), 0, 100));
			for (int32 Remaining = Quantity; Remaining > 0;)
			{
				const int32 Count = FMath::Min(Remaining, OutItem->GetMaxStackSize());
				Result.Add(FWEItemInstance::Make(OutItem, Count, Condition, Quality));
				Remaining -= Count;
			}
			continue;
		}
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
	ClearManualState();
	Work.StepIndex = INDEX_NONE;
	Work.Worker = nullptr;
	Work.RepairKind = EWERepairStepKind::None;
	Work.RepairFault = INDEX_NONE;
	Work.bInstall = false;
	if (PlacedItem)
	{
		PlacedItem->HandleRemovedFromWorkbench();
		PlacedItem = nullptr;
	}
	ForceNetUpdate();
}

// --- Reassembly ------------------------------------------------------------------------------------------

TArray<FWEInstallNeed> AWEWorkbench::GetInstallNeeds(const UWEItemDefinition* Device, int32 Part)
{
	TArray<FWEInstallNeed> Needs;
	const UWEAssemblyDefinition* Assembly = Device && Device->HasAssembly() ? Device->Assembly.Get() : nullptr;
	const FWEDisassemblyStep* Step = Assembly && Assembly->Parts.IsValidIndex(Part) ? Device->Disassembly->GetStep(Part) : nullptr;
	if (!Step || Assembly->Parts[Part].bShell)
	{
		return Needs;
	}
	bool bHasPart = false;
	for (const FWEDisassemblyOutput& Out : Step->Outputs)
	{
		bHasPart |= Out.Item && Out.bInheritCondition;
	}
	// A part that is scrapped on removal (CD drive: electronics scrap and plastic) cannot go back; a cover can (only screws).
	if (!bHasPart && !Assembly->Parts[Part].bLaysBeside)
	{
		return Needs;
	}
	for (const FWEDisassemblyOutput& Out : Step->Outputs)
	{
		// What reliably comes out of the part goes back in: the part itself and its fixed hardware (screws).
		if (Out.Item && Out.Chance >= 1.f && (Out.bInheritCondition || Out.MinQuantity == Out.MaxQuantity))
		{
			FWEInstallNeed& Need = Needs.AddDefaulted_GetRef();
			Need.Item = Out.Item;
			Need.Count = FMath::Max(1, Out.MinQuantity);
			Need.bPart = Out.bInheritCondition;
			Need.Accepts.Add(Out.Item);
			if (Need.bPart)
			{
				for (const TObjectPtr<UWEItemDefinition>& Alt : Assembly->Parts[Part].Alternatives)
				{
					Need.Accepts.Add(Alt.Get());
				}
			}
		}
	}
	return Needs;
}

bool AWEWorkbench::IsReinstallable(int32 Part) const
{
	const UWEItemDefinition* Def = PlacedItem ? PlacedItem->GetItem().Definition.Get() : nullptr;
	return GetAssembly() && GetInstallNeeds(Def, Part).Num() > 0;
}

bool AWEWorkbench::HasInstallablePart() const
{
	const UWEAssemblyDefinition* Assembly = GetAssembly();
	if (!Assembly || (PlacedItem->GetItem().IsCustomerItem() && !PlacedItem->GetItem().Repair.HasFlag(FWERepairState::CustomerUpgrade)))
	{
		return false;
	}
	for (int32 i = 0; i < Assembly->Parts.Num(); ++i)
	{
		if (IsReinstallable(i) && Assembly->CanInstall(i, PlacedItem->GetItem().RemovedParts))
		{
			return true;
		}
	}
	return false;
}

bool AWEWorkbench::FindInstallItems(const AWECharacter* Character, const UWEInventoryComponent* InStorage, const UWEItemDefinition* Item,
	int32 Count, TArray<TPair<UWEInventoryComponent*, int32>>& OutSlots)
{
	OutSlots.Reset();
	if (!Item || Count <= 0)
	{
		return Count <= 0;
	}
	struct FCandidate
	{
		UWEInventoryComponent* Inventory;
		int32 Slot;
		int32 Quantity;
		EWEItemCondition Condition;
		uint8 Quality;
	};
	TArray<FCandidate> Candidates;
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
			if (Candidate.Definition == Item && !Candidate.IsCustomerItem() && !Candidate.IsPartiallyDisassembled() && !Candidate.IsLocked())
			{
				Candidates.Add({ const_cast<UWEInventoryComponent*>(Source), Slot.SlotIndex, Candidate.Quantity, Candidate.Condition, Candidate.Quality });
			}
		}
	}
	// The best parts go in first (stable: inventory before bench storage when equal).
	Candidates.StableSort([](const FCandidate& A, const FCandidate& B) { return A.Condition != B.Condition ? A.Condition < B.Condition : A.Quality > B.Quality; });
	int32 Found = 0;
	for (const FCandidate& C : Candidates)
	{
		if (Found >= Count)
		{
			break;
		}
		OutSlots.Add({ C.Inventory, C.Slot });
		Found += C.Quantity;
	}
	return Found >= Count;
}

int32 AWEWorkbench::PickInstallVariant(const AWECharacter* Character, const UWEInventoryComponent* InStorage, const FWEInstallNeed& Need)
{
	// The best variant there is enough of goes in (a better graphics card before the old one).
	for (int32 v = Need.Accepts.Num() - 1; v >= 0; --v)
	{
		TArray<TPair<UWEInventoryComponent*, int32>> Slots;
		if (Need.Accepts[v] && FindInstallItems(Character, InStorage, Need.Accepts[v], Need.Count, Slots))
		{
			return v;
		}
	}
	return INDEX_NONE;
}

bool AWEWorkbench::CanStartInstall(const AWECharacter* Character, FText& OutReason, int32 Part) const
{
	const UWEAssemblyDefinition* Assembly = GetAssembly();
	const FWEDisassemblyStep* Step = GetStepAt(Part);
	if (!Character || !Assembly || !Step || !Assembly->Parts.IsValidIndex(Part))
	{
		return false;
	}
	const FWEItemInstance& Item = PlacedItem->GetItem();
	if (Item.IsCustomerItem() && !Item.Repair.HasFlag(FWERepairState::CustomerUpgrade))
	{
		OutReason = LOCTEXT("CustomerNoBuild", "Kundengerät – nicht umbauen");
		return false;
	}
	if (!Item.IsPartRemoved(Part))
	{
		OutReason = LOCTEXT("InstallAlreadyIn", "Ist schon eingebaut");
		return false;
	}
	const TArray<FWEInstallNeed> Needs = GetInstallNeeds(Item.Definition, Part);
	if (Needs.Num() == 0)
	{
		OutReason = LOCTEXT("InstallScrapped", "Wurde beim Ausbau verwertet – lässt sich nicht wieder einbauen");
		return false;
	}
	const FText Blocker = Assembly->GetInstallBlocker(Part, Item.RemovedParts);
	if (!Blocker.IsEmpty())
	{
		OutReason = Blocker;
		return false;
	}
	if (IsBusy())
	{
		OutReason = Work.Worker == Character ? LOCTEXT("InstallWorking", "Arbeite …") : LOCTEXT("InstallBusyOther", "Wird gerade bearbeitet");
		return false;
	}
	if (Character->GetHeldItem())
	{
		OutReason = LOCTEXT("InstallHandsFull", "Hände sind voll");
		return false;
	}
	if (FindToolSpeed(Character, Step->RequiredTool) <= 0.f)
	{
		OutReason = FText::Format(LOCTEXT("InstallToolMissing", "Werkzeug fehlt: {0}"), WEItems::GetToolText(Step->RequiredTool));
		return false;
	}
	TArray<FString> Missing;
	for (const FWEInstallNeed& Need : Needs)
	{
		if (PickInstallVariant(Character, Storage, Need) == INDEX_NONE)
		{
			const FString Name = Need.Accepts.Num() > 1 ? FString::Printf(TEXT("%s (oder besser)"), *Need.Item->DisplayName.ToString()) : Need.Item->DisplayName.ToString();
			Missing.Add(Need.Count > 1 ? FString::Printf(TEXT("%d× %s"), Need.Count, *Name) : Name);
		}
	}
	if (Missing.Num() > 0)
	{
		OutReason = FText::Format(LOCTEXT("InstallMissing", "Fehlt (Inventar oder Werkbanklager): {0}"), FText::FromString(FString::Join(Missing, TEXT(", "))));
		return false;
	}
	return true;
}

bool AWEWorkbench::TryStartInstall(AWECharacter* Character, int32 Part)
{
	FText Reason;
	if (!HasAuthority() || !CanStartInstall(Character, Reason, Part))
	{
		UE_LOG(LogWENet, Log, TEXT("%s: install refused for %s: %s"), *GetName(), *GetNameSafe(Character), *Reason.ToString());
		return false;
	}
	const FWEDisassemblyStep* Step = GetStepAt(Part);
	const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	const float Speed = FindToolSpeed(Character, Step->RequiredTool) * (Progression ? Progression->GetWorkSpeedMultiplier() : 1.f) * BenchSpeedMultiplier
		* (Character ? Character->GetWorkBoost() : 1.f);
	FWEItemInstance Item = PlacedItem->GetItem();
	if (!Item.Repair.HasFlag(FWERepairState::FaultsRolled))
	{
		EnsureFaultsRolled(Item);
		PlacedItem->SetItem(Item);
	}
	Work.RepairKind = EWERepairStepKind::None;
	Work.RepairFault = INDEX_NONE;
	Work.bInstall = true;
	Work.StepIndex = Part;
	Work.Worker = Character;
	Work.Duration = FMath::Max(0.1f, Step->BaseSeconds * UWEGameRulesComponent::GetRules(this).RepairTimeFactor() / Speed);
	Work.StartServerTime = GetServerTime();
	GetWorldTimerManager().SetTimer(WorkTimer, this, &AWEWorkbench::CompleteActiveStep, Work.Duration, false);
	RaiseEvent(EWEWorkbenchEvent::StepStarted);
	UE_LOG(LogWENet, Log, TEXT("%s: %s installs part %d (%.1fs)"), *GetName(), *Character->GetName(), Part, Work.Duration);
	return true;
}

void AWEWorkbench::CompleteInstall(AWECharacter* Worker)
{
	const int32 Part = Work.StepIndex;
	Work.StepIndex = INDEX_NONE;
	Work.Worker = nullptr;
	Work.bInstall = false;
	const UWEAssemblyDefinition* Assembly = GetAssembly();
	FWEItemInstance Item = PlacedItem->GetItem();
	const TArray<FWEInstallNeed> Needs = GetInstallNeeds(Item.Definition, Part);
	// Everything has to be there (it may have been moved away while working); then it is used up.
	TArray<int32> Variants;
	for (const FWEInstallNeed& Need : Needs)
	{
		const int32 Variant = PickInstallVariant(Worker, Storage, Need);
		if (Variant == INDEX_NONE)
		{
			ShowMessage(Worker, FText::Format(LOCTEXT("InstallGone", "Fehlt: {0}"), Need.Item->DisplayName));
			RaiseEvent(EWEWorkbenchEvent::StepCancelled);
			return;
		}
		Variants.Add(Variant);
	}
	int32 Worst = INDEX_NONE;
	int32 QualitySum = 0;
	int32 Units = 0;
	int32 PartVariant = 0;
	FText PartName;
	for (int32 n = 0; n < Needs.Num(); ++n)
	{
		const FWEInstallNeed& Need = Needs[n];
		if (Need.bPart)
		{
			PartVariant = Variants[n];
		}
		TArray<TPair<UWEInventoryComponent*, int32>> Slots;
		FindInstallItems(Worker, Storage, Need.Accepts[Variants[n]], Need.Count, Slots);
		int32 Left = Need.Count;
		for (const TPair<UWEInventoryComponent*, int32>& Slot : Slots)
		{
			FWEItemInstance Used;
			const FWEItemInstance* InSlot = Slot.Key->GetItemInSlot(Slot.Value);
			const int32 Take = InSlot ? FMath::Min(Left, InSlot->Quantity) : 0;
			if (Take <= 0 || Slot.Key->RemoveFromSlot(Slot.Value, Take, Used) != EWEInventoryResult::Success)
			{
				continue;
			}
			Left -= Take;
			if (Need.bPart)
			{
				Worst = FMath::Max(Worst, static_cast<int32>(Used.Condition));
				QualitySum += Used.Quality * Take;
				Units += Take;
				PartName = Used.Definition->DisplayName;
			}
		}
	}
	Item.RemovedParts &= ~(1 << Part);
	Item.DisassemblyStep = Assembly->CountTakenApart(Item.RemovedParts);
	if (Units > 0)
	{
		Item.SetInstalledCondition(Part, Worst);
		Item.SetPartVariant(Part, PartVariant);
		// The device's quality follows its parts.
		const int32 Inside = FMath::Max(1, Assembly->Parts.Num() - FMath::CountBits(static_cast<uint32>(Item.RemovedParts)) - 1);
		Item.Quality = static_cast<uint8>(FMath::Clamp((Item.Quality * Inside + QualitySum / Units) / (Inside + 1), 0, 100));
		// Whatever was wrong with the part that used to sit here left with it.
		if (const UWERepairDefinition* Repair = Item.Definition->CanBeRepaired() ? Item.Definition->Repair.Get() : nullptr)
		{
			for (const FWEInstallNeed& Need : Needs)
			{
				for (int32 f = 0; f < Repair->Faults.Num() && f < UWERepairDefinition::MaxFaults; ++f)
				{
					if (Need.bPart && Repair->Faults[f].Part == Need.Item)
					{
						const uint8 Bit = static_cast<uint8>(1 << f);
						Item.Repair.Faults &= ~Bit;
						Item.Repair.Removed &= ~Bit;
						Item.Repair.Replaced &= ~Bit;
					}
				}
			}
		}
	}
	if (!Assembly->Parts[Part].bServiceAccess)
	{
		Item.Repair.SetFlag(FWERepairState::NeedsTest);
	}
	PlacedItem->SetItem(Item);
	RaiseEvent(EWEWorkbenchEvent::StepCompleted);
	UE_LOG(LogWENet, Log, TEXT("%s: part %d (%s) installed%s"), *GetName(), Part, *Assembly->Parts[Part].DisplayName.ToString(),
		Units > 0 ? *FString::Printf(TEXT(", condition %d"), Worst) : TEXT(""));
	if (Worker && Worker->IsPlayerControlled())
	{
		ShowMessage(Worker, Units > 0
			? FText::Format(LOCTEXT("InstalledPart", "Eingebaut: {0} ({1})"), PartName, WEItems::GetConditionText(static_cast<EWEItemCondition>(Worst)))
			: FText::Format(LOCTEXT("InstalledCover", "Angebracht: {0}"), Assembly->Parts[Part].DisplayName));
	}
}

float AWEWorkbench::GetProgress() const
{
	if (!IsBusy() || Work.Duration <= 0.f)
	{
		return 0.f;
	}
	if (Work.bManual)
	{
		// Hands-on: share of the tasks done.
		return Work.TaskCount > 0 ? FMath::Clamp(FMath::CountBits(Work.TaskMask) / static_cast<float>(Work.TaskCount), 0.f, 1.f) : 0.f;
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
		// (Phase 21: not on devices taken apart part by part - their parts move visibly, and the white puffs filled the close-up.)
		if (!GetAssemblyView())
		{
			AWEFx::Burst(this, EWEFxKind::Dust, At, FVector::UpVector, FLinearColor(0.5f, 0.47f, 0.42f), FxScale);
			AWEFx::Burst(this, EWEFxKind::Chips, At, FVector::UpVector, FLinearColor(0.35f, 0.36f, 0.38f), 0.35f * FxScale);
		}
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
	case EWEWorkbenchEvent::TaskDone:      Sound = SoundStepStart; break;
	default: break;
	}
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, WorkPoint->GetComponentLocation());
	}
}

void AWEWorkbench::UpdateAssemblyView()
{
	// A device with a part layout on the bench is shown part by part (on every machine that renders).
	AWEPickupItem* Device = PlacedItem;
	const bool bWant = Device && Device->GetItem().Definition && Device->GetItem().Definition->HasAssembly();
	if (AssemblyView && (!bWant || AssemblyView->GetItem() != Device))
	{
		AssemblyView->Destroy();
		AssemblyView = nullptr;
	}
	if (bWant && !AssemblyView && GetWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AssemblyView = GetWorld()->SpawnActor<AWEAssemblyView>(Device->GetActorLocation(), Device->GetActorRotation(), Params);
		if (AssemblyView)
		{
			AssemblyView->Init(this, Device);
		}
	}
}

void AWEWorkbench::UpdateIndicator()
{
	UpdateAssemblyView();
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

void AWEWorkbench::ApplyKnownFaults()
{
	// Phase 21, rule "Diagnose erforderlich" off: what is broken is known as soon as the device lies here.
	if (!HasAuthority() || !PlacedItem || UWEGameRulesComponent::GetRules(this).bDiagnosisRequired)
	{
		return;
	}
	FWEItemInstance Item = PlacedItem->GetItem();
	if (!Item.Definition || !Item.Definition->CanBeRepaired() || !Item.IsBroken() || Item.Repair.HasFlag(FWERepairState::Diagnosed))
	{
		return;
	}
	EnsureFaultsRolled(Item);
	Item.Repair.SetFlag(FWERepairState::Diagnosed);
	PlacedItem->SetItem(Item);
}

bool AWEWorkbench::RollMistake(AWECharacter* Worker, EWEToolType Tool)
{
	// Phase 21, rule "Reparaturfehler": a better tool slips less often.
	const float Chance = UWEGameRulesComponent::GetRules(this).MistakeChance() / FMath::Max(1.f, FindToolSpeed(Worker, Tool));
	return Chance > 0.f && Random.FRand() < Chance;
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
	// Phase 21: "Einfach" never has a second fault, "Detailliert" more often (also on merely damaged devices).
	const EWERepairComplexity Complexity = UWEGameRulesComponent::GetRules(this).Complexity;
	const float SecondChance = Complexity == EWERepairComplexity::Simple ? 0.f
		: Complexity == EWERepairComplexity::Detailed ? FMath::Min(1.f, Repair->SecondFaultChance * 2.f) : Repair->SecondFaultChance;
	const bool bCanHaveTwo = Item.Condition >= EWEItemCondition::Defective || (Complexity == EWERepairComplexity::Detailed && Item.Condition >= EWEItemCondition::Damaged);
	if (bCanHaveTwo && Random.FRand() < SecondChance)
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
	bool bTakenApart = Item.IsPartiallyDisassembled();
	if (const UWEAssemblyDefinition* Assembly = Def->HasAssembly() ? Def->Assembly.Get() : nullptr)
	{
		// Put back together: the function test shows whether it runs (needs every part it cannot run without).
		if (Item.NeedsFunctionTest())
		{
			const TArray<FText> Missing = Assembly->GetMissingForTest(Item.RemovedParts);
			if (Missing.Num() > 0)
			{
				OutReason = FText::Format(LOCTEXT("TestMissing", "Für den Funktionstest fehlt noch: {0}"), FText::Join(FText::FromString(TEXT(", ")), Missing));
				return Info;
			}
			Info.Kind = EWERepairStepKind::Test;
			Info.Text = LOCTEXT("FunctionTestBuilt", "Funktionstest");
			Info.Tool = Repair->TestTool;
			Info.Seconds = Repair->TestSeconds;
			return Info;
		}
		// Opening the case (side panel) is service access, not disassembly; optional parts do not matter either.
		bTakenApart = false;
		for (int32 i = 0; i < Assembly->Parts.Num(); ++i)
		{
			bTakenApart |= Item.IsPartRemoved(i) && !Assembly->Parts[i].bServiceAccess && !Assembly->Parts[i].bOptional;
		}
		// A broken part that was put in: it has to come out again (it is not one of the repair faults).
		if (!bTakenApart && Item.HasBrokenInstalledPart())
		{
			TArray<FText> Bad;
			for (int32 i = 0; i < Assembly->Parts.Num(); ++i)
			{
				if (Item.GetInstalledCondition(i) >= static_cast<int32>(EWEItemCondition::Damaged))
				{
					Bad.Add(Assembly->Parts[i].DisplayName);
				}
			}
			OutReason = FText::Format(LOCTEXT("BadInstalled", "Defektes Teil eingebaut: {0} – unter „Zerlegen“ ausbauen und ersetzen"), FText::Join(FText::FromString(TEXT(", ")), Bad));
			return Info;
		}
	}
	if (bTakenApart)
	{
		OutReason = Def->HasAssembly() ? LOCTEXT("RepairPartlyBuild", "Teilweise zerlegt – erst unter „Zusammenbauen“ wieder einbauen")
			: LOCTEXT("RepairPartly", "Teilweise zerlegt – nicht mehr reparierbar");
		return Info;
	}
	if (Item.Condition == EWEItemCondition::Scrap)
	{
		OutReason = LOCTEXT("RepairScrap", "Schrott – nicht mehr zu retten");
		return Info;
	}
	if (!Item.IsBroken())
	{
		// Phase 21: maintenance - first the dust, then fresh thermal paste, then refurbishing.
		if (!Item.Repair.HasFlag(FWERepairState::Cleaned) && Item.GetWear().Dirt > 0.15f)
		{
			Info.Kind = EWERepairStepKind::Clean;
			Info.Text = LOCTEXT("CleanDust", "Staub und Schmutz ausblasen");
			Info.Tool = EWEToolType::AirDuster;
			Info.Seconds = 3.5f;
			return Info;
		}
		if (Repair->PasteMaterial && !Item.Repair.HasFlag(FWERepairState::PasteRenewed))
		{
			Info.Kind = EWERepairStepKind::Paste;
			Info.Text = LOCTEXT("RenewPaste", "Wärmeleitpaste erneuern");
			Info.Seconds = 5.f;
			Info.Material = Repair->PasteMaterial;
			return Info;
		}
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
	if (IsInstallJob() && GetAssembly() && GetAssembly()->Parts.IsValidIndex(Work.StepIndex))
	{
		return FText::Format(LOCTEXT("InstallJob", "{0} einbauen"), GetAssembly()->Parts[Work.StepIndex].DisplayName);
	}
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
	ApplyKnownFaults();
	const FWERepairStepInfo Info = GetNextRepairStep(Reason);
	const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	const float Speed = FindToolSpeed(Character, Info.Tool) * (Progression ? Progression->GetWorkSpeedMultiplier() : 1.f) * BenchSpeedMultiplier
		* (Character ? Character->GetWorkBoost() : 1.f);

	// Phase 21: what is broken is decided when the diagnosis starts, so switching on shows the real symptoms and every
	// measuring point its real result (the player still has to find out).
	if (Info.Kind == EWERepairStepKind::Diagnose && !PlacedItem->GetItem().Repair.HasFlag(FWERepairState::FaultsRolled))
	{
		FWEItemInstance Rolled = PlacedItem->GetItem();
		EnsureFaultsRolled(Rolled);
		PlacedItem->SetItem(Rolled);
	}
	Work.StepIndex = 0; // marks the bench busy; the repair kind says what is done
	Work.bInstall = false;
	Work.RepairKind = Info.Kind;
	Work.RepairFault = static_cast<int8>(Info.FaultIndex);
	Work.Worker = Character;
	Work.Duration = FMath::Max(0.1f, Info.Seconds * UWEGameRulesComponent::GetRules(this).RepairTimeFactor() / Speed);
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
		if (RollMistake(Worker, Fault->Tool))
		{
			Message = LOCTEXT("MistakeRemove", "Abgerutscht – die Schraube sitzt noch. Noch einmal ansetzen.");
			Event = EWEWorkbenchEvent::StepCancelled;
			UE_LOG(LogWENet, Log, TEXT("%s: repair mistake while removing (%s)"), *GetName(), *Fault->DisplayName.ToString());
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
		if (Fault && RollMistake(Worker, Fault->Tool))
		{
			// The part is not used up: it simply did not click in.
			Message = LOCTEXT("MistakeInstall", "Nicht richtig eingerastet – das Teil sitzt schief. Noch einmal einsetzen.");
			Event = EWEWorkbenchEvent::StepCancelled;
			UE_LOG(LogWENet, Log, TEXT("%s: repair mistake while installing (%s)"), *GetName(), *Fault->DisplayName.ToString());
			break;
		}
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
		const bool bRebuilt = Item.NeedsFunctionTest();
		const UWEAssemblyDefinition* Assembly = Item.Definition->HasAssembly() ? Item.Definition->Assembly.Get() : nullptr;
		Item.Repair.Flags &= ~FWERepairState::NeedsTest;
		if (Item.Repair.Faults == 0 && !Item.HasBrokenInstalledPart())
		{
			const bool bWasBroken = Item.IsBroken();
			if (bRebuilt)
			{
				// Rebuilt: as good as its worst part (a device that was broken before is "Gebraucht" at best).
				int32 Worst = bWasBroken ? static_cast<int32>(EWEItemCondition::Used) : static_cast<int32>(Item.Condition);
				for (int32 i = 0; i < FWEItemInstance::MaxInstalledParts; ++i)
				{
					Worst = FMath::Max(Worst, Item.GetInstalledCondition(i));
				}
				Item.Condition = static_cast<EWEItemCondition>(FMath::Clamp(Worst, 0, static_cast<int32>(EWEItemCondition::Used)));
				if (bWasBroken)
				{
					Item.Quality = static_cast<uint8>(FMath::Min<int32>(90, Item.Quality + Repair->RepairQualityGain));
				}
			}
			else
			{
				Item.Condition = FMath::Min(Item.Condition, EWEItemCondition::Used);
				Item.Quality = static_cast<uint8>(FMath::Min<int32>(90, Item.Quality + Repair->RepairQualityGain));
			}
			Item.Repair.Removed = 0;
			Item.Repair.Replaced = 0;
			Event = EWEWorkbenchEvent::Finished;
			Message = FText::Format(bRebuilt ? LOCTEXT("TestPassedBuilt", "Funktionstest bestanden: {0} läuft ({1}, Qualität {2} %)")
				: LOCTEXT("TestPassed", "Funktionstest bestanden: {0} läuft wieder ({1}, Qualität {2} %)"),
				Name, WEItems::GetConditionText(Item.Condition), FText::AsNumber(Item.Quality));
			if (const int32 Bench = WEBenchmark::Score(Item); Bench > 0)
			{
				// Phase 19: a PC also shows how fast it is.
				Message = FText::Format(LOCTEXT("TestBenchmark", "{0} · Benchmark {1} Punkte"), Message, WEBenchmark::PointsText(Bench));
			}
			UE_LOG(LogWENet, Log, TEXT("%s: %s %s"), *GetName(), *Name.ToString(), bRebuilt ? TEXT("rebuilt and tested") : TEXT("repaired"));
			if (bWasBroken || !bRebuilt)
			{
				if (UWEMilestoneComponent* Milestones = UWEMilestoneComponent::Get(this)) { Milestones->AddCount(EWEMilestoneKind::DevicesRepaired); }
			}
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
			for (int32 i = 0; Assembly && i < Assembly->Parts.Num(); ++i)
			{
				if (Item.GetInstalledCondition(i) >= static_cast<int32>(EWEItemCondition::Damaged))
				{
					Failed.Add(FString::Printf(TEXT("%s defekt"), *Assembly->Parts[i].DisplayName.ToString()));
				}
			}
			// The bad spare part has to come out again.
			Item.Repair.Replaced &= ~Item.Repair.Faults;
			Item.Repair.Removed &= ~Item.Repair.Faults;
			Event = EWEWorkbenchEvent::StepCancelled;
			if (bRebuilt)
			{
				// Now it is known that (and why) it does not run.
				Item.Repair.SetFlag(FWERepairState::FaultsRolled);
				Item.Repair.SetFlag(FWERepairState::Diagnosed);
				Item.Condition = FMath::Max(Item.Condition, EWEItemCondition::Defective);
			}
			Message = FText::Format(bRebuilt ? LOCTEXT("TestFailedBuilt", "Funktionstest fehlgeschlagen: {0}")
				: LOCTEXT("TestFailed", "Funktionstest fehlgeschlagen: {0} – das Ersatzteil taugt nichts"),
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
	case EWERepairStepKind::Clean:
		// Phase 21: dust out - it looks better, runs cooler and is worth a bit more.
		Item.Repair.SetFlag(FWERepairState::Cleaned);
		Item.Quality = static_cast<uint8>(FMath::Min<int32>(95, Item.Quality + 5));
		Event = EWEWorkbenchEvent::Finished;
		Message = FText::Format(LOCTEXT("Cleaned", "Gereinigt: {0} ist staubfrei (Qualität {1} %)"), Name, FText::AsNumber(Item.Quality));
		break;
	case EWERepairStepKind::Paste:
	{
		UWEInventoryComponent* Source = nullptr;
		int32 Slot = INDEX_NONE;
		FWEItemInstance Used;
		if (!FindSparePart(Worker, Storage, Repair->PasteMaterial, Source, Slot) || Source->RemoveFromSlot(Slot, 1, Used) != EWEInventoryResult::Success)
		{
			Message = FText::Format(LOCTEXT("PasteGone", "Material fehlt: {0}"), Repair->PasteMaterial ? Repair->PasteMaterial->DisplayName : FText::GetEmpty());
			Event = EWEWorkbenchEvent::StepCancelled;
			break;
		}
		Item.Repair.SetFlag(FWERepairState::PasteRenewed);
		Item.Quality = static_cast<uint8>(FMath::Min<int32>(95, Item.Quality + 5));
		Event = EWEWorkbenchEvent::Finished;
		Message = FText::Format(LOCTEXT("PasteDone", "Neue Wärmeleitpaste: {0} bleibt kühler (Qualität {1} %)"), Name, FText::AsNumber(Item.Quality));
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

// --- Work view (Phase 17.5) ----------------------------------------------------------------------------

FWEWorkPlan AWEWorkbench::GetWorkPlan(bool bRepair) const
{
	const UWEItemDefinition* Def = PlacedItem ? PlacedItem->GetItem().Definition.Get() : nullptr;
	if (!Def)
	{
		return FWEWorkPlan();
	}
	if (!bRepair)
	{
		// The part being worked on, else the default part / next step.
		const int32 Part = GetActivePart() != INDEX_NONE ? GetActivePart() : GetNextStepIndex();
		if (IsInstallJob())
		{
			return WEWorkPlan::ForAssemblyInstall(Def, Part);
		}
		return GetAssembly() ? WEWorkPlan::ForAssemblyPart(Def, Part) : WEWorkPlan::ForDisassembly(Def, Part);
	}
	FText Reason;
	return WEWorkPlan::ForRepair(Def, GetNextRepairStep(Reason));
}

FWEWorkPlan AWEWorkbench::GetPartPlan(int32 Part, bool bInstall) const
{
	const UWEItemDefinition* Def = PlacedItem ? PlacedItem->GetItem().Definition.Get() : nullptr;
	if (!Def || !GetAssembly())
	{
		return FWEWorkPlan();
	}
	return bInstall ? WEWorkPlan::ForAssemblyInstall(Def, Part) : WEWorkPlan::ForAssemblyPart(Def, Part);
}

bool AWEWorkbench::TryStartManualStep(AWECharacter* Character, bool bRepair, int32 Part, bool bInstall)
{
	if (!HasAuthority())
	{
		return false;
	}
	bInstall = bInstall && !bRepair && Part != INDEX_NONE;
	const FWEWorkPlan Plan = bRepair ? GetWorkPlan(true) : (GetAssembly() && Part != INDEX_NONE ? GetPartPlan(Part, bInstall) : GetWorkPlan(false));
	if (!Plan.IsValid() || !(bRepair ? TryStartRepairStep(Character) : bInstall ? TryStartInstall(Character, Part) : TryStartStep(Character, Part)))
	{
		return false;
	}
	// No timer: the step ends when the last task is done by hand.
	GetWorldTimerManager().ClearTimer(WorkTimer);
	Work.bManual = true;
	Work.TaskCount = static_cast<uint8>(Plan.Tasks.Num());
	Work.TaskMask = 0;
	LastTaskTime = GetWorld()->GetTimeSeconds();
	// Walked away from the keyboard: the bench is freed again after a while.
	GetWorldTimerManager().SetTimer(ManualIdleTimer, this, &AWEWorkbench::CancelActiveStep, 180.f, false);
	ForceNetUpdate();
	UE_LOG(LogWENet, Log, TEXT("%s: %s works by hand (%d tasks, %.1fs)"), *GetName(), *Character->GetName(), Plan.Tasks.Num(), Work.Duration);
	return true;
}

bool AWEWorkbench::DoManualTask(AWECharacter* Character, int32 TaskIndex, EWEToolType Tool)
{
	if (!HasAuthority() || !Work.bManual || !Character || Work.Worker != Character || !PlacedItem)
	{
		return false;
	}
	if (!UWEInteractionComponent::IsWithinReach(Character, this, WorkReach))
	{
		ShowMessage(Character, LOCTEXT("ManualTooFar", "Zu weit von der Werkbank weg"));
		return false;
	}
	const FWEWorkPlan Plan = GetWorkPlan(IsRepairJob());
	if (Plan.Tasks.Num() != Work.TaskCount || !Plan.IsTaskAvailable(TaskIndex, Work.TaskMask))
	{
		UE_LOG(LogWENet, Log, TEXT("%s: task %d refused (not available)"), *GetName(), TaskIndex);
		return false;
	}
	const FWEWorkTask& Task = Plan.Tasks[TaskIndex];
	if (Task.Tool != Tool || (Tool != EWEToolType::None && FindToolSpeed(Character, Tool) <= 0.f))
	{
		UE_LOG(LogWENet, Log, TEXT("%s: task %d refused (wrong tool)"), *GetName(), TaskIndex);
		return false;
	}
	// Work takes time: the client holds the button for the task's share of the step (with tolerance for lag).
	const float Now = GetWorld()->GetTimeSeconds();
	const float Expected = Plan.GetTaskSeconds(TaskIndex, Work.Duration);
	if (Now - LastTaskTime < Expected * 0.5f - 0.05f)
	{
		UE_LOG(LogWENet, Log, TEXT("%s: task %d refused (too fast: %.2fs < %.2fs)"), *GetName(), TaskIndex, Now - LastTaskTime, Expected);
		return false;
	}
	LastTaskTime = Now;
	Work.TaskMask |= (1u << TaskIndex);
	GetWorldTimerManager().SetTimer(ManualIdleTimer, this, &AWEWorkbench::CancelActiveStep, 180.f, false);
	if (FMath::CountBits(Work.TaskMask) >= Work.TaskCount)
	{
		CompleteActiveStep();
	}
	else
	{
		RaiseEvent(EWEWorkbenchEvent::TaskDone);
	}
	return true;
}

void AWEWorkbench::CancelManualStep(AWECharacter* Character)
{
	if (HasAuthority() && Work.bManual && Work.Worker == Character)
	{
		CancelActiveStep();
	}
}

void AWEWorkbench::AimWorkCamera()
{
	FVector Center = WorkPoint->GetComponentLocation() + FVector(0.f, 0.f, 12.f);
	float Radius = 25.f;
	if (PlacedItem)
	{
		FVector Origin, Extent;
		PlacedItem->GetActorBounds(true, Origin, Extent);
		if (!Extent.IsNearlyZero())
		{
			Center = Origin;
			Radius = FMath::Clamp(Extent.Size(), 12.f, 90.f);
		}
	}
	// From the front edge (where the player stands) and above, looking down at about 55 degrees.
	const FVector Front = GetActorForwardVector();
	// Far enough that the whole device (and the parts tray beside it) fits between the side panels of the view.
	const FVector Eye = Center + Front * Radius * 1.8f + FVector::UpVector * Radius * 2.5f + FVector(0.f, 0.f, 10.f);
	WorkCamera->SetWorldLocationAndRotation(Eye, (Center - Eye).Rotation());
}

#undef LOCTEXT_NAMESPACE
