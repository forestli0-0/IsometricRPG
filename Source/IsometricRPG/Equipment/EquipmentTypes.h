#pragma once

#include "CoreMinimal.h"
#include "Inventory/InventoryTypes.h"
#include "EquipmentTypes.generated.h"

UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	None        UMETA(DisplayName = "None"),
	Head        UMETA(DisplayName = "Head"),
	Chest       UMETA(DisplayName = "Chest"),
	Legs        UMETA(DisplayName = "Legs"),
	Hands       UMETA(DisplayName = "Hands"),
	Feet        UMETA(DisplayName = "Feet"),
	WeaponMain  UMETA(DisplayName = "Main Weapon"),
	WeaponOff   UMETA(DisplayName = "Off-hand"),
	Accessory1  UMETA(DisplayName = "Accessory 1"),
	Accessory2  UMETA(DisplayName = "Accessory 2"),
	MAX         UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FEquipmentSlotData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	EEquipmentSlot Slot = EEquipmentSlot::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FInventoryItemStack ItemStack;

	bool operator==(const FEquipmentSlotData& Other) const
	{
		return Slot == Other.Slot && ItemStack == Other.ItemStack;
	}

	bool operator!=(const FEquipmentSlotData& Other) const
	{
		return !(*this == Other);
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEquipmentSlotChangedSignature, EEquipmentSlot, Slot, const FEquipmentSlotData&, SlotData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEquipmentRefreshedSignature, const TArray<FEquipmentSlotData>&, EquippedSlots);
