#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "IsometricRPG/IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "HUDSkillSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UProgressBar;
class UTexture2D;

/** Lightweight view-model describing a single skill slot. */
USTRUCT(BlueprintType)
struct FHUDSkillSlotViewModel
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    ESkillSlot Slot = ESkillSlot::Invalid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    TSubclassOf<UGameplayAbility> AbilityClass = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    FGameplayTag AbilityPrimaryTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    FText HotkeyLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    TObjectPtr<UTexture2D> Icon = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    bool bIsUnlocked = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    bool bIsEquipped = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    float ResourceCost = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    float CooldownDuration = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    float CooldownRemaining = 0.f;
};

/**
 * Represents a single skill or action slot within the loadout bar. All runtime logic lives in C++.
 */
UCLASS()
class ISOMETRICRPG_API UHUDSkillSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Populate the slot using the provided data and refresh bound sub-widgets. */
    void SetSlotData(const FHUDSkillSlotViewModel& InData);

    /** Clears the slot, marking it as empty and resetting visuals. */
    void ClearSlot();

    /** Starts a cooldown timer that is ticked and rendered entirely in C++. */
    void BeginCooldown(float DurationSeconds, float InitialRemainingSeconds = -1.f);

    /** Interrupts the cooldown timer if active and hides the overlay. */
    void CancelCooldown();

    /** Returns true when the slot currently has valid data assigned. */
    bool HasData() const { return bHasData; }

    /** Returns true while the cooldown overlay is active. */
    bool IsOnCooldown() const { return bCooldownActive; }

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void ApplySlotDataToWidgets();
    void ApplyCooldownToWidgets(float RemainingSeconds);
    void UpdateEmptyState();
    float GetWorldTime() const;

private:
    /** Cached model for the slot. */
    FHUDSkillSlotViewModel SlotData;

    /** Whether the slot currently holds valid data. */
    bool bHasData = false;

    /** Cooldown bookkeeping (in seconds). */
    bool bCooldownActive = false;
    float CooldownDuration = 0.f;
    float CooldownEndTime = 0.f;

    /** Sub-widgets configured in the designer but driven via C++. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> IconImage;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> NameText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> HotkeyText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> CooldownProgress;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> EmptyStateOverlay;
};
