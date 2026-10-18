#include "Economy/WETraderTerminal.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Economy/WEAuctionComponent.h"
#include "Economy/WECompanyComponent.h"
#include "Economy/WEContractComponent.h"
#include "Economy/WEMarketComponent.h"
#include "Economy/WETraderDefinition.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Framework/WEPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Interaction/WEInteractionComponent.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Net/UnrealNetwork.h"
#include "Player/WECharacter.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "WELog.h"
#include "World/WEPlaceholderVisuals.h"
#include "World/WESurface.h"
#include "Progression/WEMilestoneComponent.h"
#include "Progression/WEProgressionComponent.h"
#include "Progression/WEUpgradeDefinition.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "UI/WEComputerWidgets.h"

#define LOCTEXT_NAMESPACE "WETraderTerminal"

namespace WETerminal
{
	void AddBox(UInstancedStaticMeshComponent* ISM, const FVector& Center, const FVector& Size)
	{
		ISM->AddInstance(FTransform(FRotator::ZeroRotator, Center, Size / 100.f));
	}

	FString Describe(const FWEItemInstance& Item)
	{
		return Item.Quantity > 1
			? FString::Printf(TEXT("%d× %s"), Item.Quantity, *Item.GetDisplayName().ToString())
			: Item.GetDisplayName().ToString();
	}
}

AWETraderTerminal::AWETraderTerminal()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(5.f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlaceholderMat(WEPlaceholder::MaterialPath);
	PlaceholderMaterial = PlaceholderMat.Object;

	Body = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Body"));
	Body->SetMobility(EComponentMobility::Static);
	Body->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RootComponent = Body;
	Screen = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Screen"));
	Screen->SetupAttachment(Body);
	Screen->SetMobility(EComponentMobility::Static);
	Screen->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	if (CubeMesh.Succeeded())
	{
		Body->SetStaticMesh(CubeMesh.Object);
		Screen->SetStaticMesh(CubeMesh.Object);
	}
	StockRandom.Initialize(1234);

	// Phase 17.5: the modelled workshop PC (meshes are assigned in SetupModel, the old boxes stay as fallback).
	auto Part = [this](const TCHAR* Name, bool bCollide)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Body);
		C->SetMobility(EComponentMobility::Movable);
		C->SetCollisionProfileName(bCollide ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		return C;
	};
	DeskMesh = Part(TEXT("DeskMesh"), true);
	TowerMesh = Part(TEXT("TowerMesh"), true);
	MonitorMesh = Part(TEXT("MonitorMesh"), true);
	KeyboardMesh = Part(TEXT("KeyboardMesh"), false);
	MouseMesh = Part(TEXT("MouseMesh"), false);
	LampMesh = Part(TEXT("LampMesh"), false);
	ChairMesh = Part(TEXT("ChairMesh"), true);
	ClipboardMesh = Part(TEXT("ClipboardMesh"), false);
	ScreenWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("ScreenWidget"));
	ScreenWidget->SetupAttachment(Body);
	ScreenWidget->SetWidgetSpace(EWidgetSpace::World);
	ScreenWidget->SetDrawSize(FVector2D(1280.f, 720.f));
	ScreenWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ScreenWidget->SetWidgetClass(UWEDesktopIdleWidget::StaticClass());
	ScreenWidget->SetBlendMode(EWidgetBlendMode::Opaque);
	ScreenWidget->SetTickWhenOffscreen(false);
	ScreenWidget->SetRedrawTime(0.5f);
	ScreenCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ScreenCamera"));
	ScreenCamera->SetupAttachment(Body);
	ScreenCamera->SetFieldOfView(62.f);
	ScreenCamera->bConstrainAspectRatio = false;
}

namespace WETerminalLayout
{
	// Local layout (front = +X, desk centre at the origin). Monitor screen: 52 x 29.4 cm, centre below.
	const FVector MonitorPos(-25.f, 0.f, 79.f);
	const FVector ScreenCenter(-22.2f, 0.f, 117.6f);
	constexpr float ScreenWidth = 52.f;
}

