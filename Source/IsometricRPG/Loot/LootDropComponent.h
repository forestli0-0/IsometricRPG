#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IsometricRPG/Inventory/InventoryTypes.h"
#include "LootDropComponent.generated.h"

class ULootTableDataAsset;
class ALootPickup;
class UIsometricRPGAttributeSetBase;

UCLASS(ClassGroup = (Loot), meta = (BlueprintSpawnableComponent))
class ISOMETRICRPG_API ULootDropComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    ULootDropComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    TObjectPtr<ULootTableDataAsset> LootTable;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    TSubclassOf<ALootPickup> LootPickupClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    float ScatterRadius = 120.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    bool bDestroyOwnerAfterDrop = true;

    UFUNCTION(BlueprintCallable, Category = "Loot")
    void DropLoot();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY()
    TWeakObjectPtr<UIsometricRPGAttributeSetBase> CachedAttributeSet;

    bool bLootDropped = false;

    UFUNCTION()
    void HandleHealthChanged(UIsometricRPGAttributeSetBase* AttributeSet, float NewHealth);

    void SpawnPickup(const FInventoryItemStack& Stack) const;
    FVector GetRandomScatterLocation() const;

    void BindToAttributeSet();
    void UnbindFromAttributeSet();
};
