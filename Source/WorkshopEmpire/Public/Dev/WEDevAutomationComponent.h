#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WEDevAutomationComponent.generated.h"

class AWECharacter;

/**
 * Development-only scripted player actions for unattended tests (never added in Shipping).
 *
 * Added to the local PlayerController when one of these command-line switches is present:
 *   -WESmokeRole=A|B   two-client network smoke test (pick up / conflict / drop / hand-over / reach check)
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
