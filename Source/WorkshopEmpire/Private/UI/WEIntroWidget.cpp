#include "UI/WEIntroWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "FileMediaSource.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "MediaPlayer.h"
#include "MediaSoundComponent.h"
#include "MediaTexture.h"
#include "Misc/Paths.h"
#include "UI/WEUIKit.h"
#include "WELog.h"

#define LOCTEXT_NAMESPACE "WEIntro"

namespace WEIntro
{
	constexpr float StartTimeout = 4.f;   // the video did not start: skip it
	constexpr float MaxLength = 180.f;    // safety net
}

FString UWEIntroWidget::GetMoviePath()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Movies/intro.mp4"));
}

bool UWEIntroWidget::HasMovie()
{
	return IFileManager::Get().FileExists(*GetMoviePath());
}

void UWEIntroWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;
	UBorder* Black = WEUI::Box(WidgetTree, FLinearColor::Black, FMargin(0.f), FLinearColor::Transparent, 0.f);
	if (UOverlaySlot* S = Root->AddChildToOverlay(Black))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Fill);
	}
	// The video keeps its 16:9 shape at every resolution (black bars where the screen is wider or taller).
	UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>();
	Scale->SetStretch(EStretch::ScaleToFit);
	Video = WidgetTree->ConstructWidget<UImage>();
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(1920.f);
	Size->SetHeightOverride(1080.f);
	Size->SetContent(Video);
	Scale->SetContent(Size);
	if (UOverlaySlot* S = Root->AddChildToOverlay(Scale))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Fill);
	}
	SkipHint = WEUI::Text(WidgetTree, LOCTEXT("Skip", "[Leertaste] Überspringen"), 12, WEUI::Col::TextDim, WEUI::EFont::Strong);
	SkipHint->SetRenderOpacity(0.f);
	if (UOverlaySlot* S = Root->AddChildToOverlay(SkipHint))
	{
		S->SetHorizontalAlignment(HAlign_Right);
		S->SetVerticalAlignment(VAlign_Bottom);
		S->SetPadding(FMargin(0.f, 0.f, 36.f, 28.f));
	}
}

void UWEIntroWidget::Start(TFunction<void()> InOnFinished)
{
	OnFinished = MoveTemp(InOnFinished);
	bFinished = false;
	Elapsed = 0.f;
	Player = NewObject<UMediaPlayer>(this);
	Player->PlayOnOpen = true;
	Player->SetLooping(false);
	Player->OnEndReached.AddDynamic(this, &UWEIntroWidget::HandleEndReached);
	Player->OnMediaOpenFailed.AddDynamic(this, &UWEIntroWidget::HandleOpenFailed);
	Texture = NewObject<UMediaTexture>(this);
	Texture->AutoClear = true;
	Texture->ClearColor = FLinearColor::Black;
	Texture->SetMediaPlayer(Player);
	Texture->UpdateResource();
	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.ImageSize = FVector2D(1920.f, 1080.f);
	Video->SetBrush(Brush);
	if (APlayerController* PC = GetOwningPlayer())
	{
		Sound = NewObject<UMediaSoundComponent>(PC);
		Sound->SetMediaPlayer(Player);
		Sound->bIsUISound = true;
		Sound->RegisterComponent();
		Sound->Start();
	}
	UFileMediaSource* Source = NewObject<UFileMediaSource>(this);
	Source->SetFilePath(GetMoviePath());
	if (!Player->OpenSource(Source))
	{
		UE_LOG(LogWorkshopEmpire, Warning, TEXT("Intro: could not open %s"), *GetMoviePath());
		Finish();
		return;
	}
	UE_LOG(LogWorkshopEmpire, Display, TEXT("Intro: playing %s"), *GetMoviePath());
}

void UWEIntroWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bFinished)
	{
		return;
	}
	Elapsed += InDeltaTime;
	if (SkipHint)
	{
		SkipHint->SetRenderOpacity(FMath::Clamp((Elapsed - 1.f) / 0.6f, 0.f, 0.7f));
	}
	const bool bPlaying = Player && Player->IsPlaying();
	if ((!bPlaying && Elapsed > WEIntro::StartTimeout && (!Player || !Player->IsPreparing())) || Elapsed > WEIntro::MaxLength)
	{
		UE_LOG(LogWorkshopEmpire, Display, TEXT("Intro: ended after %.1f s (playing=%d)"), Elapsed, bPlaying ? 1 : 0);
		Finish();
	}
}

FReply UWEIntroWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::SpaceBar || Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		Finish();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UWEIntroWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Finish();
	return FReply::Handled();
}

void UWEIntroWidget::HandleEndReached()
{
	Finish();
}

void UWEIntroWidget::HandleOpenFailed(FString FailedUrl)
{
	UE_LOG(LogWorkshopEmpire, Warning, TEXT("Intro: playback failed for %s"), *FailedUrl);
	Finish();
}

void UWEIntroWidget::Finish()
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	if (Player)
	{
		Player->OnEndReached.RemoveAll(this);
		Player->OnMediaOpenFailed.RemoveAll(this);
		Player->Close();
	}
	if (Sound)
	{
		Sound->Stop();
		Sound->DestroyComponent();
		Sound = nullptr;
	}
	RemoveFromParent();
	if (TFunction<void()> Done = MoveTemp(OnFinished))
	{
		Done();
	}
}

#undef LOCTEXT_NAMESPACE
