#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WEHUDWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UHorizontalBox;
class UImage;
class UProgressBar;
class UTextBlock;
class UVerticalBox;
class UWEInteractionComponent;

/** A hint placed in the 3D world (tutorial): marker over Location with a short label. */
struct FWEWorldMarker
{
	FVector Location = FVector::ZeroVector;
	FText Label;
};

/**
 * First-person HUD (Phase 17.5 redesign): crosshair, interaction prompts with key caps, the objective card
 * (tutorial / next goal), company status (tier, account, power), stacked notifications, held item, work
 * progress, examine card, build mode list and markers in the world. Built in C++ in the WEUI style; it only
 * reads gameplay state.
 */
UCLASS()
class WORKSHOPEMPIRE_API UWEHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Formats one prompt line, e.g. "[E] Aufheben". Public for tests. */
	static FText FormatPrompt(const FText& KeyText, const FText& ActionText);

	/** Short notification in the upper centre ("Inventar voll", "Gespeichert"). */
	void ShowMessage(const FText& Message, float Seconds = 3.5f);

	/** Hints in the world (tutorial). Empty = none. */
	void SetWorldMarkers(const TArray<FWEWorldMarker>& Markers);

	/** Hides everything but notifications (workbench close-up, Ferrit OS). */
	void SetFocusMode(bool bFocus);

	/** Texts of the visible notifications (tests). */
	TArray<FString> GetMessageTexts() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UWEInteractionComponent* GetInteraction() const;

	void BuildStatusCard();
	void BuildObjectiveCard();
	void BuildCenter();
	void BuildBottom();
	void BuildExamineCard();
	void BuildBuildPanel();

	void UpdatePrompts();
	void UpdateStatus();
	void UpdateObjective();
	void UpdateWorkProgress();
	void UpdateHeld();
	void UpdateExamine();
	void UpdateBuildPanel();
	void UpdateToasts(float DeltaTime);
	void UpdateMarkers();

	static constexpr int32 MaxPromptLines = 6;
	static constexpr int32 MaxToasts = 4;
	static constexpr int32 MaxMarkers = 3;
	static constexpr int32 BuildRows = 12;

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY(Transient) TObjectPtr<UImage> Crosshair;

	UPROPERTY(Transient) TArray<TObjectPtr<UHorizontalBox>> PromptRows;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> PromptKeys;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> PromptCaps;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> PromptTexts;

	// Status card (top right)
	UPROPERTY(Transient) TObjectPtr<UWidget> StatusCard;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TierText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BalanceText;
	UPROPERTY(Transient) TObjectPtr<UWidget> PowerRow;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> PowerBar;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PowerText;

	// Objective card (top left)
	UPROPERTY(Transient) TObjectPtr<UWidget> ObjectiveCard;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ObjectiveLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ObjectiveText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ObjectiveHint;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> ObjectiveProgress;

	// Bottom: held item and work progress
	UPROPERTY(Transient) TObjectPtr<UWidget> HeldCard;
	UPROPERTY(Transient) TObjectPtr<UImage> HeldIcon;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HeldName;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HeldDetail;
	UPROPERTY(Transient) TObjectPtr<UWidget> WorkCard;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WorkText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WorkPercent;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> WorkProgress;

	// Examine card (right)
	UPROPERTY(Transient) TObjectPtr<UWidget> ExamineCard;
	UPROPERTY(Transient) TObjectPtr<UImage> ExamineIcon;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ExamineTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ExamineDescription;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> ExamineDetails;

	// Build mode (left)
	UPROPERTY(Transient) TObjectPtr<UWidget> BuildCard;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> BuildList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BuildStatusText;
	UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> BuildRowFrames;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> BuildRowNames;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> BuildRowPrices;

	// Notifications
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> ToastBox;
	struct FToast
	{
		TObjectPtr<UBorder> Frame;
		TObjectPtr<UTextBlock> Label;
		TObjectPtr<UImage> IconImage;
		float Age = 0.f;
		float Life = 0.f;
		bool bUsed = false;
	};
	TArray<FToast> Toasts;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> ToastWidgets;

	// World markers
	TArray<FWEWorldMarker> Markers;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> MarkerWidgets;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> MarkerLabels;

	bool bFocusMode = false;
	float PulseTime = 0.f;
	FString LastBuildSignature;
	FString LastExamineTitle;
};