void AWETraderTerminal::SetupModel()
{
	auto Load = [](const TCHAR* Path) { return Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, Path, nullptr, LOAD_Quiet | LOAD_NoWarn)); };
	UStaticMesh* Desk = Load(TEXT("/Game/WorkshopEmpire/Art/Models/SM_metal_office_desk.SM_metal_office_desk"));
	UStaticMesh* Monitor = Load(TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_Monitor.SM_Gen_Monitor"));
	bModelled = Desk && Monitor;
	Body->SetVisibility(!bModelled);
	Screen->SetVisibility(!bModelled);
	Body->SetCollisionEnabled(bModelled ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	Screen->SetCollisionEnabled(bModelled ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	if (!bModelled)
	{
		ScreenWidget->SetVisibility(false);
		ScreenCamera->SetRelativeLocation(FVector(40.f, 0.f, 105.f));
		ScreenCamera->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
		return;
	}
	auto Place = [](UStaticMeshComponent* C, UStaticMesh* Mesh, const FVector& At, float Yaw, float Scale = 1.f)
	{
		C->SetStaticMesh(Mesh);
		C->SetRelativeLocation(At);
		C->SetRelativeRotation(FRotator(0.f, Yaw, 0.f));
		C->SetRelativeScale3D(FVector(Scale));
		C->SetVisibility(Mesh != nullptr);
	};
	Place(DeskMesh, Desk, FVector(0.f, 0.f, 0.f), 90.f);
	Place(MonitorMesh, Monitor, WETerminalLayout::MonitorPos, 90.f);
	Place(TowerMesh, Load(TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_PCTower.SM_Gen_PCTower")), FVector(-18.f, 72.f, 0.f), 90.f);
	Place(KeyboardMesh, Load(TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_Keyboard.SM_Gen_Keyboard")), FVector(12.f, -4.f, 79.2f), 90.f);
	Place(MouseMesh, Load(TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_Mouse.SM_Gen_Mouse")), FVector(14.f, 30.f, 79.2f), 98.f);
	Place(LampMesh, Load(TEXT("/Game/WorkshopEmpire/Art/Models/SM_desk_lamp_arm_01.SM_desk_lamp_arm_01")), FVector(-28.f, -72.f, 79.2f), 40.f);
	Place(ClipboardMesh, Load(TEXT("/Game/WorkshopEmpire/Art/Models/SM_clipboard.SM_clipboard")), FVector(8.f, -52.f, 79.4f), 75.f);
	Place(ChairMesh, Load(TEXT("/Game/WorkshopEmpire/Art/Models/SM_SchoolChair_01.SM_SchoolChair_01")), FVector(78.f, 26.f, 0.f), -115.f);
	// The live screen sits a hair in front of the dark glass; its picture faces +X like the monitor.
	ScreenWidget->SetVisibility(true);
	ScreenWidget->SetRelativeLocation(WETerminalLayout::ScreenCenter + FVector(0.15f, 0.f, 0.f));
	ScreenWidget->SetRelativeRotation(FRotator::ZeroRotator);
	ScreenWidget->SetRelativeScale3D(FVector(WETerminalLayout::ScreenWidth / ScreenWidget->GetDrawSize().X));
	// Camera straight in front of the screen; the field of view frames the monitor.
	ScreenCamera->SetRelativeLocation(WETerminalLayout::ScreenCenter + FVector(46.f, 0.f, 0.f));
	ScreenCamera->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
}

void AWETraderTerminal::SetLocalUser(AWECharacter* Character, bool bUsing)
{
	if (UWEDesktopIdleWidget* Idle = ScreenWidget ? Cast<UWEDesktopIdleWidget>(ScreenWidget->GetUserWidgetObject()) : nullptr)
	{
		Idle->SetLocalUse(bUsing);
	}
}

void AWETraderTerminal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWETraderTerminal, Lines);
	DOREPLIFETIME(AWETraderTerminal, UserName);
}

void AWETraderTerminal::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Small desk with a monitor; front faces +X.
	Body->ClearInstances();
	Screen->ClearInstances();
	WETerminal::AddBox(Body, FVector(0.f, 0.f, 73.f), FVector(60.f, 110.f, 4.f));
	for (const float SY : { -1.f, 1.f })
	{
		WETerminal::AddBox(Body, FVector(0.f, SY * 50.f, 35.5f), FVector(55.f, 5.f, 71.f));
	}
	WETerminal::AddBox(Screen, FVector(-12.f, 0.f, 105.f), FVector(4.f, 55.f, 34.f));
	WETerminal::AddBox(Screen, FVector(-14.f, 0.f, 80.f), FVector(10.f, 10.f, 10.f));
	WESurface::Apply(Body, EWESurface::PaintedMetal, FLinearColor(0.32f, 0.35f, 0.4f), 0.4f);
	WEPlaceholder::ApplyTint(Screen, PlaceholderMaterial, FLinearColor(0.05f, 0.25f, 0.55f));
	SetupModel();
}

void AWETraderTerminal::BeginPlay()
{
	Super::BeginPlay();
	WESurface::Apply(Body, EWESurface::PaintedMetal, FLinearColor(0.32f, 0.35f, 0.4f), 0.4f);
	WEPlaceholder::ApplyTint(Screen, PlaceholderMaterial, FLinearColor(0.05f, 0.25f, 0.55f));
	SetupModel();
	if (GetNetMode() == NM_DedicatedServer)
	{
		ScreenWidget->SetVisibility(false);
	}
	else if (UWEDesktopIdleWidget* Idle = Cast<UWEDesktopIdleWidget>(ScreenWidget->GetUserWidgetObject()))
	{
		Idle->SetTerminal(this);
	}

	if (HasAuthority())
	{
		if (Lines.Num() == 0)
		{
			StockRandom.Initialize(FMath::Rand());
			Restock();
		}
		float Minutes = 0.f;
		for (const UWETraderDefinition* Trader : Traders)
		{
			if (Trader && Trader->RestockMinutes > 0.f)
			{
				Minutes = Minutes > 0.f ? FMath::Min(Minutes, Trader->RestockMinutes) : Trader->RestockMinutes;
			}
		}
		if (Minutes > 0.f)
		{
			GetWorldTimerManager().SetTimer(RestockTimer, this, &AWETraderTerminal::Restock, Minutes * 60.f, true);
		}
	}
}

void AWETraderTerminal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RestockTimer);
	Super::EndPlay(EndPlayReason);
}

// --- Interaction ---------------------------------------------------------------------------------

FText AWETraderTerminal::GetInteractionText(const AWECharacter* Interactor) const
{
	return LOCTEXT("Open", "Werkstatt-PC benutzen");
}

bool AWETraderTerminal::CanInteract(const AWECharacter* Interactor, FText& OutReason) const
{
	return Interactor != nullptr;
}

void AWETraderTerminal::Interact(AWECharacter* Interactor)
{
	if (AWEPlayerController* PC = Interactor ? Cast<AWEPlayerController>(Interactor->GetController()) : nullptr)
	{
		UserName = PlayerNameOf(Interactor);
		ForceNetUpdate();
		PC->ClientOpenTrade(this);
	}
}

bool AWETraderTerminal::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	OutInfo.Title = LOCTEXT("Title", "Werkstatt-PC");
	OutInfo.Description = LOCTEXT("Desc", "Der Firmenrechner mit Ferrit OS: Marktplatz, Aufträge, Auktionen, Personal, Finanzen, Unternehmen und Gestaltung.");
	for (const UWETraderDefinition* Trader : Traders)
	{
		if (Trader)
		{
			OutInfo.Details.Add(Trader->DisplayName);
		}
	}
	return true;
}

bool AWETraderTerminal::IsWithinUseRange(const AWECharacter* Character) const
{
	return UWEInteractionComponent::IsWithinReach(Character, this, UseRange);
}

// --- Offers --------------------------------------------------------------------------------------

int32 AWETraderTerminal::NumOffers() const
{
	return Lines.Num();
}

bool AWETraderTerminal::ResolveOffer(int32 OfferIndex, const UWETraderDefinition*& OutTrader, const FWETraderStockEntry*& OutEntry) const
{
	if (!Lines.IsValidIndex(OfferIndex))
	{
		return false;
	}
	const FWEStockLine& Line = Lines[OfferIndex];
	OutTrader = Traders.IsValidIndex(Line.TraderIndex) ? Traders[Line.TraderIndex].Get() : nullptr;
	OutEntry = OutTrader && OutTrader->Stock.IsValidIndex(Line.EntryIndex) ? &OutTrader->Stock[Line.EntryIndex] : nullptr;
	return OutEntry && OutEntry->Item && Line.Item.Definition == OutEntry->Item;
}

TArray<int32> AWETraderTerminal::GetStockRemaining() const
{
	TArray<int32> Result;
	for (const FWEStockLine& Line : Lines)
	{
		Result.Add(Line.Remaining);
	}
	return Result;
}

TArray<FWEMarketOffer> AWETraderTerminal::GetOffers() const
{
	TArray<FWEMarketOffer> Offers;
	const UWEMarketComponent* Market = UWEMarketComponent::Get(this);
	for (int32 i = 0; i < NumOffers(); ++i)
	{
		const UWETraderDefinition* Trader = nullptr;
		const FWETraderStockEntry* Entry = nullptr;
		if (!ResolveOffer(i, Trader, Entry))
		{
			continue;
		}
		FWEMarketOffer& Offer = Offers.AddDefaulted_GetRef();
		Offer.OfferIndex = i;
		Offer.TraderName = Trader->DisplayName;
		Offer.Item = Lines[i].Item;
		Offer.Item.Quantity = 1;
		Offer.Remaining = Lines[i].Remaining;
		Offer.UnitPrice = UWEMarketComponent::GetBuyPrice(Offer.Item, Entry->PriceMultiplier, Market);
	}
	return Offers;
}

FWEMoney AWETraderTerminal::GetBestSellPrice(const FWEItemInstance& Item, const UWETraderDefinition** OutTrader) const
{
	const UWEMarketComponent* Market = UWEMarketComponent::Get(this);
	FWEMoney Best;
	const UWETraderDefinition* BestTrader = nullptr;
	for (const UWETraderDefinition* Trader : Traders)
	{
		const FWEMoney Price = UWEMarketComponent::GetSellPrice(Item, Trader, Market);
		if (Price > Best)
		{
			Best = Price;
			BestTrader = Trader;
		}
	}
	if (OutTrader)
	{
		*OutTrader = BestTrader;
	}
	return Best;
}

void AWETraderTerminal::Restock()
{
	if (!HasAuthority())
	{
		return;
	}
	// Every restock rolls which offers are there and in which state (rotating used goods, Phase 7).
	Lines.Reset();
	for (int32 TraderIndex = 0; TraderIndex < Traders.Num(); ++TraderIndex)
	{
		const UWETraderDefinition* Trader = Traders[TraderIndex];
		for (int32 EntryIndex = 0; Trader && EntryIndex < Trader->Stock.Num(); ++EntryIndex)
		{
			const FWETraderStockEntry& Entry = Trader->Stock[EntryIndex];
			if (!Entry.Item || (Entry.Chance < 1.f && StockRandom.FRand() >= Entry.Chance))
			{
				continue;
			}
			const int32 Condition = FMath::Min(static_cast<int32>(Entry.Condition) + StockRandom.RandRange(0, Entry.ConditionSpread), static_cast<int32>(EWEItemCondition::Scrap));
			const int32 Quality = FMath::Clamp(static_cast<int32>(Entry.Quality) + StockRandom.RandRange(-Entry.QualitySpread, Entry.QualitySpread), 0, 100);
			FWEStockLine& Line = Lines.AddDefaulted_GetRef();
			Line.TraderIndex = TraderIndex;
			Line.EntryIndex = EntryIndex;
			Line.Item = FWEItemInstance::Make(Entry.Item, 1, static_cast<EWEItemCondition>(Condition), static_cast<uint8>(Quality));
			Line.Remaining = Entry.Quantity;
		}
	}
	ForceNetUpdate();
}

void AWETraderTerminal::SetStockLines(const TArray<FWEStockLine>& InLines)
{
	if (!HasAuthority())
	{
		return;
	}
	Lines.Reset();
	for (const FWEStockLine& Line : InLines)
	{
		// Only keep lines that still match the trader data (data may have changed since saving).
		const UWETraderDefinition* Trader = Traders.IsValidIndex(Line.TraderIndex) ? Traders[Line.TraderIndex].Get() : nullptr;
		if (Trader && Trader->Stock.IsValidIndex(Line.EntryIndex) && Trader->Stock[Line.EntryIndex].Item == Line.Item.Definition)
		{
			Lines.Add(Line);
		}
	}
	ForceNetUpdate();
}

// --- Trades (server) -----------------------------------------------------------------------------

FString AWETraderTerminal::PlayerNameOf(const AWECharacter* Character) const
{
	const APlayerState* PS = Character ? Character->GetPlayerState() : nullptr;
	return PS ? PS->GetPlayerName() : FString();
}

void AWETraderTerminal::Notify(AWECharacter* Character, const FText& Message) const
{
	if (AWEPlayerController* PC = Character ? Cast<AWEPlayerController>(Character->GetController()) : nullptr)
	{
		PC->ClientShowMessage(Message);
	}
}

FText AWETraderTerminal::ResultText(EWETradeResult Result)
{
	switch (Result)
	{
	case EWETradeResult::Success:        return LOCTEXT("ResOk", "Erledigt");
	case EWETradeResult::Duplicate:      return LOCTEXT("ResDup", "Anfrage wurde bereits bearbeitet");
	case EWETradeResult::OutOfReach:     return LOCTEXT("ResReach", "Zu weit vom Terminal entfernt");
	case EWETradeResult::InvalidItem:    return LOCTEXT("ResItem", "Gegenstand nicht verfügbar");
	case EWETradeResult::NoBuyer:        return LOCTEXT("ResNoBuyer", "Niemand kauft das");
	case EWETradeResult::NotEnoughMoney: return LOCTEXT("ResMoney", "Nicht genug Geld auf dem Firmenkonto");
	case EWETradeResult::OutOfStock:     return LOCTEXT("ResStock", "Nicht mehr vorrätig");
	case EWETradeResult::ContractFailed: return LOCTEXT("ResContract", "Auftrag nicht möglich");
	case EWETradeResult::Protected:      return LOCTEXT("ResProtected", "Geschützt – erst freigeben oder entsperren");
	}
	return FText::GetEmpty();
}

FText AWETraderTerminal::ProtectedText(const FWEItemInstance& Item)
{
	if (Item.IsLocked())
	{
		return FText::Format(LOCTEXT("ProtLocked", "{0} ist gesperrt – erst entsperren"), Item.GetDisplayName());
	}
	if (Item.IsFavorite())
	{
		return FText::Format(LOCTEXT("ProtFav", "{0} ist ein Favorit – Markierung erst entfernen"), Item.GetDisplayName());
	}
	return FText::Format(LOCTEXT("ProtTool", "{0} ist deine Ausrüstung – erst zum Verkauf freigeben"), Item.GetDisplayName());
}

EWETradeResult AWETraderTerminal::SellFromInventory(AWECharacter* Character, UWEInventoryComponent* Inventory, int32 Slot, int32 Quantity, const FGuid& RequestId)
{
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!HasAuthority() || !Company || !Character)
	{
		return EWETradeResult::InvalidItem;
	}
	if (!IsWithinUseRange(Character))
	{
		return EWETradeResult::OutOfReach;
	}
	const FWEItemInstance* Item = Inventory && Inventory->CanBeAccessedBy(Character) ? Inventory->GetItemInSlot(Slot) : nullptr;
	if (!Item)
	{
		return EWETradeResult::InvalidItem;
	}
	if (Item->IsCustomerItem())
	{
		Notify(Character, LOCTEXT("CustomerNoSale", "Gehört einem Kunden – über den Auftrag zurückgeben"));
		return EWETradeResult::InvalidItem;
	}
	if (Item->IsSaleProtected())
	{
		Notify(Character, ProtectedText(*Item));
		return EWETradeResult::Protected;
	}
	FWEItemInstance Selling = *Item;
	Selling.Quantity = Quantity <= 0 ? Item->Quantity : FMath::Min(Quantity, Item->Quantity);
	const UWETraderDefinition* Trader = nullptr;
	const FWEMoney Price = GetBestSellPrice(Selling, &Trader);
	if (!Trader || Price.IsZero())
	{
		Notify(Character, ResultText(EWETradeResult::NoBuyer));
		return EWETradeResult::NoBuyer;
	}
	if (!Company->RegisterRequest(RequestId))
	{
		return EWETradeResult::Duplicate;
	}

	// Goods first, then money: if removing fails, nothing is paid.
	FWEItemInstance Removed;
	if (Inventory->RemoveFromSlot(Slot, Selling.Quantity, Removed) != EWEInventoryResult::Success)
	{
		return EWETradeResult::InvalidItem;
	}
	const FString What = WETerminal::Describe(Removed);
	Company->Deposit(Price, FString::Printf(TEXT("Verkauf: %s an %s"), *What, *Trader->DisplayName.ToString()), PlayerNameOf(Character));
	Notify(Character, FText::FromString(FString::Printf(TEXT("+%s  für %s"), *Price.ToDisplayString(), *What)));
	UE_LOG(LogWENet, Log, TEXT("%s sold %s for %s"), *Character->GetName(), *What, *Price.ToDisplayString());
	if (UWEMilestoneComponent* Milestones = UWEMilestoneComponent::Get(this)) { Milestones->AddRareFind(Removed.Definition); }
	if (UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this)) { Progression->NotifyEvent(EWEProgressEvent::ItemSold); }
	return EWETradeResult::Success;
}

