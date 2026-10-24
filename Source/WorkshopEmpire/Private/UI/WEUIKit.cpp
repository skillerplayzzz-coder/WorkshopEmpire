#include "UI/WEUIKit.h"
#include "Components/EditableTextBox.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Items/WEItemDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"
#include "WEMoney.h"

namespace WEUI
{
	namespace Col
	{
		FLinearColor Hex(uint32 RGB, float Alpha)
		{
			FLinearColor C = FLinearColor::FromSRGBColor(FColor((RGB >> 16) & 0xFF, (RGB >> 8) & 0xFF, RGB & 0xFF));
			C.A = Alpha;
			return C;
		}

		FLinearColor WithAlpha(const FLinearColor& C, float Alpha)
		{
			return FLinearColor(C.R, C.G, C.B, Alpha);
		}

		// Mike's "fresh and modern" look (his references): dark petrol glass, teal accent, soft lines, bright status colours.
		const FLinearColor Backdrop = Hex(0x02100F, 0.58f);
		const FLinearColor Panel = Hex(0x0C1C1E, 0.95f);
		const FLinearColor PanelDark = Hex(0x071315, 0.97f);
		const FLinearColor Raised = Hex(0x112729);
		const FLinearColor RaisedHover = Hex(0x173439);
		const FLinearColor Line = Hex(0x1C383B);
		const FLinearColor LineStrong = Hex(0x2A565A);
		const FLinearColor Accent = Hex(0x1DB89E);
		const FLinearColor AccentHover = Hex(0x2BD3B5);
		const FLinearColor AccentDark = Hex(0x0F7F6C);
		const FLinearColor Text = Hex(0xEAF5F2);
		const FLinearColor TextDim = Hex(0x9BB4B0);
		const FLinearColor TextMuted = Hex(0x5E7A76);
		const FLinearColor TextOnAccent = Hex(0xF4FFFC);
		const FLinearColor Good = Hex(0x3FD98A);
		const FLinearColor Bad = Hex(0xF0625A);
		const FLinearColor Warn = Hex(0xF2B544);
		const FLinearColor Info = Hex(0x4FA6F2);
		const FLinearColor Money = Hex(0x8EE05A);
		const FLinearColor OsBlue = Hex(0x123A3B);
		const FLinearColor OsBlueDark = Hex(0x0B2627);
	}

	FSlateFontInfo Font(EFont Kind, int32 Size, int32 LetterSpacing)
	{
		const char* Face = "Regular";
		switch (Kind)
		{
		case EFont::Strong: Face = "Bold"; break;
		case EFont::Head:   Face = "BoldCondensed"; break;
		case EFont::Light:  Face = "Light"; break;
		case EFont::Black:  Face = "Black"; break;
		case EFont::Mono:   Face = "Mono"; break;
		default: break;
		}
		FSlateFontInfo Info = FCoreStyle::GetDefaultFontStyle(Face, Size);
		Info.LetterSpacing = LetterSpacing;
		return Info;
	}

