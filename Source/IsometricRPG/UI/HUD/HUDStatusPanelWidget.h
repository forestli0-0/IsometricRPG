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
 * 在左下角显示：头像、生命/防御数值条，以及当前生效的增益/减益图标。
 */
UCLASS()
class ISOMETRICRPG_API UHUDStatusPanelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 更新聚焦的战斗属性面板（攻击力、法强、护甲等）。 */
    void SetChampionStats(const FHUDChampionStatsViewModel& InStats);

    /** 更新头像贴图和提示（是否处于战斗、是否有可分配的等级点）。 */
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
