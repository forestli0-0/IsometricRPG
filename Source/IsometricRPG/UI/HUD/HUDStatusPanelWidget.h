#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDStatusPanelWidget.generated.h"

class UImage;
class UProgressBar;
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
    /** Updates the primary health and optional shield values. */
    void SetHealthValues(float InCurrentHealth, float InMaxHealth, float InCurrentShield);

    /** Updates the active buff/debuff indicators using gameplay tags. */
    void SetStatusTags(const TArray<FName>& InTags);

    /** Updates the portrait icon and alert indicators. */
    void SetPortraitData(UTexture2D* InPortraitTexture, bool bInCombat, bool bHasLevelUp);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;

private:
    void RefreshHealthWidgets();
    void RefreshStatusWidgets();
    void RefreshPortraitWidgets();

private:
    float CurrentHealth = 0.f;
    float MaxHealth = 0.f;
    float CurrentShield = 0.f;

    TArray<FName> ActiveStatusTags;

    TObjectPtr<UTexture2D> PortraitTexture;
    bool bIsInCombat = false;
    bool bHasPendingLevelUp = false;

    /** Bound sub-widgets configured in the designer but driven via C++. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> HealthProgressBar;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> ShieldProgressBar;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> HealthValueText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ShieldValueText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> StatusSummaryText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> PortraitImage;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidget> CombatIndicator;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidget> LevelUpIndicator;
};
