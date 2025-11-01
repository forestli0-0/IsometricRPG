#include "InventoryTypes.h"
#include "Inventory/InventoryItemData.h"

UInventoryItemData* FInventoryItemStack::Resolve() const
{
    if (ItemData.IsNull())
    {
        return nullptr;
    }

    return ItemData.IsValid() ? ItemData.Get() : ItemData.LoadSynchronous();
}

bool FInventoryItemStack::IsValid() const
{
    // 需要 UInventoryItemData 的完整类型，因为 TSoftObjectPtr::IsValid -> Get -> DynamicCast。
    return Quantity > 0 && ItemData.IsValid();
}
