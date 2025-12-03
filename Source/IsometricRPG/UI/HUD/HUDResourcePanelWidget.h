#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModelTypes.h"
#include "HUDResourcePanelWidget.generated.h"

class UHUDInventorySlotWidget;

/**
 * Displays the inventory carousel and quick utility buttons in the bottom-right corner.
 */
UCLASS()
class ISOMETRICRPG_API UHUDResourcePanelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Updates the right-side equipment slots. */
    void SetItemSlots(const TArray<FHUDItemSlotViewModel>& InSlots);

    /** Updates the configurable quick-access buttons (wards, recall, etc.). */
    void SetUtilityButtons(const TArray<FHUDItemSlotViewModel>& InButtons);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;

private:
    void RefreshItemSlots();
    void RefreshUtilityButtons();
    void RegisterItemSlotWidgets();
    void RegisterUtilityButtonWidgets();

private:
    TArray<FHUDItemSlotViewModel> CachedItemSlots;
    TArray<FHUDItemSlotViewModel> CachedUtilityButtons;

    /** Bound sub-widgets used for visual updates. */

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> ItemSlot_1;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> ItemSlot_2;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> ItemSlot_3;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> ItemSlot_4;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> ItemSlot_5;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> ItemSlot_6;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> UtilityButton_1;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> UtilityButton_2;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> UtilityButton_3;

    TArray<TObjectPtr<UHUDInventorySlotWidget>> ItemSlotWidgets;
    TArray<TObjectPtr<UHUDInventorySlotWidget>> UtilityButtonWidgets;
};
