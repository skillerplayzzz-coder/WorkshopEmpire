#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WECharacterLook.generated.h"

/** The parts of a character's look the creator offers (order = rows of the creator). */
enum class EWELookField : uint8
{
	Body,
	Face,
	Skin,
	Hair,
	HairColor,
	Outfit,
	OutfitColor,
	Shoes,
	Headwear,
	Glasses,
	Num
};

/**
 * How a player looks (Phase 18c): body, face, skin tone, hair, clothes, shoes, cap and glasses. Chosen in the
 * character creator, kept in the local profile, replicated on the character and stored in the save game.
 * All fields are indices into the option lists of WELook.
 */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWECharacterLook
{
	GENERATED_BODY()

	UPROPERTY() uint8 Body = 0;        // 0 male, 1 female
	UPROPERTY() uint8 Face = 0;        // brows, beard, freckles, make-up (the list depends on the body)
	UPROPERTY() uint8 Skin = 1;
	UPROPERTY() uint8 Hair = 1;        // 0 = none
	UPROPERTY() uint8 HairColor = 1;
	UPROPERTY() uint8 Outfit = 0;
	UPROPERTY() uint8 OutfitColor = 0;
	UPROPERTY() uint8 Shoes = 1;
	UPROPERTY() uint8 Headwear = 0;
	UPROPERTY() uint8 Glasses = 0;
	/** Counts changes on the server (0 = never set); not part of the look itself. */
	UPROPERTY() uint8 Revision = 0;

	uint8 Get(EWELookField Field) const;
	void Set(EWELookField Field, uint8 Value);
	/** Every field inside its option range. */
	void Clamp();
	bool operator==(const FWECharacterLook& Other) const;
	bool operator!=(const FWECharacterLook& Other) const { return !(*this == Other); }
	FString ToString() const;
};

namespace WELook
{
	WORKSHOPEMPIRE_API int32 NumOptions(EWELookField Field);
	WORKSHOPEMPIRE_API FText GetFieldName(EWELookField Field);
	WORKSHOPEMPIRE_API FText GetOptionName(EWELookField Field, int32 Option, int32 Body = 0);
	/** Options of a field for this body (faces: women have no beards but make-up). */
	WORKSHOPEMPIRE_API int32 NumOptionsFor(EWELookField Field, int32 Body);

	/** What a face option shows (Phase 19). */
	struct FFace
	{
		bool bFineBrows = false;
		const TCHAR* Beard = nullptr;  // follower mesh: Hair_Beard, Hair_Moustache, Hair_Goatee
		bool bStubble = false;
		bool bFreckles = false;
		bool bMakeup = false;
	};
	WORKSHOPEMPIRE_API FFace GetFace(const FWECharacterLook& Look);
	/** Steps one field (wraps around) and keeps the look valid. */
	WORKSHOPEMPIRE_API void Step(FWECharacterLook& Look, EWELookField Field, int32 Direction);
	/** A random but sensible look (new players without a profile). */
	WORKSHOPEMPIRE_API FWECharacterLook Random(int32 Seed);

	/** Local profile of this computer's player (Saved/SaveGames/WEProfile.sav). */
	WORKSHOPEMPIRE_API FWECharacterLook LoadProfileLook(bool* bOutFound = nullptr);
	WORKSHOPEMPIRE_API void SaveProfileLook(const FWECharacterLook& Look);
}

UCLASS()
class WORKSHOPEMPIRE_API UWEProfileSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() int32 Version = 1;
	UPROPERTY() FWECharacterLook Look;
};
