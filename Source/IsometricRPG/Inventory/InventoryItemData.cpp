#include "Inventory/InventoryItemData.h"

const FPrimaryAssetType UInventoryItemData::InventoryItemType(TEXT("InventoryItem"));

UInventoryItemData::UInventoryItemData()
{
}

FPrimaryAssetId UInventoryItemData::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(InventoryItemType, GetFName());
}
