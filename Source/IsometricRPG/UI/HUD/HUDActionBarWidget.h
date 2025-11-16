#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IsometricRPG/IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "HUDActionBarWidget.generated.h"

struct FHUDSkillSlotViewModel;
class UHUDSkillLoadoutWidget;
class UHUDSkillSlotWidget;

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

protected:
    UHUDSkillSlotWidget* FindSlot(ESkillSlot Slot) const;
    void RegisterSlot(ESkillSlot Slot, UHUDSkillSlotWidget* SlotWidget);

    /** Widget that renders the skill slots and item slots. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDSkillLoadoutWidget> SkillLoadout;

    /** Individual skill slots keyed by the ESkillSlot enum. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDSkillSlotWidget> Slot_Q;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDSkillSlotWidget> Slot_W;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDSkillSlotWidget> Slot_E;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDSkillSlotWidget> Slot_R;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDSkillSlotWidget> Slot_D;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDSkillSlotWidget> Slot_F;

private:
    TMap<ESkillSlot, TObjectPtr<UHUDSkillSlotWidget>> SlotLookup;
};
