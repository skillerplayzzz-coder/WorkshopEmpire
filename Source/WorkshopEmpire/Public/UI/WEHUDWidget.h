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

	static constexpr int32 MaxPromptLines = 3;

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

	UPROPERTY(Transient)
	TObjectPtr<UBorder> MessagePanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MessageText;

	float MessageTimeLeft = 0.f;
};