EWETradeResult AWETraderTerminal::SellHeldItem(AWECharacter* Character, const FGuid& RequestId)
{
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	AWEPickupItem* Held = Character ? Character->GetHeldItem() : nullptr;
	if (!HasAuthority() || !Company || !Held)
	{
		return EWETradeResult::InvalidItem;
	}
	if (!IsWithinUseRange(Character))
	{
		return EWETradeResult::OutOfReach;
	}
	const FWEItemInstance Item = Held->GetItem();
	if (Item.IsCustomerItem())
	{
		Notify(Character, LOCTEXT("CustomerNoSaleHeld", "Gehört einem Kunden – über den Auftrag zurückgeben"));
		return EWETradeResult::InvalidItem;
	}
	if (Item.IsSaleProtected())
	{
		Notify(Character, ProtectedText(Item));
		return EWETradeResult::Protected;
	}
	const UWETraderDefinition* Trader = nullptr;
	const FWEMoney Price = GetBestSellPrice(Item, &Trader);
	if (!Trader || Price.IsZero())
	{
		Notify(Character, ResultText(EWETradeResult::NoBuyer));
		return EWETradeResult::NoBuyer;
	}
	if (!Company->RegisterRequest(RequestId))
	{
		return EWETradeResult::Duplicate;
	}
	Character->ReleaseHeldItem();
	Held->Destroy();
	const FString What = WETerminal::Describe(Item);
	Company->Deposit(Price, FString::Printf(TEXT("Verkauf: %s an %s"), *What, *Trader->DisplayName.ToString()), PlayerNameOf(Character));
	Notify(Character, FText::FromString(FString::Printf(TEXT("+%s  für %s"), *Price.ToDisplayString(), *What)));
	UE_LOG(LogWENet, Log, TEXT("%s sold carried %s for %s"), *Character->GetName(), *What, *Price.ToDisplayString());
	if (UWEMilestoneComponent* Milestones = UWEMilestoneComponent::Get(this)) { Milestones->AddRareFind(Item.Definition); }
	if (UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this)) { Progression->NotifyEvent(EWEProgressEvent::ItemSold); }
	return EWETradeResult::Success;
}

