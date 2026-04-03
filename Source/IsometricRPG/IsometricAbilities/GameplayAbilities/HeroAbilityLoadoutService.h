#pragma once

#include "CoreMinimal.h"

#include "IsometricAbilities/Types/EquippedAbilityInfo.h"

class UAbilitySystemComponent;
class UGameplayAbility;

class ISOMETRICRPG_API FHeroAbilityLoadoutService
{
public:
    static int32 GetSkillBarSlotCount();
    static int32 IndexFromSlot(ESkillSlot Slot);

    static bool EquipAbilityToSlot(
        UAbilitySystemComponent& AbilitySystemComponent,
        UObject* SourceObject,
        TArray<FEquippedAbilityInfo>& EquippedAbilities,
        TSubclassOf<UGameplayAbility> NewAbilityClass,
        ESkillSlot Slot);

    static bool UnequipAbilityFromSlot(
        UAbilitySystemComponent& AbilitySystemComponent,
        TArray<FEquippedAbilityInfo>& EquippedAbilities,
        ESkillSlot Slot);

    static void InitializeDefaultAbilities(
        UAbilitySystemComponent& AbilitySystemComponent,
        UObject* SourceObject,
        const TArray<FEquippedAbilityInfo>& DefaultAbilities,
        TArray<FEquippedAbilityInfo>& EquippedAbilities);

private:
    static int32 ResolveInputID(ESkillSlot Slot);
    static bool IsSameAbilityClass(const FEquippedAbilityInfo& EquippedInfo, TSubclassOf<UGameplayAbility> AbilityClass);
    static void GrantAbility(UAbilitySystemComponent& AbilitySystemComponent, UObject* SourceObject, FEquippedAbilityInfo& EquippedInfo, bool bRemoveExistingFirst);
    static void ClearAbility(UAbilitySystemComponent& AbilitySystemComponent, FEquippedAbilityInfo& EquippedInfo);
};
