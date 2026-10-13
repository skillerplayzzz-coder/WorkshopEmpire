#include "Save/WESaveSystem.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/FileManager.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Player/WECharacter.h"
#include "Save/WESaveGame.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "WELog.h"
#include "World/WEGarageDoor.h"
#include "Workshop/WEWorkbench.h"
#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WEMarketComponent.h"
#include "Economy/WETraderTerminal.h"
#include "Progression/WEProgressionComponent.h"

#define LOCTEXT_NAMESPACE "WESaveSystem"

// --- Paths -------------------------------------------------------------------------------------------

bool FWESaveSystem::IsValidSlotName(const FString& Slot)
{
	if (Slot.IsEmpty() || Slot.Len() > 32)
	{
		return false;
	}
	for (const TCHAR C : Slot)
	{
		if (!FChar::IsAlnum(C) && C != TEXT('_') && C != TEXT('-'))
		{
			return false;
		}
	}
	return true;
}

FString FWESaveSystem::GetSaveDirectory()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), TEXT("WorkshopEmpire"));
}

FString FWESaveSystem::GetSlotPath(const FString& Slot)
{
	return FPaths::Combine(GetSaveDirectory(), Slot + TEXT(".wesave"));
}

// --- Item conversion -----------------------------------------------------------------------------

FWESavedItem FWESaveSystem::ToSaved(const FWEItemInstance& Item)
{
	FWESavedItem Saved;
	Saved.ItemId = Item.Definition ? Item.Definition->ItemId : NAME_None;
	Saved.InstanceId = Item.InstanceId;
	Saved.Quantity = Item.Quantity;
	Saved.Condition = Item.Condition;
	Saved.Quality = Item.Quality;
	Saved.DisassemblyStep = Item.DisassemblyStep;
	return Saved;
}

bool FWESaveSystem::FromSaved(const FWESavedItem& Saved, FWEItemInstance& OutItem)
{
	UWEItemDefinition* Definition = UWEItemDefinition::FindById(Saved.ItemId);
	if (!Definition || Saved.Quantity <= 0)
	{
		UE_LOG(LogWorkshopEmpire, Warning, TEXT("Save: unknown or invalid item '%s' skipped"), *Saved.ItemId.ToString());
		return false;
	}
	OutItem.Definition = Definition;
	OutItem.InstanceId = Saved.InstanceId.IsValid() ? Saved.InstanceId : FGuid::NewGuid();
	OutItem.Quantity = FMath::Min(Saved.Quantity, Definition->GetMaxStackSize());
	OutItem.Condition = Saved.Condition;
	OutItem.Quality = FMath::Min<uint8>(Saved.Quality, 100);
	OutItem.DisassemblyStep = Saved.DisassemblyStep;
	return true;
}

TArray<FWESavedSlot> FWESaveSystem::ToSavedSlots(const TArray<FWEInventorySlotData>& Slots)
{
	TArray<FWESavedSlot> Result;
	for (const FWEInventorySlotData& Slot : Slots)
	{
		Result.Add({ Slot.SlotIndex, ToSaved(Slot.Item) });
	}
	return Result;
}

TArray<FWEInventorySlotData> FWESaveSystem::FromSavedSlots(const TArray<FWESavedSlot>& Slots)
{
	TArray<FWEInventorySlotData> Result;
	for (const FWESavedSlot& Slot : Slots)
	{
		FWEItemInstance Item;
		if (FromSaved(Slot.Item, Item))
		{
			Result.Add({ Slot.SlotIndex, Item });
		}
	}
	return Result;
}

// --- Capture / apply -------------------------------------------------------------------------------

FString FWESaveSystem::GetPlayerKey(const APlayerController* PC)
{
	if (!PC)
	{
		return FString();
	}
	if (PC->IsLocalController())
	{
		return TEXT("Host");
	}
	const APlayerState* PS = PC->PlayerState;
	if (PS && PS->GetUniqueId().IsValid())
	{
		return PS->GetUniqueId().ToString();
	}
	return PS ? PS->GetPlayerName() : PC->GetName();
}

