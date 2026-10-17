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
