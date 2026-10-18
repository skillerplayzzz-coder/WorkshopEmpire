#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WEIntroWidget.generated.h"

class UImage;
class UMediaPlayer;
class UMediaSoundComponent;
class UMediaTexture;
class UTextBlock;

/**
 * Intro video at game start (Content/Movies/intro.mp4), shown full screen over the main menu before it appears.
 * [Esc], [Leertaste], [Enter] or a click skips it. Calls OnFinished when it ends, is skipped or cannot play.
 */
UCLASS()
class WORKSHOPEMPIRE_API UWEIntroWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Path of the video on disk (staged next to the game in packaged builds). */
	static FString GetMoviePath();
	static bool HasMovie();

	void Start(TFunction<void()> InOnFinished);
	void Finish();
	bool IsPlaying() const { return !bFinished; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UFUNCTION() void HandleEndReached();
	UFUNCTION() void HandleOpenFailed(FString FailedUrl);

	UPROPERTY(Transient) TObjectPtr<UMediaPlayer> Player;
	UPROPERTY(Transient) TObjectPtr<UMediaTexture> Texture;
	UPROPERTY(Transient) TObjectPtr<UMediaSoundComponent> Sound;
	UPROPERTY(Transient) TObjectPtr<UImage> Video;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SkipHint;

	TFunction<void()> OnFinished;
	bool bFinished = false;
	float Elapsed = 0.f;
	float Fade = 0.f;
};
