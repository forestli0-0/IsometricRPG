#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/InventoryTypes.h"
#include "LootTableDataAsset.generated.h"

class UInventoryItemData;

USTRUCT(BlueprintType)
struct FLootTableEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    TSoftObjectPtr<UInventoryItemData> ItemData;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
    int32 MinQuantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
    int32 MaxQuantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DropChance = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    bool bUnique = false;
};

UCLASS(BlueprintType)
class ISOMETRICRPG_API ULootTableDataAsset : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    int32 MaxDropsPerRoll = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    TArray<FLootTableEntry> Entries;

    UFUNCTION(BlueprintCallable, Category = "Loot")
    void GenerateLoot(TArray<FInventoryItemStack>& OutStacks, const FRandomStream& RandomStream) const;
};
