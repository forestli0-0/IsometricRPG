#include "UI/HUD/HUDStatusPanelWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "UObject/NameTypes.h"

namespace
{
    FString BuildStatusSummary(const TArray<FName>& TagNames)
    {
        if (TagNames.Num() == 0)
        {
            return TEXT("");
        }

        FString Summary;
        for (int32 Index = 0; Index < TagNames.Num(); ++Index)
        {
            Summary += TagNames[Index].ToString();
            if (Index < TagNames.Num() - 1)
            {
                Summary += TEXT(" | ");
            }
        }
        return Summary;
    }
}

void UHUDStatusPanelWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    RefreshHealthWidgets();
    RefreshStatusWidgets();
    RefreshPortraitWidgets();
}

void UHUDStatusPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();

    RefreshHealthWidgets();
    RefreshStatusWidgets();
    RefreshPortraitWidgets();
}

void UHUDStatusPanelWidget::SetHealthValues(float InCurrentHealth, float InMaxHealth, float InCurrentShield)
{
    CurrentHealth = FMath::Max(0.f, InCurrentHealth);
    MaxHealth = FMath::Max(0.f, InMaxHealth);
    CurrentShield = FMath::Max(0.f, InCurrentShield);

    RefreshHealthWidgets();
}

void UHUDStatusPanelWidget::SetStatusTags(const TArray<FName>& InTags)
{
    ActiveStatusTags = InTags;
    RefreshStatusWidgets();
}

void UHUDStatusPanelWidget::SetPortraitData(UTexture2D* InPortraitTexture, bool bInCombat, bool bHasLevelUp)
{
    PortraitTexture = InPortraitTexture;
    bIsInCombat = bInCombat;
    bHasPendingLevelUp = bHasLevelUp;

    RefreshPortraitWidgets();
}

void UHUDStatusPanelWidget::RefreshHealthWidgets()
{
    const float SafeMaxHealth = FMath::Max(MaxHealth, KINDA_SMALL_NUMBER);
    const float HealthPercent = FMath::Clamp(CurrentHealth / SafeMaxHealth, 0.f, 1.f);

    if (HealthProgressBar)
    {
        HealthProgressBar->SetPercent(HealthPercent);
    }

    if (HealthValueText)
    {
        const FString HealthString = FString::Printf(TEXT("%.0f / %.0f"), CurrentHealth, MaxHealth);
        HealthValueText->SetText(FText::FromString(HealthString));
    }

    if (ShieldProgressBar)
    {
        const float ShieldPercent = FMath::Clamp(CurrentShield / SafeMaxHealth, 0.f, 1.f);
        ShieldProgressBar->SetPercent(ShieldPercent);
        ShieldProgressBar->SetVisibility(ShieldPercent > 0.f ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (ShieldValueText)
    {
        if (CurrentShield > 0.f)
        {
            ShieldValueText->SetVisibility(ESlateVisibility::Visible);
            ShieldValueText->SetText(FText::FromString(FString::Printf(TEXT("+%.0f"), CurrentShield)));
        }
        else
        {
            ShieldValueText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UHUDStatusPanelWidget::RefreshStatusWidgets()
{
    if (StatusSummaryText)
    {
        const FString Summary = BuildStatusSummary(ActiveStatusTags);
        if (Summary.IsEmpty())
        {
            StatusSummaryText->SetText(FText::GetEmpty());
            StatusSummaryText->SetVisibility(ESlateVisibility::Collapsed);
        }
        else
        {
            StatusSummaryText->SetText(FText::FromString(Summary));
            StatusSummaryText->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }
}

void UHUDStatusPanelWidget::RefreshPortraitWidgets()
{
    if (PortraitImage)
    {
        PortraitImage->SetBrushFromTexture(PortraitTexture);
        PortraitImage->SetColorAndOpacity(FLinearColor::White);
    }

    if (CombatIndicator)
    {
        CombatIndicator->SetVisibility(bIsInCombat ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (LevelUpIndicator)
    {
        LevelUpIndicator->SetVisibility(bHasPendingLevelUp ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}
