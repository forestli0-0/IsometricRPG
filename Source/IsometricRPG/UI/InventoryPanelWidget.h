#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/InventoryTypes.h"
#include "InventoryPanelWidget.generated.h"

class UInventoryComponent;

USTRUCT(BlueprintType)
struct FInventoryGridSlot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 Index = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 Row = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 Column = 0;
};

UCLASS()
class ISOMETRICRPG_API UInventoryPanelWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void BindToInventory(UInventoryComponent* InInventory);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void UnbindFromInventory();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    UInventoryComponent* GetInventory() const { return BoundInventory.Get(); }

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Inventory")
    void RefreshInventory(const TArray<FInventoryItemSlot>& Slots);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Inventory")
    void OnInventorySlotUpdated(int32 SlotIndex, const FInventoryItemSlot& SlotData);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Inventory")
    void ShowContextMenu(int32 SlotIndex, const FInventoryItemSlot& SlotData);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Inventory")
    void HideContextMenu();

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Inventory|Cooldowns")
    void OnInventoryItemCooldownStarted(FName ItemId, float Duration, float EndTime);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Inventory|Cooldowns")
    void OnInventoryItemCooldownEnded(FName ItemId);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Inventory|Cooldowns")
    void OnInventoryCooldownsRefreshed(const TArray<FInventoryItemCooldown>& ActiveCooldowns);

protected:
    virtual void NativeDestruct() override;

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    TWeakObjectPtr<UInventoryComponent> BoundInventory;

    UFUNCTION()
    void HandleInventorySlotChanged(int32 SlotIndex, const FInventoryItemSlot& SlotData);

    UFUNCTION()
    void HandleInventoryRefreshed(const TArray<FInventoryItemSlot>& Slots);

    UFUNCTION()
    void HandleInventoryCooldownStarted(FName ItemId, float Duration, float EndTime);

    UFUNCTION()
    void HandleInventoryCooldownEnded(FName ItemId);

    UFUNCTION()
    void HandleInventoryCooldownsRefreshed(const TArray<FInventoryItemCooldown>& ActiveCooldowns);
};
