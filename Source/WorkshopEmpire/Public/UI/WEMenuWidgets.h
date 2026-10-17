#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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
	void Configure(const FText& Title, const FText& Detail, const FText& ButtonLabel, bool bButtonEnabled, TFunction<void()> InOnClicked);

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION()
	void HandleClicked();

	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailText;
	UPROPERTY(Transient) TObjectPtr<UButton> Button;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ButtonText;
	TFunction<void()> OnClicked;
};

/**
 * Main menu (Phase 13): host a new game or a save, join a game found over Steam / in the LAN or by
 * IP address, player name (LAN), quit. Talks only to UWESessionSubsystem.
 */
UCLASS()
class WORKSHOPEMPIRE_API UWEMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 0 = main, 1 = load a save, 2 = join. */
	void ShowPage(int32 Page);
	void ShowMessage(const FText& Message);
	/** Text of what the lists show (tests, change detection). */
	FString BuildSignature() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UButton* MakeButton(UVerticalBox* Parent, const FText& Label, float Width = 360.f);
	void Rebuild();

	UFUNCTION() void HandleHostNew();
	UFUNCTION() void HandleShowLoad();
	UFUNCTION() void HandleShowJoin();
	UFUNCTION() void HandleQuit();
	UFUNCTION() void HandleGraphics();
	UFUNCTION() void HandleBack();
	UFUNCTION() void HandleSearchSteam();
	UFUNCTION() void HandleSearchLAN();
	UFUNCTION() void HandleJoinAddress();
	UFUNCTION() void HandleNameCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	bool WantsLAN() const;

	UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ServiceText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GraphicsText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MessageText;
	UPROPERTY(Transient) TObjectPtr<UEditableTextBox> NameBox;
	UPROPERTY(Transient) TObjectPtr<UCheckBox> LANCheck;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> SaveList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> GameList;
	UPROPERTY(Transient) TObjectPtr<UEditableTextBox> AddressBox;
	UPROPERTY(Transient) TObjectPtr<UButton> SteamSearchButton;
	FString LastSignature;
};

/**
 * In-game menu on [Esc] (Phase 13). The co-op game keeps running. Shows the session (Steam lobby /
 * LAN with the host's IP), every player with ping, host marker and - for the host - "Entfernen";
 * buttons: continue, invite Steam friends, save (host), back to the main menu, quit.
 */
UCLASS()
class WORKSHOPEMPIRE_API UWEPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FString BuildSignature() const;
	/** "Spieler (3/4)" line etc. for tests. */
	int32 GetListedPlayers() const { return ListedPlayers; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	void Rebuild();
	bool IsHost() const;

	UFUNCTION() void HandleContinue();
	UFUNCTION() void HandleInvite();
	UFUNCTION() void HandleSave();
	UFUNCTION() void HandleLeave();
	UFUNCTION() void HandleGraphics();
	UFUNCTION() void HandleQuit();

	UPROPERTY(Transient) TObjectPtr<UTextBlock> SessionText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayersTitle;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> PlayerList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> InfoText;
	UPROPERTY(Transient) TObjectPtr<UButton> SaveButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LeaveText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GraphicsText;
	FString LastSignature;
	int32 ListedPlayers = 0;
};
