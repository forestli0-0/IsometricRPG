#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IsometricRPG/IsometricAbilities/Types/EquippedAbilityInfo.h"
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

    /** Forwards health information to the status panel. */
    void UpdateHealth(float CurrentHealth, float MaxHealth, float ShieldValue);

    /** Forwards active status tags to the status panel. */
    void UpdateStatusEffects(const TArray<FName>& TagNames);

    /** Updates portrait and alert information. */
    void UpdatePortrait(UTexture2D* PortraitTexture, bool bIsInCombat, bool bHasLevelUp);

    /** Updates primary/secondary resource values. */
    void UpdateResources(float CurrentPrimary, float MaxPrimary, float CurrentSecondary, float MaxSecondary);

    /** Updates level and experience progress. */
    void UpdateExperience(int32 CurrentLevel, float CurrentExperience, float RequiredExperience);

    /** Updates currency displays. */
    void UpdateCurrencies(const TMap<FName, int32>& CurrencyValues);

protected:
    /** Bottom-center action bar (skills, items, summon slots). */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDActionBarWidget> ActionBar;

    /** Bottom-left status panel (portrait, health, buffs). */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDStatusPanelWidget> StatusPanel;

    /** Bottom-right resource panel (mana/energy, experience, currencies). */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDResourcePanelWidget> ResourcePanel;
};
