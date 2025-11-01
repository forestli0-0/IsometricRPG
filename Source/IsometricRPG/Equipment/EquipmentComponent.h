#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Equipment/EquipmentTypes.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "Inventory/InventoryTypes.h"
#include "Inventory/InventoryItemData.h"
#include "EquipmentComponent.generated.h"

class UAbilitySystemComponent;
class UInventoryComponent;
//class UInventoryItemData;

USTRUCT()
struct FEquipmentAppliedModifier
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayAttribute Attribute;

	UPROPERTY()
	float Magnitude = 0.f;

	FEquipmentAppliedModifier() = default;
	FEquipmentAppliedModifier(const FGameplayAttribute& InAttribute, float InMagnitude)
		: Attribute(InAttribute)
		, Magnitude(InMagnitude)
	{
	}
};

/**
 * Handles equipping/unequipping inventory items and applying their stat modifiers.
 */
UCLASS(ClassGroup = (Inventory), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class ISOMETRICRPG_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	bool bAutoReturnUnequippedItemsToInventory = true;

	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FEquipmentSlotChangedSignature OnEquipmentSlotChanged;

	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FEquipmentRefreshedSignature OnEquipmentRefreshed;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipItemFromInventory(int32 InventorySlotIndex, EEquipmentSlot TargetSlot);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool UnequipSlot(EEquipmentSlot Slot, bool bReturnToInventory = true);

	UFUNCTION(BlueprintPure, Category = "Equipment")
	const TArray<FEquipmentSlotData>& GetEquippedSlots() const { return EquippedSlots; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	FEquipmentSlotData GetSlotData(EEquipmentSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool HasItemEquipped(EEquipmentSlot Slot) const;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipItemDirect(UInventoryItemData* ItemData, EEquipmentSlot TargetSlot);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_EquippedSlots(const TArray<FEquipmentSlotData>& OldSlots);

private:
	void InitializeSlots();
	int32 GetSlotInternalIndex(EEquipmentSlot Slot) const;
	void CacheDependencies();
	void ForceOwnerNetUpdate() const;
	bool InternalEquip(UInventoryItemData* ItemData, EEquipmentSlot TargetSlot, int32 SourceInventorySlotIndex);
	void ApplyAttributeModifiers(EEquipmentSlot Slot, const UInventoryItemData* ItemData);
	void ClearAttributeModifiers(EEquipmentSlot Slot);
	bool ResolveAttributeFromTag(const FGameplayTag& Tag, FGameplayAttribute& OutAttribute) const;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedSlots)
	TArray<FEquipmentSlotData> EquippedSlots;

	UPROPERTY()
	TObjectPtr<UInventoryComponent> CachedInventoryComponent;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;

	TMap<EEquipmentSlot, TArray<FEquipmentAppliedModifier>> AppliedModifiersBySlot;
};

