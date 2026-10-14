#include "Player/WEInputConfig.h"

#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

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
	Config->InventoryAction = WEInput::MakeAction(Config, TEXT("IA_Inventory"), EInputActionValueType::Boolean);
	Config->QuickSaveAction = WEInput::MakeAction(Config, TEXT("IA_QuickSave"), EInputActionValueType::Boolean);
	Config->QuickLoadAction = WEInput::MakeAction(Config, TEXT("IA_QuickLoad"), EInputActionValueType::Boolean);

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
	Context->MapKey(Config->InventoryAction, EKeys::Tab);
	Context->MapKey(Config->InventoryAction, EKeys::I);
	Context->MapKey(Config->QuickSaveAction, EKeys::F5);
	Context->MapKey(Config->QuickLoadAction, EKeys::F9);

	return Config;
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
