#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WETradeWidget.generated.h"

class AWETraderTerminal;
class UButton;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UWETradeWidget;

UENUM()
enum class EWETradeRowKind : uint8
{
	Sell,
	SellHeld,
	Buy,
	Contract,
	Upgrade,
	Auction,
	Info,
	/** Phase 11: applicant (button: hire) and employee (buttons: pause/resume, fire). */
	Applicant,
	Employee
};

/** One line in a trade list with up to two buttons. Built in C++. */
UCLASS()
class WORKSHOPEMPIRE_API UWETradeRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(UWETradeWidget* InOwner, EWETradeRowKind InKind, const FText& Title, const FText& Detail, const FText& Price,
		const FText& Button1, bool bButton1Enabled, const FText& Button2 = FText::GetEmpty(), bool bButton2Enabled = false);

	EWETradeRowKind Kind = EWETradeRowKind::Info;
	int32 InventorySlot = INDEX_NONE;
	int32 Quantity = 0;
	int32 OfferIndex = INDEX_NONE;
	FGuid ContractId;
	bool bContractAccepted = false;
	FName UpgradeId;
	FGuid LotId;
	TWeakObjectPtr<class AWEEmployee> Employee;
	bool bEmployeePaused = false;
	/** Wrap width for the texts (0 = by row kind). Set before Configure. */
	float WrapOverride = 0.f;

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION()
	void HandleButton1();

	UFUNCTION()
	void HandleButton2();

	UPROPERTY(Transient) TObjectPtr<UWETradeWidget> Owner;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PriceText;
	UPROPERTY(Transient) TObjectPtr<UButton> Button1;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Button1Text;
	UPROPERTY(Transient) TObjectPtr<UButton> Button2;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Button2Text;
};

/**
 * Market screen of the online terminal: company balance, sell list (player inventory + carried
 * item, best trader price), buy list (all traders' stock) and customer orders, plus recent bookings.
 * Only sends requests through AWEPlayerController; lists rebuild when the replicated state changes.
 */
UCLASS()
class WORKSHOPEMPIRE_API UWETradeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(AWETraderTerminal* InTerminal);
	void HandleRowAction(const UWETradeRowWidget* Row, int32 ButtonIndex);

	/** 0 = market (sell/buy/orders/upgrades), 1 = auctions and other sources. */
	void ShowPage(int32 Page);

	/** Text summary of what the lists show (tests, change detection). */
	FString BuildSignature() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	void Rebuild();
	void RebuildAuctions(const struct FWEMoney& Balance);
	UVerticalBox* MakeColumn(const FText& Title, TObjectPtr<UScrollBox>& OutList, float Width = 300.f);
	class UButton* MakeTabButton(const FText& Label, TObjectPtr<UTextBlock>& OutText);

	UFUNCTION()
	void HandleTabMarket();

	UFUNCTION()
	void HandleTabAuctions();
	UFUNCTION()
	void HandleTabStaff();
	void RebuildStaff(const struct FWEMoney& Balance);
	UWETradeRowWidget* AddRow(UScrollBox* List);

	UPROPERTY(Transient) TObjectPtr<AWETraderTerminal> Terminal;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BalanceText;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> SellList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> BuyList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> ContractList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> UpgradeList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> AuctionList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> SourcesList;
	UPROPERTY(Transient) TObjectPtr<class UWidgetSwitcher> Pages;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TabMarketText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TabAuctionsText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TabStaffText;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> ApplicantList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> StaffList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> LogisticsList;
	int32 CurrentPage = 0;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HistoryText;

	FString LastSignature;
};
