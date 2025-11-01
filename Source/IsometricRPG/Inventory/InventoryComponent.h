#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/InventoryTypes.h"
#include "InventoryComponent.generated.h"

struct FTimerHandle;

UCLASS(ClassGroup = (Inventory), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class ISOMETRICRPG_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();

    /** Maximum number of slots available in the backpack */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 Capacity = 30;

    UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
    FInventorySlotChangedSignature OnInventorySlotChanged;

    UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
    FInventoryRefreshedSignature OnInventoryRefreshed;

    UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
    FInventoryItemUsedSignature OnInventoryItemUsed;

    UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
    FInventoryItemCooldownStartedSignature OnInventoryItemCooldownStarted;

    UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
    FInventoryItemCooldownEndedSignature OnInventoryItemCooldownEnded;

    UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
    FInventoryCooldownsRefreshedSignature OnInventoryCooldownsRefreshed;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    int32 AddItem(UInventoryItemData* ItemData, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveItemById(FName ItemId, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveItemAtIndex(int32 SlotIndex, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool MoveItem(int32 FromIndex, int32 ToIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    int32 CountItem(FName ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool UseItemAtIndex(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool IsItemOnCooldown(FName ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    float GetRemainingCooldown(FName ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    const TArray<FInventoryItemCooldown>& GetActiveCooldowns() const { return ActiveCooldowns; }

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    const TArray<FInventoryItemSlot>& GetSlots() const { return Slots; }

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    const FInventoryItemSlot& GetSlotChecked(int32 SlotIndex) const;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnRep_Slots(const TArray<FInventoryItemSlot>& OldSlots);

    UFUNCTION()
    void OnRep_ActiveCooldowns(const TArray<FInventoryItemCooldown>& OldCooldowns);

private:
    UPROPERTY(ReplicatedUsing = OnRep_Slots)
    TArray<FInventoryItemSlot> Slots;

    UPROPERTY(ReplicatedUsing = OnRep_ActiveCooldowns)
    TArray<FInventoryItemCooldown> ActiveCooldowns;

    int32 AddToExistingStacks(UInventoryItemData* ItemData, int32 Quantity);
    int32 AddToEmptySlots(UInventoryItemData* ItemData, int32 Quantity);

    void EnsureCapacity();
    void BroadcastSlotChanged(int32 SlotIndex);
    void BroadcastRefresh();

    static bool SlotsEqual(const FInventoryItemSlot& A, const FInventoryItemSlot& B);

    bool TryApplyItemUsageEffects(UInventoryItemData* ItemData);
    void StartItemCooldownInternal(FName ItemId, float Duration);
    void HandleCooldownExpired(FName ItemId);
    void RebuildCooldownIndex();
    int32 FindCooldownIndex(FName ItemId) const;
    FInventoryItemCooldown* FindCooldownEntryMutable(FName ItemId);
    const FInventoryItemCooldown* FindCooldownEntry(FName ItemId) const;
    void ForceOwnerNetUpdate() const;

    TMap<FName, int32> CooldownIndexLookup;
    TMap<FName, FTimerHandle> ActiveCooldownHandles;
};
