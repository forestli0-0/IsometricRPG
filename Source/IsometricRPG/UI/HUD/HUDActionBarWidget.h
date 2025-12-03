#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IsometricRPG/IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "UI/HUD/HUDViewModelTypes.h"
#include "Materials/MaterialInterface.h"
#include "HUDActionBarWidget.generated.h"

struct FHUDSkillSlotViewModel;
class UHUDSkillLoadoutWidget;
class UHUDSkillSlotWidget;

class UHorizontalBox;
class UProgressBar;
class UTextBlock;
class UOverlay;
class UImage;
/**
 * Bottom-center action bar containing skills, consumables, and contextual prompts.
 */
UCLASS()
class ISOMETRICRPG_API UHUDActionBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Populates the slot map once the widget tree is ready. */
    virtual void NativeConstruct() override;

    /** Assigns view data for a specific slot. */
    void SetSlot(const FHUDSkillSlotViewModel& ViewModel);

    /** Clears an individual slot. */
    void ClearSlot(ESkillSlot Slot);

    /** Clears all registered slots. */
    void ClearAllSlots();

    /** Applies cooldown information to a specific slot. */
    void ApplyCooldown(ESkillSlot Slot, float Duration, float Remaining);

    /** Sets the combined health/mana values rendered below the skill bar. */
    void SetVitals(float InCurrentHealth, float InMaxHealth, float InCurrentMana, float InMaxMana);

    /** Updates the experience bar rendered just below the vitals. */
    void SetExperience(int32 InCurrentLevel, float InCurrentXP, float InRequiredXP);

    /** Updates the lightweight buff strip displayed above the skills. */
    void SetStatusTags(const TArray<FName>& InTags);

    /** Updates the buff strip with curated icon entries (preferred over text tags). */
    void SetStatusBuffs(const TArray<FHUDBuffIconViewModel>& InBuffs);

protected:
    UHUDSkillSlotWidget* FindSlot(ESkillSlot Slot) const;
    void RefreshVitalWidgets();
    void RefreshExperienceWidgets();
    void RefreshBuffWidgets();

    /** Widget that renders the skill slots and item slots. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDSkillLoadoutWidget> SkillLoadout;

    /** Vital bars displayed beneath the skill slots. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> HealthBar;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> ManaBar;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> HealthText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ManaText;

    /** Experience bar + labels rendered under the vitals. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> ExperienceBar;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ExperienceText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> LevelText;

    /** Horizontal box containing buff tag chips above the skill bar. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHorizontalBox> BuffStrip;

    /** Optional overlay that hosts buff icon panels; if null we create overlays locally. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buffs", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UMaterialInterface> BuffCooldownMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buffs", meta = (AllowPrivateAccess = "true"))
    FName BuffCooldownPercentParam = TEXT("Percent");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buffs", meta = (AllowPrivateAccess = "true"))
    FLinearColor BuffCooldownTint = FLinearColor(0.f, 0.f, 0.f, 0.65f);

private:
    TMap<ESkillSlot, TObjectPtr<UHUDSkillSlotWidget>> SlotLookup;
    void RebuildSlotLookup();

    float CurrentHealth = 0.f;
    float MaxHealth = 1.f;
    float CurrentMana = 0.f;
    float MaxMana = 1.f;

    int32 CurrentLevel = 1;
    float CurrentExperience = 0.f;
    float RequiredExperience = 1.f;

    TArray<FName> CachedBuffTags; // legacy debug path
    TArray<FHUDBuffIconViewModel> CachedBuffIcons;

    /** Style: buff icon size in the strip */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style", meta = (AllowPrivateAccess = "true"))
    FVector2D BuffIconSize = FVector2D(20.f, 20.f);
};
