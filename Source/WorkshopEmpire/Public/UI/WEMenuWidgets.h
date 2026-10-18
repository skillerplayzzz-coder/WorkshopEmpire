#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/WESettings.h"
#include "UI/WEUIKit.h"
#include "Player/WECharacterLook.h"
#include "WEMenuWidgets.generated.h"

class UButton;
class UCheckBox;
class UEditableTextBox;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;

/** One line in a menu list (save game, found game, player) with an optional button. Built in C++. */
UCLASS()
class WORKSHOPEMPIRE_API UWEMenuRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(const FText& Title, const FText& Detail, const FText& ButtonLabel, bool bButtonEnabled, TFunction<void()> InOnClicked,
		FName IconName = NAME_None, bool bPrimary = true);

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION()
	void HandleClicked();

	UPROPERTY(Transient) TObjectPtr<class UImage> RowIcon;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailText;
	UPROPERTY(Transient) TObjectPtr<UButton> Button;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ButtonText;
	TFunction<void()> OnClicked;
};

class UWESettingsPanel;

/** A setting with a value between arrow buttons ("Auflösung  ‹ 1920 × 1080 ›"). */
UCLASS()
class WORKSHOPEMPIRE_API UWESettingRow : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UWESettingsPanel* InOwner, int32 InIndex, const FText& Label, FName IconName);
	/** Phase 18c: rows outside the settings page (character creator) call this instead of the panel. */
	void SetupCallback(TFunction<void(int32 Direction)> InOnStep, const FText& Label, FName IconName);
	void SetValue(const FText& Value);

protected:
	virtual void NativeOnInitialized() override;
	UFUNCTION() void HandleLeft();
	UFUNCTION() void HandleRight();

	UPROPERTY(Transient) TObjectPtr<UWESettingsPanel> Owner;
	TFunction<void(int32)> OnStep;
	UPROPERTY(Transient) TObjectPtr<class UImage> RowIcon;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LabelText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ValueText;
	int32 Index = 0;
};

/**
 * Phase 17.5 settings page (main menu and [Esc] menu): graphics quality, resolution, display mode, VSync, frame
 * limit, UI scale, mouse sensitivity, invert Y, field of view, volume. "Übernehmen" applies and stores.
 */
UCLASS()
class WORKSHOPEMPIRE_API UWESettingsPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	void Step(int32 Index, int32 Direction);
	void Reload();
	const WESettings::FValues& GetValues() const { return Values; }

protected:
	virtual void NativeOnInitialized() override;
	void Refresh();
	UFUNCTION() void HandleApply();
	UFUNCTION() void HandleDefaults();

	enum ERow : int32 { Quality, Resolution, WindowMode, VSync, FrameLimit, UIScale, Mouse, InvertY, Fov, Volume, Count };

	UPROPERTY(Transient) TArray<TObjectPtr<UWESettingRow>> Rows;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	WESettings::FValues Values;
	int32 QualityLevel = 2;
	TArray<FIntPoint> Resolutions;
};

/**
 * Main menu (Phase 17.5 layout over the Phase 13 functions): continue the newest company, new game (company name,
 * tutorial), multiplayer (host / join by Steam, LAN or IP), settings, quit. The 3D workshop scene shows behind it.
 * Talks only to UWESessionSubsystem.
 */
