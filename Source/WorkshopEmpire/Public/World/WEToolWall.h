#pragma once

#include "CoreMinimal.h"
#include "Items/WEItemTypes.h"
#include "World/WEPlaceholderProp.h"
#include "WEToolWall.generated.h"

class UWEItemDefinition;
class UStaticMeshComponent;

/**
 * Phase 21: the tool wall (Werkzeugwand). A pegboard with one hook per storage slot; every tool in the
 * storage hangs on its hook as a model, an empty hook shows the painted outline of the tool that belongs
 * there. [E] opens the storage as before, [R] hangs up all tools the player carries, [Q] takes the tool
 * the player looks at. Tools on a wall within reach count as at hand at the workbench.
 * Server: storage changes only. Clients: the hanging models follow the replicated storage.
 */
UCLASS()
class WORKSHOPEMPIRE_API AWEToolWall : public AWEPlaceholderProp
{
	GENERATED_BODY()

public:
	/** How far from the wall (cm) its tools still count as at hand. */
	static constexpr float ReachCm = 600.f;
	static constexpr int32 HookColumns = 5;

	/** All tool definitions the character can use: its own inventory plus tool walls within reach. */
	static void GatherTools(const AActor* Character, TArray<const UWEItemDefinition*>& OutTools);

	/** Hook position (actor space) of a storage slot. */
	FVector GetHookLocation(int32 Slot) const;
	/** Storage slot of the tool the character looks at (INDEX_NONE = none). */
	int32 FindAimedTool(const AWECharacter* Character) const;
	/** Hangs up every tool from the character's inventory. Server. Returns the number hung up. */
	int32 HangUpTools(AWECharacter* Character);
	/** Takes the tool from a hook into the character's inventory. Server. */
	bool TakeTool(AWECharacter* Character, int32 Slot);
	/** Tests: hooks that show a tool model right now. */
	int32 GetHangingCount() const;

	// IWEInteractable
	virtual bool GetSecondaryInteraction(const AWECharacter* Interactor, FText& OutText) const override;
	virtual void InteractSecondary(AWECharacter* Interactor) override;
	virtual bool GetAlternativeInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const override;
	virtual void InteractAlternative(AWECharacter* Interactor) override;
	virtual bool GetExamineInfo(FWEExamineInfo& OutInfo) const override;

protected:
	virtual void BeginPlay() override;
	virtual void RebuildVisuals() override;
	void OnStorageChanged(class UWEInventoryComponent* Changed);
	void ApplyStorageChange();
	bool bRefreshQueued = false;
	void RefreshHooks();
	static int32 CountTools(const class UWEInventoryComponent* Inventory);

	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Board;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Hooks;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Hanging;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Outlines;
	/** Per hook: the tool that hung there last (its outline stays when it is taken). */
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMesh>> OutlineMesh;
	TArray<EWEToolType> OutlineType;
	bool bBoundStorage = false;
};
