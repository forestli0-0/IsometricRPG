#pragma once

#include "CoreMinimal.h"

struct FGameplayAbilityActivationInfo;
struct FGameplayAbilitySpecHandle;
struct FGameplayAbilityTargetDataHandle;
struct FGameplayAbilityActorInfo;

class UGA_HeroBaseAbility;

class ISOMETRICRPG_API FHeroAbilityFlowHelper
{
public:
    static void ResetRuntimeAbilityState(UGA_HeroBaseAbility& Ability);

    static bool TryCommitAndExecuteAbility(
        UGA_HeroBaseAbility& Ability,
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const TCHAR* Phase,
        const TCHAR* OtherCheckFailureReason);

    static void HandleTargetDataReady(
        UGA_HeroBaseAbility& Ability,
        const FGameplayAbilityTargetDataHandle& Data);

    static void HandleTargetingCancelled(UGA_HeroBaseAbility& Ability);

    static void HandleMontageCompleted(UGA_HeroBaseAbility& Ability);

    static void HandleMontageInterruptedOrCancelled(UGA_HeroBaseAbility& Ability);
};