UCLASS()
class WORKSHOPEMPIRE_API UWEMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 0 = home, 1 = load a save, 2 = multiplayer (join), 3 = new game, 4 = settings, 5 = character creator. */
	void ShowPage(int32 Page);
	/** Phase 18c character creator (also used by the dev shots). */
	void StepLook(int32 Field, int32 Direction);
	const FWECharacterLook& GetEditLook() const { return EditLook; }
	class AWEAvatarPreview* GetPreview() const { return Preview.Get(); }
	void TurnPreview(float Degrees);
	void ShowMessage(const FText& Message);
	/** Text of what the lists show (tests, change detection). */
	FString BuildSignature() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UButton* MakeMenuButton(UVerticalBox* Parent, const FText& Label, FName IconName, TObjectPtr<UTextBlock>* OutDetail = nullptr);
	void Rebuild();

	UFUNCTION() void HandleContinue();
	UFUNCTION() void HandleShowNew();
	UFUNCTION() void HandleStartNew();
	UFUNCTION() void HandleHostNew();
	UFUNCTION() void HandleShowLoad();
	UFUNCTION() void HandleShowJoin();
	UFUNCTION() void HandleShowSettings();
	UFUNCTION() void HandleQuit();
	UFUNCTION() void HandleBack();
	UFUNCTION() void HandleSearchSteam();
	UFUNCTION() void HandleSearchLAN();
	UFUNCTION() void HandleJoinAddress();
	UFUNCTION() void HandleNameCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	UFUNCTION() void HandleShowCreator();
	UFUNCTION() void HandleEditLook();
	UFUNCTION() void HandleRandomLook();
	UFUNCTION() void HandleSaveLook();
	UFUNCTION() void HandleCloseUp();
	void RefreshLook();
	void OpenPreview();
	void ClosePreview();

	bool WantsLAN() const;

	UPROPERTY(Transient) TObjectPtr<UWidget> PageCard;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PageTitle;
	UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ServiceText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ContinueDetail;
	UPROPERTY(Transient) TObjectPtr<UButton> ContinueButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MessageText;
	UPROPERTY(Transient) TObjectPtr<UEditableTextBox> NameBox;
	UPROPERTY(Transient) TObjectPtr<UEditableTextBox> CompanyBox;
	UPROPERTY(Transient) TObjectPtr<UCheckBox> TutorialCheck;
	UPROPERTY(Transient) TObjectPtr<UCheckBox> LANCheck;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> SaveList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> GameList;
	UPROPERTY(Transient) TObjectPtr<UEditableTextBox> AddressBox;
	UPROPERTY(Transient) TObjectPtr<UButton> SteamSearchButton;
	UPROPERTY(Transient) TObjectPtr<UWESettingsPanel> Settings;
	FString LatestSlot;
	FString LastSignature;
	WEUI::FOpenAnim CardAnim;
	WEUI::FOpenAnim IntroAnim;
	UPROPERTY(Transient) TObjectPtr<UWidget> LeftColumn;
	// Character creator.
	UPROPERTY(Transient) TArray<TObjectPtr<UWESettingRow>> LookRows;
	UPROPERTY(Transient) TObjectPtr<UWidget> CreatorStartButton;
	UPROPERTY(Transient) TObjectPtr<UWidget> CreatorSaveButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CreatorStatus;
	TWeakObjectPtr<class AWEAvatarPreview> Preview;
	FWECharacterLook EditLook;
	bool bCreatorForNewGame = false;
	bool bCloseUp = false;
};

/**
 * In-game menu on [Esc] (Phase 13, restyled in 17.5). The co-op game keeps running. Shows the session (Steam lobby /
 * LAN with the host's IP), every player with ping, host marker and - for the host - "Entfernen"; buttons: continue,
 * save (host), settings, tutorial (restart / skip), invite Steam friends, back to the main menu, quit.
 */
UCLASS()
class WORKSHOPEMPIRE_API UWEPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FString BuildSignature() const;
	/** "Spieler (3/4)" line etc. for tests. */
	int32 GetListedPlayers() const { return ListedPlayers; }
	/** Right side: settings (true) or the session (false). */
	void ShowSettings(bool bShow);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	void Rebuild();
	bool IsHost() const;

	UFUNCTION() void HandleContinue();
	UFUNCTION() void HandleInvite();
	UFUNCTION() void HandleSave();
	UFUNCTION() void HandleLeave();
	UFUNCTION() void HandleSettings();
	UFUNCTION() void HandleTutorial();
	UFUNCTION() void HandleQuit();

	UPROPERTY(Transient) TObjectPtr<UTextBlock> SessionText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayersTitle;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> PlayerList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> InfoText;
	UPROPERTY(Transient) TObjectPtr<UButton> SaveButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LeaveText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TutorialText;
	UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY(Transient) TObjectPtr<UWESettingsPanel> Settings;
	UPROPERTY(Transient) TObjectPtr<UWidget> Card;
	WEUI::FOpenAnim Anim;
	FString LastSignature;
	int32 ListedPlayers = 0;
};
