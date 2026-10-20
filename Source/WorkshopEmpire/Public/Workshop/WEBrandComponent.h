#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WEBrandComponent.generated.h"

/** The company's logo and colours (Phase 20). Indices into the lists of WEBrand. */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWECompanyBrand
{
	GENERATED_BODY()

	UPROPERTY() uint8 Symbol = 0;
	UPROPERTY() uint8 SymbolColor = 0;
	UPROPERTY() uint8 Background = 0;
	/** Colour of the work clothes (index into the outfit colours of the character look). */
	UPROPERTY() uint8 WorkwearColor = 0;
	/** Text under the symbol; empty = the company (save slot) name. */
	UPROPERTY() FString Name;

	bool operator==(const FWECompanyBrand& O) const
	{
		return Symbol == O.Symbol && SymbolColor == O.SymbolColor && Background == O.Background && WorkwearColor == O.WorkwearColor && Name == O.Name;
	}
	bool operator!=(const FWECompanyBrand& O) const { return !(*this == O); }
	void Clamp();
	uint32 Hash() const;
};

namespace WEBrand
{
	/** Icon names of the symbols (UI icons). */
	WORKSHOPEMPIRE_API const TArray<FName>& Symbols();
	WORKSHOPEMPIRE_API FText SymbolName(int32 Index);
	/** Colours for symbol and background. */
	WORKSHOPEMPIRE_API const TArray<FLinearColor>& Colors();
	WORKSHOPEMPIRE_API FText ColorName(int32 Index);
	/** Names of the work clothes colours (the outfit colours of the character look). */
	WORKSHOPEMPIRE_API int32 NumWorkwearColors();
	WORKSHOPEMPIRE_API FText WorkwearColorName(int32 Index);
	constexpr int32 MaxNameLength = 18;
}

/**
 * Phase 20: the company's brand - a symbol in two colours, a name and the colour of the work clothes. Set in the
 * Ferrit OS ("Gestaltung"), free of charge. Shown on the overalls of players and employees, on the vans, above the
 * garage door and on the company sign (UWELogoComponent). Lives on the game state, replicated, saved.
 */
UCLASS(ClassGroup = (WorkshopEmpire))
class WORKSHOPEMPIRE_API UWEBrandComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWEBrandComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	static UWEBrandComponent* Get(const UObject* WorldContext);
	/** The brand of the world WorldContext is in (defaults when there is none, e.g. in the menu). */
	static FWECompanyBrand GetBrandOf(const UObject* WorldContext);

	const FWECompanyBrand& GetBrand() const { return Brand; }
	/** Name shown under the logo (the brand's name or the company name). */
	FString GetDisplayName() const;

	/** Server: new brand (clamped). False if nothing changed. */
	bool SetBrand(const FWECompanyBrand& InBrand);

	DECLARE_MULTICAST_DELEGATE(FOnBrandChanged);
	FOnBrandChanged OnBrandChanged;

protected:
	UFUNCTION() void OnRep_Brand();

	UPROPERTY(ReplicatedUsing = OnRep_Brand)
	FWECompanyBrand Brand;
};
