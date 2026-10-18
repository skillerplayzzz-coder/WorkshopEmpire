#include "Workshop/WEWorkshopStyleComponent.h"

#include "Economy/WECompanyComponent.h"
#include "EngineUtils.h"
#include "Framework/WEGameState.h"
#include "Framework/WEPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Player/WECharacter.h"
#include "Progression/WEProgressionComponent.h"
#include "WELog.h"
#include "World/WEGarageBuilding.h"
#include "World/WEGarageDoor.h"

#define LOCTEXT_NAMESPACE "WEStyle"

uint8 FWEZoneStyle::Get(EWEStyleCategory Category) const
{
	switch (Category)
	{
	case EWEStyleCategory::WallPaint: return WallPaint;
	case EWEStyleCategory::Floor:     return Floor;
	case EWEStyleCategory::Lighting:  return Lighting;
	default:                          return Door;
	}
}

void FWEZoneStyle::Set(EWEStyleCategory Category, uint8 Value)
{
	switch (Category)
	{
	case EWEStyleCategory::WallPaint: WallPaint = Value; break;
	case EWEStyleCategory::Floor:     Floor = Value; break;
	case EWEStyleCategory::Lighting:  Lighting = Value; break;
	default:                          Door = Value; break;
	}
}

UWEWorkshopStyleComponent::UWEWorkshopStyleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UWEWorkshopStyleComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWEWorkshopStyleComponent, Styles);
}

void UWEWorkshopStyleComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyAll();
}

UWEWorkshopStyleComponent* UWEWorkshopStyleComponent::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AWEGameState* GameState = World ? World->GetGameState<AWEGameState>() : nullptr;
	return GameState ? GameState->GetWorkshopStyle() : nullptr;
}

namespace WEStyleData
{
	FWEStyleOption Paint(const FText& Name, uint32 Hex, int32 Tier, int64 Cost, EWESurface Surface = EWESurface::PaintedWall, float Desat = 1.f)
	{
		FWEStyleOption O;
		O.Name = Name;
		O.Swatch = FLinearColor::FromSRGBColor(FColor((Hex >> 16) & 0xFF, (Hex >> 8) & 0xFF, Hex & 0xFF));
		O.Tint = O.Swatch * 1.15f;
		O.Tint.A = 1.f;
		O.MinTier = Tier;
		O.CostCents = Cost;
		O.Surface = Surface;
		O.Desaturation = Desat;
		return O;
	}

	FWEStyleOption Original(const FText& Name, uint32 Hex)
	{
		FWEStyleOption O = Paint(Name, Hex, 1, 0, EWESurface::Placeholder, 0.f);
		O.Tint = FLinearColor::White;
		return O;
	}

	FWEStyleOption Light(const FText& Name, uint32 Hex, int32 Tier, int64 Cost, float Factor, const FLinearColor& Color)
	{
		FWEStyleOption O = Paint(Name, Hex, Tier, Cost, EWESurface::Placeholder, 0.f);
		O.LightFactor = Factor;
		O.LightColor = Color;
		return O;
	}
}

