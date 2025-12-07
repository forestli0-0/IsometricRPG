#include "UI/HUD/HUDStatusPanelWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Internationalization/Text.h"

void UHUDStatusPanelWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    RefreshStatWidgets();
    RefreshPortraitWidgets();
}

void UHUDStatusPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();

    RefreshStatWidgets();
    RefreshPortraitWidgets();
}

void UHUDStatusPanelWidget::SetChampionStats(const FHUDChampionStatsViewModel& InStats)
{
    CachedStats = InStats;
    bHasStats = true;
    RefreshStatWidgets();
}

void UHUDStatusPanelWidget::SetPortraitData(UTexture2D* InPortraitTexture, bool bInCombat, bool bHasLevelUp)
{
    PortraitTexture = InPortraitTexture;
    bIsInCombat = bInCombat;
    bHasPendingLevelUp = bHasLevelUp;

    RefreshPortraitWidgets();
}

void UHUDStatusPanelWidget::RefreshStatWidgets()
{
    if (!bHasStats)
    {
        const TArray<TObjectPtr<UTextBlock>> StatTexts = {
            AttackDamageText, AbilityPowerText, ArmorText,
            MagicResistText, PhysicalResistText, FireResistText, IceResistText, LightningResistText,
            ArmorPenetrationText, MagicPenetrationText, ElementalPenetrationText,
            AttackSpeedText, CritChanceText, MoveSpeedText
        };

        for (TObjectPtr<UTextBlock> StatText : StatTexts)
        {
            if (StatText)
            {
                StatText->SetText(FText::GetEmpty());
                StatText->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
        return;
    }

    auto SetStatText = [](UTextBlock* Target, float Value)
    {
        if (Target)
        {
            Target->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Value)));
            Target->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    };

    SetStatText(AttackDamageText, CachedStats.AttackDamage);
    SetStatText(AbilityPowerText, CachedStats.AbilityPower);
    SetStatText(ArmorText, CachedStats.Armor);
    SetStatText(MagicResistText, CachedStats.MagicResist);
    SetStatText(PhysicalResistText, CachedStats.PhysicalResist);
    SetStatText(FireResistText, CachedStats.FireResist);
    SetStatText(IceResistText, CachedStats.IceResist);
    SetStatText(LightningResistText, CachedStats.LightningResist);
    SetStatText(ArmorPenetrationText, CachedStats.ArmorPenetration);
    SetStatText(MagicPenetrationText, CachedStats.MagicPenetration);
    SetStatText(ElementalPenetrationText, CachedStats.ElementalPenetration);
    SetStatText(AttackSpeedText, CachedStats.AttackSpeed);
    SetStatText(CritChanceText, CachedStats.CritChance);
    SetStatText(MoveSpeedText, CachedStats.MoveSpeed);
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
