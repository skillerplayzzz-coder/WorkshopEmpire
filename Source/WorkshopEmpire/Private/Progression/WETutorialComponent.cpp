#include "Progression/WETutorialComponent.h"

#include "EngineUtils.h"
#include "Framework/WEGameState.h"
#include "Framework/WEPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "WELog.h"

#define LOCTEXT_NAMESPACE "WETutorial"

UWETutorialComponent::UWETutorialComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UWETutorialComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWETutorialComponent, State);
	DOREPLIFETIME(UWETutorialComponent, Step);
}

void UWETutorialComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority() && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(MoveTimer, this, &UWETutorialComponent::CheckMovement, 0.5f, true);
	}
}

void UWETutorialComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(MoveTimer);
	}
	Super::EndPlay(EndPlayReason);
}

UWETutorialComponent* UWETutorialComponent::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AWEGameState* GameState = World ? World->GetGameState<AWEGameState>() : nullptr;
	return GameState ? GameState->GetTutorial() : nullptr;
}

const TArray<FWETutorialStep>& UWETutorialComponent::GetSteps()
{
	static const TArray<FWETutorialStep> Steps = []()
	{
		auto S = [](const FText& Title, const FText& Text, const TCHAR* Key, EWETutorialEvent Event, const TCHAR* Target)
		{
			FWETutorialStep Step;
			Step.Title = Title;
			Step.Text = Text;
			Step.Key = FText::FromString(Key);
			Step.Event = Event;
			Step.Target = Target;
			return Step;
		};
		TArray<FWETutorialStep> All;
		All.Add(S(LOCTEXT("T0", "Umsehen und laufen"), LOCTEXT("T0Text", "Geh zur Werkbank. Mit Shift läufst du schneller."), TEXT("W A S D"), EWETutorialEvent::Moved, TEXT("WEWorkbench")));
		All.Add(S(LOCTEXT("T1", "Werkzeug nehmen"), LOCTEXT("T1Text", "Nimm den Schraubendreher von der Werkbank."), TEXT("E"), EWETutorialEvent::ToolStored, TEXT("Item:Screwdriver")));
		All.Add(S(LOCTEXT("T2", "Inventar"), LOCTEXT("T2Text", "Öffne dein Inventar. Werkzeuge sind gegen Verkauf geschützt."), TEXT("Tab"), EWETutorialEvent::InventoryOpened, TEXT("")));
		All.Add(S(LOCTEXT("T3", "Gerät auf die Werkbank"), LOCTEXT("T3Text", "Trag den alten Computer zur Werkbank und leg ihn ab."), TEXT("E"), EWETutorialEvent::ItemPlacedOnBench, TEXT("Item:OldComputer")));
		All.Add(S(LOCTEXT("T4", "Arbeitsansicht"), LOCTEXT("T4Text", "Schau auf das Gerät auf der Werkbank und öffne die Arbeitsansicht."), TEXT("E"), EWETutorialEvent::WorkViewOpened, TEXT("WEWorkbench")));
		All.Add(S(LOCTEXT("T5", "Zerlegen"), LOCTEXT("T5Text", "Werkzeug wählen, Schrauben mit gedrückter Maustaste lösen, Teile herausziehen."), TEXT("Maus"), EWETutorialEvent::DisassemblyStep, TEXT("")));
		All.Add(S(LOCTEXT("T6", "Werkstatt-PC"), LOCTEXT("T6Text", "Setz dich an den PC und öffne den Marktplatz."), TEXT("E"), EWETutorialEvent::ComputerOpened, TEXT("WETraderTerminal")));
		All.Add(S(LOCTEXT("T7", "Verkaufen"), LOCTEXT("T7Text", "Verkaufe ausgebaute Teile im Marktplatz."), TEXT("E"), EWETutorialEvent::ItemSold, TEXT("WETraderTerminal")));
		All.Add(S(LOCTEXT("T8", "Kundenauftrag"), LOCTEXT("T8Text", "Nimm im Programm \"Aufträge\" einen Auftrag an."), TEXT("E"), EWETutorialEvent::ContractAccepted, TEXT("WETraderTerminal")));
		All.Add(S(LOCTEXT("T9", "Reparieren"), LOCTEXT("T9Text", "Prüfe ein defektes Gerät auf der Werkbank mit dem Multimeter (gibt es im Werkzeughandel)."), TEXT("E"), EWETutorialEvent::RepairStep, TEXT("WEWorkbench")));
		All.Add(S(LOCTEXT("T10", "Speichern"), LOCTEXT("T10Text", "Speichere dein Unternehmen."), TEXT("F5"), EWETutorialEvent::GameSaved, TEXT("")));
		return All;
	}();
	return Steps;
}

