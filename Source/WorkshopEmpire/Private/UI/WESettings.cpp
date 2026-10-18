#include "UI/WESettings.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#include "AudioDevice.h"
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/UserInterfaceSettings.h"
#include "EngineUtils.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "Player/WECharacter.h"

#define LOCTEXT_NAMESPACE "WESettings"

namespace WESettings
{
	static const TCHAR* Section = TEXT("WorkshopEmpire.Settings");

	FValues Load()
	{
		FValues V;
		if (GConfig)
		{
			GConfig->GetFloat(Section, TEXT("UIScale"), V.UIScale, GGameUserSettingsIni);
			GConfig->GetFloat(Section, TEXT("MouseSensitivity"), V.MouseSensitivity, GGameUserSettingsIni);
			GConfig->GetBool(Section, TEXT("InvertY"), V.bInvertY, GGameUserSettingsIni);
			GConfig->GetFloat(Section, TEXT("FieldOfView"), V.FieldOfView, GGameUserSettingsIni);
			GConfig->GetFloat(Section, TEXT("MasterVolume"), V.MasterVolume, GGameUserSettingsIni);
		}
		V.UIScale = FMath::Clamp(V.UIScale, 0.8f, 1.3f);
		V.MouseSensitivity = FMath::Clamp(V.MouseSensitivity, 0.2f, 3.f);
		V.FieldOfView = FMath::Clamp(V.FieldOfView, 70.f, 110.f);
		V.MasterVolume = FMath::Clamp(V.MasterVolume, 0.f, 1.f);
		if (UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr)
		{
			V.Resolution = S->GetScreenResolution();
			V.WindowMode = static_cast<int32>(S->GetFullscreenMode());
			V.bVSync = S->IsVSyncEnabled();
			V.FrameLimit = S->GetFrameRateLimit();
		}
		return V;
	}

	void ApplyGameValues(const FValues& V)
	{
		if (UUserInterfaceSettings* UI = GetMutableDefault<UUserInterfaceSettings>())
		{
			UI->ApplicationScale = V.UIScale;
		}
		IConsoleManager& Console = IConsoleManager::Get();
		if (IConsoleVariable* Sens = Console.FindConsoleVariable(TEXT("we.MouseSensitivity")))
		{
			Sens->Set(V.MouseSensitivity, ECVF_SetByGameSetting);
		}
		if (IConsoleVariable* Invert = Console.FindConsoleVariable(TEXT("we.InvertY")))
		{
			Invert->Set(V.bInvertY, ECVF_SetByGameSetting);
		}
		if (IConsoleVariable* Fov = Console.FindConsoleVariable(TEXT("we.FieldOfView")))
		{
			Fov->Set(V.FieldOfView, ECVF_SetByGameSetting);
		}
		if (GEngine)
		{
			if (FAudioDeviceHandle Audio = GEngine->GetMainAudioDevice())
			{
				Audio->SetTransientPrimaryVolume(V.MasterVolume);
			}
			// Running characters take the new field of view right away.
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				for (TActorIterator<AWECharacter> It(World); World && It; ++It)
				{
					if (UCameraComponent* Camera = It->FindComponentByClass<UCameraComponent>())
					{
						Camera->SetFieldOfView(V.FieldOfView);
					}
				}
			}
		}
	}

	void Save(const FValues& V)
	{
		if (GConfig)
		{
			GConfig->SetFloat(Section, TEXT("UIScale"), V.UIScale, GGameUserSettingsIni);
			GConfig->SetFloat(Section, TEXT("MouseSensitivity"), V.MouseSensitivity, GGameUserSettingsIni);
			GConfig->SetBool(Section, TEXT("InvertY"), V.bInvertY, GGameUserSettingsIni);
			GConfig->SetFloat(Section, TEXT("FieldOfView"), V.FieldOfView, GGameUserSettingsIni);
			GConfig->SetFloat(Section, TEXT("MasterVolume"), V.MasterVolume, GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);
		}
		if (UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr)
		{
			S->SetScreenResolution(V.Resolution);
			S->SetFullscreenMode(static_cast<EWindowMode::Type>(FMath::Clamp(V.WindowMode, 0, 2)));
			S->SetVSyncEnabled(V.bVSync);
			S->SetFrameRateLimit(V.FrameLimit);
			S->ApplySettings(false);
			S->SaveSettings();
		}
		ApplyGameValues(V);
	}

	TArray<FIntPoint> GetResolutions()
	{
		TArray<FIntPoint> Result;
		FScreenResolutionArray Modes;
		if (RHIGetAvailableResolutions(Modes, true))
		{
			for (const FScreenResolutionRHI& M : Modes)
			{
				if (M.Width >= 1024 && M.Height >= 720)
				{
					Result.AddUnique(FIntPoint(M.Width, M.Height));
				}
			}
		}
		if (Result.Num() == 0)
		{
			Result = { FIntPoint(1280, 720), FIntPoint(1600, 900), FIntPoint(1920, 1080), FIntPoint(2560, 1440) };
		}
		Result.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X * A.Y < B.X * B.Y; });
		return Result;
	}

	FText GetWindowModeName(int32 Mode)
	{
		switch (Mode)
		{
		case 0:  return LOCTEXT("Fullscreen", "Vollbild");
		case 1:  return LOCTEXT("Borderless", "Randloses Fenster");
		default: return LOCTEXT("Windowed", "Fenster");
		}
	}

	FText GetFrameLimitName(float Limit)
	{
		return Limit <= 0.f ? LOCTEXT("Unlimited", "Unbegrenzt") : FText::Format(LOCTEXT("Fps", "{0} fps"), FText::AsNumber(FMath::RoundToInt(Limit)));
	}
}

#undef LOCTEXT_NAMESPACE
