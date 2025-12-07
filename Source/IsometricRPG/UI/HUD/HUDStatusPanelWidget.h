#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModelTypes.h"
#include "HUDStatusPanelWidget.generated.h"

class UImage;
class UTextBlock;
class UWidget;
class UTexture2D;

/**
 * Displays portrait, health/defense bars, and active buffs in the bottom-left corner.
 */
UCLASS()
class ISOMETRICRPG_API UHUDStatusPanelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Updates the focused combat stats block. */
    void SetChampionStats(const FHUDChampionStatsViewModel& InStats);

    /** Updates the portrait icon and alert indicators. */
    void SetPortraitData(UTexture2D* InPortraitTexture, bool bInCombat, bool bHasLevelUp);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;

private:
    void RefreshStatWidgets();
    void RefreshPortraitWidgets();

private:
    FHUDChampionStatsViewModel CachedStats;
    bool bHasStats = false;

    TObjectPtr<UTexture2D> PortraitTexture;
    bool bIsInCombat = false;
    bool bHasPendingLevelUp = false;

    /** Bound stat rows. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> AttackDamageText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> AbilityPowerText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ArmorText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> MagicResistText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> PhysicalResistText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> FireResistText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> IceResistText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> LightningResistText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ArmorPenetrationText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> MagicPenetrationText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ElementalPenetrationText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> AttackSpeedText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> CritChanceText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> MoveSpeedText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> PortraitImage;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidget> CombatIndicator;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidget> LevelUpIndicator;
};
