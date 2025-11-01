#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "InventoryItemData.generated.h"

class UGameplayAbility;

UENUM(BlueprintType)
enum class EInventoryItemCategory : uint8
{
    None        UMETA(DisplayName = "None"),
    Equipment   UMETA(DisplayName = "Equipment"),
    Consumable  UMETA(DisplayName = "Consumable"),
    Quest       UMETA(DisplayName = "Quest"),
    Crafting    UMETA(DisplayName = "Crafting"),
    Currency    UMETA(DisplayName = "Currency"),
    Misc        UMETA(DisplayName = "Misc")
};

USTRUCT(BlueprintType)
struct FInventoryItemStatModifier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FGameplayTag TargetTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FGameplayAttribute TargetAttribute; // 修复点：添加此成员

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    float Magnitude = 0.f;

    FInventoryItemStatModifier()
        : Magnitude(0.f)
    {}
};

/**
 * Data-driven definition for inventory items (Diablo-style ARPG).
 */
UCLASS(BlueprintType)
class ISOMETRICRPG_API UInventoryItemData : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UInventoryItemData();

    static const FPrimaryAssetType InventoryItemType;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    /** Unique ID used for save/load and comparisons */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    FName ItemId;

    /** Display label shown in UI */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    FText DisplayName;

    /** Flavor text / gameplay description */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta=(MultiLine=true))
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    EInventoryItemCategory Category = EInventoryItemCategory::None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
    TObjectPtr<UTexture2D> Icon;

    /** Maximum items per stack. 1 means not stackable */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta=(ClampMin="1"))
    int32 MaxStackSize = 1;

    /** Weight contributes to encumbrance calculations */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta=(ClampMin="0"))
    float Weight = 0.f;

    /** Value in in-game currency */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta=(ClampMin="0"))
    int32 GoldValue = 0;

    /** Optional tags for filtering and gameplay interactions */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    FGameplayTagContainer ItemTags;

    /** Optional ability granted when equipped/consumed */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay")
    TSubclassOf<UGameplayAbility> GrantedAbility;

    /** Attribute modifiers applied while the item is equipped */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay")
    TArray<FInventoryItemStatModifier> AttributeModifiers;

    /** Whether the item can be dropped on the ground */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay")
    bool bCanDrop = true;

    /** Whether the item should be consumed (stack reduced) when used successfully */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay")
    bool bConsumeOnUse = true;

    /** Cooldown applied after successful use (seconds) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay", meta = (ClampMin = "0.0"))
    float CooldownDuration = 0.f;

    UFUNCTION(BlueprintPure, Category = "Item")
    bool IsStackable() const { return MaxStackSize > 1; }

    UFUNCTION(BlueprintPure, Category = "Item")
    bool MatchesTag(const FGameplayTag& InTag) const { return ItemTags.HasTag(InTag); }
};
