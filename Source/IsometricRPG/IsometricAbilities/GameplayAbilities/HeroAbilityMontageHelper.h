#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"

class ACharacter;
class UAnimMontage;

class ISOMETRICRPG_API FHeroAbilityMontageHelper
{
public:
    static float ResolvePlayRate(const UAnimMontage* MontageToPlay, float CooldownDuration);

    static bool TryFaceCharacterToCurrentTarget(
        ACharacter& Character,
        const FVector& CurrentAimDirection,
        const FGameplayAbilityTargetDataHandle& TargetDataHandle);
};