void AWETraderTerminal::DeliverPurchase(AWECharacter* Character, const FWEItemInstance& Item)
{
	FWEItemInstance Rest = Item;
	if (UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr; Inventory && Item.Definition->bFitsInInventory)
	{
		int32 Added = 0;
		Inventory->TryAddItem(Item, Added);
		Rest.Quantity -= Added;
		// A bought tool that lands directly in the inventory counts like one picked up by hand.
		UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
		if (Added > 0 && Item.Definition->ToolType != EWEToolType::None && Progression)
		{
			Progression->NotifyEvent(EWEProgressEvent::ToolStored);
		}
	}
	if (Rest.Quantity <= 0)
	{
		return;
	}
	FTransform Drop = GetActorTransform();
	Drop.SetLocation(GetActorLocation() + GetActorForwardVector() * 80.f + FVector(0.f, 0.f, 40.f));
	if (AWEDeliveryZone* Zone = AWEDeliveryZone::Find(GetWorld()))
	{
		Drop = Zone->NextDropTransform();
	}
	AWEPickupItem::SpawnWorldItem(GetWorld(), Rest, Drop);
}

EWETradeResult AWETraderTerminal::Buy(AWECharacter* Character, int32 OfferIndex, int32 Quantity, const FGuid& RequestId)
{
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!HasAuthority() || !Company || !Character)
	{
		return EWETradeResult::InvalidItem;
	}
	if (!IsWithinUseRange(Character))
	{
		return EWETradeResult::OutOfReach;
	}
	const UWETraderDefinition* Trader = nullptr;
	const FWETraderStockEntry* Entry = nullptr;
	if (!ResolveOffer(OfferIndex, Trader, Entry))
	{
		return EWETradeResult::InvalidItem;
	}
	Quantity = FMath::Max(1, Quantity);
	if (Lines[OfferIndex].Remaining < Quantity)
	{
		Notify(Character, ResultText(EWETradeResult::OutOfStock));
		return EWETradeResult::OutOfStock;
	}
	const FWEItemInstance Unit = Lines[OfferIndex].Item;
	const FWEItemInstance Bought = FWEItemInstance::Make(Entry->Item, Quantity, Unit.Condition, Unit.Quality);
	const FWEMoney Price = UWEMarketComponent::GetBuyPrice(Bought, Entry->PriceMultiplier, UWEMarketComponent::Get(this));
	if (!Company->CanAfford(Price))
	{
		Notify(Character, ResultText(EWETradeResult::NotEnoughMoney));
		return EWETradeResult::NotEnoughMoney;
	}
	if (!Company->RegisterRequest(RequestId))
	{
		return EWETradeResult::Duplicate;
	}

	// Money and stock first (both checked above), then hand over the goods.
	const FString What = WETerminal::Describe(Bought);
	if (!Company->TryWithdraw(Price, FString::Printf(TEXT("Einkauf: %s bei %s"), *What, *Trader->DisplayName.ToString()), PlayerNameOf(Character)))
	{
		return EWETradeResult::NotEnoughMoney;
	}
	Lines[OfferIndex].Remaining -= Quantity;
	ForceNetUpdate();

	// Non-stackable items become separate instances.
	if (Entry->Item->GetMaxStackSize() > 1)
	{
		DeliverPurchase(Character, Bought);
	}
	else
	{
		for (int32 i = 0; i < Quantity; ++i)
		{
			DeliverPurchase(Character, FWEItemInstance::Make(Entry->Item, 1, Unit.Condition, Unit.Quality));
		}
	}
	const bool bToPallet = !Entry->Item->bFitsInInventory;
	Notify(Character, FText::FromString(FString::Printf(TEXT("-%s  %s gekauft%s"), *Price.ToDisplayString(), *What,
		bToPallet ? TEXT(" (steht auf der Lieferpalette)") : TEXT(""))));
	UE_LOG(LogWENet, Log, TEXT("%s bought %s for %s"), *Character->GetName(), *What, *Price.ToDisplayString());
	if (UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this)) { Progression->NotifyEvent(EWEProgressEvent::PurchaseMade); }
	return EWETradeResult::Success;
}

