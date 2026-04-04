#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"

class ACharacter;
class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UGA_HeroBaseAbility;

class ISOMETRICRPG_API FHeroAbilityMontageHelper
{
public:
    static float ResolvePlayRate(const UAnimMontage* MontageToPlay, float CooldownDuration);

    static bool TryFaceCharacterToCurrentTarget(
        ACharacter& Character,
        const FVector& CurrentAimDirection,
        const FGameplayAbilityTargetDataHandle& TargetDataHandle);

    static UAbilityTask_PlayMontageAndWait* PlayAbilityMontage(
        UGA_HeroBaseAbility& Ability,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityTargetDataHandle& TargetDataHandle,
        const FVector& CurrentAimDirection,
        UAnimMontage* MontageToPlay,
        float CooldownDuration,
        bool bFaceTarget);

    static void BindBaseMontageTaskCallbacks(
        UGA_HeroBaseAbility& Ability,
        UAbilityTask_PlayMontageAndWait& MontageTask,
        bool bEndAbilityWhenBaseMontageEnds,
        bool bCancelAbilityWhenBaseMontageInterrupted);
};