const TArray<FWEStyleOption>& UWEWorkshopStyleComponent::GetOptions(EWEStyleCategory Category)
{
	using namespace WEStyleData;
	static const TArray<FWEStyleOption> Walls = []()
	{
		TArray<FWEStyleOption> A;
		A.Add(Original(LOCTEXT("WallOrig", "Originalzustand"), 0xD8D4CA));
		A.Add(Paint(LOCTEXT("WallWhite", "Reinweiß"), 0xEFEDE6, 1, 15000));
		A.Add(Paint(LOCTEXT("WallGrey", "Lichtgrau"), 0xB9BCBE, 1, 15000));
		A.Add(Paint(LOCTEXT("WallYellow", "Signalgelb"), 0xE8B630, 1, 18000));
		A.Add(Paint(LOCTEXT("WallBlue", "Werkstattblau"), 0x3E6A93, 1, 18000));
		A.Add(Paint(LOCTEXT("WallGreen", "Racinggrün"), 0x2F5E45, 2, 22000));
		A.Add(Paint(LOCTEXT("WallRed", "Ochsenblut"), 0x7C2F28, 2, 22000));
		A.Add(Paint(LOCTEXT("WallAnthracite", "Anthrazit"), 0x3B3F44, 2, 22000));
		A.Add(Paint(LOCTEXT("WallOrange", "Industrieorange"), 0xD7682A, 3, 26000));
		A.Add(Paint(LOCTEXT("WallBrick", "Sichtmauerwerk"), 0xB06E52, 2, 60000, EWESurface::Brick, 0.f));
		A.Last().Tint = FLinearColor::White;
		A.Add(Paint(LOCTEXT("WallConcrete", "Sichtbeton"), 0x9A9A96, 3, 75000, EWESurface::ConcreteWall, 0.f));
		A.Last().Tint = FLinearColor::White;
		return A;
	}();
	static const TArray<FWEStyleOption> Floors = []()
	{
		TArray<FWEStyleOption> A;
		A.Add(Original(LOCTEXT("FloorOrig", "Betonboden (alt)"), 0x8C8984));
		A.Add(Paint(LOCTEXT("FloorEpoxyGrey", "Epoxid grau"), 0x8E9497, 1, 35000, EWESurface::ConcreteFloor, 0.85f));
		A.Last().Roughness = 0.6f;
		A.Add(Paint(LOCTEXT("FloorEpoxyGreen", "Epoxid grün"), 0x5E7F62, 2, 42000, EWESurface::ConcreteFloor, 0.85f));
		A.Last().Roughness = 0.6f;
		A.Add(Paint(LOCTEXT("FloorEpoxyBlue", "Epoxid blau"), 0x55728E, 2, 42000, EWESurface::ConcreteFloor, 0.85f));
		A.Last().Roughness = 0.6f;
		A.Add(Paint(LOCTEXT("FloorRubber", "Gummi-Noppenmatten"), 0x2A2C2E, 1, 48000, EWESurface::TreadPlate, 1.f));
		A.Last().Roughness = 1.2f;
		A.Add(Paint(LOCTEXT("FloorPlywood", "Holzdielen (Siebdruck)"), 0xA57A52, 1, 52000, EWESurface::Plywood, 0.f));
		A.Last().Tint = FLinearColor(0.9f, 0.85f, 0.8f);
		A.Add(Paint(LOCTEXT("FloorTread", "Riffelblech"), 0x9FA3A6, 3, 120000, EWESurface::TreadPlate, 0.f));
		A.Last().Tint = FLinearColor::White;
		A.Add(Paint(LOCTEXT("FloorOsb", "OSB-Platten"), 0xC2A06E, 1, 30000, EWESurface::Osb, 0.f));
		A.Last().Tint = FLinearColor::White;
		return A;
	}();
	static const TArray<FWEStyleOption> Lights = []()
	{
		TArray<FWEStyleOption> A;
		A.Add(Light(LOCTEXT("LightOrig", "Leuchtstoffröhren"), 0xFFE9CF, 1, 0, 1.f, FLinearColor(1.f, 0.92f, 0.82f)));
		A.Add(Light(LOCTEXT("LightWarm", "Warmweiß, gemütlich"), 0xFFCB8A, 1, 20000, 0.8f, FLinearColor(1.f, 0.74f, 0.48f)));
		A.Add(Light(LOCTEXT("LightCold", "LED kaltweiß"), 0xE3EEFF, 1, 40000, 1.35f, FLinearColor(0.86f, 0.93f, 1.f)));
		A.Add(Light(LOCTEXT("LightDaylight", "Tageslicht-Panels"), 0xF6F8FF, 2, 70000, 1.6f, FLinearColor(0.95f, 0.97f, 1.f)));
		A.Add(Light(LOCTEXT("LightFlood", "Industriestrahler"), 0xFFE3B0, 3, 110000, 2.1f, FLinearColor(1.f, 0.88f, 0.68f)));
		return A;
	}();
	static const TArray<FWEStyleOption> Doors = []()
	{
		TArray<FWEStyleOption> A;
		A.Add(Original(LOCTEXT("DoorOrig", "Verzinkt (original)"), 0xB7BBBE));
		A.Add(Paint(LOCTEXT("DoorAnthracite", "Anthrazit"), 0x34383C, 1, 12000, EWESurface::PaintedShutter, 1.f));
		A.Add(Paint(LOCTEXT("DoorRed", "Signalrot"), 0xA8322B, 1, 12000, EWESurface::PaintedShutter, 1.f));
		A.Add(Paint(LOCTEXT("DoorBlue", "Enzianblau"), 0x24527F, 1, 12000, EWESurface::PaintedShutter, 1.f));
		A.Add(Paint(LOCTEXT("DoorYellow", "Rapsgelb"), 0xE2B12A, 1, 12000, EWESurface::PaintedShutter, 1.f));
		A.Add(Paint(LOCTEXT("DoorGreen", "Moosgrün"), 0x355E3B, 2, 15000, EWESurface::PaintedShutter, 1.f));
		A.Add(Paint(LOCTEXT("DoorWhite", "Verkehrsweiß"), 0xE9E8E2, 2, 15000, EWESurface::PaintedShutter, 1.f));
		return A;
	}();
	switch (Category)
	{
	case EWEStyleCategory::WallPaint: return Walls;
	case EWEStyleCategory::Floor:     return Floors;
	case EWEStyleCategory::Lighting:  return Lights;
	default:                          return Doors;
	}
}

