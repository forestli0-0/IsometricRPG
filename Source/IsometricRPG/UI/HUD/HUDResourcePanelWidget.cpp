#include "UI/HUD/HUDResourcePanelWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

namespace
{
    FString BuildCurrencySummary(const TMap<FName, int32>& CurrencyValues)
    {
        if (CurrencyValues.Num() == 0)
        {
            return TEXT("");
        }

        FString Summary;
        bool bFirst = true;
        for (const TPair<FName, int32>& Pair : CurrencyValues)
        {
            if (!bFirst)
            {
                Summary += TEXT("  ");
            }
            Summary += FString::Printf(TEXT("%s: %d"), *Pair.Key.ToString(), Pair.Value);
            bFirst = false;
        }
        return Summary;
    }
}

void UHUDResourcePanelWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    RefreshResourceWidgets();
    RefreshExperienceWidgets();
    RefreshCurrencyWidgets();
}

void UHUDResourcePanelWidget::NativeConstruct()
{
    Super::NativeConstruct();

    RefreshResourceWidgets();
    RefreshExperienceWidgets();
    RefreshCurrencyWidgets();
}

void UHUDResourcePanelWidget::SetResourceValues(float InCurrentPrimary, float InMaxPrimary, float InCurrentSecondary, float InMaxSecondary)
{
    CurrentPrimary = FMath::Max(0.f, InCurrentPrimary);
    MaxPrimary = FMath::Max(0.f, InMaxPrimary);
    CurrentSecondary = FMath::Max(0.f, InCurrentSecondary);
    MaxSecondary = FMath::Max(0.f, InMaxSecondary);

    RefreshResourceWidgets();
}

void UHUDResourcePanelWidget::SetExperienceValues(int32 InCurrentLevel, float InCurrentXP, float InRequiredXP)
{
    CurrentLevel = FMath::Max(1, InCurrentLevel);
    CurrentExperience = FMath::Max(0.f, InCurrentXP);
    RequiredExperience = FMath::Max(KINDA_SMALL_NUMBER, InRequiredXP);

    RefreshExperienceWidgets();
}

void UHUDResourcePanelWidget::SetCurrencies(const TMap<FName, int32>& InCurrencyValues)
{
    CurrencyValues = InCurrencyValues;
    RefreshCurrencyWidgets();
}

void UHUDResourcePanelWidget::RefreshResourceWidgets()
{
    const float SafeMaxPrimary = FMath::Max(MaxPrimary, KINDA_SMALL_NUMBER);
    const float PrimaryPercent = FMath::Clamp(CurrentPrimary / SafeMaxPrimary, 0.f, 1.f);
    if (PrimaryResourceProgressBar)
    {
        PrimaryResourceProgressBar->SetPercent(PrimaryPercent);
    }

    if (PrimaryResourceText)
    {
        PrimaryResourceText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), CurrentPrimary, MaxPrimary)));
    }

    const float SafeMaxSecondary = FMath::Max(MaxSecondary, KINDA_SMALL_NUMBER);
    const float SecondaryPercent = FMath::Clamp(CurrentSecondary / SafeMaxSecondary, 0.f, 1.f);
    if (SecondaryResourceProgressBar)
    {
        SecondaryResourceProgressBar->SetPercent(SecondaryPercent);
        SecondaryResourceProgressBar->SetVisibility(MaxSecondary > 0.f ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }

    if (SecondaryResourceText)
    {
        if (MaxSecondary > 0.f)
        {
            SecondaryResourceText->SetVisibility(ESlateVisibility::HitTestInvisible);
            SecondaryResourceText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), CurrentSecondary, MaxSecondary)));
        }
        else
        {
            SecondaryResourceText->SetVisibility(ESlateVisibility::Hidden);
            SecondaryResourceText->SetText(FText::GetEmpty());
        }
    }
}

void UHUDResourcePanelWidget::RefreshExperienceWidgets()
{
    const float SafeRequiredXP = FMath::Max(RequiredExperience, KINDA_SMALL_NUMBER);
    const float XPPercent = FMath::Clamp(CurrentExperience / SafeRequiredXP, 0.f, 1.f);

    if (ExperienceProgressBar)
    {
        ExperienceProgressBar->SetPercent(XPPercent);
    }

    if (ExperienceText)
    {
        ExperienceText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), CurrentExperience, SafeRequiredXP)));
    }

    if (LevelText)
    {
        LevelText->SetText(FText::AsNumber(CurrentLevel));
    }
}

void UHUDResourcePanelWidget::RefreshCurrencyWidgets()
{
    if (CurrencySummaryText)
    {
        const FString Summary = BuildCurrencySummary(CurrencyValues);
        if (Summary.IsEmpty())
        {
            CurrencySummaryText->SetVisibility(ESlateVisibility::Hidden);
            CurrencySummaryText->SetText(FText::GetEmpty());
        }
        else
        {
            CurrencySummaryText->SetVisibility(ESlateVisibility::HitTestInvisible);
            CurrencySummaryText->SetText(FText::FromString(Summary));
        }
    }
}
