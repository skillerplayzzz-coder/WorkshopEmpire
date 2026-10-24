#pragma once

#include "CoreMinimal.h"
#include "Components/SlateWrapperTypes.h"
#include "Fonts/SlateFontInfo.h"
#include "Layout/Margin.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UHorizontalBoxSlot;
class UImage;
class USoundBase;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UVerticalBoxSlot;
class UWidget;
class UWidgetTree;
class UWEItemDefinition;
struct FWEMoney;

/**
 * Phase 17.5: the one visual language of every screen - an industrial workshop look (dark steel panels,
 * thin lines, safety amber, condensed headings, white line icons), built in C++ like the widgets themselves.
 * Widgets use these helpers instead of picking colours and fonts on their own, so the game looks the same
 * everywhere and can be restyled in one place.
 */
class UEditableTextBox;

namespace WEUI
{
	/** Colours (sRGB hex, converted to linear). */
	namespace Col
	{
		WORKSHOPEMPIRE_API extern const FLinearColor Backdrop;   // dims the 3D scene behind full-screen menus
		WORKSHOPEMPIRE_API extern const FLinearColor Panel;      // window body
		WORKSHOPEMPIRE_API extern const FLinearColor PanelDark;  // inset areas, lists
		WORKSHOPEMPIRE_API extern const FLinearColor Raised;     // rows, cards
		WORKSHOPEMPIRE_API extern const FLinearColor RaisedHover;
		WORKSHOPEMPIRE_API extern const FLinearColor Line;
		WORKSHOPEMPIRE_API extern const FLinearColor LineStrong;
		WORKSHOPEMPIRE_API extern const FLinearColor Accent;     // safety amber
		WORKSHOPEMPIRE_API extern const FLinearColor AccentHover;
		WORKSHOPEMPIRE_API extern const FLinearColor AccentDark;
		WORKSHOPEMPIRE_API extern const FLinearColor Text;
		WORKSHOPEMPIRE_API extern const FLinearColor TextDim;
		WORKSHOPEMPIRE_API extern const FLinearColor TextMuted;
		WORKSHOPEMPIRE_API extern const FLinearColor TextOnAccent;
		WORKSHOPEMPIRE_API extern const FLinearColor Good;
		WORKSHOPEMPIRE_API extern const FLinearColor Bad;
		WORKSHOPEMPIRE_API extern const FLinearColor Warn;       // amber: closed, waiting, attention
		WORKSHOPEMPIRE_API extern const FLinearColor Info;
		WORKSHOPEMPIRE_API extern const FLinearColor Money;
		WORKSHOPEMPIRE_API extern const FLinearColor OsBlue;     // Ferrit OS title bars
		WORKSHOPEMPIRE_API extern const FLinearColor OsBlueDark;

		WORKSHOPEMPIRE_API FLinearColor Hex(uint32 RGB, float Alpha = 1.f);
		WORKSHOPEMPIRE_API FLinearColor WithAlpha(const FLinearColor& C, float Alpha);
	}

	enum class EFont : uint8
	{
		Body,     // Roboto Regular
		Strong,   // Roboto Bold
		Head,     // Roboto Bold Condensed (headings, labels, buttons)
		Light,
		Black,
		Mono
	};

	WORKSHOPEMPIRE_API FSlateFontInfo Font(EFont Kind, int32 Size, int32 LetterSpacing = 0);

	/** Text block in the house style (no wrapping unless WrapAt > 0). */
	WORKSHOPEMPIRE_API UTextBlock* Text(UWidgetTree* Tree, const FText& Value, int32 Size, const FLinearColor& Color = Col::Text, EFont Kind = EFont::Body, float WrapAt = 0.f);
	/** Upper-case condensed heading with a little letter spacing. */
	WORKSHOPEMPIRE_API UTextBlock* Heading(UWidgetTree* Tree, const FText& Value, int32 Size, const FLinearColor& Color = Col::Text);

	/** Flat box brush with a thin outline; Radius 0-3 keeps the hard industrial edge. */
	WORKSHOPEMPIRE_API FSlateBrush Brush(const FLinearColor& Fill, const FLinearColor& Outline = FLinearColor::Transparent, float OutlineWidth = 0.f, float Radius = 2.f);
	WORKSHOPEMPIRE_API UBorder* Box(UWidgetTree* Tree, const FLinearColor& Fill, const FMargin& Padding, const FLinearColor& Outline = FLinearColor::Transparent, float Radius = 2.f);
	/** Phase 19 glow-up: a soft drop shadow around a panel (returns the wrapper to add to the layout instead of the panel). */
	WORKSHOPEMPIRE_API UWidget* Shadowed(UWidgetTree* Tree, UWidget* Panel, float Spread = 18.f);
	/** Phase 19 glow-up: a big window panel - rounded, a faint top light, a soft shadow. */
	WORKSHOPEMPIRE_API UBorder* WindowPanel(UWidgetTree* Tree, const FMargin& Padding, const FLinearColor& Fill);

	enum class EButton : uint8
	{
		Primary,    // amber, the main action
		Secondary,  // steel, outlined
		Ghost,      // text only, for toolbars
		Danger,     // fire, sell building, leave
		Tab,        // inactive tab
		TabActive,
		Tile,       // big square launcher buttons (desktop icons, main menu cards)
		Icon        // small square icon button (window controls)
	};

	WORKSHOPEMPIRE_API FButtonStyle ButtonStyle(EButton Kind);
	WORKSHOPEMPIRE_API FLinearColor ButtonTextColor(EButton Kind);

