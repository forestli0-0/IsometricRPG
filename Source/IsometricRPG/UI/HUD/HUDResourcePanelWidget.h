#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDResourcePanelWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * Displays mana/energy, experience, and currency indicators in the bottom-right corner.
 */
UCLASS()
class ISOMETRICRPG_API UHUDResourcePanelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Updates primary/secondary resource values (e.g. mana, energy). */
    void SetResourceValues(float InCurrentPrimary, float InMaxPrimary, float InCurrentSecondary, float InMaxSecondary);

    /** Updates experience progress. */
    void SetExperienceValues(int32 InCurrentLevel, float InCurrentXP, float InRequiredXP);

    /** Updates currency displays. */
    void SetCurrencies(const TMap<FName, int32>& InCurrencyValues);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;

private:
    void RefreshResourceWidgets();
    void RefreshExperienceWidgets();
    void RefreshCurrencyWidgets();

private:
    float CurrentPrimary = 0.f;
    float MaxPrimary = 0.f;
    float CurrentSecondary = 0.f;
    float MaxSecondary = 0.f;

    int32 CurrentLevel = 1;
    float CurrentExperience = 0.f;
    float RequiredExperience = 1.f;

    TMap<FName, int32> CurrencyValues;

    /** Bound sub-widgets used for visual updates. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> PrimaryResourceProgressBar;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> SecondaryResourceProgressBar;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> ExperienceProgressBar;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> PrimaryResourceText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> SecondaryResourceText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ExperienceText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> LevelText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> CurrencySummaryText;
};