UWESaveGame* FWESaveSystem::Capture(UWorld* World)
{
	UWESaveGame* Save = NewObject<UWESaveGame>();
	if (!World)
	{
		return Save;
	}
	Save->MapName = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());
	Save->SavedAtUtc = FDateTime::UtcNow();

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		AWECharacter* Character = PC ? Cast<AWECharacter>(PC->GetPawn()) : nullptr;
		if (!Character)
		{
			continue;
		}
		FWESavedPlayer& Player = Save->Players.AddDefaulted_GetRef();
		Player.PlayerKey = GetPlayerKey(PC);
		Player.PawnTransform = Character->GetActorTransform();
		Player.ControlRotation = PC->GetControlRotation();
		if (const UWEInventoryComponent* Inventory = Character->GetPlayerInventory())
		{
			Player.Inventory = ToSavedSlots(Inventory->GetAllSlots());
		}
	}

	for (TActorIterator<AWEPickupItem> It(World); It; ++It)
	{
		AWEPickupItem* Item = *It;
		if (!IsValid(Item) || !Item->GetItem().IsValid())
		{
			continue;
		}
		FWESavedWorldItem& Saved = Save->WorldItems.AddDefaulted_GetRef();
		Saved.Item = ToSaved(Item->GetItem());
		// Held items are stored where the holder would put them down.
		Saved.Transform = Item->GetHolder() ? Item->GetHolder()->ComputeDropTransform(Item) : Item->GetActorTransform();
		Saved.Transform.SetScale3D(FVector::OneVector);
		Saved.PlacedOn = Item->GetPlacedOn() ? Item->GetPlacedOn()->GetFName() : NAME_None;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->IsA<APlayerState>())
		{
			continue; // player inventories are saved with the player
		}
		if (const UWEInventoryComponent* Inventory = Actor->FindComponentByClass<UWEInventoryComponent>())
		{
			FWESavedContainer& Container = Save->Containers.AddDefaulted_GetRef();
			Container.ActorName = Actor->GetFName();
			Container.Slots = ToSavedSlots(Inventory->GetAllSlots());
		}
	}

	for (TActorIterator<AWEGarageDoor> It(World); It; ++It)
	{
		Save->Doors.Add({ It->GetFName(), It->IsOpen() });
	}

	// Economy (version 3).
	if (const UWECompanyComponent* Company = UWECompanyComponent::Get(World))
	{
		Save->CompanyBalanceCents = Company->GetBalance().GetCents();
	}
	if (const UWEMarketComponent* Market = UWEMarketComponent::Get(World))
	{
		Save->MarketDemand = Market->GetAllDemand();
	}
	if (const UWEContractComponent* Contracts = UWEContractComponent::Get(World))
	{
		const float Now = Contracts->GetServerTime();
		for (const FWEContract& Contract : Contracts->GetContracts())
		{
			FWESavedContract& Saved = Save->Contracts.AddDefaulted_GetRef();
			Saved.Id = Contract.Id;
			Saved.Title = Contract.Title.ToString();
			Saved.ItemId = Contract.Item ? Contract.Item->ItemId : NAME_None;
			Saved.Quantity = Contract.Quantity;
			Saved.MinCondition = Contract.MinCondition;
			Saved.RewardCents = Contract.Reward.GetCents();
			Saved.bAccepted = Contract.State == EWEContractState::Accepted;
			Saved.RemainingSeconds = FMath::Max(0.f, Contract.ExpiresAt - Now);
			Saved.DeliverySeconds = Contract.DeliverySeconds;
			Saved.AcceptedBy = Contract.AcceptedBy;
		}
	}
	for (TActorIterator<AWETraderTerminal> It(World); It; ++It)
	{
		Save->Stocks.Add({ It->GetFName(), It->GetStockRemaining() });
	}

	// Progression (version 4).
	if (const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(World))
	{
		Save->bHasProgression = true;
		Save->OwnedUpgrades = Progression->GetOwnedUpgrades();
		Save->GuideStep = Progression->GetGuideStep();
	}
	return Save;
}

