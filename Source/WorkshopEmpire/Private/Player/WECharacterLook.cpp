#include "Player/WECharacterLook.h"

#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "WELook"

namespace WELookData
{
	const TCHAR* ProfileSlot = TEXT("WEProfile");

	const TArray<FText>& Options(EWELookField Field)
	{
		static const TArray<FText> Body = { LOCTEXT("Male", "Männlich"), LOCTEXT("Female", "Weiblich") };
		static const TArray<FText> Face = { LOCTEXT("FaceA", "Kräftige Brauen"), LOCTEXT("FaceB", "Feine Brauen"), LOCTEXT("FaceC", "Vollbart"),
			LOCTEXT("FaceD", "Vollbart, feine Brauen") };
		static const TArray<FText> Skin = { LOCTEXT("Skin0", "Sehr hell"), LOCTEXT("Skin1", "Hell"), LOCTEXT("Skin2", "Mittel"), LOCTEXT("Skin3", "Dunkel"),
			LOCTEXT("Skin4", "Sehr dunkel") };
		static const TArray<FText> Hair = { LOCTEXT("Hair0", "Keine Haare"), LOCTEXT("Hair1", "Seitenscheitel"), LOCTEXT("Hair2", "Kurz rasiert"),
			LOCTEXT("Hair3", "Kurzhaar"), LOCTEXT("Hair4", "Lang"), LOCTEXT("Hair5", "Dutt") };
		static const TArray<FText> HairColor = { LOCTEXT("HC0", "Schwarz"), LOCTEXT("HC1", "Dunkelbraun"), LOCTEXT("HC2", "Hellbraun"), LOCTEXT("HC3", "Blond"),
			LOCTEXT("HC4", "Rot"), LOCTEXT("HC5", "Grau") };
		static const TArray<FText> Outfit = { LOCTEXT("Out0", "Arbeitsoverall"), LOCTEXT("Out1", "Freizeit: Polo und Chino"), LOCTEXT("Out2", "Hoodie und Jeans"),
			LOCTEXT("Out3", "T-Shirt und Arbeitshose"), LOCTEXT("Out4", "Jacke und Hose") };
		static const TArray<FText> OutfitColor = { LOCTEXT("OC0", "Blau"), LOCTEXT("OC1", "Grau"), LOCTEXT("OC2", "Grün"), LOCTEXT("OC3", "Rot"), LOCTEXT("OC4", "Schwarz") };
		static const TArray<FText> Shoes = { LOCTEXT("Shoe0", "Sneaker"), LOCTEXT("Shoe1", "Arbeitsschuhe"), LOCTEXT("Shoe2", "Stiefel") };
		static const TArray<FText> Headwear = { LOCTEXT("Head0", "Ohne"), LOCTEXT("Head1", "Basecap"), LOCTEXT("Head2", "Mütze") };
		static const TArray<FText> Glasses = { LOCTEXT("Glass0", "Ohne"), LOCTEXT("Glass1", "Sonnenbrille") };
		static const TArray<FText> None;
		switch (Field)
		{
		case EWELookField::Body:        return Body;
		case EWELookField::Face:        return Face;
		case EWELookField::Skin:        return Skin;
		case EWELookField::Hair:        return Hair;
		case EWELookField::HairColor:   return HairColor;
		case EWELookField::Outfit:      return Outfit;
		case EWELookField::OutfitColor: return OutfitColor;
		case EWELookField::Shoes:       return Shoes;
		case EWELookField::Headwear:    return Headwear;
		case EWELookField::Glasses:     return Glasses;
		default:                        return None;
		}
	}
}

uint8 FWECharacterLook::Get(EWELookField Field) const
{
	switch (Field)
	{
	case EWELookField::Body:        return Body;
	case EWELookField::Face:        return Face;
	case EWELookField::Skin:        return Skin;
	case EWELookField::Hair:        return Hair;
	case EWELookField::HairColor:   return HairColor;
	case EWELookField::Outfit:      return Outfit;
	case EWELookField::OutfitColor: return OutfitColor;
	case EWELookField::Shoes:       return Shoes;
	case EWELookField::Headwear:    return Headwear;
	case EWELookField::Glasses:     return Glasses;
	default:                        return 0;
	}
}

void FWECharacterLook::Set(EWELookField Field, uint8 Value)
{
	switch (Field)
	{
	case EWELookField::Body:        Body = Value; break;
	case EWELookField::Face:        Face = Value; break;
	case EWELookField::Skin:        Skin = Value; break;
	case EWELookField::Hair:        Hair = Value; break;
	case EWELookField::HairColor:   HairColor = Value; break;
	case EWELookField::Outfit:      Outfit = Value; break;
	case EWELookField::OutfitColor: OutfitColor = Value; break;
	case EWELookField::Shoes:       Shoes = Value; break;
	case EWELookField::Headwear:    Headwear = Value; break;
	case EWELookField::Glasses:     Glasses = Value; break;
	default: break;
	}
}

