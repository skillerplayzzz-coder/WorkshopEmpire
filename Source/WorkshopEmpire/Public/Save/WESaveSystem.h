#pragma once

#include "CoreMinimal.h"

class APlayerController;
class UWESaveGame;
class UWorld;
struct FWEItemInstance;
struct FWESavedItem;
struct FWESavedSlot;
struct FWEInventorySlotData;

/**
 * Captures/applies the world state to/from UWESaveGame and reads/writes save files safely.
 *
 * File format: "WESV" magic, format version, payload size, CRC32, then the UE SaveGame payload.
 * Writes go to "<slot>.tmp" first; the previous file is kept as "<slot>.bak". Loading checks
 * magic/size/CRC before deserialising and falls back to the backup if the main file is damaged.
 *
 * Server/host only. All functions are static so they can be tested without a running session.
 */
class WORKSHOPEMPIRE_API FWESaveSystem
{
public:
	static constexpr uint32 FileMagic = 0x56534557; // "WESV"
	static constexpr uint32 FileFormatVersion = 1;

	/** Letters, digits, '_' and '-' only, 1-32 chars. */
	static bool IsValidSlotName(const FString& Slot);
	static FString GetSaveDirectory();
	static FString GetSlotPath(const FString& Slot);

	/** Full save: capture + write. */
	static bool SaveWorld(UWorld* World, const FString& Slot, FText& OutError);

	/** Full load: read (with backup fallback) + validate + apply. OutSave receives the loaded data (for late joiners). */
	static bool LoadWorld(UWorld* World, const FString& Slot, FText& OutError, UWESaveGame** OutSave = nullptr);

	static UWESaveGame* Capture(UWorld* World);
	static void Apply(UWorld* World, const UWESaveGame* Save);
	static void ApplyPlayer(APlayerController* PC, const UWESaveGame* Save);

	static bool WriteToFile(const UWESaveGame* Save, const FString& Path, FText& OutError);
	static UWESaveGame* ReadFromFile(const FString& Path, FText& OutError, bool* bOutUsedBackup = nullptr);

	static FString GetPlayerKey(const APlayerController* PC);

	static FWESavedItem ToSaved(const FWEItemInstance& Item);
	static bool FromSaved(const FWESavedItem& Saved, FWEItemInstance& OutItem);
	static TArray<FWESavedSlot> ToSavedSlots(const TArray<FWEInventorySlotData>& Slots);
	static TArray<FWEInventorySlotData> FromSavedSlots(const TArray<FWESavedSlot>& Slots);

private:
	static UWESaveGame* ReadSingleFile(const FString& Path, FText& OutError);
};