void FWESaveSystem::Apply(UWorld* World, const UWESaveGame* Save)
{
	if (!World || !Save)
	{
		return;
	}

	// 1) Free all hands and workbenches, then replace every world item with the saved ones.
	for (TActorIterator<AWEWorkbench> It(World); It; ++It)
	{
		It->ResetForLoad();
	}
	for (TActorIterator<AWECharacter> It(World); It; ++It)
	{
		It->ReleaseHeldItem();
	}
	TArray<AWEPickupItem*> ToDestroy;
	for (TActorIterator<AWEPickupItem> It(World); It; ++It)
	{
		ToDestroy.Add(*It);
	}
	for (AWEPickupItem* Item : ToDestroy)
	{
		Item->Destroy();
	}
	for (const FWESavedWorldItem& Saved : Save->WorldItems)
	{
		FWEItemInstance Item;
		if (FromSaved(Saved.Item, Item))
		{
			AWEPickupItem* Spawned = AWEPickupItem::SpawnWorldItem(World, Item, Saved.Transform);
			if (Spawned && !Saved.PlacedOn.IsNone())
			{
				for (TActorIterator<AWEWorkbench> Bench(World); Bench; ++Bench)
				{
					if (Bench->GetFName() == Saved.PlacedOn)
					{
						Bench->PlaceItem(Spawned);
					}
				}
			}
		}
	}

	// 2) Upgrades first (they change storage sizes), then containers and doors, matched by actor name.
	if (Save->bHasProgression)
	{
		if (UWEProgressionComponent* Progression = UWEProgressionComponent::Get(World))
		{
			Progression->SetOwnedUpgrades(Save->OwnedUpgrades);
			Progression->SetGuideStep(Save->GuideStep);
		}
	}
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (UWEInventoryComponent* Inventory = Actor->FindComponentByClass<UWEInventoryComponent>(); Inventory && !Actor->IsA<APlayerState>())
		{
			const FWESavedContainer* Saved = Save->Containers.FindByPredicate([Actor](const FWESavedContainer& C) { return C.ActorName == Actor->GetFName(); });
			Inventory->SetAllSlots(Saved ? FromSavedSlots(Saved->Slots) : TArray<FWEInventorySlotData>());
		}
		if (AWEGarageDoor* Door = Cast<AWEGarageDoor>(Actor))
		{
			if (const FWESavedDoor* Saved = Save->Doors.FindByPredicate([Door](const FWESavedDoor& D) { return D.ActorName == Door->GetFName(); }))
			{
				Door->SetOpen(Saved->bOpen);
			}
		}
		if (AWETraderTerminal* Terminal = Cast<AWETraderTerminal>(Actor))
		{
			if (const FWESavedStock* Saved = Save->Stocks.FindByPredicate([Terminal](const FWESavedStock& S) { return S.ActorName == Terminal->GetFName(); }))
			{
				Terminal->SetStockRemaining(Saved->Remaining);
			}
		}
	}

	// Economy (version 3; older saves keep the current account and market).
	if (Save->CompanyBalanceCents >= 0)
	{
		if (UWECompanyComponent* Company = UWECompanyComponent::Get(World))
		{
			Company->SetBalance(FWEMoney::FromCents(Save->CompanyBalanceCents));
		}
		if (UWEMarketComponent* Market = UWEMarketComponent::Get(World); Market && Save->MarketDemand.Num() > 0)
		{
			Market->SetAllDemand(Save->MarketDemand);
		}
		if (UWEContractComponent* Contracts = UWEContractComponent::Get(World))
		{
			const float Now = Contracts->GetServerTime();
			TArray<FWEContract> Restored;
			for (const FWESavedContract& Saved : Save->Contracts)
			{
				FWEContract Contract;
				Contract.Id = Saved.Id;
				Contract.Title = FText::FromString(Saved.Title);
				Contract.Item = UWEItemDefinition::FindById(Saved.ItemId);
				Contract.Quantity = Saved.Quantity;
				Contract.MinCondition = Saved.MinCondition;
				Contract.Reward = FWEMoney::FromCents(Saved.RewardCents);
				Contract.State = Saved.bAccepted ? EWEContractState::Accepted : EWEContractState::Offered;
				Contract.ExpiresAt = Now + Saved.RemainingSeconds;
				Contract.DeliverySeconds = Saved.DeliverySeconds;
				Contract.AcceptedBy = Saved.AcceptedBy;
				if (Contract.Item)
				{
					Restored.Add(Contract);
				}
			}
			Contracts->SetContracts(Restored);
		}
	}

	// 3) Players currently connected.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ApplyPlayer(It->Get(), Save);
	}
}

