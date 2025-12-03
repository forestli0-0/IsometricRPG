#include "UI/HUD/HUDResourcePanelWidget.h"

#include "UI/HUD/HUDInventorySlotWidget.h"

void UHUDResourcePanelWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    RegisterItemSlotWidgets();
    RegisterUtilityButtonWidgets();
    RefreshItemSlots();
    RefreshUtilityButtons();
}

void UHUDResourcePanelWidget::NativeConstruct()
{
    Super::NativeConstruct();

    RegisterItemSlotWidgets();
    RegisterUtilityButtonWidgets();
    RefreshItemSlots();
    RefreshUtilityButtons();
}

void UHUDResourcePanelWidget::SetItemSlots(const TArray<FHUDItemSlotViewModel>& InSlots)
{
    CachedItemSlots = InSlots;
    RefreshItemSlots();
}

void UHUDResourcePanelWidget::SetUtilityButtons(const TArray<FHUDItemSlotViewModel>& InButtons)
{
    CachedUtilityButtons = InButtons;
    RefreshUtilityButtons();
}

void UHUDResourcePanelWidget::RefreshItemSlots()
{
    if (ItemSlotWidgets.Num() == 0)
    {
        return;
    }

    for (int32 Index = 0; Index < ItemSlotWidgets.Num(); ++Index)
    {
        if (UHUDInventorySlotWidget* SlotWidget = ItemSlotWidgets[Index])
        {
            if (CachedItemSlots.IsValidIndex(Index))
            {
                SlotWidget->SetSlotData(CachedItemSlots[Index]);
            }
            else
            {
                SlotWidget->ClearSlot();
            }
        }
    }
}

void UHUDResourcePanelWidget::RefreshUtilityButtons()
{
    if (UtilityButtonWidgets.Num() == 0)
    {
        return;
    }

    for (int32 Index = 0; Index < UtilityButtonWidgets.Num(); ++Index)
    {
        if (UHUDInventorySlotWidget* ButtonWidget = UtilityButtonWidgets[Index])
        {
            if (CachedUtilityButtons.IsValidIndex(Index))
            {
                ButtonWidget->SetSlotData(CachedUtilityButtons[Index]);
            }
            else
            {
                ButtonWidget->ClearSlot();
            }
        }
    }
}

void UHUDResourcePanelWidget::RegisterItemSlotWidgets()
{
    ItemSlotWidgets.Reset();

    const TObjectPtr<UHUDInventorySlotWidget> OrderedSlots[] = {
        ItemSlot_1, ItemSlot_2, ItemSlot_3, ItemSlot_4,
        ItemSlot_5, ItemSlot_6
    };

    for (const TObjectPtr<UHUDInventorySlotWidget>& SlotWidget : OrderedSlots)
    {
        ItemSlotWidgets.Add(SlotWidget);
    }
}

void UHUDResourcePanelWidget::RegisterUtilityButtonWidgets()
{
    UtilityButtonWidgets.Reset();

    const TObjectPtr<UHUDInventorySlotWidget> OrderedButtons[] = {
        UtilityButton_1, UtilityButton_2, UtilityButton_3
    };

    for (const TObjectPtr<UHUDInventorySlotWidget>& ButtonWidget : OrderedButtons)
    {
        UtilityButtonWidgets.Add(ButtonWidget);
    }
}
