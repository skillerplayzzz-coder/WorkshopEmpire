#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WEHUDWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UImage;
class UTextBlock;
class UVerticalBox;
class UWEInteractionComponent;

/**
 * Minimal first-person HUD: crosshair, interaction hints ("[E] Aufheben"), held item and the
 * examine panel. Built in C++ so it needs no widget asset; a designer can later subclass it in
 * UMG and restyle it. The widget only reads gameplay state, it never changes it.
 */
UCLASS()
class WORKSHOPEMPIRE_API UWEHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Formats one prompt line, e.g. "[E] Aufheben". Public for tests. */
	static FText FormatPrompt(const FText& KeyText, const FText& ActionText);

	/** Short notification in the upper centre ("Inventar voll", "Gespeichert"). */
	void ShowMessage(const FText& Message, float Seconds = 3.f);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UWEInteractionComponent* GetInteraction() const;
	UTextBlock* MakeText(int32 Size, const FLinearColor& Color, bool bWrap = false);

	static constexpr int32 MaxPromptLines = 5;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Canvas;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Crosshair;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> PromptLines;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeldText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ExaminePanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ExamineTitle;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ExamineDescription;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ExamineDetails;

	/** Work progress (disassembly step) of the local player. */
	UPROPERTY(Transient)
	TObjectPtr<class UProgressBar> WorkProgress;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> WorkText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> WorkBox;

	void UpdateWorkProgress();

	/** "Erste Schritte" / next goal, top left. */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> GuidePanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GuideText;

	/** Company balance, top right. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BalanceText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> MessagePanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MessageText;

	float MessageTimeLeft = 0.f;

	/** Build mode panel (Phase 8), left side. */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> BuildPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BuildMenuText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BuildStatusText;

	void UpdateBuildPanel();
};