void FWESaveSystem::ApplyPlayer(APlayerController* PC, const UWESaveGame* Save)
{
	AWECharacter* Character = PC ? Cast<AWECharacter>(PC->GetPawn()) : nullptr;
	if (!Character || !Save)
	{
		return;
	}
	const FString Key = GetPlayerKey(PC);
	const FWESavedPlayer* Saved = Save->Players.FindByPredicate([&Key](const FWESavedPlayer& P) { return P.PlayerKey == Key; });
	if (!Saved)
	{
		return;
	}
	Character->TeleportTo(Saved->PawnTransform.GetLocation(), Saved->PawnTransform.Rotator(), false, true);
	PC->ClientSetRotation(Saved->ControlRotation);
	if (UWEInventoryComponent* Inventory = Character->GetPlayerInventory())
	{
		Inventory->SetAllSlots(FromSavedSlots(Saved->Inventory));
	}
}

// --- Files -----------------------------------------------------------------------------------------

bool FWESaveSystem::WriteToFile(const UWESaveGame* Save, const FString& Path, FText& OutError)
{
	TArray<uint8> Payload;
	if (!Save || !UGameplayStatics::SaveGameToMemory(const_cast<UWESaveGame*>(Save), Payload))
	{
		OutError = LOCTEXT("SerializeFailed", "Spielstand konnte nicht erstellt werden.");
		return false;
	}

	TArray<uint8> FileBytes;
	FMemoryWriter Writer(FileBytes);
	uint32 Magic = FileMagic;
	uint32 Format = FileFormatVersion;
	uint32 Size = Payload.Num();
	uint32 Crc = FCrc::MemCrc32(Payload.GetData(), Payload.Num());
	Writer << Magic << Format << Size << Crc;
	Writer.Serialize(Payload.GetData(), Payload.Num());

	IFileManager& FM = IFileManager::Get();
	FM.MakeDirectory(*FPaths::GetPath(Path), true);
	const FString TempPath = Path + TEXT(".tmp");
	const FString BackupPath = Path + TEXT(".bak");

	if (!FFileHelper::SaveArrayToFile(FileBytes, *TempPath))
	{
		OutError = LOCTEXT("WriteFailed", "Spielstand konnte nicht geschrieben werden.");
		return false;
	}
	if (FM.FileExists(*Path) && !FM.Move(*BackupPath, *Path, /*Replace*/ true))
	{
		UE_LOG(LogWorkshopEmpire, Warning, TEXT("Save: could not create backup %s"), *BackupPath);
	}
	if (!FM.Move(*Path, *TempPath, /*Replace*/ true))
	{
		OutError = LOCTEXT("MoveFailed", "Spielstand konnte nicht abgeschlossen werden.");
		return false;
	}
	return true;
}

