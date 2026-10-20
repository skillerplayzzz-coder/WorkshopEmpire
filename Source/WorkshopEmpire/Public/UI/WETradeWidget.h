#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/WEUIKit.h"
#include "WETradeWidget.generated.h"

class AWETraderTerminal;
class UBorder;
class UButton;
class UCanvasPanel;
class UCanvasPanelSlot;
class UHorizontalBox;
class UImage;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;
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
	Employee,
	/** Phase 17.5: protected equipment / marked goods (button: release, unlock, unfavourite). */
	Protected,
	/** Phase 17.5: an old building (buttons: clear, sell). */
	Building,
	/** Phase 17.5: a look for a building (button: apply). */
	Style,
	/** Phase 17.5: tutorial (buttons: start/restart, skip). */
	Tutorial,
	/** Phase 19 classifieds: a private seller's ad (buy, offer / take the counter offer), a stack to list, an own ad (accept buyer, withdraw). */
	ClassifiedAd,
	ClassifiedNew,
	ClassifiedOwn
};

/** One line in a Ferrit OS list: picture, title, details, chips, price and up to two buttons. Built in C++. */
UCLASS()
class WORKSHOPEMPIRE_API UWETradeRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(UWETradeWidget* InOwner, EWETradeRowKind InKind, const FText& Title, const FText& Detail, const FText& Price,
		const FText& Button1, bool bButton1Enabled, const FText& Button2 = FText::GetEmpty(), bool bButton2Enabled = false);
	/** Picture on the left: an item's rendered icon, a line icon, or a colour swatch. */
	void SetPicture(class UTexture2D* ItemTexture, FName LineIcon = NAME_None, const FLinearColor& Tint = FLinearColor::White);
	void SetSwatch(const FLinearColor& Color);
	void AddChip(const FText& Label, const FLinearColor& Color);
	/** Button looks (default: first primary, second secondary). */
	void SetButtonKinds(WEUI::EButton First, WEUI::EButton Second);
	void SetHighlight(const FLinearColor& Outline);

	EWETradeRowKind Kind = EWETradeRowKind::Info;
	int32 InventorySlot = INDEX_NONE;
	/** Phase 19: storage the sell row is from (none = the player inventory). */
	TWeakObjectPtr<class UWEInventoryComponent> SourceInventory;
	int32 Quantity = 0;
	int32 OfferIndex = INDEX_NONE;
	FGuid ContractId;
	bool bContractAccepted = false;
	FName UpgradeId;
	FGuid LotId;
	TWeakObjectPtr<class AWEEmployee> Employee;
	bool bEmployeePaused = false;
	/** Phase 17.5: protected rows (mark to clear), building rows, style rows. */
	uint8 MarkToClear = 0;
	uint8 StyleCategory = 0;
	FName StyleZone;
	/** Phase 19: prices behind the two buttons (classifieds). */
	int64 PriceA = 0;
	int64 PriceB = 0;
	/** Wrap width for the texts (0 = default). Set before Configure. */
	float WrapOverride = 0.f;

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION()
	void HandleButton1();

	UFUNCTION()
	void HandleButton2();

	UPROPERTY(Transient) TObjectPtr<UWETradeWidget> Owner;
	UPROPERTY(Transient) TObjectPtr<UBorder> Frame;
	UPROPERTY(Transient) TObjectPtr<UWidget> PictureBox;
	UPROPERTY(Transient) TObjectPtr<UImage> Picture;
	UPROPERTY(Transient) TObjectPtr<UBorder> SwatchBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailText;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> Chips;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PriceText;
	UPROPERTY(Transient) TObjectPtr<UButton> Button1;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Button1Text;
	UPROPERTY(Transient) TObjectPtr<UButton> Button2;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Button2Text;
};

/**
 * Ferrit OS - the desktop of the workshop PC (Phase 17.5, replaces the Phase 4-12 market screen). Apps open as
 * windows that can be moved, minimised and closed; a taskbar shows the open windows, the company account, open
 * orders and the clock. Apps: Marktplatz (sell / buy / workshop shop), Aufträge, Auktionen, Personal & Logistik,
 * Finanzen, Unternehmen (tier, milestones, old buildings), Gestaltung and Hilfe (tutorial, controls).
 *
 * Only sends requests through AWEPlayerController; lists rebuild when the replicated state changes. Valuable sales
 * and building sales ask in a dialog first.
 */
