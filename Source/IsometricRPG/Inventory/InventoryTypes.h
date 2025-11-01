#pragma once

#include "CoreMinimal.h"
#include "InventoryTypes.generated.h"

class UInventoryItemData;

USTRUCT(BlueprintType)
struct FInventoryItemStack
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    TSoftObjectPtr<UInventoryItemData> ItemData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "0"))
    int32 Quantity = 0;

    bool IsValid() const;

    UInventoryItemData* Resolve() const;

    bool operator==(const FInventoryItemStack& Other) const
    {
        return this->ItemData.ToSoftObjectPath() == Other.ItemData.ToSoftObjectPath() && Quantity == Other.Quantity;
    }

    bool operator!=(const FInventoryItemStack& Other) const
    {
        return !(*this == Other);
    }
};

USTRUCT(BlueprintType)
struct FInventoryItemSlot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 Index = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FInventoryItemStack Stack;

    bool operator==(const FInventoryItemSlot& Other) const
    {
        return Index == Other.Index && Stack == Other.Stack;
    }

    bool operator!=(const FInventoryItemSlot& Other) const
    {
        return !(*this == Other);
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInventorySlotChangedSignature, int32, SlotIndex, const FInventoryItemSlot&, SlotData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryRefreshedSignature, const TArray<FInventoryItemSlot>&, NewInventory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInventoryItemUsedSignature, int32, SlotIndex, const FInventoryItemSlot&, SlotData);

USTRUCT(BlueprintType)
struct FInventoryItemCooldown
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FName ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    float Duration = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    float CooldownEndTime = 0.f;

    bool operator==(const FInventoryItemCooldown& Other) const
    {
        return ItemId == Other.ItemId
            && FMath::IsNearlyEqual(Duration, Other.Duration)
            && FMath::IsNearlyEqual(CooldownEndTime, Other.CooldownEndTime);
    }

    bool operator!=(const FInventoryItemCooldown& Other) const
    {
        return !(*this == Other);
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FInventoryItemCooldownStartedSignature, FName, ItemId, float, Duration, float, EndTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryItemCooldownEndedSignature, FName, ItemId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryCooldownsRefreshedSignature, const TArray<FInventoryItemCooldown>&, ActiveCooldowns);
