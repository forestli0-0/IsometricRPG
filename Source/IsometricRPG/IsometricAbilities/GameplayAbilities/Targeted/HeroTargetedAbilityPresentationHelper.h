#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"

class AActor;
class ANiagaraActor;
class UGA_TargetedAbility;

class ISOMETRICRPG_API FHeroTargetedAbilityPresentationHelper
{
public:
    static void SpawnRangeIndicator(UGA_TargetedAbility& Ability, float RangeToApply);

    static void DestroyRangeIndicator(TObjectPtr<ANiagaraActor>& ActiveRangeIndicatorNiagaraActor);

    static void SyncCurrentAbilityTargets(AActor* AvatarActor, const FGameplayAbilityTargetDataHandle& Data);

    static void ClearCurrentAbilityTargets(AActor* AvatarActor);
};