FText UWEWorkshopStyleComponent::GetCategoryName(EWEStyleCategory Category)
{
	switch (Category)
	{
	case EWEStyleCategory::WallPaint: return LOCTEXT("CatWall", "Wandfarbe");
	case EWEStyleCategory::Floor:     return LOCTEXT("CatFloor", "Bodenbelag");
	case EWEStyleCategory::Lighting:  return LOCTEXT("CatLight", "Beleuchtung");
	default:                          return LOCTEXT("CatDoor", "Tor");
	}
}

FName UWEWorkshopStyleComponent::GetZoneOf(const AActor* Actor)
{
	if (Actor)
	{
		for (const FName& Tag : Actor->Tags)
		{
			const FString S = Tag.ToString();
			if (S.StartsWith(TEXT("WEUpgrade.")))
			{
				return FName(*S.RightChop(10));
			}
		}
	}
	return TEXT("Garage");
}

FText UWEWorkshopStyleComponent::GetZoneName(FName Zone)
{
	if (Zone == TEXT("WorkshopExtension")) return LOCTEXT("ZoneExt", "Werkstatt-Erweiterung");
	if (Zone == TEXT("CommercialHall"))    return LOCTEXT("ZoneHall", "Gewerbehalle");
	if (Zone == TEXT("FactoryHall"))       return LOCTEXT("ZoneFactory", "Fabrikhalle");
	return LOCTEXT("ZoneGarage", "Garage");
}

float UWEWorkshopStyleComponent::GetZoneCostFactor(FName Zone)
{
	if (Zone == TEXT("WorkshopExtension")) return 1.5f;
	if (Zone == TEXT("CommercialHall"))    return 3.5f;
	if (Zone == TEXT("FactoryHall"))       return 4.5f;
	return 1.f;
}

TArray<FName> UWEWorkshopStyleComponent::GetOwnedZones() const
{
	TArray<FName> Zones = { TEXT("Garage") };
	if (const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this))
	{
		for (const FName Building : { FName(TEXT("WorkshopExtension")), FName(TEXT("CommercialHall")), FName(TEXT("FactoryHall")) })
		{
			if (Progression->HasUpgrade(Building))
			{
				Zones.Add(Building);
			}
		}
	}
	return Zones;
}

FWEZoneStyle UWEWorkshopStyleComponent::GetZoneStyle(FName Zone) const
{
	if (const FWEZoneStyle* Found = Styles.FindByPredicate([Zone](const FWEZoneStyle& S) { return S.Zone == Zone; }))
	{
		return *Found;
	}
	FWEZoneStyle Default;
	Default.Zone = Zone;
	return Default;
}

FWEMoney UWEWorkshopStyleComponent::GetPrice(FName Zone, EWEStyleCategory Category, int32 Option) const
{
	const TArray<FWEStyleOption>& Options = GetOptions(Category);
	if (!Options.IsValidIndex(Option))
	{
		return FWEMoney();
	}
	// Doors and lamps do not grow with the floor area as much as paint and floors do.
	const float Factor = Category == EWEStyleCategory::Door ? FMath::Sqrt(GetZoneCostFactor(Zone)) : GetZoneCostFactor(Zone);
	return FWEMoney::FromCents(FMath::RoundToInt64(Options[Option].CostCents * Factor / 100.0) * 100);
}

FText UWEWorkshopStyleComponent::GetLockReason(FName Zone, EWEStyleCategory Category, int32 Option) const
{
	const TArray<FWEStyleOption>& Options = GetOptions(Category);
	if (!Options.IsValidIndex(Option))
	{
		return LOCTEXT("Unknown", "Unbekannt");
	}
	if (!GetOwnedZones().Contains(Zone))
	{
		return LOCTEXT("NotOwned", "Gebäude gehört nicht der Firma");
	}
	const UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this);
	const int32 Tier = Progression ? Progression->GetCompanyTier() : 1;
	if (Tier < Options[Option].MinTier)
	{
		return FText::Format(LOCTEXT("NeedsTier", "Ab {0}"), UWEProgressionComponent::GetTierName(Options[Option].MinTier));
	}
	return FText::GetEmpty();
}