EWETradeResult AWETraderTerminal::BuyUpgrade(AWECharacter* Character, FName UpgradeId, const FGuid& RequestId)
{
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	const FWEUpgradeDefinition* Upgrade = Progression && Progression->GetCatalog() ? Progression->GetCatalog()->Find(UpgradeId) : nullptr;
	if (!HasAuthority() || !Company || !Character || !Upgrade)
	{
		return EWETradeResult::InvalidItem;
	}
	if (!IsWithinUseRange(Character))
	{
		return EWETradeResult::OutOfReach;
	}
	const FText Blocked = Progression->GetBlockReason(*Upgrade);
	if (!Blocked.IsEmpty())
	{
		Notify(Character, Blocked);
		return EWETradeResult::OutOfStock;
	}
	if (!Company->CanAfford(Upgrade->Cost))
	{
		Notify(Character, ResultText(EWETradeResult::NotEnoughMoney));
		return EWETradeResult::NotEnoughMoney;
	}
	if (!Company->RegisterRequest(RequestId))
	{
		return EWETradeResult::Duplicate;
	}
	if (!Company->TryWithdraw(Upgrade->Cost, FString::Printf(TEXT("Werkstatt: %s"), *Upgrade->DisplayName.ToString()), PlayerNameOf(Character)))
	{
		return EWETradeResult::NotEnoughMoney;
	}
	const int32 TierBefore = Progression->GetCompanyTier();
	if (!Progression->GrantUpgrade(UpgradeId))
	{
		// Should not happen (checked above); never keep the money without the upgrade.
		Company->Deposit(Upgrade->Cost, TEXT("Rückbuchung Werkstatt-Upgrade"), PlayerNameOf(Character));
		return EWETradeResult::InvalidItem;
	}
	// A new company tier is part of the buyer's message (the general tier message would be replaced by it).
	Notify(Character, Progression->GetCompanyTier() > TierBefore
		? FText::Format(LOCTEXT("UpgradeBoughtTier", "Werkstatt verbessert: {0} – neue Unternehmensstufe: {1}!"), Upgrade->DisplayName, UWEProgressionComponent::GetTierName(Progression->GetCompanyTier()))
		: FText::Format(LOCTEXT("UpgradeBought", "Werkstatt verbessert: {0}"), Upgrade->DisplayName));
	Progression->NotifyEvent(EWEProgressEvent::PurchaseMade);
	UE_LOG(LogWENet, Log, TEXT("%s bought upgrade %s for %s"), *Character->GetName(), *UpgradeId.ToString(), *Upgrade->Cost.ToDisplayString());
	return EWETradeResult::Success;
}

