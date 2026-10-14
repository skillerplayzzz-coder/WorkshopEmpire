#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WEWorkshopLayoutComponent.generated.h"

class AWECharacter;
class AWEPlaceholderProp;
class UWEFurnitureCatalog;
struct FWEFurnitureDefinition;

UENUM()
enum class EWEBuildResult : uint8
{
	Success,
	InvalidFurniture,
	OutsideArea,
	Blocked,
	NotEnoughMoney,
	NotMovable,
	NotEmpty,
	Duplicate
};

/**
 * Workshop layout (§14 "Bau- und Platzierungssystem"): which furniture can be bought and where it
 * may stand. Lives on AWEGameState. The rules (CanPlaceAt) run on the client for the preview and
 * again on the server before anything is booked or moved.
 *
 * - Place: inside an AWEBuildArea, no overlap with walls, furniture, items or players; costs money.
 * - Move: any furniture that is not busy and has nothing lying on it; storage contents move along.
 * - Remove: only bought furniture, empty; 50 % of the price back.
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWEWorkshopLayoutComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEWorkshopLayoutComponent();

	static UWEWorkshopLayoutComponent* Get(const UObject* WorldContext);

	const UWEFurnitureCatalog* GetCatalog() const { return Catalog; }
	void SetCatalog(UWEFurnitureCatalog* InCatalog) { Catalog = InCatalog; }
	const FWEFurnitureDefinition* FindDefinition(FName Id) const;

	/** Footprint size (X depth, Y width, Z height) of a catalog entry or an existing prop. */
	static FVector GetFootprint(const AWEPlaceholderProp* Prop);

	/** Grid (cm) and rotation step (degrees) used for snapping. */
	static constexpr float GridSize = 10.f;
	static FTransform Snap(const FVector& Location, float Yaw);

	/** Placement rule check (any machine). Ignore: the prop being moved. */
	bool CanPlaceAt(const FVector& Size, const FTransform& Transform, const AActor* Ignore, FText& OutReason) const;

	/** Moving/removing rule: not busy, nothing lying on it (any machine). */
	static bool CanPickUpFurniture(const AWEPlaceholderProp* Prop, FText& OutReason);

	// --- Server only --------------------------------------------------------------------------------

	EWEBuildResult PlaceFurniture(AWECharacter* By, FName FurnitureId, const FTransform& Transform, const FGuid& RequestId, AWEPlaceholderProp** OutProp = nullptr);
	EWEBuildResult MoveFurniture(AWECharacter* By, AWEPlaceholderProp* Prop, const FTransform& Transform);
	EWEBuildResult RemoveFurniture(AWECharacter* By, AWEPlaceholderProp* Prop, const FGuid& RequestId);

	/** Spawns catalog furniture without booking (loading saves). Name keeps container contents matchable. */
	AWEPlaceholderProp* SpawnFurniture(FName FurnitureId, const FTransform& Transform, FName ActorName = NAME_None);

	static FText ResultText(EWEBuildResult Result);

protected:
	void Notify(AWECharacter* Character, const FText& Message) const;

	UPROPERTY(EditAnywhere, Category = "Build")
	TObjectPtr<UWEFurnitureCatalog> Catalog;
};