UCLASS()
class WORKSHOPEMPIRE_API UWETradeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	enum class EApp : uint8 { Market, Auctions, Staff, Company, Orders, Finance, Style, Help, Classifieds, Count };

	void Setup(AWETraderTerminal* InTerminal);
	void HandleRowAction(const UWETradeRowWidget* Row, int32 ButtonIndex);

	/** Phase 4-12 page numbers (tests and dev runs): 0 market, 1 auctions, 2 staff, 3 company; 4-7 orders, finance, style, help. */
	void ShowPage(int32 Page);
	/** Tabs of the "Gestaltung" app (4 = company logo); also used by the dev shots. */
	void ShowStyleTab(int32 Tab);
	void OpenApp(EApp App);
	void CloseApp(EApp App);
	bool IsAppOpen(EApp App) const;
	/** Market app tab: 0 sell, 1 buy, 2 workshop shop. */
	void ShowMarketTab(int32 Tab);

	/** Text summary of what the lists show (tests, change detection). */
	FString BuildSignature() const;

	/** Modal question; OnConfirm runs when the player confirms. */
	void ShowConfirm(const FText& Title, const FText& Message, const FText& ConfirmLabel, TFunction<void()> OnConfirm, bool bDanger = false);
	bool IsConfirmOpen() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	struct FAppWindow
	{
		TObjectPtr<UBorder> Frame;
		TObjectPtr<UBorder> TitleBar;
		TObjectPtr<UVerticalBox> Body;
		TObjectPtr<UButton> TaskButton;
		TObjectPtr<UTextBlock> TaskLabel;
		bool bOpen = false;
		bool bMinimized = false;
		bool bBuilt = false;
		FVector2D Position = FVector2D::ZeroVector;
		FVector2D Size = FVector2D(1040.f, 640.f);
		WEUI::FOpenAnim Anim;
	};

	// Desktop pieces
	void BuildDesktop();
	void BuildTaskbar();
	void BuildStartMenu();
	void BuildDialog();
	FAppWindow& Window(EApp App) { return Windows[static_cast<int32>(App)]; }
	const FAppWindow& Window(EApp App) const { return Windows[static_cast<int32>(App)]; }
	void CreateWindow(EApp App);
	void FocusApp(EApp App);
	void MinimizeApp(EApp App);
	void UpdateTaskbar();
	FVector2D GetDesktopSize() const;
	static FText AppTitle(EApp App);
	static FName AppIcon(EApp App);
	static FLinearColor AppColor(EApp App);

	// App contents (built once per window, lists refreshed on change)
	void BuildMarketApp(UVerticalBox* Body);
	void BuildOrdersApp(UVerticalBox* Body);
	void BuildAuctionsApp(UVerticalBox* Body);
	void BuildStaffApp(UVerticalBox* Body);
	void BuildFinanceApp(UVerticalBox* Body);
	void BuildCompanyApp(UVerticalBox* Body);
	void BuildStyleApp(UVerticalBox* Body);
	void BuildHelpApp(UVerticalBox* Body);
	void BuildClassifiedsApp(UVerticalBox* Body);

	void Rebuild();
	void RebuildMarket(const struct FWEMoney& Balance);
	void RebuildOrders();
	void RebuildAuctions(const struct FWEMoney& Balance);
	void RebuildStaff(const struct FWEMoney& Balance);
	void RebuildFinance();
	void RebuildCompany();
	void RebuildStyle(const struct FWEMoney& Balance);
	void RebuildHelp();
	void RebuildClassifieds(const struct FWEMoney& Balance);
	UFUNCTION() void HandleClassifiedsTab0();
	UFUNCTION() void HandleClassifiedsTab1();

	UWETradeRowWidget* AddRow(UScrollBox* List, float Wrap = 0.f);
	void AddSection(UScrollBox* List, const FText& Title, FName IconName = NAME_None);
	void AddInfo(UScrollBox* List, const FText& Title, const FText& Detail, const FText& Price = FText::GetEmpty(), float Wrap = 0.f);
	UScrollBox* MakeList(UVerticalBox* Parent, bool bFill = true);
	/** Tab bar inside an app; returns the switcher the pages go into. */
	UWidgetSwitcher* MakeTabs(UVerticalBox* Parent, const TArray<FText>& Labels, const TArray<FName>& Icons, TArray<TObjectPtr<UButton>>& OutButtons,
		TArray<TObjectPtr<UTextBlock>>& OutLabels, TArray<TObjectPtr<UImage>>& OutIcons);
	void SelectTab(UWidgetSwitcher* Switcher, TArray<TObjectPtr<UButton>>& Buttons, TArray<TObjectPtr<UTextBlock>>& Labels, TArray<TObjectPtr<UImage>>& Icons, int32 Index);

	UFUNCTION() void HandleStart();
	UFUNCTION() void HandleLogout();
	UFUNCTION() void HandleConfirmYes();
	UFUNCTION() void HandleConfirmNo();
	UFUNCTION() void HandleMarketTab0();
	UFUNCTION() void HandleMarketTab1();
	UFUNCTION() void HandleMarketTab2();
	UFUNCTION() void HandleStaffTab0();
	UFUNCTION() void HandleStaffTab1();
	UFUNCTION() void HandleStaffTab2();
	UFUNCTION() void HandleStyleCat0();
	UFUNCTION() void HandleStyleCat1();
	UFUNCTION() void HandleStyleCat2();
	UFUNCTION() void HandleStyleCat3();
	UFUNCTION() void HandleStyleCat4();
	UFUNCTION() void HandleBrandApplyName();
	/** Phase 20: the logo editor (fifth tab of "Gestaltung"). */
	void BuildBrandEditor();
	void RefreshBrandEditor();
	void StepBrand(int32 Field, int32 Direction);
	UFUNCTION() void HandleStyleZoneNext();
	UFUNCTION() void HandleBuyFilterNext();
	UFUNCTION() void HandleApp0();
	UFUNCTION() void HandleApp1();
	UFUNCTION() void HandleApp2();
	UFUNCTION() void HandleApp3();
	UFUNCTION() void HandleApp4();
	UFUNCTION() void HandleApp5();
	UFUNCTION() void HandleApp6();
	UFUNCTION() void HandleApp7();
	UFUNCTION() void HandleApp8();
	UFUNCTION() void HandleTask0();
	UFUNCTION() void HandleTask1();
	UFUNCTION() void HandleTask2();
	UFUNCTION() void HandleTask3();
	UFUNCTION() void HandleTask4();
	UFUNCTION() void HandleTask5();
	UFUNCTION() void HandleTask6();
	UFUNCTION() void HandleTask7();
	UFUNCTION() void HandleTask8();
	UFUNCTION() void HandleClose0();
	UFUNCTION() void HandleClose1();
	UFUNCTION() void HandleClose2();
	UFUNCTION() void HandleClose3();
	UFUNCTION() void HandleClose4();
	UFUNCTION() void HandleClose5();
	UFUNCTION() void HandleClose6();
	UFUNCTION() void HandleClose7();
	UFUNCTION() void HandleClose8();
	UFUNCTION() void HandleMin0();
	UFUNCTION() void HandleMin1();
	UFUNCTION() void HandleMin2();
	UFUNCTION() void HandleMin3();
	UFUNCTION() void HandleMin4();
	UFUNCTION() void HandleMin5();
	UFUNCTION() void HandleMin6();
	UFUNCTION() void HandleMin7();
	UFUNCTION() void HandleMin8();
	void TaskClicked(EApp App);

	UPROPERTY(Transient) TObjectPtr<AWETraderTerminal> Terminal;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Desktop;
	UPROPERTY(Transient) TObjectPtr<UWidget> DesktopRoot;
	UPROPERTY(Transient) TObjectPtr<UWidget> TaskbarWidget;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> TaskButtons;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TrayBalance;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TrayOrders;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TrayClock;
	UPROPERTY(Transient) TObjectPtr<UWidget> StartMenu;
	UPROPERTY(Transient) TObjectPtr<UWidget> DialogLayer;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DialogTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DialogMessage;
	UPROPERTY(Transient) TObjectPtr<UButton> DialogYes;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DialogYesText;
	TFunction<void()> PendingConfirm;

	FAppWindow Windows[static_cast<int32>(EApp::Count)];
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> WindowWidgets;
	int32 TopZ = 10;
	EApp ActiveApp = EApp::Market;
	int32 CurrentPage = 0;
	int32 OpenedCount = 0;

	// Dragging a window by its title bar
	int32 DragApp = INDEX_NONE;
	FVector2D DragOffset = FVector2D::ZeroVector;

	// Market
	UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> MarketPages;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> MarketTabButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> MarketTabLabels;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> MarketTabIcons;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> SellList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> BuyList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> UpgradeList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BuyFilterText;
	int32 BuyFilter = 0;
	int32 MarketTab = 0;
	// Orders, auctions
	UPROPERTY(Transient) TObjectPtr<UScrollBox> ContractList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> AuctionList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> SourcesList;
	// Staff
	UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> StaffPages;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> StaffTabButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> StaffTabLabels;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> StaffTabIcons;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> ApplicantList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> StaffList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> LogisticsList;
	// Finance, company
	UPROPERTY(Transient) TObjectPtr<UTextBlock> FinanceBalance;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> BooksList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> HistoryList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> TierList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> MilestoneList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> BuildingList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> ContractsOverview;
	// Style
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StyleZoneText;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> StyleCatButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> StyleCatLabels;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> StyleCatIcons;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> StyleList;
	int32 StyleZoneIndex = 0;
	int32 StyleCategory = 0;
	UPROPERTY(Transient) TArray<TObjectPtr<class UWESettingRow>> BrandRows;
	UPROPERTY(Transient) TObjectPtr<class UEditableTextBox> BrandNameBox;
	UPROPERTY(Transient) TObjectPtr<UImage> BrandPreviewIcon;
	UPROPERTY(Transient) TObjectPtr<UBorder> BrandPreviewBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BrandPreviewName;
	bool bBrandEditorBuilt = false;
	// Help
	UPROPERTY(Transient) TObjectPtr<UScrollBox> HelpList;
	// Classifieds (Phase 19)
	UPROPERTY(Transient) TObjectPtr<class UWidgetSwitcher> ClassifiedPages;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> ClassifiedTabButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ClassifiedTabLabels;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> ClassifiedTabIcons;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> AdsList;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> OwnAdsList;

	FString LastSignature;
	float ClockAccumulated = 1.f;
	/** < 0 while the view still moves to the monitor; then fades the desktop in. */
	float FadeIn = 1.f;
};
