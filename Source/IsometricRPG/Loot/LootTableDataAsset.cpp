#include "Loot/LootTableDataAsset.h"

#include "Inventory/InventoryItemData.h"

void ULootTableDataAsset::GenerateLoot(TArray<FInventoryItemStack>& OutStacks, const FRandomStream& RandomStream) const
{
    if (Entries.Num() == 0)
    {
        return;
    }

    FRandomStream RNG = RandomStream; // 复制一份，避免修改原始流

    int32 DropsSpawned = 0;
    TSet<int32> UniqueDroppedIndices;

    for (int32 Index = 0; Index < Entries.Num(); ++Index)
    {
        if (MaxDropsPerRoll > 0 && DropsSpawned >= MaxDropsPerRoll)
        {
            break;
        }

        const FLootTableEntry& Entry = Entries[Index];
        if (Entry.ItemData.IsNull() || Entry.MaxQuantity <= 0)
        {
            continue;
        }

        if (Entry.bUnique && UniqueDroppedIndices.Contains(Index))
        {
            continue;
        }

        const float Roll = RNG.FRand();
        if (Roll > Entry.DropChance)
        {
            continue;
        }

        const int32 Quantity = (Entry.MinQuantity == Entry.MaxQuantity)
            ? Entry.MinQuantity
            : RNG.RandRange(Entry.MinQuantity, Entry.MaxQuantity);

        if (Quantity <= 0)
        {
            continue;
        }

        FInventoryItemStack Stack;
        Stack.ItemData = Entry.ItemData;
        Stack.Quantity = Quantity;
        OutStacks.Add(Stack);

        ++DropsSpawned;
        if (Entry.bUnique)
        {
            UniqueDroppedIndices.Add(Index);
        }
    }
}