EWETradeResult AWETraderTerminal::SellBuilding(AWECharacter* Character, FName UpgradeId, const FGuid& RequestId)
{
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	const FWEUpgradeDefinition* Building = Progression && Progression->GetCatalog() ? Progression->GetCatalog()->Find(UpgradeId) : nullptr;
	if (!HasAuthority() || !Company || !Character || !Building || !Progression->CanOfferBuildingSale(UpgradeId))
	{
		return EWETradeResult::InvalidItem;
	}
	if (!IsWithinUseRange(Character))
	{
		return EWETradeResult::OutOfReach;
	}
	const TArray<FText> Blockers = Progression->GetBuildingSaleBlockers(UpgradeId);
	if (Blockers.Num() > 0)
	{
		Notify(Character, Blockers[0]);
		return EWETradeResult::Protected;
	}
	if (!Company->RegisterRequest(RequestId))
	{
		return EWETradeResult::Duplicate;
	}
	const FWEMoney Price = Progression->GetBuildingSalePrice(UpgradeId);
	if (!Progression->RevokeBuilding(UpgradeId))
	{
		return EWETradeResult::InvalidItem;
	}
	Company->Deposit(Price, FString::Printf(TEXT("Gebäude verkauft: %s"), *Building->DisplayName.ToString()), PlayerNameOf(Character));
	const FText Message = FText::Format(LOCTEXT("BuildingSold", "{0} verkauft: +{1}"), Building->DisplayName, FText::FromString(Price.ToDisplayString()));
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(It->Get()))
		{
			PC->ClientShowMessage(Message);
		}
	}
	UE_LOG(LogWENet, Log, TEXT("%s sold building %s for %s"), *Character->GetName(), *UpgradeId.ToString(), *Price.ToDisplayString());
	return EWETradeResult::Success;
}

