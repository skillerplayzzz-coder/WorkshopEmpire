#include "UI/WEGraphicsSettings.h"

#include "GameFramework/GameUserSettings.h"
#include "Scalability.h"

#define LOCTEXT_NAMESPACE "WEGraphics"

int32 WEGraphics::GetQuality()
{
	// The overall level is -1 when the groups differ (e.g. changed in the console); use the lowest group then.
	const Scalability::FQualityLevels Levels = Scalability::GetQualityLevels();
	return FMath::Clamp(Levels.GetMinQualityLevel(), 0, NumLevels - 1);
}

void WEGraphics::SetQuality(int32 Level, bool bSave)
{
	Level = FMath::Clamp(Level, 0, NumLevels - 1);
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetOverallScalabilityLevel(Level);
		Settings->ApplyNonResolutionSettings();
		if (bSave)
		{
			Settings->SaveSettings();
		}
	}
	else
	{
		Scalability::FQualityLevels Levels = Scalability::GetQualityLevels();
		Levels.SetFromSingleQualityLevel(Level);
		Scalability::SetQualityLevels(Levels);
	}
}

void WEGraphics::ApplyFirstStartDefaults()
{
	static const TCHAR* Section = TEXT("WorkshopEmpire.Graphics");
	constexpr int32 DefaultsVersion = 3;
	int32 Applied = 0;
	GConfig->GetInt(Section, TEXT("DefaultsVersion"), Applied, GGameUserSettingsIni);
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (Applied >= DefaultsVersion || !Settings)
	{
		return;
	}
	// A packaged game starts on "Episch" at 100 % resolution when nothing is saved yet (too slow for mid-range cards).
	Settings->SetOverallScalabilityLevel(2);
	Scalability::FQualityLevels Levels = Scalability::GetQualityLevels();
	Levels.SetFromSingleQualityLevel(2);
	Scalability::SetQualityLevels(Levels);
	// Render resolution by the screen (TSR scales up): about 960 rendered lines - fewer get grainy (Lumen, TSR).
	// RTX 3060, garage, "Hoch": 1440p at 100 % 47 fps, at 67 % 64-70 fps. Never below 67 %.
	FIntPoint Screen = Settings->GetScreenResolution();
	if (Screen.Y <= 0)
	{
		Screen = Settings->GetDesktopResolution();
	}
	const float Percent = FMath::Clamp(FMath::RoundToFloat(960.f / FMath::Max(Screen.Y, 1) * 100.f), 67.f, 100.f);
	GConfig->SetFloat(TEXT("WorkshopEmpire.Settings"), TEXT("RenderScale"), Percent, GGameUserSettingsIni);
	Settings->ApplyNonResolutionSettings();
	Settings->SaveSettings();
	GConfig->SetInt(Section, TEXT("DefaultsVersion"), DefaultsVersion, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
	UE_LOG(LogTemp, Log, TEXT("Graphics: first-start defaults applied (Hoch), shadows now %d, render resolution %.0f %%"), Scalability::GetQualityLevels().ShadowQuality, Percent);
}

int32 WEGraphics::CycleQuality(bool bSave)
{
	const int32 Next = (GetQuality() + 1) % NumLevels;
	SetQuality(Next, bSave);
	return Next;
}

FText WEGraphics::GetQualityName(int32 Level)
{
	switch (FMath::Clamp(Level, 0, NumLevels - 1))
	{
	case 0: return LOCTEXT("Low", "Niedrig");
	case 1: return LOCTEXT("Medium", "Mittel");
	case 2: return LOCTEXT("High", "Hoch");
	default: return LOCTEXT("Epic", "Episch");
	}
}

FText WEGraphics::GetButtonLabel()
{
	return FText::Format(LOCTEXT("Button", "Grafik: {0}"), GetQualityName(GetQuality()));
}

#undef LOCTEXT_NAMESPACE