void FWECharacterLook::Clamp()
{
	for (int32 f = 0; f < static_cast<int32>(EWELookField::Num); ++f)
	{
		const EWELookField Field = static_cast<EWELookField>(f);
		Set(Field, static_cast<uint8>(FMath::Clamp<int32>(Get(Field), 0, FMath::Max(0, WELook::NumOptions(Field) - 1))));
	}
	if (Body == 1 && Face >= 2)
	{
		Face -= 2;
	}
}

bool FWECharacterLook::operator==(const FWECharacterLook& Other) const
{
	for (int32 f = 0; f < static_cast<int32>(EWELookField::Num); ++f)
	{
		if (Get(static_cast<EWELookField>(f)) != Other.Get(static_cast<EWELookField>(f)))
		{
			return false;
		}
	}
	return true;
}

FString FWECharacterLook::ToString() const
{
	FString Out;
	for (int32 f = 0; f < static_cast<int32>(EWELookField::Num); ++f)
	{
		Out += FString::Printf(TEXT("%d"), Get(static_cast<EWELookField>(f)));
		Out += f + 1 < static_cast<int32>(EWELookField::Num) ? TEXT(".") : TEXT("");
	}
	return Out;
}

int32 WELook::NumOptions(EWELookField Field)
{
	return WELookData::Options(Field).Num();
}

FText WELook::GetFieldName(EWELookField Field)
{
	switch (Field)
	{
	case EWELookField::Body:        return LOCTEXT("FBody", "Körper");
	case EWELookField::Face:        return LOCTEXT("FFace", "Gesicht");
	case EWELookField::Skin:        return LOCTEXT("FSkin", "Hautton");
	case EWELookField::Hair:        return LOCTEXT("FHair", "Frisur");
	case EWELookField::HairColor:   return LOCTEXT("FHairColor", "Haarfarbe");
	case EWELookField::Outfit:      return LOCTEXT("FOutfit", "Kleidung");
	case EWELookField::OutfitColor: return LOCTEXT("FOutfitColor", "Farbe der Kleidung");
	case EWELookField::Shoes:       return LOCTEXT("FShoes", "Schuhe");
	case EWELookField::Headwear:    return LOCTEXT("FHeadwear", "Kopfbedeckung");
	case EWELookField::Glasses:     return LOCTEXT("FGlasses", "Brille");
	default:                        return FText::GetEmpty();
	}
}

FText WELook::GetOptionName(EWELookField Field, int32 Option)
{
	const TArray<FText>& List = WELookData::Options(Field);
	return List.IsValidIndex(Option) ? List[Option] : FText::GetEmpty();
}

void WELook::Step(FWECharacterLook& Look, EWELookField Field, int32 Direction)
{
	// Women get the two brow styles only (no beard).
	const int32 Count = Field == EWELookField::Face && Look.Body == 1 ? 2 : FMath::Max(1, NumOptions(Field));
	Look.Set(Field, static_cast<uint8>((Look.Get(Field) + (Direction >= 0 ? 1 : -1) + Count) % Count));
	Look.Clamp();
}

FWECharacterLook WELook::Random(int32 Seed)
{
	FRandomStream Rng(Seed);
	FWECharacterLook Look;
	for (int32 f = 0; f < static_cast<int32>(EWELookField::Num); ++f)
	{
		const EWELookField Field = static_cast<EWELookField>(f);
		Look.Set(Field, static_cast<uint8>(Rng.RandRange(0, NumOptions(Field) - 1)));
	}
	// Keep it plausible: most people go without sunglasses and cap.
	Look.Glasses = Rng.FRand() < 0.2f ? 1 : 0;
	Look.Headwear = Rng.FRand() < 0.35f ? Look.Headwear : 0;
	Look.Hair = Look.Hair == 0 && Rng.FRand() < 0.7f ? 1 : Look.Hair;
	Look.Clamp();
	return Look;
}

FWECharacterLook WELook::LoadProfileLook(bool* bOutFound)
{
	if (const UWEProfileSave* Profile = Cast<UWEProfileSave>(UGameplayStatics::LoadGameFromSlot(WELookData::ProfileSlot, 0)))
	{
		if (bOutFound) { *bOutFound = true; }
		FWECharacterLook Look = Profile->Look;
		Look.Clamp();
		return Look;
	}
	if (bOutFound) { *bOutFound = false; }
	return FWECharacterLook();
}

void WELook::SaveProfileLook(const FWECharacterLook& Look)
{
	UWEProfileSave* Profile = Cast<UWEProfileSave>(UGameplayStatics::CreateSaveGameObject(UWEProfileSave::StaticClass()));
	Profile->Look = Look;
	Profile->Look.Clamp();
	UGameplayStatics::SaveGameToSlot(Profile, WELookData::ProfileSlot, 0);
}

#undef LOCTEXT_NAMESPACE