EWETradeResult AWETraderTerminal::ClearBuilding(AWECharacter* Character, FName UpgradeId)
{
	UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	if (!HasAuthority() || !Character || !Progression || !Progression->CanOfferBuildingSale(UpgradeId))
	{
		return EWETradeResult::InvalidItem;
	}
	if (!IsWithinUseRange(Character))
	{
		return EWETradeResult::OutOfReach;
	}
	const int32 Moved = Progression->ClearBuildingForSale(UpgradeId);
	Notify(Character, Moved > 0
		? FText::Format(LOCTEXT("Cleared", "{0} Dinge umgelagert: Gegenstände auf die Lieferpalette, Mitarbeiter in den Hof"), FText::AsNumber(Moved))
		: LOCTEXT("NothingToClear", "Nichts zum Umlagern gefunden"));
	return EWETradeResult::Success;
}

EWETradeResult AWETraderTerminal::AcceptContract(AWECharacter* Character, const FGuid& ContractId, const FGuid& RequestId)
{
	UWEContractComponent* Contracts = UWEContractComponent::Get(this);
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!HasAuthority() || !Contracts || !Company || !Character)
	{
		return EWETradeResult::ContractFailed;
	}
	if (!IsWithinUseRange(Character))
	{
		return EWETradeResult::OutOfReach;
	}
	if (!Company->RegisterRequest(RequestId))
	{
		return EWETradeResult::Duplicate;
	}
	const EWEContractResult Result = Contracts->Accept(ContractId, PlayerNameOf(Character));
	if (Result != EWEContractResult::Success)
	{
		Notify(Character, Result == EWEContractResult::TooManyAccepted
			? LOCTEXT("TooMany", "Es laufen schon genug Aufträge")
			: ResultText(EWETradeResult::ContractFailed));
		return EWETradeResult::ContractFailed;
	}
	Notify(Character, LOCTEXT("Accepted", "Auftrag angenommen"));
	return EWETradeResult::Success;
}

