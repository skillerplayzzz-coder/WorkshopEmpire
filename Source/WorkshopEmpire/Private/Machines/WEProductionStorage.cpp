#include "Machines/WEProductionStorage.h"

#include "Framework/WEPlayerController.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Machines/WEMachine.h"
#include "Net/UnrealNetwork.h"
#include "Player/WECharacter.h"
#include "WELog.h"

#define LOCTEXT_NAMESPACE "WEProductionStorage"

AWEProductionStorage::AWEProductionStorage()
{
	PropType = EWEPlaceholderPropType::Crate;
	Size = FVector(100.f, 120.f, 80.f);
	bHasStorage = true;
	StorageSlots = 30;
	StorageMaxWeightKg = 400.f;
}

void AWEProductionStorage::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWEProductionStorage, bOutputOn);
	DOREPLIFETIME(AWEProductionStorage, OutputFilter);
}

void AWEProductionStorage::RebuildVisuals()
{
	Super::RebuildVisuals();
	GetStorage()->SetAcceptsBulky(true);
}

FText AWEProductionStorage::GetOutputText() const
{
	if (!bOutputOn)
	{
		return LOCTEXT("OutOff", "Aus");
	}
	return OutputFilter ? OutputFilter->DisplayName : LOCTEXT("OutAll", "Alles");
}

void AWEProductionStorage::SetOutput(bool bInOn, UWEItemDefinition* Filter)
{
	if (!HasAuthority())
	{
		return;
	}
	bOutputOn = bInOn;
	OutputFilter = Filter;
	ForceNetUpdate();
	UE_LOG(LogWENet, Log, TEXT("%s: output %s"), *GetName(), *GetOutputText().ToString());
}

void AWEProductionStorage::CycleOutput()
{
	// The choices: everything, every material currently in the store (in slot order), off.
	TArray<UWEItemDefinition*> Choices;
	for (const FWEInventorySlotData& Slot : GetStorage()->GetAllSlots())
	{
		if (Slot.Item.Definition)
		{
			Choices.AddUnique(Slot.Item.Definition);
		}
	}
	if (!bOutputOn)
	{
		SetOutput(true, nullptr);
		return;
	}
	if (!OutputFilter)
	{
		SetOutput(Choices.Num() > 0, Choices.Num() > 0 ? Choices[0] : nullptr);
		return;
	}
	const int32 Index = Choices.IndexOfByKey(OutputFilter.Get());
	if (Choices.IsValidIndex(Index + 1) && Index != INDEX_NONE)
	{
		SetOutput(true, Choices[Index + 1]);
	}
	else
	{
		SetOutput(false, nullptr);
	}
}

bool AWEProductionStorage::TakeForBelt(FWEItemInstance& OutItem, const AWEPlaceholderProp* LineDestination)
{
	if (!HasAuthority() || !bOutputOn)
	{
		return false;
	}
	const AWEMachine* Machine = Cast<AWEMachine>(LineDestination);
	for (const FWEInventorySlotData& Slot : GetStorage()->GetAllSlots())
	{
		if (Slot.Item.Definition && (!OutputFilter || Slot.Item.Definition == OutputFilter) && !Slot.Item.IsCustomerItem()
			&& (!Machine || Machine->CanAcceptFromBelt(Slot.Item)))
		{
			const int32 SlotIndex = Slot.SlotIndex;
			const int32 Quantity = Slot.Item.Quantity;
			return GetStorage()->RemoveFromSlot(SlotIndex, Quantity, OutItem) == EWEInventoryResult::Success;
		}
	}
	return false;
}

bool AWEProductionStorage::GetSettingInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const
{
	if (!Interactor)
	{
		return false;
	}
	OutText = FText::Format(LOCTEXT("OutputSetting", "Ausgabe aufs Band: {0}"), GetOutputText());
	return true;
}

void AWEProductionStorage::InteractSetting(AWECharacter* Interactor)
{
	CycleOutput();
	if (AWEPlayerController* PC = Interactor ? Cast<AWEPlayerController>(Interactor->GetController()) : nullptr)
	{
		PC->ClientShowMessage(FText::Format(LOCTEXT("OutputChosen", "Materiallager gibt aus: {0}"), GetOutputText()));
	}
}

bool AWEProductionStorage::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	Super::GetExamineInfo(OutInfo);
	OutInfo.Description = LOCTEXT("Desc", "Produktionslager: nimmt Material von Förderbändern an und gibt es auf ein Band ab, das direkt daneben beginnt. [T] wählt, was hinausgeht.");
	OutInfo.Details.Add(FText::Format(LOCTEXT("ExOutput", "Ausgabe aufs Band: {0}"), GetOutputText()));
	return true;
}

#undef LOCTEXT_NAMESPACE