void UWEWorkshopStyleComponent::GetLighting(FName Zone, float& OutFactor, FLinearColor& OutColor) const
{
	const TArray<FWEStyleOption>& Options = GetOptions(EWEStyleCategory::Lighting);
	const int32 Index = GetZoneStyle(Zone).Lighting;
	const FWEStyleOption& Option = Options.IsValidIndex(Index) ? Options[Index] : Options[0];
	OutFactor = Option.LightFactor;
	OutColor = Option.LightColor;
}

EWEStyleResult UWEWorkshopStyleComponent::Apply(AWECharacter* By, FName Zone, EWEStyleCategory Category, int32 Option, const FGuid& RequestId)
{
	UWECompanyComponent* Company = UWECompanyComponent::Get(this);
	AWEPlayerController* PC = By ? Cast<AWEPlayerController>(By->GetController()) : nullptr;
	auto Tell = [PC](const FText& Message) { if (PC) { PC->ClientShowMessage(Message); } };
	if (!GetOwner()->HasAuthority() || !Company || !GetOptions(Category).IsValidIndex(Option) || Option > 255)
	{
		return EWEStyleResult::Invalid;
	}
	const FText Lock = GetLockReason(Zone, Category, Option);
	if (!Lock.IsEmpty())
	{
		Tell(Lock);
		return EWEStyleResult::Locked;
	}
	FWEZoneStyle Style = GetZoneStyle(Zone);
	if (Style.Get(Category) == Option)
	{
		return EWEStyleResult::AlreadySet;
	}
	const FWEMoney Price = GetPrice(Zone, Category, Option);
	if (!Company->CanAfford(Price))
	{
		Tell(NSLOCTEXT("WETraderTerminal", "ResMoney", "Nicht genug Geld auf dem Firmenkonto"));
		return EWEStyleResult::NotEnoughMoney;
	}
	if (!Company->RegisterRequest(RequestId))
	{
		return EWEStyleResult::Duplicate;
	}
	const FWEStyleOption& Chosen = GetOptions(Category)[Option];
	const APlayerState* PS = By ? By->GetPlayerState() : nullptr;
	if (!Price.IsZero() && !Company->TryWithdraw(Price, FString::Printf(TEXT("Gestaltung: %s %s – %s"), *GetZoneName(Zone).ToString(),
		*GetCategoryName(Category).ToString(), *Chosen.Name.ToString()), PS ? PS->GetPlayerName() : FString()))
	{
		return EWEStyleResult::NotEnoughMoney;
	}
	Style.Set(Category, static_cast<uint8>(Option));
	if (FWEZoneStyle* Existing = Styles.FindByPredicate([Zone](const FWEZoneStyle& S) { return S.Zone == Zone; }))
	{
		*Existing = Style;
	}
	else
	{
		Styles.Add(Style);
	}
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Style %s %s -> %s (%s)"), *Zone.ToString(), *GetCategoryName(Category).ToString(), *Chosen.Name.ToString(), *Price.ToDisplayString());
	Tell(FText::Format(LOCTEXT("Applied", "{0}: {1} – {2}"), GetZoneName(Zone), GetCategoryName(Category), Chosen.Name));
	OnRep_Styles();
	return EWEStyleResult::Success;
}

void UWEWorkshopStyleComponent::SetAllStyles(const TArray<FWEZoneStyle>& InStyles)
{
	if (GetOwner()->HasAuthority())
	{
		Styles = InStyles;
		OnRep_Styles();
	}
}

void UWEWorkshopStyleComponent::OnRep_Styles()
{
	ApplyAll();
	OnStyleChanged.Broadcast();
}

void UWEWorkshopStyleComponent::ApplyAll()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	auto OptionOf = [this](FName Zone, EWEStyleCategory Category) -> const FWEStyleOption*
	{
		const TArray<FWEStyleOption>& Options = GetOptions(Category);
		const int32 Index = GetZoneStyle(Zone).Get(Category);
		return Index > 0 && Options.IsValidIndex(Index) ? &Options[Index] : nullptr;
	};
	for (TActorIterator<AWEGarageBuilding> It(World); It; ++It)
	{
		const FName Zone = GetZoneOf(*It);
		It->ApplyStyle(OptionOf(Zone, EWEStyleCategory::WallPaint), OptionOf(Zone, EWEStyleCategory::Floor));
	}
	for (TActorIterator<AWEGarageDoor> It(World); It; ++It)
	{
		It->ApplyStyle(OptionOf(GetZoneOf(*It), EWEStyleCategory::Door));
	}
	// Lamps: the progression's light pass multiplies upgrades (LED) and the chosen lighting.
	if (UWEProgressionComponent* Progression = UWEProgressionComponent::Get(this))
	{
		Progression->ApplyEffects();
	}
}

#undef LOCTEXT_NAMESPACE
