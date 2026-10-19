#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/WECharacterLook.h"
#include "UI/WEUIKit.h"
#include "WEMirrorWidget.generated.h"

class UTextBlock;
class UWESettingRow;

/**
 * Phase 19: the look editor at the workshop mirror. One row per part of the look (like the character creator);
 * every change is sent to the server at once, so the player sees it in the mirror and the others see it too.
 * "Fertig" keeps the look in the local profile as well.
 */
UCLASS()
class WORKSHOPEMPIRE_API UWEMirrorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Steps one field of the look (also used by the dev shots). */
	void StepLook(int32 Field, int32 Direction);
	const FWECharacterLook& GetEditLook() const { return EditLook; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	void Refresh();
	void Send();
	UFUNCTION() void HandleRandom();
	UFUNCTION() void HandleView();
	UFUNCTION() void HandleDone();

	UPROPERTY(Transient) TArray<TObjectPtr<UWESettingRow>> Rows;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;
	UPROPERTY(Transient) TObjectPtr<UWidget> Card;
	FWECharacterLook EditLook;
	WEUI::FOpenAnim Anim;
	bool bWasThirdPerson = false;
};