	UTextBlock* Text(UWidgetTree* Tree, const FText& Value, int32 Size, const FLinearColor& Color, EFont Kind, float WrapAt)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>();
		T->SetFont(Font(Kind, Size));
		T->SetColorAndOpacity(FSlateColor(Color));
		T->SetText(Value);
		if (WrapAt > 0.f)
		{
			T->SetWrapTextAt(WrapAt);
			T->SetWrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping);
		}
		return T;
	}

	UTextBlock* Heading(UWidgetTree* Tree, const FText& Value, int32 Size, const FLinearColor& Color)
	{
		UTextBlock* T = Text(Tree, Upper(Value), Size, Color, EFont::Head);
		T->SetFont(Font(EFont::Head, Size, 60));
		return T;
	}

	FSlateBrush Brush(const FLinearColor& Fill, const FLinearColor& Outline, float OutlineWidth, float Radius)
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.TintColor = FSlateColor(Fill);
		// Rounder everywhere (the old 2-4 px corners looked hard): every rounded shape gets softer corners.
		const float R = Radius > 0.f ? FMath::Max(Radius * 2.5f, 5.f) : 0.f;
		B.OutlineSettings = FSlateBrushOutlineSettings(FVector4(R, R, R, R), FSlateColor(Outline), OutlineWidth);
		// A radius of 10 or more means "pill / circle": half the height (a fixed radius larger than that makes pointed ends).
		B.OutlineSettings.RoundingType = Radius >= 10.f ? ESlateBrushRoundingType::HalfHeightRadius : ESlateBrushRoundingType::FixedRadius;
		return B;
	}

	UBorder* Box(UWidgetTree* Tree, const FLinearColor& Fill, const FMargin& Padding, const FLinearColor& Outline, float Radius)
	{
		UBorder* B = Tree->ConstructWidget<UBorder>();
		B->SetBrush(Brush(Fill, Outline, Outline.A > 0.f ? 1.f : 0.f, Radius));
		B->SetPadding(Padding);
		return B;
	}

	UWidget* Shadowed(UWidgetTree* Tree, UWidget* Panel, float Spread)
	{
		UOverlay* O = Tree->ConstructWidget<UOverlay>();
		UImage* Shadow = Tree->ConstructWidget<UImage>();
		FSlateBrush B;
		B.SetResourceObject(UITexture(TEXT("T_PanelShadow")));
		B.DrawAs = ESlateBrushDrawType::Box;
		B.Margin = FMargin(0.25f);
		B.ImageSize = FVector2D(64.f, 64.f);
		Shadow->SetBrush(B);
		Shadow->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* S = O->AddChildToOverlay(Shadow))
		{
			S->SetHorizontalAlignment(HAlign_Fill);
			S->SetVerticalAlignment(VAlign_Fill);
		}
		if (UOverlaySlot* S = O->AddChildToOverlay(Panel))
		{
			S->SetHorizontalAlignment(HAlign_Fill);
			S->SetVerticalAlignment(VAlign_Fill);
			// The light comes from above: a little more shadow below.
			S->SetPadding(FMargin(Spread, Spread * 0.7f, Spread, Spread * 1.3f));
		}
		return O;
	}

	UBorder* WindowPanel(UWidgetTree* Tree, const FMargin& Padding, const FLinearColor& Fill)
	{
		UBorder* B = Tree->ConstructWidget<UBorder>();
		B->SetBrush(Brush(Fill, Col::WithAlpha(Col::LineStrong, 0.85f), 1.f, 6.f));
		B->SetPadding(Padding);
		return B;
	}

	static FSlateSound SlateSound(ESound Kind)
	{
		FSlateSound S;
		if (USoundBase* Base = Sound(Kind))
		{
			S.SetResourceObject(Base);
		}
		return S;
	}

	FButtonStyle ButtonStyle(EButton Kind)
	{
		FButtonStyle S;
		FLinearColor Normal = Col::Raised, Hover = Col::RaisedHover, Pressed = Col::Panel, Outline = Col::Line, HoverOutline = Col::LineStrong;
		float Radius = 4.f;
		FMargin Padding(14.f, 7.f);
		switch (Kind)
		{
		case EButton::Primary:
			Normal = Col::Accent; Hover = Col::AccentHover; Pressed = Col::AccentDark; Outline = Col::AccentDark; HoverOutline = Col::Accent;
			break;
		case EButton::Ghost:
			Normal = FLinearColor::Transparent; Hover = Col::WithAlpha(Col::RaisedHover, 0.8f); Pressed = Col::Raised; Outline = FLinearColor::Transparent; HoverOutline = FLinearColor::Transparent;
			Padding = FMargin(10.f, 6.f);
			break;
		case EButton::Danger:
			Normal = Col::Hex(0x2A1A19); Hover = Col::Hex(0x4A2320); Pressed = Col::Hex(0x1E1312); Outline = Col::Hex(0x7A3029); HoverOutline = Col::Bad;
			break;
		// Tabs are pills: the active one filled with the accent (like Mike's reference).
		case EButton::Tab:
			Normal = FLinearColor::Transparent; Hover = Col::WithAlpha(Col::RaisedHover, 0.9f); Pressed = Col::Raised; Outline = FLinearColor::Transparent; HoverOutline = Col::Line;
			Radius = 5.f; Padding = FMargin(16.f, 8.f);
			break;
		case EButton::TabActive:
			Normal = Col::Accent; Hover = Col::AccentHover; Pressed = Col::AccentDark; Outline = Col::AccentHover; HoverOutline = Col::AccentHover;
			Radius = 5.f; Padding = FMargin(16.f, 8.f);
			break;
		case EButton::Tile:
			Normal = Col::WithAlpha(Col::Raised, 0.0f); Hover = Col::WithAlpha(Col::Hex(0x9CC6F0), 0.18f); Pressed = Col::WithAlpha(Col::Hex(0x9CC6F0), 0.30f);
			Outline = FLinearColor::Transparent; HoverOutline = Col::WithAlpha(Col::Hex(0x9CC6F0), 0.55f);
			Padding = FMargin(6.f);
			break;
		case EButton::Icon:
			Normal = FLinearColor::Transparent; Hover = Col::WithAlpha(FLinearColor::White, 0.12f); Pressed = Col::WithAlpha(FLinearColor::White, 0.2f);
			Outline = FLinearColor::Transparent; HoverOutline = FLinearColor::Transparent; Padding = FMargin(6.f, 4.f);
			break;
		default:
			break;
		}
		S.SetNormal(Brush(Normal, Outline, 1.f, Radius));
		S.SetHovered(Brush(Hover, HoverOutline, 1.f, Radius));
		S.SetPressed(Brush(Pressed, HoverOutline, 1.f, Radius));
		S.SetDisabled(Brush(Col::WithAlpha(Col::Raised, Kind == EButton::Ghost || Kind == EButton::Tab ? 0.f : 0.45f), Col::WithAlpha(Col::Line, 0.5f), 1.f, Radius));
		S.SetNormalPadding(Padding);
		S.SetPressedPadding(FMargin(Padding.Left, Padding.Top + 1.f, Padding.Right, Padding.Bottom - 1.f));
		S.SetPressedSound(SlateSound(Kind == EButton::Tab || Kind == EButton::TabActive ? ESound::Toggle : ESound::Click));
		if (Kind == EButton::Tile)
		{
			S.SetHoveredSound(SlateSound(ESound::Hover));
		}
		return S;
	}

	FLinearColor ButtonTextColor(EButton Kind)
	{
		switch (Kind)
		{
		case EButton::Primary: return Col::TextOnAccent;
		case EButton::TabActive: return Col::TextOnAccent;
		case EButton::Danger:  return Col::Hex(0xF2A59C);
		case EButton::Tab:     return Col::TextDim;
		case EButton::Ghost:   return Col::TextDim;
		default:               return Col::Text;
		}
	}

	UButton* Button(UWidgetTree* Tree, const FText& Label, EButton Kind, FName IconName, int32 FontSize, UTextBlock** OutLabel, UImage** OutIcon)
	{
		UButton* B = Tree->ConstructWidget<UButton>();
		B->SetStyle(ButtonStyle(Kind));
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>();
		B->SetContent(Row);
		const FLinearColor TextColor = ButtonTextColor(Kind);
		if (!IconName.IsNone())
		{
			UImage* I = Icon(Tree, IconName, FontSize + 5.f, TextColor);
			AddH(Row, I, FMargin(0.f, 0.f, Label.IsEmpty() ? 0.f : 7.f, 0.f));
			if (OutIcon)
			{
				*OutIcon = I;
			}
		}
		if (!Label.IsEmpty())
		{
			UTextBlock* T = Text(Tree, Label, FontSize, TextColor, EFont::Head);
			T->SetFont(Font(EFont::Head, FontSize, 20));
			AddH(Row, T);
			if (OutLabel)
			{
				*OutLabel = T;
			}
		}
		if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Row->Slot))
		{
			ContentSlot->SetHorizontalAlignment(HAlign_Center);
			ContentSlot->SetVerticalAlignment(VAlign_Center);
		}
		return B;
	}

	UButton* Button(UWidgetTree* Tree, const FText& Label, EButton Kind, FName IconName, int32 FontSize, TObjectPtr<UTextBlock>* OutLabel, TObjectPtr<UImage>* OutIcon)
	{
		UTextBlock* L = nullptr;
		UImage* I = nullptr;
		UButton* B = Button(Tree, Label, Kind, IconName, FontSize, &L, &I);
		if (OutLabel)
		{
			*OutLabel = L;
		}
		if (OutIcon)
		{
			*OutIcon = I;
		}
		return B;
	}

	void SetButtonKind(UButton* B, EButton Kind, UTextBlock* Label, UImage* IconImage)
	{
		if (!B)
		{
			return;
		}
		B->SetStyle(ButtonStyle(Kind));
		const FLinearColor TextColor = ButtonTextColor(Kind);
		if (Label)
		{
			Label->SetColorAndOpacity(FSlateColor(Kind == EButton::TabActive ? Col::TextOnAccent : TextColor));
		}
		if (IconImage)
		{
			IconImage->SetColorAndOpacity(Kind == EButton::TabActive ? Col::TextOnAccent : TextColor);
		}
	}

	template <typename T>
	static T* LoadQuiet(const FString& Path)
	{
		return Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet));
	}

	UTexture2D* UITexture(const TCHAR* Name)
	{
		static TMap<FString, TWeakObjectPtr<UTexture2D>> Cache;
		const FString Key(Name);
		if (const TWeakObjectPtr<UTexture2D>* Found = Cache.Find(Key); Found && Found->IsValid())
		{
			return Found->Get();
		}
		UTexture2D* Tex = LoadQuiet<UTexture2D>(FString::Printf(TEXT("/Game/WorkshopEmpire/UI/%s.%s"), Name, *FPaths::GetBaseFilename(Key)));
		Cache.Add(Key, Tex);
		return Tex;
	}

	UTexture2D* IconTexture(FName Name)
	{
		if (Name.IsNone())
		{
			return nullptr;
		}
		static TMap<FName, TWeakObjectPtr<UTexture2D>> Cache;
		if (const TWeakObjectPtr<UTexture2D>* Found = Cache.Find(Name); Found && Found->IsValid())
		{
			return Found->Get();
		}
		const FString Asset = FString::Printf(TEXT("T_Icon_%s"), *Name.ToString());
		UTexture2D* Tex = LoadQuiet<UTexture2D>(FString::Printf(TEXT("/Game/WorkshopEmpire/UI/Icons/%s.%s"), *Asset, *Asset));
		Cache.Add(Name, Tex);
		return Tex;
	}

	UTexture2D* FurnitureIcon(FName FurnitureId)
	{
		static TMap<FName, TWeakObjectPtr<UTexture2D>> Cache;
		if (const TWeakObjectPtr<UTexture2D>* Found = Cache.Find(FurnitureId); Found && Found->IsValid())
		{
			return Found->Get();
		}
		const FString Asset = FString::Printf(TEXT("T_Furn_%s"), *FurnitureId.ToString());
		UTexture2D* Tex = LoadQuiet<UTexture2D>(FString::Printf(TEXT("/Game/WorkshopEmpire/UI/ItemIcons/%s.%s"), *Asset, *Asset));
		if (Tex)
		{
			Cache.Add(FurnitureId, Tex);
		}
		return Tex;
	}

	UTexture2D* ItemIcon(const UWEItemDefinition* Definition)
	{
		if (!Definition)
		{
			return nullptr;
		}
		if (Definition->Icon)
		{
			return Definition->Icon;
		}
		static TMap<FName, TWeakObjectPtr<UTexture2D>> Cache;
		static TSet<FName> Missing;
		if (const TWeakObjectPtr<UTexture2D>* Found = Cache.Find(Definition->ItemId); Found && Found->IsValid())
		{
			return Found->Get();
		}
		if (Missing.Contains(Definition->ItemId))
		{
			return nullptr;
		}
		const FString Asset = FString::Printf(TEXT("T_Item_%s"), *Definition->ItemId.ToString());
		UTexture2D* Tex = LoadQuiet<UTexture2D>(FString::Printf(TEXT("/Game/WorkshopEmpire/UI/ItemIcons/%s.%s"), *Asset, *Asset));
		if (Tex)
		{
			Cache.Add(Definition->ItemId, Tex);
		}
		else
		{
			Missing.Add(Definition->ItemId);
		}
		return Tex;
	}

	FName CategoryIcon(uint8 Category)
	{
		switch (static_cast<EWEItemCategory>(Category))
		{
		case EWEItemCategory::Electronics:        return TEXT("Desktop");
		case EWEItemCategory::HouseholdAppliance: return TEXT("Home");
		case EWEItemCategory::PowerTool:          return TEXT("Screwdriver");
		case EWEItemCategory::Motor:              return TEXT("Gear");
		case EWEItemCategory::Machine:            return TEXT("Factory");
		case EWEItemCategory::VehiclePart:        return TEXT("Truck");
		case EWEItemCategory::ScrapMetal:         return TEXT("Recycle");
		case EWEItemCategory::RawMaterial:        return TEXT("Box");
		case EWEItemCategory::SparePart:          return TEXT("Screw");
		case EWEItemCategory::Product:            return TEXT("Tag");
		case EWEItemCategory::Collectible:        return TEXT("Medal");
		case EWEItemCategory::RareFind:           return TEXT("Star");
		case EWEItemCategory::Tool:               return TEXT("Wrench");
		default:                                  return TEXT("Box");
		}
	}

	FName ToolIcon(uint8 ToolType)
	{
		switch (static_cast<EWEToolType>(ToolType))
		{
		case EWEToolType::Screwdriver:   return TEXT("Screwdriver");
		case EWEToolType::Pliers:        return TEXT("Pliers");
		case EWEToolType::SideCutter:    return TEXT("SideCutter");
		case EWEToolType::Hammer:        return TEXT("Hammer");
		case EWEToolType::Multimeter:    return TEXT("Multimeter");
		case EWEToolType::SolderingIron: return TEXT("SolderingIron");
		case EWEToolType::AngleGrinder:  return TEXT("AngleGrinder");
	case EWEToolType::AirDuster:     return TEXT("Wind");
		default:                         return TEXT("Hand");
		}
	}

	FLinearColor RarityColor(uint8 Rarity)
	{
		switch (static_cast<EWEItemRarity>(Rarity))
		{
		case EWEItemRarity::Uncommon: return Col::Hex(0x7FBF6A);
		case EWEItemRarity::Rare:     return Col::Hex(0x5AA2E6);
		case EWEItemRarity::VeryRare: return Col::Hex(0xC07BE8);
		default:                      return Col::TextMuted;
		}
	}

	UImage* Icon(UWidgetTree* Tree, FName Name, float Size, const FLinearColor& Tint)
	{
		UImage* I = Tree->ConstructWidget<UImage>();
		if (UTexture2D* Tex = IconTexture(Name))
		{
			I->SetBrushFromTexture(Tex);
		}
		I->SetDesiredSizeOverride(FVector2D(Size, Size));
		I->SetColorAndOpacity(Tint);
		return I;
	}

	UWidget* Hazard(UWidgetTree* Tree, float Height, float Width)
	{
		// The new look: a short accent pill instead of the yellow-black hazard stripes.
		UBorder* Bar = Box(Tree, Col::Accent, FMargin(0.f), FLinearColor::Transparent, 4.f);
		USizeBox* Size = Tree->ConstructWidget<USizeBox>();
		Size->SetHeightOverride(FMath::Min(Height, 4.f));
		Size->SetWidthOverride(Width > 0.f ? FMath::Min(Width, 56.f) : 56.f);
		Size->SetContent(Bar);
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>();
		AddH(Row, Size);
		return Row;
	}

	UWidget* Divider(UWidgetTree* Tree, const FLinearColor& Color, float Thickness, bool bVertical)
	{
		UBorder* Line = Box(Tree, Color, FMargin(0.f), FLinearColor::Transparent, 0.f);
		USizeBox* Size = Tree->ConstructWidget<USizeBox>();
		if (bVertical)
		{
			Size->SetWidthOverride(Thickness);
		}
		else
		{
			Size->SetHeightOverride(Thickness);
		}
		Size->SetContent(Line);
		return Size;
	}

	UWidget* SectionHeader(UWidgetTree* Tree, const FText& Title, FName IconName, UTextBlock** OutRight, int32 Size)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>();
		UBorder* Bar = Box(Tree, Col::Accent, FMargin(0.f), FLinearColor::Transparent, 2.f);
		AddH(Row, Sized(Tree, Bar, 3.f, Size + 6.f), FMargin(0.f, 0.f, 8.f, 0.f));
		if (!IconName.IsNone())
		{
			AddH(Row, Icon(Tree, IconName, Size + 4.f, Col::TextDim), FMargin(0.f, 0.f, 7.f, 0.f));
		}
		AddH(Row, Heading(Tree, Title, Size, Col::Text), FMargin(), true);
		if (OutRight)
		{
			*OutRight = Text(Tree, FText::GetEmpty(), Size - 2, Col::TextDim, EFont::Strong);
			AddH(Row, *OutRight);
		}
		return Row;
	}

	UWidget* KeyCap(UWidgetTree* Tree, const FText& Key, int32 Size)
	{
		UBorder* Cap = Box(Tree, Col::Hex(0xE9E5DB, 0.95f), FMargin(6.f, 1.f, 6.f, 2.f), Col::Hex(0xFFFFFF), 3.f);
		UTextBlock* T = Text(Tree, Key, Size, Col::Hex(0x0A1A1A), EFont::Head); // dark on the light key cap
		Cap->SetContent(T);
		Cap->SetHorizontalAlignment(HAlign_Center);
		return Cap;
	}

	UBorder* Chip(UWidgetTree* Tree, const FText& Label, const FLinearColor& Color, UTextBlock** OutText)
	{
		// Status pill: tinted, a dot in front, normal case (like "● Available" in Mike's reference).
		UBorder* C = Box(Tree, Col::WithAlpha(Color, 0.14f), FMargin(8.f, 2.f, 9.f, 2.f), Col::WithAlpha(Color, 0.45f), 5.f);
		UTextBlock* T = Text(Tree, FText::Format(NSLOCTEXT("WEUIKit", "ChipFmt", "● {0}"), Label), 10, Color, EFont::Strong);
		C->SetContent(T);
		if (OutText)
		{
			*OutText = T;
		}
		return C;
	}

	UVerticalBoxSlot* AddV(UVerticalBox* Box, UWidget* Child, const FMargin& Padding, EHorizontalAlignment HAlign, bool bFill)
	{
		UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Child);
		if (S)
		{
			S->SetPadding(Padding);
			S->SetHorizontalAlignment(HAlign);
			if (bFill)
			{
				S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		}
		return S;
	}

	UHorizontalBoxSlot* AddH(UHorizontalBox* Box, UWidget* Child, const FMargin& Padding, bool bFill, EVerticalAlignment VAlign)
	{
		UHorizontalBoxSlot* S = Box->AddChildToHorizontalBox(Child);
		if (S)
		{
			S->SetPadding(Padding);
			S->SetVerticalAlignment(VAlign);
			if (bFill)
			{
				S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		}
		return S;
	}

	UWidget* Sized(UWidgetTree* Tree, UWidget* Child, float Width, float Height)
	{
		USizeBox* Size = Tree->ConstructWidget<USizeBox>();
		if (Width > 0.f)
		{
			Size->SetWidthOverride(Width);
		}
		if (Height > 0.f)
		{
			Size->SetHeightOverride(Height);
		}
		Size->SetContent(Child);
		return Size;
	}

	USoundBase* Sound(ESound Kind)
	{
		const TCHAR* Name = nullptr;
		switch (Kind)
		{
		case ESound::Click:   Name = TEXT("click_002"); break;
		case ESound::Hover:   Name = TEXT("select_001"); break;
		case ESound::Open:    Name = TEXT("open_002"); break;
		case ESound::Close:   Name = TEXT("close_002"); break;
		case ESound::Confirm: Name = TEXT("confirmation_002"); break;
		case ESound::Error:   Name = TEXT("error_004"); break;
		case ESound::Toggle:  Name = TEXT("toggle_002"); break;
		case ESound::Drop:    Name = TEXT("drop_002"); break;
		case ESound::Cash:    Name = TEXT("confirmation_001"); break;
		case ESound::Notify:  Name = TEXT("switch_003"); break;
		}
		static TMap<uint8, TWeakObjectPtr<USoundBase>> Cache;
		if (const TWeakObjectPtr<USoundBase>* Found = Cache.Find(static_cast<uint8>(Kind)); Found && Found->IsValid())
		{
			return Found->Get();
		}
		USoundBase* S = Name ? LoadQuiet<USoundBase>(FString::Printf(TEXT("/Game/WorkshopEmpire/Audio/Kenney/%s.%s"), Name, Name)) : nullptr;
		Cache.Add(static_cast<uint8>(Kind), S);
		return S;
	}

	void Play(const UObject* WorldContext, ESound Kind, float Volume)
	{
		if (USoundBase* S = Sound(Kind); S && WorldContext && WorldContext->GetWorld() && !IsRunningDedicatedServer())
		{
			UGameplayStatics::PlaySound2D(WorldContext, S, Volume);
		}
	}

	FText MoneyText(const FWEMoney& Value)
	{
		return FText::FromString(Value.ToDisplayString());
	}

	FText Upper(const FText& Value)
	{
		return Value.ToUpper();
	}

	void FOpenAnim::Tick(UWidget* Target, float DeltaTime)
	{
		if (!Target)
		{
			return;
		}
		Time = FMath::Min(Time + DeltaTime, Duration);
		const float A = Duration > 0.f ? Time / Duration : 1.f;
		const float E = 1.f - FMath::Pow(1.f - A, 3.f);
		Target->SetRenderOpacity(E);
		Target->SetRenderTranslation(FVector2D(0.f, Offset * (1.f - E)));
	}
}

UEditableTextBox* WEUI::InputField(UWidgetTree* Tree, const FText& Value, const FText& Hint, int32 FontSize)
{
	UEditableTextBox* Field = Tree->ConstructWidget<UEditableTextBox>();
	FEditableTextBoxStyle Style = FEditableTextBoxStyle::GetDefault();
	Style.SetBackgroundImageNormal(Brush(Col::PanelDark, Col::LineStrong, 1.f, 3.f));
	Style.SetBackgroundImageHovered(Brush(Col::PanelDark, Col::AccentDark, 1.f, 3.f));
	Style.SetBackgroundImageFocused(Brush(Col::PanelDark, Col::Accent, 1.5f, 3.f));
	Style.SetBackgroundImageReadOnly(Brush(Col::WithAlpha(Col::PanelDark, 0.6f), Col::Line, 1.f, 3.f));
	Style.SetForegroundColor(FSlateColor(Col::Text));
	Style.SetFont(Font(EFont::Body, FontSize));
	Style.SetPadding(FMargin(9.f, 5.f));
	Field->WidgetStyle = Style;
	Field->SetText(Value);
	Field->SetHintText(Hint);
	return Field;
}