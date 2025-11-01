#include "UI/SettingsMenuWidget.h"

#include "Engine/GameEngine.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

void USettingsMenuWidget::ApplyQualityPreset(int32 QualityLevel)
{
    if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
    {
        Settings->SetOverallScalabilityLevel(QualityLevel);
        Settings->ApplySettings(false);
        OnSettingsApplied();
    }
}

void USettingsMenuWidget::SetResolution(int32 Width, int32 Height, bool bFullscreen)
{
    if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
    {
        Settings->SetScreenResolution(FIntPoint(Width, Height));
        Settings->SetFullscreenMode(bFullscreen ? EWindowMode::Fullscreen : EWindowMode::Windowed);
        Settings->ApplyResolutionSettings(false);
        OnSettingsApplied();
    }
}

void USettingsMenuWidget::SetFrameRateLimit(float FrameRate)
{
    if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
    {
        Settings->SetFrameRateLimit(FrameRate);
        Settings->ApplySettings(false);
        OnSettingsApplied();
    }
}

void USettingsMenuWidget::SetMasterVolume(float Volume)
{
    const float ClampedVolume = FMath::Clamp(Volume, 0.f, 1.f);
    if (MasterSoundMix && MasterSoundClass)
    {
        UGameplayStatics::SetSoundMixClassOverride(this, MasterSoundMix, MasterSoundClass, ClampedVolume, 1.0f, 0.1f, true);
        EnsureSoundMixApplied();
    }

    OnMasterVolumeChanged(ClampedVolume);
}

void USettingsMenuWidget::SetDisplayGamma(float Gamma)
{
    const float NormalizedGamma = FMath::Clamp(Gamma, 0.5f, 5.0f);
    if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
    {
        // UGameUserSettings 没有 SetDisplayGamma 方法，使用 ApplyNonResolutionSettings 并设置控制台变量
        IConsoleVariable* GammaCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DisplayGamma"));
        if (GammaCVar)
        {
            GammaCVar->Set(NormalizedGamma, ECVF_SetByGameSetting);
        }
        Settings->ApplyNonResolutionSettings();
        OnSettingsApplied();
    }
}

void USettingsMenuWidget::CommitSettings(bool bSaveToDisk)
{
    if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
    {
        Settings->ApplySettings(false);
        if (bSaveToDisk)
        {
            Settings->SaveSettings();
        }
        OnSettingsApplied();
    }
}

void USettingsMenuWidget::EnsureSoundMixApplied()
{
    if (MasterSoundMix)
    {
        UGameplayStatics::PushSoundMixModifier(this, MasterSoundMix);
    }
}
