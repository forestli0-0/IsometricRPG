#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"

class AActor;

class ISOMETRICRPG_API FHeroAbilityTargetDataHelper
{
public:
    static const FGameplayAbilityTargetData* GetPrimaryTargetData(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

    static AActor* GetPrimaryActor(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

    static AActor* GetPrimaryActor(const FGameplayAbilityTargetData& TargetData);

    static bool TryGetTargetLocation(const FGameplayAbilityTargetDataHandle& TargetDataHandle, FVector& OutLocation);

    static bool TryGetTargetLocation(const FGameplayAbilityTargetData& TargetData, FVector& OutLocation);
};
