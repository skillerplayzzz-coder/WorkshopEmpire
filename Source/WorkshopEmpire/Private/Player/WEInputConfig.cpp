#include "Player/WEInputConfig.h"

#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/ConfigCacheIni.h"
#include "Player/WECharacter.h"

#define LOCTEXT_NAMESPACE "WEInput"

namespace WEInput
{
	static UInputAction* MakeAction(UObject* Outer, FName Name, EInputActionValueType ValueType)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, Name);
		Action->ValueType = ValueType;
		return Action;
	}

	/** Maps a key to a 2D action. bSwizzle moves the 1D key value to Y, bNegate flips it. */
	static void MapAxisKey(UInputMappingContext* Context, const UInputAction* Action, const FKey& Key, bool bSwizzle, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
		if (bSwizzle)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Context));
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
		}
	}
}

UWEInputConfig* UWEInputConfig::CreateDefault(UObject* Outer)
{
	UWEInputConfig* Config = NewObject<UWEInputConfig>(Outer, TEXT("WEDefaultInputConfig"), RF_Transient);

	Config->MoveAction = WEInput::MakeAction(Config, TEXT("IA_Move"), EInputActionValueType::Axis2D);
	Config->LookAction = WEInput::MakeAction(Config, TEXT("IA_Look"), EInputActionValueType::Axis2D);
	Config->JumpAction = WEInput::MakeAction(Config, TEXT("IA_Jump"), EInputActionValueType::Boolean);
	Config->SprintAction = WEInput::MakeAction(Config, TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	Config->CrouchAction = WEInput::MakeAction(Config, TEXT("IA_Crouch"), EInputActionValueType::Boolean);
	Config->InteractAction = WEInput::MakeAction(Config, TEXT("IA_Interact"), EInputActionValueType::Boolean);
	Config->ExamineAction = WEInput::MakeAction(Config, TEXT("IA_Examine"), EInputActionValueType::Boolean);
	Config->SecondaryAction = WEInput::MakeAction(Config, TEXT("IA_Secondary"), EInputActionValueType::Boolean);
	Config->AlternativeAction = WEInput::MakeAction(Config, TEXT("IA_Alternative"), EInputActionValueType::Boolean);
	Config->BuildAction = WEInput::MakeAction(Config, TEXT("IA_Build"), EInputActionValueType::Boolean);
	Config->RemoveAction = WEInput::MakeAction(Config, TEXT("IA_Remove"), EInputActionValueType::Boolean);
	Config->CycleAction = WEInput::MakeAction(Config, TEXT("IA_Cycle"), EInputActionValueType::Axis1D);
	Config->SettingAction = WEInput::MakeAction(Config, TEXT("IA_Setting"), EInputActionValueType::Boolean);
	Config->InventoryAction = WEInput::MakeAction(Config, TEXT("IA_Inventory"), EInputActionValueType::Boolean);
	Config->QuickSaveAction = WEInput::MakeAction(Config, TEXT("IA_QuickSave"), EInputActionValueType::Boolean);
	Config->QuickLoadAction = WEInput::MakeAction(Config, TEXT("IA_QuickLoad"), EInputActionValueType::Boolean);
	Config->PauseAction = WEInput::MakeAction(Config, TEXT("IA_Pause"), EInputActionValueType::Boolean);
	Config->ViewAction = WEInput::MakeAction(Config, TEXT("IA_View"), EInputActionValueType::Boolean);

	UInputMappingContext* Context = NewObject<UInputMappingContext>(Config, TEXT("IMC_Default"));
	Config->MappingContext = Context;

	// Move: X = right, Y = forward.
	WEInput::MapAxisKey(Context, Config->MoveAction, EKeys::W, /*bSwizzle*/ true, /*bNegate*/ false);
	WEInput::MapAxisKey(Context, Config->MoveAction, EKeys::S, true, true);
	WEInput::MapAxisKey(Context, Config->MoveAction, EKeys::D, false, false);
	WEInput::MapAxisKey(Context, Config->MoveAction, EKeys::A, false, true);

	Context->MapKey(Config->LookAction, EKeys::Mouse2D);
	Context->MapKey(Config->JumpAction, EKeys::SpaceBar);
	Context->MapKey(Config->SprintAction, EKeys::LeftShift);
	Context->MapKey(Config->CrouchAction, EKeys::LeftControl);
	Context->MapKey(Config->CrouchAction, EKeys::C);
	Context->MapKey(Config->InteractAction, EKeys::E);
	Context->MapKey(Config->ExamineAction, EKeys::F);
	Context->MapKey(Config->SecondaryAction, EKeys::R);
	Context->MapKey(Config->AlternativeAction, EKeys::Q);
	Context->MapKey(Config->BuildAction, EKeys::B);
	Context->MapKey(Config->RemoveAction, EKeys::X);
	Context->MapKey(Config->CycleAction, EKeys::MouseWheelAxis);
	Context->MapKey(Config->SettingAction, EKeys::T);
	Context->MapKey(Config->InventoryAction, EKeys::Tab);
	Context->MapKey(Config->InventoryAction, EKeys::I);
	Context->MapKey(Config->QuickSaveAction, EKeys::F5);
	Context->MapKey(Config->QuickLoadAction, EKeys::F9);
	Context->MapKey(Config->PauseAction, EKeys::Escape);
	Context->MapKey(Config->ViewAction, EKeys::V);

	// Phase 20: remember where each rebindable key sits, then put in the player's own keys.
	for (const WEKeys::FEntry& Entry : WEKeys::GetEntries())
	{
		const UInputAction* Action = Config->GetActionForBinding(Entry.Id);
		const TArray<FEnhancedActionKeyMapping>& Mappings = Context->GetMappings();
		for (int32 i = 0; i < Mappings.Num(); ++i)
		{
			if (Mappings[i].Action == Action && Mappings[i].Key == Entry.Default)
			{
				Config->BindingIndex.Add(Entry.Id, i);
				break;
			}
		}
	}
	Config->ApplyKeyBindings();
	return Config;
}

UInputAction* UWEInputConfig::GetActionForBinding(FName Id) const
{
	static const TMap<FName, int32> Slots = {
		{ TEXT("MoveForward"), 0 }, { TEXT("MoveBack"), 0 }, { TEXT("MoveLeft"), 0 }, { TEXT("MoveRight"), 0 },
		{ TEXT("Jump"), 1 }, { TEXT("Sprint"), 2 }, { TEXT("Crouch"), 3 }, { TEXT("Interact"), 4 }, { TEXT("Examine"), 5 },
		{ TEXT("Secondary"), 6 }, { TEXT("Alternative"), 7 }, { TEXT("Setting"), 8 }, { TEXT("Build"), 9 }, { TEXT("Remove"), 10 },
		{ TEXT("Inventory"), 11 }, { TEXT("View"), 12 }, { TEXT("QuickSave"), 13 }, { TEXT("QuickLoad"), 14 } };
	const int32* Slot = Slots.Find(Id);
	if (!Slot)
	{
		return nullptr;
	}
	UInputAction* const Actions[] = { MoveAction, JumpAction, SprintAction, CrouchAction, InteractAction, ExamineAction, SecondaryAction,
		AlternativeAction, SettingAction, BuildAction, RemoveAction, InventoryAction, ViewAction, QuickSaveAction, QuickLoadAction };
	return Actions[*Slot];
}

void UWEInputConfig::ApplyKeyBindings()
{
	if (!MappingContext)
	{
		return;
	}
	for (const TPair<FName, int32>& Pair : BindingIndex)
	{
		if (MappingContext->GetMappings().IsValidIndex(Pair.Value))
		{
			MappingContext->GetMapping(Pair.Value).Key = WEKeys::GetKey(Pair.Key);
		}
	}
}

namespace WEKeys
{
	static const TCHAR* Section = TEXT("WorkshopEmpire.Keys");

	const TArray<FEntry>& GetEntries()
	{
		static const TArray<FEntry> Entries = {
			{ TEXT("MoveForward"), LOCTEXT("KForward", "Vorwärts"), EKeys::W },
			{ TEXT("MoveBack"), LOCTEXT("KBack", "Rückwärts"), EKeys::S },
			{ TEXT("MoveLeft"), LOCTEXT("KLeft", "Links"), EKeys::A },
			{ TEXT("MoveRight"), LOCTEXT("KRight", "Rechts"), EKeys::D },
			{ TEXT("Jump"), LOCTEXT("KJump", "Springen"), EKeys::SpaceBar },
			{ TEXT("Sprint"), LOCTEXT("KSprint", "Rennen"), EKeys::LeftShift },
			{ TEXT("Crouch"), LOCTEXT("KCrouch", "Ducken"), EKeys::LeftControl },
			{ TEXT("Interact"), LOCTEXT("KInteract", "Benutzen"), EKeys::E },
			{ TEXT("Examine"), LOCTEXT("KExamine", "Untersuchen"), EKeys::F },
			{ TEXT("Secondary"), LOCTEXT("KSecondary", "Zweitaktion (Nehmen, Ablegen)"), EKeys::R },
			{ TEXT("Alternative"), LOCTEXT("KAlternative", "Nächster Arbeitsschritt"), EKeys::Q },
			{ TEXT("Setting"), LOCTEXT("KSetting", "Einstellung am Objekt"), EKeys::T },
			{ TEXT("Build"), LOCTEXT("KBuild", "Baumodus"), EKeys::B },
			{ TEXT("Remove"), LOCTEXT("KRemove", "Entfernen / Wartung"), EKeys::X },
			{ TEXT("Inventory"), LOCTEXT("KInventory", "Inventar"), EKeys::Tab },
			{ TEXT("View"), LOCTEXT("KView", "Ansicht wechseln"), EKeys::V },
			{ TEXT("QuickSave"), LOCTEXT("KQuickSave", "Schnellspeichern"), EKeys::F5 },
			{ TEXT("QuickLoad"), LOCTEXT("KQuickLoad", "Schnellladen"), EKeys::F9 },
		};
		return Entries;
	}

	static const FEntry* Find(FName Id)
	{
		return GetEntries().FindByPredicate([Id](const FEntry& E) { return E.Id == Id; });
	}

	FText GetLabel(FName Id)
	{
		const FEntry* E = Find(Id);
		return E ? E->Label : FText::FromName(Id);
	}

	FKey GetKey(FName Id)
	{
		const FEntry* E = Find(Id);
		if (!E)
		{
			return EKeys::Invalid;
		}
		FString Stored;
		if (GConfig && GConfig->GetString(Section, *Id.ToString(), Stored, GGameUserSettingsIni) && !Stored.IsEmpty())
		{
			const FKey Key(*Stored);
			if (Key.IsValid())
			{
				return Key;
			}
		}
		return E->Default;
	}

	static void Store(FName Id, const FKey& Key)
	{
		if (!GConfig)
		{
			return;
		}
		const FEntry* E = Find(Id);
		if (E && Key == E->Default)
		{
			GConfig->RemoveKey(Section, *Id.ToString(), GGameUserSettingsIni);
		}
		else
		{
			GConfig->SetString(Section, *Id.ToString(), *Key.GetFName().ToString(), GGameUserSettingsIni);
		}
	}

	bool SetKey(FName Id, const FKey& Key, FName& OutSwapped)
	{
		OutSwapped = NAME_None;
		if (!Find(Id) || !Key.IsValid() || Key == EKeys::Escape || Key == EKeys::LeftMouseButton || Key.IsGamepadKey() || Key.IsAxis1D() || Key.IsAxis2D())
		{
			return false;
		}
		const FKey Old = GetKey(Id);
		for (const FEntry& Other : GetEntries())
		{
			if (Other.Id != Id && GetKey(Other.Id) == Key)
			{
				Store(Other.Id, Old);
				OutSwapped = Other.Id;
				break;
			}
		}
		Store(Id, Key);
		if (GConfig)
		{
			GConfig->Flush(false, GGameUserSettingsIni);
		}
		ApplyLive();
		return true;
	}

	void ResetAll()
	{
		if (GConfig)
		{
			GConfig->EmptySection(Section, GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);
		}
		ApplyLive();
	}

	void ApplyLive()
	{
		if (!GEngine)
		{
			return;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			for (TActorIterator<AWECharacter> It(World); World && It; ++It)
			{
				if (!It->IsLocallyControlled())
				{
					continue;
				}
				if (UWEInputConfig* Config = It->GetMutableInputConfig())
				{
					Config->ApplyKeyBindings();
				}
				const APlayerController* PC = Cast<APlayerController>(It->GetController());
				if (UEnhancedInputLocalPlayerSubsystem* Input = PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr)
				{
					Input->RequestRebuildControlMappings();
				}
			}
		}
	}
}

FKey UWEInputConfig::GetKeyForAction(const UInputAction* Action) const
{
	if (MappingContext && Action)
	{
		for (const FEnhancedActionKeyMapping& Mapping : MappingContext->GetMappings())
		{
			if (Mapping.Action == Action && !Mapping.Key.IsGamepadKey())
			{
				return Mapping.Key;
			}
		}
	}
	return EKeys::Invalid;
}

#undef LOCTEXT_NAMESPACE