UWESaveGame* FWESaveSystem::ReadSingleFile(const FString& Path, FText& OutError)
{
	TArray<uint8> FileBytes;
	if (!FFileHelper::LoadFileToArray(FileBytes, *Path, FILEREAD_Silent))
	{
		OutError = LOCTEXT("NotFound", "Spielstand nicht gefunden.");
		return nullptr;
	}
	constexpr int32 HeaderSize = 4 * sizeof(uint32);
	if (FileBytes.Num() < HeaderSize)
	{
		OutError = LOCTEXT("Corrupt", "Spielstand ist beschädigt.");
		return nullptr;
	}

	FMemoryReader Reader(FileBytes);
	uint32 Magic = 0, Format = 0, Size = 0, Crc = 0;
	Reader << Magic << Format << Size << Crc;
	if (Magic != FileMagic || Format > FileFormatVersion || Size != static_cast<uint32>(FileBytes.Num() - HeaderSize)
		|| FCrc::MemCrc32(FileBytes.GetData() + HeaderSize, Size) != Crc)
	{
		OutError = LOCTEXT("Corrupt", "Spielstand ist beschädigt.");
		return nullptr;
	}

	TArray<uint8> Payload(FileBytes.GetData() + HeaderSize, Size);
	UWESaveGame* Save = Cast<UWESaveGame>(UGameplayStatics::LoadGameFromMemory(Payload));
	if (!Save)
	{
		OutError = LOCTEXT("Corrupt", "Spielstand ist beschädigt.");
		return nullptr;
	}
	if (Save->SaveVersion > UWESaveGame::CurrentVersion)
	{
		OutError = LOCTEXT("TooNew", "Spielstand stammt aus einer neueren Spielversion.");
		return nullptr;
	}
	// Future: migrate older SaveVersion values here.
	return Save;
}

UWESaveGame* FWESaveSystem::ReadFromFile(const FString& Path, FText& OutError, bool* bOutUsedBackup)
{
	if (bOutUsedBackup)
	{
		*bOutUsedBackup = false;
	}
	FText MainError;
	if (UWESaveGame* Save = ReadSingleFile(Path, MainError))
	{
		return Save;
	}
	FText BackupError;
	if (UWESaveGame* Save = ReadSingleFile(Path + TEXT(".bak"), BackupError))
	{
		UE_LOG(LogWorkshopEmpire, Warning, TEXT("Save: %s unreadable (%s), loaded backup instead"), *Path, *MainError.ToString());
		if (bOutUsedBackup)
		{
			*bOutUsedBackup = true;
		}
		return Save;
	}
	OutError = MainError;
	return nullptr;
}

bool FWESaveSystem::SaveWorld(UWorld* World, const FString& Slot, FText& OutError)
{
	if (!IsValidSlotName(Slot))
	{
		OutError = LOCTEXT("BadSlot", "Ungültiger Name für den Spielstand.");
		return false;
	}
	if (!World || World->GetNetMode() == NM_Client)
	{
		OutError = LOCTEXT("HostOnly", "Nur der Host kann speichern.");
		return false;
	}
	const UWESaveGame* Save = Capture(World);
	const bool bOk = WriteToFile(Save, GetSlotPath(Slot), OutError);
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Save '%s': %s (%d players, %d world items, %d containers)"), *Slot,
		bOk ? TEXT("ok") : *OutError.ToString(), Save->Players.Num(), Save->WorldItems.Num(), Save->Containers.Num());
	return bOk;
}

bool FWESaveSystem::LoadWorld(UWorld* World, const FString& Slot, FText& OutError, UWESaveGame** OutSave)
{
	if (!IsValidSlotName(Slot))
	{
		OutError = LOCTEXT("BadSlot", "Ungültiger Name für den Spielstand.");
		return false;
	}
	if (!World || World->GetNetMode() == NM_Client)
	{
		OutError = LOCTEXT("HostOnlyLoad", "Nur der Host kann laden.");
		return false;
	}
	bool bUsedBackup = false;
	UWESaveGame* Save = ReadFromFile(GetSlotPath(Slot), OutError, &bUsedBackup);
	if (!Save)
	{
		return false;
	}
	if (Save->MapName != UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()))
	{
		OutError = LOCTEXT("OtherMap", "Spielstand gehört zu einer anderen Karte.");
		return false;
	}
	Apply(World, Save);
	if (OutSave)
	{
		*OutSave = Save;
	}
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Load '%s': ok%s (%d players, %d world items, %d containers)"), *Slot,
		bUsedBackup ? TEXT(" (from backup)") : TEXT(""), Save->Players.Num(), Save->WorldItems.Num(), Save->Containers.Num());
	return true;
}

#undef LOCTEXT_NAMESPACE
