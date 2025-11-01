#include "UI/InventoryPanelWidget.h"

#include "Inventory/InventoryComponent.h"

void UInventoryPanelWidget::BindToInventory(UInventoryComponent* InInventory)
{
    if (BoundInventory.Get() == InInventory)
    {
        return;
    }

    UnbindFromInventory();

    if (!InInventory)
    {
        return;
    }

    BoundInventory = InInventory;
    InInventory->OnInventorySlotChanged.AddDynamic(this, &UInventoryPanelWidget::HandleInventorySlotChanged);
    InInventory->OnInventoryRefreshed.AddDynamic(this, &UInventoryPanelWidget::HandleInventoryRefreshed);
    InInventory->OnInventoryItemCooldownStarted.AddDynamic(this, &UInventoryPanelWidget::HandleInventoryCooldownStarted);
    InInventory->OnInventoryItemCooldownEnded.AddDynamic(this, &UInventoryPanelWidget::HandleInventoryCooldownEnded);
    InInventory->OnInventoryCooldownsRefreshed.AddDynamic(this, &UInventoryPanelWidget::HandleInventoryCooldownsRefreshed);
    HandleInventoryRefreshed(InInventory->GetSlots());
    HandleInventoryCooldownsRefreshed(InInventory->GetActiveCooldowns());
}

void UInventoryPanelWidget::UnbindFromInventory()
{
    if (UInventoryComponent* Inventory = BoundInventory.Get())
    {
        Inventory->OnInventorySlotChanged.RemoveDynamic(this, &UInventoryPanelWidget::HandleInventorySlotChanged);
        Inventory->OnInventoryRefreshed.RemoveDynamic(this, &UInventoryPanelWidget::HandleInventoryRefreshed);
        Inventory->OnInventoryItemCooldownStarted.RemoveDynamic(this, &UInventoryPanelWidget::HandleInventoryCooldownStarted);
        Inventory->OnInventoryItemCooldownEnded.RemoveDynamic(this, &UInventoryPanelWidget::HandleInventoryCooldownEnded);
        Inventory->OnInventoryCooldownsRefreshed.RemoveDynamic(this, &UInventoryPanelWidget::HandleInventoryCooldownsRefreshed);
    }

    BoundInventory = nullptr;
}

void UInventoryPanelWidget::NativeDestruct()
{
    UnbindFromInventory();
    Super::NativeDestruct();
}

void UInventoryPanelWidget::HandleInventorySlotChanged(int32 SlotIndex, const FInventoryItemSlot& SlotData)
{
    OnInventorySlotUpdated(SlotIndex, SlotData);
}

void UInventoryPanelWidget::HandleInventoryRefreshed(const TArray<FInventoryItemSlot>& Slots)
{
    RefreshInventory(Slots);
}

void UInventoryPanelWidget::HandleInventoryCooldownStarted(FName ItemId, float Duration, float EndTime)
{
    OnInventoryItemCooldownStarted(ItemId, Duration, EndTime);
}

void UInventoryPanelWidget::HandleInventoryCooldownEnded(FName ItemId)
{
    OnInventoryItemCooldownEnded(ItemId);
}

void UInventoryPanelWidget::HandleInventoryCooldownsRefreshed(const TArray<FInventoryItemCooldown>& ActiveCooldowns)
{
    OnInventoryCooldownsRefreshed(ActiveCooldowns);
}