	/**
	 * Button with an optional icon left of the label. Label empty = icon only. Hover/press sounds included.
	 * OutLabel / OutIcon return the inner widgets (for state changes).
	 */
	WORKSHOPEMPIRE_API UButton* Button(UWidgetTree* Tree, const FText& Label, EButton Kind = EButton::Secondary, FName IconName = NAME_None,
		int32 FontSize = 13, UTextBlock** OutLabel = nullptr, UImage** OutIcon = nullptr);
	/** Same, for widget members (TObjectPtr). */
	WORKSHOPEMPIRE_API UButton* Button(UWidgetTree* Tree, const FText& Label, EButton Kind, FName IconName, int32 FontSize, TObjectPtr<UTextBlock>* OutLabel, TObjectPtr<UImage>* OutIcon = nullptr);
	/** Re-styles a button made with Button() (e.g. tab active/inactive), label colour included. */
	WORKSHOPEMPIRE_API void SetButtonKind(UButton* Button, EButton Kind, UTextBlock* Label = nullptr, UImage* IconImage = nullptr);

	/** Line icon from /Game/WorkshopEmpire/UI/Icons (T_Icon_<Name>); nullptr if missing. */
	WORKSHOPEMPIRE_API UTexture2D* IconTexture(FName Name);
	WORKSHOPEMPIRE_API UImage* Icon(UWidgetTree* Tree, FName Name, float Size, const FLinearColor& Tint = Col::Text);
	/** Rendered picture of the item (baked from its model), or nullptr. */
	WORKSHOPEMPIRE_API UTexture2D* ItemIcon(const UWEItemDefinition* Definition);
	/** Phase 18d: picture of a piece of furniture (baked with -WEBakeIcons: UI/ItemIcons/T_Furn_<Id>), or null. */
	WORKSHOPEMPIRE_API UTexture2D* FurnitureIcon(FName FurnitureId);
	/** UI texture by name below /Game/WorkshopEmpire/UI (T_HazardStripe, T_FerritLogo, ...). */
	WORKSHOPEMPIRE_API UTexture2D* UITexture(const TCHAR* Name);
	/** Line icon standing for an item category (fallback picture). */
	WORKSHOPEMPIRE_API FName CategoryIcon(uint8 Category);
	/** Line icon of a tool type (EWEToolType). */
	WORKSHOPEMPIRE_API FName ToolIcon(uint8 ToolType);
	/** Colour of a rarity (EWEItemRarity). */
	WORKSHOPEMPIRE_API FLinearColor RarityColor(uint8 Rarity);

	/** Amber/black diagonal stripe (tiled). */
	WORKSHOPEMPIRE_API UWidget* Hazard(UWidgetTree* Tree, float Height, float Width = 0.f);
	WORKSHOPEMPIRE_API UWidget* Divider(UWidgetTree* Tree, const FLinearColor& Color = Col::Line, float Thickness = 1.f, bool bVertical = false);
	/** Accent bar + upper-case title (+ optional icon and right-aligned text). */
	WORKSHOPEMPIRE_API UWidget* SectionHeader(UWidgetTree* Tree, const FText& Title, FName IconName = NAME_None, UTextBlock** OutRight = nullptr, int32 Size = 14);
	/** "E" drawn as a key cap. */
	WORKSHOPEMPIRE_API UWidget* KeyCap(UWidgetTree* Tree, const FText& Key, int32 Size = 12);
	/** Coloured chip with a short label ("Selten", "Gesperrt"). */
	WORKSHOPEMPIRE_API UBorder* Chip(UWidgetTree* Tree, const FText& Label, const FLinearColor& Color, UTextBlock** OutText = nullptr);
	/** Phase 19: single-line text input in the kit style (search fields, prices). */
	WORKSHOPEMPIRE_API UEditableTextBox* InputField(UWidgetTree* Tree, const FText& Value, const FText& Hint, int32 FontSize = 13);

	// --- Layout helpers ---------------------------------------------------------------------------------------
	WORKSHOPEMPIRE_API UVerticalBoxSlot* AddV(UVerticalBox* Box, UWidget* Child, const FMargin& Padding = FMargin(), EHorizontalAlignment HAlign = HAlign_Fill, bool bFill = false);
	WORKSHOPEMPIRE_API UHorizontalBoxSlot* AddH(UHorizontalBox* Box, UWidget* Child, const FMargin& Padding = FMargin(), bool bFill = false, EVerticalAlignment VAlign = VAlign_Center);
	/** Fixed size (0 = free) around Child. */
	WORKSHOPEMPIRE_API UWidget* Sized(UWidgetTree* Tree, UWidget* Child, float Width, float Height = 0.f);

	// --- Sound -------------------------------------------------------------------------------------------------
	enum class ESound : uint8 { Click, Hover, Open, Close, Confirm, Error, Toggle, Drop, Cash, Notify };
	WORKSHOPEMPIRE_API USoundBase* Sound(ESound Kind);
	WORKSHOPEMPIRE_API void Play(const UObject* WorldContext, ESound Kind, float Volume = 1.f);

	// --- Text ----------------------------------------------------------------------------------------------------
	WORKSHOPEMPIRE_API FText MoneyText(const FWEMoney& Value);
	WORKSHOPEMPIRE_API FText Upper(const FText& Value);

	/**
	 * Opening animation of a screen: fades in and slides up a few pixels over Duration seconds (ease out).
	 * Call Start() when the screen opens and Tick() every frame with the widget to move.
	 */
	struct WORKSHOPEMPIRE_API FOpenAnim
	{
		float Time = 1.f;
		float Duration = 0.18f;
		float Offset = 18.f;
		void Start() { Time = 0.f; }
		bool IsRunning() const { return Time < Duration; }
		void Tick(UWidget* Target, float DeltaTime);
	};
}
