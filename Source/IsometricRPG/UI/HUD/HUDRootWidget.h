#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IsometricRPG/IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "UI/HUD/HUDViewModelTypes.h"
#include "HUDRootWidget.generated.h"

class UHUDActionBarWidget;
class UHUDStatusPanelWidget;
class UHUDResourcePanelWidget;
class UTexture2D;
struct FHUDSkillSlotViewModel;

/**
 * Root HUD widget responsible for arranging the bottom combat HUD and auxiliary overlays.
 */
UCLASS()
class ISOMETRICRPG_API UHUDRootWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Returns the action bar section displayed at the bottom center. */
    UFUNCTION(BlueprintPure, Category = "HUD")
    UHUDActionBarWidget* GetActionBar() const { return ActionBar; }

    /** Returns the status panel shown in the bottom-left corner. */
    UFUNCTION(BlueprintPure, Category = "HUD")
    UHUDStatusPanelWidget* GetStatusPanel() const { return StatusPanel; }

    /** Returns the resource panel displayed in the bottom-right corner. */
    UFUNCTION(BlueprintPure, Category = "HUD")
    UHUDResourcePanelWidget* GetResourcePanel() const { return ResourcePanel; }

    /** Routes ability slot view-model data to the action bar. */
    void SetAbilitySlot(const FHUDSkillSlotViewModel& ViewModel);

    /** Clears a specific ability slot. */
    void ClearAbilitySlot(ESkillSlot Slot);

    /** Clears all ability slots. */
    void ClearAllAbilitySlots();

    /** Updates cooldown values for a specific slot. */
    void UpdateAbilityCooldown(ESkillSlot Slot, float Duration, float Remaining);

    /** Updates cached health values and forwards them to the vitals bar. */
    void UpdateHealth(float CurrentHealth, float MaxHealth, float ShieldValue);

    /** Updates the champion stat summary block. */
    void UpdateChampionStats(const FHUDChampionStatsViewModel& Stats);

    /** Forwards active status tags to the buff strip. */
    void UpdateStatusEffects(const TArray<FName>& TagNames);

    /** Forwards curated buff icons to the action bar strip. */
    void UpdateStatusBuffs(const TArray<FHUDBuffIconViewModel>& Buffs);

    /** Updates portrait and alert information. */
    void UpdatePortrait(UTexture2D* PortraitTexture, bool bIsInCombat, bool bHasLevelUp);

    /** Updates the mana/energy values rendered beneath the action bar. */
    void UpdateResources(float CurrentPrimary, float MaxPrimary, float CurrentSecondary, float MaxSecondary);

    /** Updates the experience bar rendered on the action bar. */
    void UpdateExperience(int32 CurrentLevel, float CurrentExperience, float RequiredExperience);

    /** Updates right-side equipment slots. */
    void UpdateItemSlots(const TArray<FHUDItemSlotViewModel>& Slots);

    /** Updates the trio of quick utility buttons. */
    void UpdateUtilityButtons(const TArray<FHUDItemSlotViewModel>& Buttons);

protected:
    void PushVitalStateToActionBar();

    /** Bottom-center action bar (skills, items, summon slots). */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDActionBarWidget> ActionBar;

    /** Bottom-left status panel (portrait, health, buffs). */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDStatusPanelWidget> StatusPanel;

    /** Bottom-right resource panel (inventory and utility buttons). */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDResourcePanelWidget> ResourcePanel;

private:
    float CachedCurrentHealth = 0.f;
    float CachedMaxHealth = 1.f;
    float CachedCurrentMana = 0.f;
    float CachedMaxMana = 1.f;
};
