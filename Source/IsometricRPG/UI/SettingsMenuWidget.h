#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingsMenuWidget.generated.h"

class USoundMix;
class USoundClass;

UCLASS()
class ISOMETRICRPG_API USettingsMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TObjectPtr<USoundMix> MasterSoundMix;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TObjectPtr<USoundClass> MasterSoundClass;

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void ApplyQualityPreset(int32 QualityLevel);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetResolution(int32 Width, int32 Height, bool bFullscreen);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetFrameRateLimit(float FrameRate);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetMasterVolume(float Volume);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetDisplayGamma(float Gamma);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void CommitSettings(bool bSaveToDisk = true);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Settings")
    void OnSettingsApplied();

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Settings")
    void OnMasterVolumeChanged(float NewVolume);

private:
    void EnsureSoundMixApplied();
};