const FWETutorialStep* UWETutorialComponent::GetCurrentStep() const
{
	return IsActive() && GetSteps().IsValidIndex(Step) ? &GetSteps()[Step] : nullptr;
}

void UWETutorialComponent::GetMarkerLocations(TArray<FVector>& OutLocations) const
{
	const FWETutorialStep* Current = GetCurrentStep();
	UWorld* World = GetWorld();
	if (!Current || Current->Target.IsEmpty() || !World)
	{
		return;
	}
	if (Current->Target.StartsWith(TEXT("Item:")))
	{
		const FName ItemId(*Current->Target.RightChop(5));
		for (TActorIterator<AWEPickupItem> It(World); It && OutLocations.Num() < 2; ++It)
		{
			if (!It->GetHolder() && It->GetItem().Definition && It->GetItem().Definition->ItemId == ItemId)
			{
				OutLocations.Add(It->GetActorLocation() + FVector(0.f, 0.f, 35.f));
			}
		}
		return;
	}
	for (TActorIterator<AActor> It(World); It && OutLocations.Num() < 1; ++It)
	{
		if (It->GetClass()->GetName() == Current->Target && !It->IsHidden())
		{
			FVector Origin, Extent;
			It->GetActorBounds(true, Origin, Extent);
			OutLocations.Add(Origin + FVector(0.f, 0.f, Extent.Z + 30.f));
		}
	}
}

void UWETutorialComponent::OnRep_Tutorial()
{
}

void UWETutorialComponent::Announce(const FText& Message) const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(It->Get()))
		{
			PC->ClientShowMessage(Message);
		}
	}
}

void UWETutorialComponent::Start()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	State = EWETutorialState::Active;
	Step = 0;
	StartPositions.Reset();
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Tutorial started"));
	OnRep_Tutorial();
}

void UWETutorialComponent::Skip()
{
	if (!GetOwner()->HasAuthority() || State != EWETutorialState::Active)
	{
		return;
	}
	State = EWETutorialState::Skipped;
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Tutorial skipped at step %d"), Step);
	Announce(LOCTEXT("Skipped", "Tutorial übersprungen – neu starten im Menü [Esc] oder am PC unter \"Hilfe\""));
	OnRep_Tutorial();
}

void UWETutorialComponent::NotifyEvent(EWETutorialEvent Event)
{
	const FWETutorialStep* Current = GetCurrentStep();
	if (!GetOwner()->HasAuthority() || !Current || Current->Event != Event)
	{
		return;
	}
	const FText Done = Current->Title;
	++Step;
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Tutorial step %d/%d done"), Step, NumSteps());
	if (Step >= NumSteps())
	{
		State = EWETutorialState::Completed;
		Announce(LOCTEXT("Completed", "Tutorial abgeschlossen – viel Erfolg mit deinem Unternehmen!"));
	}
	else
	{
		Announce(FText::Format(LOCTEXT("StepDone", "✓ {0}"), Done));
	}
	OnRep_Tutorial();
}

void UWETutorialComponent::SetState(EWETutorialState InState, int32 InStep)
{
	if (GetOwner()->HasAuthority())
	{
		State = InState;
		Step = FMath::Clamp(InStep, 0, NumSteps());
		OnRep_Tutorial();
	}
}

void UWETutorialComponent::CheckMovement()
{
	const FWETutorialStep* Current = GetCurrentStep();
	if (!Current || Current->Event != EWETutorialEvent::Moved)
	{
		StartPositions.Reset();
		return;
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}
		const FVector* Start = StartPositions.Find(Pawn);
		if (!Start)
		{
			StartPositions.Add(Pawn, Pawn->GetActorLocation());
		}
		else if (FVector::Dist2D(*Start, Pawn->GetActorLocation()) > 250.f)
		{
			NotifyEvent(EWETutorialEvent::Moved);
			return;
		}
	}
}

#undef LOCTEXT_NAMESPACE
