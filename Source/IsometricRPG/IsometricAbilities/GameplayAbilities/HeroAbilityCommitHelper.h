#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UGameplayEffect;
class UIsometricRPGAttributeSetBase;
struct FGameplayAbilityActorInfo;
struct FGameplayAbilityActivationInfo;
struct FGameplayAbilitySpecHandle;
struct FGameplayEffectSpec;
struct FGameplayEffectSpecHandle;

class ISOMETRICRPG_API FHeroAbilityCommitHelper
{
public:
    static FString DescribeTagContainer(const FGameplayTagContainer& Tags);

    static FString DescribeMatchedOwnedCooldownTags(
        const FGameplayTagContainer* CooldownTags,
        const FGameplayTagContainer& OwnedTags);

    static bool HasMeaningfulCost(float CostMagnitude);

    static FGameplayEffectSpecHandle MakeCostGameplayEffectSpecHandle(
        const UGameplayAbility& Ability,
        const FGameplayAbilitySpecHandle& Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo& ActivationInfo,
        TSubclassOf<UGameplayEffect> CostGameplayEffectClass,
        float CostMagnitude,
        float AbilityLevel);

    static float ResolveManaCostFromSpec(
        const UGameplayAbility& Ability,
        const FGameplayEffectSpec& CostSpec);

    static bool CheckCost(
        const UGameplayAbility& Ability,
        const FGameplayAbilitySpecHandle& Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        TSubclassOf<UGameplayEffect> CostGameplayEffectClass,
        float CostMagnitude,
        FGameplayTagContainer* OptionalRelevantTags);

    static void ApplyCost(
        const UGameplayAbility& Ability,
        const FGameplayAbilitySpecHandle& Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo& ActivationInfo,
        TSubclassOf<UGameplayEffect> CostGameplayEffectClass,
        float CostMagnitude,
        float AbilityLevel);

    static bool ApplyCooldown(
        const UGameplayAbility& Ability,
        const FGameplayAbilityActorInfo* ActorInfo,
        TSubclassOf<UGameplayEffect> CooldownGameplayEffectClass,
        float CooldownDuration,
        float AbilityLevel);

    static bool ApplyAttackSpeedCooldown(
        const UGameplayAbility& Ability,
        const FGameplayAbilityActorInfo* ActorInfo,
        TSubclassOf<UGameplayEffect> CooldownGameplayEffectClass,
        const UIsometricRPGAttributeSetBase* AttributeSet,
        float AbilityLevel,
        float& OutAppliedCooldownDuration);

    static void NotifyCooldownTriggered(
        const FGameplayAbilitySpecHandle& Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        float CooldownDuration);
};
