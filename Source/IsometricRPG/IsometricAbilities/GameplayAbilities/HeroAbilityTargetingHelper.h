#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"

enum class EHeroAbilityType : uint8;
class UAbilitySystemComponent;

struct FHeroAbilityTargetingPolicy
{
    EHeroAbilityType AbilityType;
    bool bExpectsPreparedTargetData = false;
    bool bExpectsLocationTargetData = false;
    bool bExpectsActorTargetData = false;
    FVector CurrentAimDirection = FVector::ZeroVector;
    bool bIsLocalPredicted = false;
};

class ISOMETRICRPG_API FHeroAbilityTargetingHelper
{
public:
    static const TCHAR* DescribeAbilityType(EHeroAbilityType AbilityType);

    static FString DescribeTargetDataShape(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

    static bool HasRequiredExecutionTargetData(
        const FHeroAbilityTargetingPolicy& Policy,
        const FGameplayAbilityTargetDataHandle& TargetData);

    static bool ValidateExecutionTargetData(
        const UObject& AbilityObject,
        const FHeroAbilityTargetingPolicy& Policy,
        const FGameplayAbilityTargetDataHandle& TargetData,
        const TCHAR* Phase);

    static bool ShouldWaitForReplicatedTargetData(
        const FHeroAbilityTargetingPolicy& Policy,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo& ActivationInfo);

    static void SendInitialTargetDataToServer(
        const FHeroAbilityTargetingPolicy& Policy,
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo& ActivationInfo,
        const FGameplayAbilityTargetDataHandle& TargetDataHandle);

    static void CleanupReplicatedTargetDataDelegates(
        UAbilitySystemComponent* AbilitySystemComponent,
        const FGameplayAbilitySpecHandle& SpecHandle,
        const FGameplayAbilityActivationInfo& ActivationInfo,
        FDelegateHandle& TargetDataDelegateHandle,
        FDelegateHandle& CancelledDelegateHandle);
};