EWETradeResult AWETraderTerminal::BidOnAuction(AWECharacter* Character, const FGuid& LotId, const FGuid& RequestId)
{
	UWEAuctionComponent* Auctions = UWEAuctionComponent::Get(this);
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!HasAuthority() || !Auctions || !Company || !Character)
	{
		return EWETradeResult::InvalidItem;
	}
	if (!IsWithinUseRange(Character))
	{
		return EWETradeResult::OutOfReach;
	}
	if (!Company->RegisterRequest(RequestId))
	{
		return EWETradeResult::Duplicate;
	}
	FWEMoney Bid;
	switch (Auctions->PlaceBid(LotId, PlayerNameOf(Character), Bid))
	{
	case EWEAuctionResult::Leading:
		Notify(Character, FText::Format(LOCTEXT("BidLeading", "Gebot {0} – du bist Höchstbietender"), FText::FromString(Bid.ToDisplayString())));
		return EWETradeResult::Success;
	case EWEAuctionResult::Outbid:
	{
		const FWEAuctionLot* Lot = Auctions->FindLot(LotId);
		Notify(Character, FText::Format(LOCTEXT("BidOutbid", "Gebot {0} – sofort überboten von {1} ({2})"), FText::FromString(Bid.ToDisplayString()),
			FText::FromString(Lot ? Lot->LeaderName : FString()), FText::FromString(Lot ? Lot->CurrentBid.ToDisplayString() : FString())));
		return EWETradeResult::Success;
	}
	case EWEAuctionResult::AlreadyLeading:
		Notify(Character, LOCTEXT("BidAlreadyLeading", "Eure Firma bietet hier schon am höchsten"));
		return EWETradeResult::ContractFailed;
	case EWEAuctionResult::NotEnoughMoney:
		Notify(Character, ResultText(EWETradeResult::NotEnoughMoney));
		return EWETradeResult::NotEnoughMoney;
	default:
		Notify(Character, LOCTEXT("BidEnded", "Auktion ist vorbei"));
		return EWETradeResult::InvalidItem;
	}
}

EWETradeResult AWETraderTerminal::DeliverContract(AWECharacter* Character, const FGuid& ContractId, const FGuid& RequestId)
{
	UWEContractComponent* Contracts = UWEContractComponent::Get(this);
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	if (!HasAuthority() || !Contracts || !Company || !Character)
	{
		return EWETradeResult::ContractFailed;
	}
	if (!IsWithinUseRange(Character))
	{
		return EWETradeResult::OutOfReach;
	}
	const FWEContract* Contract = Contracts->FindContract(ContractId);
	const FWEMoney Reward = Contract ? Contract->Reward : FWEMoney();
	const bool bRepairOrder = Contract && Contract->Kind == EWEContractKind::Repair;
	if (!Company->RegisterRequest(RequestId))
	{
		return EWETradeResult::Duplicate;
	}
	const EWEContractResult Result = Contracts->Deliver(ContractId, Character);
	if (Result != EWEContractResult::Success)
	{
		Notify(Character, Result != EWEContractResult::MissingItems ? ResultText(EWETradeResult::ContractFailed)
			: bRepairOrder ? LOCTEXT("MissingRepair", "Das reparierte Kundengerät muss in den Händen sein")
			: LOCTEXT("Missing", "Passende Gegenstände fehlen im Inventar"));
		return EWETradeResult::ContractFailed;
	}
	Notify(Character, FText::Format(LOCTEXT("Delivered", "Auftrag erledigt: +{0}"), FText::FromString(Reward.ToDisplayString())));
	return EWETradeResult::Success;
}

// --- Delivery zone ------------------------------------------------------------------------------

AWEDeliveryZone::AWEDeliveryZone()
{
	PrimaryActorTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlaceholderMat(WEPlaceholder::MaterialPath);
	PlaceholderMaterial = PlaceholderMat.Object;

	Pallet = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pallet"));
	Pallet->SetMobility(EComponentMobility::Static);
	Pallet->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Pallet->SetRelativeScale3D(FVector(1.2f, 0.8f, 0.12f)); // place the actor 6 cm above the floor
	RootComponent = Pallet;
	if (CubeMesh.Succeeded())
	{
		Pallet->SetStaticMesh(CubeMesh.Object);
	}
}

void AWEDeliveryZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	WESurface::Apply(Pallet, EWESurface::Osb, FLinearColor(0.85f, 0.75f, 0.6f), 0.f);
}

void AWEDeliveryZone::BeginPlay()
{
	Super::BeginPlay();
	WESurface::Apply(Pallet, EWESurface::Osb, FLinearColor(0.85f, 0.75f, 0.6f), 0.f);
}

FTransform AWEDeliveryZone::NextDropTransform()
{
	// 2x2 spots on the pallet, items are dropped from a little above and settle by physics.
	const int32 Spot = NextIndex++ % 4;
	const FVector Local((Spot % 2 == 0 ? -30.f : 30.f), (Spot < 2 ? -20.f : 20.f), 60.f + (NextIndex / 4 % 3) * 40.f);
	// Rotation + location only: the root (pallet) carries a non-uniform scale.
	return FTransform(GetActorRotation(), GetActorLocation() + GetActorRotation().RotateVector(Local));
}

AWEDeliveryZone* AWEDeliveryZone::Find(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<AWEDeliveryZone> It(const_cast<UWorld*>(World));
	return It ? *It : nullptr;
}

#undef LOCTEXT_NAMESPACE
