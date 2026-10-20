#include "Workshop/WEBrandComponent.h"

#include "Framework/WEGameMode.h"
#include "Framework/WEGameState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Player/WECharacterLook.h"

#define LOCTEXT_NAMESPACE "WEBrand"

namespace WEBrand
{
	const TArray<FName>& Symbols()
	{
		static const TArray<FName> List = { TEXT("Wrench"), TEXT("Gear"), TEXT("Screwdriver"), TEXT("Hammer"), TEXT("Repair"), TEXT("Recycle"), TEXT("Factory"),
			TEXT("Star"), TEXT("Truck"), TEXT("Desktop"), TEXT("Bulb"), TEXT("Shield") };
		return List;
	}

	FText SymbolName(int32 Index)
	{
		static const TArray<FText> Names = { LOCTEXT("SWrench", "Schraubenschlüssel"), LOCTEXT("SGear", "Zahnrad"), LOCTEXT("SScrewdriver", "Schraubendreher"),
			LOCTEXT("SHammer", "Hammer"), LOCTEXT("SRepair", "Werkzeuge"), LOCTEXT("SRecycle", "Recycling"), LOCTEXT("SFactory", "Fabrik"), LOCTEXT("SStar", "Stern"),
			LOCTEXT("STruck", "Lieferwagen"), LOCTEXT("SDesktop", "Computer"), LOCTEXT("SBulb", "Glühbirne"), LOCTEXT("SShield", "Schild") };
		return Names.IsValidIndex(Index) ? Names[Index] : FText::GetEmpty();
	}

	const TArray<FLinearColor>& Colors()
	{
		static const TArray<FLinearColor> List = {
			FLinearColor::FromSRGBColor(FColor(0xF0, 0xA9, 0x2B)), // amber
			FLinearColor::FromSRGBColor(FColor(0xF2, 0xF0, 0xEA)), // white
			FLinearColor::FromSRGBColor(FColor(0x1C, 0x1E, 0x22)), // black
			FLinearColor::FromSRGBColor(FColor(0x2B, 0x5F, 0xB8)), // blue
			FLinearColor::FromSRGBColor(FColor(0xC0, 0x2E, 0x26)), // red
			FLinearColor::FromSRGBColor(FColor(0x2F, 0x8F, 0x4B)), // green
			FLinearColor::FromSRGBColor(FColor(0xE8, 0x6A, 0x1C)), // orange
			FLinearColor::FromSRGBColor(FColor(0x7A, 0x80, 0x88)), // grey
		};
		return List;
	}

	FText ColorName(int32 Index)
	{
		static const TArray<FText> Names = { LOCTEXT("CAmber", "Signalgelb"), LOCTEXT("CWhite", "Weiß"), LOCTEXT("CBlack", "Schwarz"), LOCTEXT("CBlue", "Blau"),
			LOCTEXT("CRed", "Rot"), LOCTEXT("CGreen", "Grün"), LOCTEXT("COrange", "Orange"), LOCTEXT("CGrey", "Grau") };
		return Names.IsValidIndex(Index) ? Names[Index] : FText::GetEmpty();
	}

	int32 NumWorkwearColors()
	{
		return WELook::NumOptions(EWELookField::OutfitColor);
	}

	FText WorkwearColorName(int32 Index)
	{
		return WELook::GetOptionName(EWELookField::OutfitColor, Index);
	}
}

void FWECompanyBrand::Clamp()
{
	Symbol = static_cast<uint8>(FMath::Clamp<int32>(Symbol, 0, WEBrand::Symbols().Num() - 1));
	SymbolColor = static_cast<uint8>(FMath::Clamp<int32>(SymbolColor, 0, WEBrand::Colors().Num() - 1));
	Background = static_cast<uint8>(FMath::Clamp<int32>(Background, 0, WEBrand::Colors().Num() - 1));
	WorkwearColor = static_cast<uint8>(FMath::Clamp<int32>(WorkwearColor, 0, FMath::Max(0, WEBrand::NumWorkwearColors() - 1)));
	Name = Name.TrimStartAndEnd().Left(WEBrand::MaxNameLength);
}

uint32 FWECompanyBrand::Hash() const
{
	return HashCombine(GetTypeHash(Name), (static_cast<uint32>(Symbol) << 24) | (static_cast<uint32>(SymbolColor) << 16) | (static_cast<uint32>(Background) << 8) | WorkwearColor) | 1u;
}

UWEBrandComponent::UWEBrandComponent()
{
	SetIsReplicatedByDefault(true);
	// Default: amber spanner on black - the safety colours of the game.
	Brand.Symbol = 0;
	Brand.SymbolColor = 0;
	Brand.Background = 2;
	Brand.WorkwearColor = 0;
}

void UWEBrandComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWEBrandComponent, Brand);
}

void UWEBrandComponent::BeginPlay()
{
	Super::BeginPlay();
	// Server: an unnamed brand takes the company name (the save slot) a moment after start, so clients get it too.
	if (GetOwner()->HasAuthority())
	{
		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (Brand.Name.IsEmpty())
			{
				FWECompanyBrand Named = Brand;
				Named.Name = GetDisplayName();
				SetBrand(Named);
			}
		}), 1.f, false);
	}
}

UWEBrandComponent* UWEBrandComponent::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AWEGameState* GameState = World ? World->GetGameState<AWEGameState>() : nullptr;
	return GameState ? GameState->GetBrand() : nullptr;
}

FWECompanyBrand UWEBrandComponent::GetBrandOf(const UObject* WorldContext)
{
	const UWEBrandComponent* Component = Get(WorldContext);
	return Component ? Component->GetBrand() : FWECompanyBrand();
}

FString UWEBrandComponent::GetDisplayName() const
{
	if (!Brand.Name.IsEmpty())
	{
		return Brand.Name;
	}
	const UWorld* World = GetWorld();
	if (const AWEGameMode* Mode = World ? World->GetAuthGameMode<AWEGameMode>() : nullptr)
	{
		return Mode->GetCurrentSlot().Replace(TEXT("_"), TEXT(" "));
	}
	return FString();
}

bool UWEBrandComponent::SetBrand(const FWECompanyBrand& InBrand)
{
	FWECompanyBrand New = InBrand;
	New.Clamp();
	if (New == Brand)
	{
		return false;
	}
	Brand = New;
	GetOwner()->ForceNetUpdate();
	OnRep_Brand();
	return true;
}

void UWEBrandComponent::OnRep_Brand()
{
	OnBrandChanged.Broadcast();
}

#undef LOCTEXT_NAMESPACE
