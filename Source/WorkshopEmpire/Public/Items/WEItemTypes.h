#pragma once

#include "CoreMinimal.h"
#include "WEItemTypes.generated.h"

UENUM(BlueprintType)
enum class EWEItemCategory : uint8
{
	Electronics       UMETA(DisplayName = "Elektronik"),
	HouseholdAppliance UMETA(DisplayName = "Haushaltsgerät"),
	PowerTool         UMETA(DisplayName = "Elektrowerkzeug"),
	Motor             UMETA(DisplayName = "Motor"),
	Machine           UMETA(DisplayName = "Maschine"),
	VehiclePart       UMETA(DisplayName = "Fahrzeugteil"),
	ScrapMetal        UMETA(DisplayName = "Metallschrott"),
	RawMaterial       UMETA(DisplayName = "Rohstoff"),
	SparePart         UMETA(DisplayName = "Ersatzteil"),
	Product           UMETA(DisplayName = "Fertigprodukt"),
	Collectible       UMETA(DisplayName = "Sammlerstück"),
	RareFind          UMETA(DisplayName = "Seltener Fund"),
	Tool              UMETA(DisplayName = "Werkzeug")
};

/** Condition levels from the spec (§7.3). Order matters: lower value = better condition. */
UENUM(BlueprintType)
enum class EWEItemCondition : uint8
{
	New       UMETA(DisplayName = "Neu"),
	VeryGood  UMETA(DisplayName = "Sehr gut"),
	Used      UMETA(DisplayName = "Gebraucht"),
	Damaged   UMETA(DisplayName = "Beschädigt"),
	Defective UMETA(DisplayName = "Defekt"),
	Scrap     UMETA(DisplayName = "Schrott")
};

UENUM(BlueprintType)
enum class EWEItemRarity : uint8
{
	Common    UMETA(DisplayName = "Gewöhnlich"),
	Uncommon  UMETA(DisplayName = "Ungewöhnlich"),
	Rare      UMETA(DisplayName = "Selten"),
	VeryRare  UMETA(DisplayName = "Sehr selten")
};

/** Raw material types (§12). Used by disassembly/recycling from Phase 3 on. */
UENUM(BlueprintType)
enum class EWEMaterialType : uint8
{
	Copper        UMETA(DisplayName = "Kupfer"),
	Aluminium     UMETA(DisplayName = "Aluminium"),
	Steel         UMETA(DisplayName = "Stahl"),
	Iron          UMETA(DisplayName = "Eisen"),
	Plastic       UMETA(DisplayName = "Kunststoff"),
	Glass         UMETA(DisplayName = "Glas"),
	Electronics   UMETA(DisplayName = "Elektronische Komponenten"),
	PreciousMetal UMETA(DisplayName = "Edelmetalle")
};

/** Tool categories (§9.3). A cordless drill is a Screwdriver with a higher speed multiplier. */
UENUM(BlueprintType)
enum class EWEToolType : uint8
{
	None          UMETA(DisplayName = "Keins (Hände)"),
	Screwdriver   UMETA(DisplayName = "Schraubendreher"),
	Pliers        UMETA(DisplayName = "Zange"),
	SideCutter    UMETA(DisplayName = "Seitenschneider"),
	Hammer        UMETA(DisplayName = "Hammer"),
	Multimeter    UMETA(DisplayName = "Multimeter"),
	SolderingIron UMETA(DisplayName = "Lötkolben"),
	AngleGrinder  UMETA(DisplayName = "Winkelschleifer")
};

USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEMaterialAmount
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EWEMaterialType Material = EWEMaterialType::Steel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0", Units = "kg"))
	float Kilograms = 0.f;
};

namespace WEItems
{
	WORKSHOPEMPIRE_API FText GetConditionText(EWEItemCondition Condition);
	WORKSHOPEMPIRE_API FText GetCategoryText(EWEItemCategory Category);
	WORKSHOPEMPIRE_API FText GetToolText(EWEToolType Tool);

	/** Value multiplier per condition (New 1.0 ... Scrap 0.05). Placeholder balancing, refined in Phase 4/16. */
	WORKSHOPEMPIRE_API float GetConditionValueFactor(EWEItemCondition Condition);

	/** UI colour per condition (green ... red). */
	WORKSHOPEMPIRE_API FLinearColor GetConditionColor(EWEItemCondition Condition);
}
