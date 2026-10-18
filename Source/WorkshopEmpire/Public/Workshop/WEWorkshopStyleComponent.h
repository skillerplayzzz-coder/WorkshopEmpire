#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WEMoney.h"
#include "World/WESurface.h"
#include "WEWorkshopStyleComponent.generated.h"

class AWECharacter;

/** What can be personalised per building (Phase 17.5 "Werkstatt gestalten"). */
UENUM()
enum class EWEStyleCategory : uint8
{
	WallPaint,
	Floor,
	Lighting,
	Door
};

/** Look of one building ("zone": Garage, WorkshopExtension, CommercialHall, FactoryHall). Indices into the option lists. */
USTRUCT()
struct WORKSHOPEMPIRE_API FWEZoneStyle
{
	GENERATED_BODY()

	UPROPERTY() FName Zone;
	UPROPERTY() uint8 WallPaint = 0;
	UPROPERTY() uint8 Floor = 0;
	UPROPERTY() uint8 Lighting = 0;
	UPROPERTY() uint8 Door = 0;

	uint8 Get(EWEStyleCategory Category) const;
	void Set(EWEStyleCategory Category, uint8 Value);
	bool operator==(const FWEZoneStyle& O) const { return Zone == O.Zone && WallPaint == O.WallPaint && Floor == O.Floor && Lighting == O.Lighting && Door == O.Door; }
};

/** One choice in a category: name, colour swatch, how it looks, from which company tier, price in the garage. */
struct FWEStyleOption
{
	FText Name;
	FLinearColor Swatch = FLinearColor::White;
	int32 MinTier = 1;
	int64 CostCents = 0;
	/** Surface + tint for walls/floors/doors (Placeholder = keep the building's own). */
	EWESurface Surface = EWESurface::Placeholder;
	FLinearColor Tint = FLinearColor::White;
	float Desaturation = 0.f;
	float Roughness = -1.f;
	/** Lighting: brightness factor and light colour. */
	float LightFactor = 1.f;
	FLinearColor LightColor = FLinearColor(1.f, 0.92f, 0.82f);
};

enum class EWEStyleResult : uint8
{
	Success,
	Invalid,
	Locked,
	NotEnoughMoney,
	Duplicate,
	AlreadySet
};

/**
 * Personalised look of the company's buildings (Phase 17.5): wall paint, floor covering, lighting and the
 * colour of the doors, per building. Options grow with the company tier and cost money (booked as
 * "Gestaltung"). Server-authoritative on the game state, replicated, saved; decoration itself is bought
 * as furniture in build mode.
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWEWorkshopStyleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEWorkshopStyleComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	static UWEWorkshopStyleComponent* Get(const UObject* WorldContext);

	static const TArray<FWEStyleOption>& GetOptions(EWEStyleCategory Category);
	static FText GetCategoryName(EWEStyleCategory Category);
	/** "Garage" for the original garage, otherwise the building upgrade id. */
	static FName GetZoneOf(const AActor* Actor);
	static FText GetZoneName(FName Zone);
	/** Prices scale with the size of the building. */
	static float GetZoneCostFactor(FName Zone);

	/** Zones the company owns right now (garage + bought buildings). */
	TArray<FName> GetOwnedZones() const;
	FWEZoneStyle GetZoneStyle(FName Zone) const;
	const TArray<FWEZoneStyle>& GetAllStyles() const { return Styles; }
	FWEMoney GetPrice(FName Zone, EWEStyleCategory Category, int32 Option) const;
	/** Empty if the option can be chosen now (money not checked). */
	FText GetLockReason(FName Zone, EWEStyleCategory Category, int32 Option) const;

	/** Light colour and brightness factor of the zone (used by the progression's light pass). */
	void GetLighting(FName Zone, float& OutFactor, FLinearColor& OutColor) const;

	// --- Server -------------------------------------------------------------------------------------------
	EWEStyleResult Apply(AWECharacter* By, FName Zone, EWEStyleCategory Category, int32 Option, const FGuid& RequestId);
	/** Loading / tests. */
	void SetAllStyles(const TArray<FWEZoneStyle>& InStyles);

	/** Re-applies every zone's look to the world (any machine). */
	void ApplyAll();

	DECLARE_MULTICAST_DELEGATE(FOnStyleChanged);
	FOnStyleChanged OnStyleChanged;

protected:
	UFUNCTION()
	void OnRep_Styles();

	UPROPERTY(ReplicatedUsing = OnRep_Styles)
	TArray<FWEZoneStyle> Styles;
};
