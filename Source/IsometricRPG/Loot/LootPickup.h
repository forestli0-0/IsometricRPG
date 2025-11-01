#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/InventoryTypes.h"
#include "LootPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UInventoryComponent;

UCLASS()
class ISOMETRICRPG_API ALootPickup : public AActor
{
    GENERATED_BODY()
public:
    ALootPickup();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void InitializeLoot(const FInventoryItemStack& Stack);

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
    TObjectPtr<USphereComponent> PickupTrigger;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY(ReplicatedUsing = OnRep_LootStack)
    FInventoryItemStack LootStack;

    UFUNCTION()
    void OnRep_LootStack(const FInventoryItemStack& OldValue);

    void AttemptPickup(UInventoryComponent* InventoryComponent);

    void DestroyIfDepleted();
};
