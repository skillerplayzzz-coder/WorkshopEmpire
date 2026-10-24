#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "WEDevAutomationComponent.generated.h"

class AWECharacter;

/**
 * Development-only scripted player actions for unattended tests (never added in Shipping).
 *
 * Added to the local PlayerController when one of these command-line switches is present:
 *   -WESmokeRole=A..X  network smoke tests per phase (A|B: pick up / conflict / drop / hand-over / reach check)
 *   -WEShots=<Name>    takes a series of in-game screenshots (overview, pick up, examine), then quits
 *
 * All actions go through the normal input path (UWEInteractionComponent requests -> server RPCs),
 * so the server validates them exactly like player input. Results are logged with the prefix "SMOKE".
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWEDevAutomationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEDevAutomationComponent();

	/** True if the command line requests any dev automation. */
	static bool IsRequested();

protected:
	virtual void BeginPlay() override;

	void Schedule(float Delay, TFunction<void()> Action);
	AWECharacter* GetCharacter() const;
	AActor* FindItemByName(const FString& Substring) const;
	AActor* FindDoor() const;
	void LogState(const TCHAR* Step) const;
	void LookAt(const AActor* Target) const;
	void LookYawPitch(float Yaw, float Pitch) const;
	void Shot(const FString& Suffix) const;
	void Quit() const;

	void RunSmokeRoleA();
	void RunSmokeRoleB();
	void RunShots();

	// Phase 2: inventory scenarios (-WESmokeRole=C|D, -WEShots=P2...)
	void RunSmokeRoleC();
	void RunSmokeRoleD();
	void RunInventoryShots();

	// Phase 4: economy scenarios (-WESmokeRole=G|H, -WEShots=P4...)
	void RunSmokeRoleG();
	void RunSmokeRoleH();
	void RunEconomyShots();
	class AWETraderTerminal* FindTerminal() const;
	void TeleportToTerminal(float SideOffset) const;
	void LogEconomyState(const TCHAR* Step) const;
	int32 FindOfferIndex(const FString& ItemName) const;

	// Phase 5: full loop / progression (-WESmokeRole=I|J on a listen server, -WEShots=P5...)
	void RunSmokeRoleI();
	void RunSmokeRoleJ();
	void RunLoopShots();
	void SellAllParts();
	void LogProgressState(const TCHAR* Step) const;

	// Phase 8: build scenarios (-WESmokeRole=O|P on a dedicated server, -WEShots=P8...)
	void RunSmokeRoleO();
	void RunSmokeRoleP();
	void RunBuildShots();
	void LogFurniture(const TCHAR* Step) const;
	class AWEPlaceholderProp* FindFurniture(const TCHAR* FurnitureId) const;
	int32 FurnitureIndex(const TCHAR* FurnitureId) const;

	// Phase 9: machines
	class AWEMachine* FindMachine(const TCHAR* FurnitureId) const;
	AActor* FindNearestItem(const FString& Substring) const;
	void LogMachines(const TCHAR* Step) const;
	void PlaceMachines();
	void TransferToMachine(const TCHAR* ItemId, class AWEMachine* Machine);
	void RunMachineShots();
	void RunSmokeRoleQ();
	void RunSmokeRoleR();

	// Phase 10: production line
	void PlaceFurnitureAt(const TCHAR* FurnitureId, float X, float Y, float Yaw);
	void PlaceProductionLine();
	void LogFlow(const TCHAR* Step) const;
	class AWEConveyor* FindConveyorNear(const FVector& Point) const;
	void RunFlowShots();
	void RunSmokeRoleS();
	void RunSmokeRoleT();

	// Phase 11: staff and logistics
	class AWEEmployee* FindEmployee(uint8 Role) const;
	void LogStaff(const TCHAR* Step) const;
	void BuyUpgradeAtTerminal(const TCHAR* UpgradeId);
	void TransferToStorage(const TCHAR* ItemId, class UWEInventoryComponent* Storage);
	void RunStaffShots();
	void RunSmokeRoleU();
	void RunSmokeRoleV();
	class UWEBuildComponent* GetBuild() const;

	// Phase 12: company expansion (-WESmokeRole=W|X on a dedicated server, -WEShots=P12...)
	void LogExpansion(const TCHAR* Step) const;
	class AWEGarageDoor* FindDoorNear(const FVector& Point) const;
	void RunExpansionShots();
	void RunBalanceShots();
	/** Phase 17: full late-game factory (performance, save/load at scale; -WEStressMinutes=N for a soak). */
	void RunStressShots();
	/** Phase 17: multiplayer stress client (-WESmokeRole=Z, -WEStressMinutes=N). */
	void RunSmokeRoleZ();
	/** Phase 17: UI and input test (-WEShots=P17UI, at any resolution). */
	void RunUiInputShots();
	void PressKey(const FKey& Key, bool bDown);
	void PressSlateKey(const FKey& Key);
	void TapKey(const FKey& Key);
	void LogUiFit(const TCHAR* Name, class UUserWidget* Widget);
	void StressAction();
	void StressMove();
	FString DescribeStressState() const;
	FRandomStream StressRandom;
	FVector StressMoveDirection = FVector::ForwardVector;
	int32 StressActions = 0;
	FTimerHandle StressActionTimer;
	FTimerHandle StressMoveTimer;
	void LogPerfWindow(const FString& Label, double Seconds, uint64 Frames);
	FString DescribeWorldCounts() const;
	FString StressCountsBeforeLoad;
	double SoakTime = 0.0;
	uint64 SoakFrame = 0;
	void RunSmokeRoleW();
	void RunSmokeRoleX();

	/** Phase 17.5: main menu, tutorial HUD, inventory, Ferrit OS apps, workshop look, pause menu (-WEShots=P175...). */
	void RunPolishShots();
	void RunPolishGameShots();
	int32 FindMySlot(const TCHAR* ItemId) const;
	/** Phase 17.5: hands-on disassembly in the work view (-WEShots=P175W on L_Garage). */
	void RunWorkViewShots();
	void RunVanShots();
	/** After Phase 18: PC put back together and tested in the work view, goods carried into and out of the van. */
	void RunReassemblyShots();
	/** Phase 19: wardrobe in the creator (-WEShots=P19C), night, customers, classifieds, orders, mirror (-WEShots=P19G). */
	void RunPhase19Shots();
	void RunPhase19GameShots();
	/** Phase 20: brand and logo, employees as figures, repairs, atmosphere, settings (-WEShots=P20). */
	void RunPhase20Shots();
	/** Phase 21: repair feel, conditions, cleaning, staff at work, tool wall, courier, difficulty (-WEShots=P21). */
	void RunPhase21Shots();
	void RunSmokeRoleP21(bool bActor);
	void PlaceHeldOnBench();
	/** Quality update: customer order carried to the bench, work mode, cars outside (-WEShots=QU...). */
	void RunQualityShots();
	void LogCarry(const TCHAR* Step) const;
	TWeakObjectPtr<class ACameraActor> CatCamera;
	/** Phase 20 trailer: camera moves through the scenes, every frame saved (-WEShots=P20T). */
	void RunTrailer();
	FTSTicker::FDelegateHandle TrailerTicker;
	void WorkViewTick();
	FTimerHandle WorkViewTimer;
	int32 WorkViewTasks = 0;

	// Phase 13/14: sessions through the real menu (-WESmokeRole=MH|M1|M2|M3|M5 listen server + LAN, SH|SC Steam; -WEShots=P13 / P13C)
	bool IsInMenu() const;
	void LogCoop(const TCHAR* Step) const;
	void FindAndJoin(int32 TriesLeft);
	void RunCoopRole();
	void RunSteamRole();
	void RunCoopShots();

	// Phase 15: fixed camera views for the look (-WEShots=P15...).
	void RunVisualShots();
	void ShotFrom(float At, const FVector& From, const FVector& LookAt, const TCHAR* Name);
	double PerfStart = 0.0;
	uint64 PerfFrameStart = 0;
	int32 PerfFrames = 0;
	double PerfSeconds = 0.0;
	double PerfGpuMs = 0.0;

	// Phase 7: procurement scenarios (-WESmokeRole=M|N on a dedicated server, -WEShots=P7...)
	void RunSmokeRoleM();
	void RunSmokeRoleN();
	void RunProcurementShots();
	void LogAuctionState(const TCHAR* Step) const;
	void BidUntilLeading(int32 Tries);
	class AWEPickupItem* FindItemById(const TCHAR* ItemId) const;

	// Phase 6: repair scenarios (-WESmokeRole=K|L on a dedicated server, -WEShots=P6...)
	void RunSmokeRoleK();
	void RunSmokeRoleL();
	void RunRepairShots();
	/** Keeps starting the next repair step ([Q]) until nothing is left; then calls OnAutoRepairDone. */
	void StartAutoRepair(TFunction<void()> OnDone);
	void AutoRepairTick();
	void BuyPartsForPlacedItem();
	void LogRepairState(const TCHAR* Step) const;
	void TeleportToBench(float SideOffset = 0.f) const;
	class AWEPickupItem* FindCustomerDevice() const;
	int32 FindOfferFor(const class UWEItemDefinition* Def) const;
	FTimerHandle AutoRepairTimer;
	TFunction<void()> OnAutoRepairDone;
	bool bAutoRepairSawBroken = false;

	// Phase 3: workbench scenarios (-WESmokeRole=E|F, -WEShots=P3...)
	void RunSmokeRoleE();
	void RunSmokeRoleF();
	void RunWorkbenchShots();
	class AWEWorkbench* FindBench() const;
	void LookAtPoint(const FVector& Point) const;
	void LogBenchState(const TCHAR* Step) const;
	/** Keeps starting the next disassembly step whenever the bench is idle (until the item is gone). */
	void StartAutoSteps();
	void AutoStepTick();
	FTimerHandle AutoStepTimer;
	bool bShotFinished = false;

	class UWEInventoryComponent* GetMyInventory() const;
	class UWEInventoryComponent* FindStorage(const FString& NameSubstring) const;
	class UWEInventoryComponent* FindOtherPlayerInventory() const;
	static FString DescribeInventory(const class UWEInventoryComponent* Inventory);
	int32 CountEverywhere(const FString& ItemName) const;
	int32 FindSlotWith(const class UWEInventoryComponent* Inventory, const FString& ItemName) const;
	void LogInventoryState(const TCHAR* Step) const;
	void StoreItem(const FString& ItemName);

	FString Role;
	FString ShotName;
	TArray<FTimerHandle> Timers;
};
