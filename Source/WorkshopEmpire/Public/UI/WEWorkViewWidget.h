#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/WEItemTypes.h"
#include "UI/WEUIKit.h"
#include "WEWorkViewWidget.generated.h"

class AWEWorkbench;
class AWEWorkRig;
class UBorder;
class UButton;
class UCanvasPanel;
class UHorizontalBox;
class UImage;
class UProgressBar;
class UTextBlock;
class UVerticalBox;

/**
 * Work view at the workbench (Phase 17.5): the camera looks down at the device; the player picks a tool
 * (1-9, mouse wheel) and holds the left mouse button on each screw, clip, plug, the cover and the parts
 * until the step is done - in the order a real device comes apart (screws before the cover). Repairs work
 * the same way: measuring points, broken part out, spare part in, power test, cleaning.
 *
 * Every finished task is a server request (AWEWorkbench::DoManualTask checks tool, order and time); the
 * props (AWEWorkRig) follow the bench's replicated task mask. "Automatisch" runs the step on the timer
 * as before (and employees always work that way).
 */
UCLASS()
class WORKSHOPEMPIRE_API UWEWorkViewWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(AWEWorkbench* InBench, bool bInRepair);
	AWEWorkbench* GetBench() const { return Bench.Get(); }
	bool IsRepairMode() const { return bRepair; }
	void SetRepairMode(bool bInRepair);
	/** Removes the props (when the view closes). */
	void Shutdown();

	/** Test automation: tool selection and one finished task without the mouse. */
	void DevSelectTool(EWEToolType Tool);
	int32 DevFindAvailableTask() const;
	void DevHoldTask(int32 Task);
	/** Test automation: picks the next available task, its tool, and starts holding (false if nothing to do yet). */
	bool DevWorkNext();
	bool DevIsIdle() const { return HoldTask == INDEX_NONE && PendingTask == INDEX_NONE; }
	FString DescribeState() const;
	/** Devices with a part layout: pick a part (as a click on it does). */
	void DevSelectPart(int32 Part) { SelectPart(Part); }
	int32 GetSelectedPart() const { return SelectedPart; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UFUNCTION() void HandleClose();
	UFUNCTION() void HandleAuto();
	UFUNCTION() void HandleModeDisassemble();
	UFUNCTION() void HandleModeRepair();
	UFUNCTION() void HandleTool0();
	UFUNCTION() void HandleTool1();
	UFUNCTION() void HandleTool2();
	UFUNCTION() void HandleTool3();
	UFUNCTION() void HandleTool4();
	UFUNCTION() void HandleTool5();
	UFUNCTION() void HandleTool6();
	UFUNCTION() void HandleTool7();

	void SelectToolIndex(int32 Index);
	void RefreshTools();
	void RefreshInfo();
	void UpdateMarkers();
	void TryStartWork();
	int32 FindTaskAt(const FVector2D& Position) const;
	bool CanDisassemble() const;
	bool CanRepair() const;
	void Hint(const FText& Text, bool bError = false);
	FVector2D MousePosition() const;

	// Camera around the device (Phase 18a): right mouse button turns, the wheel zooms, a picked part is brought close.
	void InitOrbit();
	void UpdateOrbit(float DeltaTime);
	float OrbitYaw = 0.f;
	float OrbitPitch = 45.f;
	float OrbitDist = 80.f;
	float OrbitGoalDist = 80.f;
	float OrbitBaseDist = 80.f;
	FVector OrbitTarget = FVector::ZeroVector;
	FVector OrbitGoal = FVector::ZeroVector;
	bool bOrbiting = false;

	// Part layout: the part the player picked.
	bool IsPartMode() const;
	class AWEAssemblyView* GetView() const;
	int32 PartUnderMouse() const;
	void SelectPart(int32 Part);
	int32 SelectedPart = INDEX_NONE;
	int32 LastRemovedParts = 0;
	class AWECharacter* GetCharacter() const;

	TWeakObjectPtr<AWEWorkbench> Bench;
	UPROPERTY(Transient) TObjectPtr<AWEWorkRig> Rig;
	bool bRepair = false;

	// Tools the player carries (index 0 = hands).
	TArray<EWEToolType> Tools;
	int32 SelectedTool = 0;
	float ToolRefreshTimer = 0.f;

	// Holding the mouse button on a task.
	int32 HoldTask = INDEX_NONE;
	float HoldTime = 0.f;
	int32 PendingTask = INDEX_NONE;
	float PendingTime = 0.f;
	float LastStartRequest = -10.f;
	uint32 MarkerSignature = 0;
	float HintTime = 0.f;

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DeviceTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DeviceInfo;
	UPROPERTY(Transient) TObjectPtr<UButton> ModeDisassemble;
	UPROPERTY(Transient) TObjectPtr<UButton> ModeRepair;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ModeDisassembleLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ModeRepairLabel;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> StepList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StepTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StepCount;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> StepBar;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HintText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> YieldText;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> ToolBar;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> AutoLabel;
	UPROPERTY(Transient) TObjectPtr<UImage> CursorIcon;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CenterText;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> Markers;
	UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> MarkerRings;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> MarkerIcons;
	UPROPERTY(Transient) TArray<TObjectPtr<UProgressBar>> MarkerBars;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> ToolButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ToolLabels;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> ToolIcons;
	FString LastInfoSignature;
	WEUI::FOpenAnim Anim;
	UPROPERTY(Transient) TObjectPtr<UWidget> LeftCard;
	UPROPERTY(Transient) TObjectPtr<UWidget> RightCard;
};
