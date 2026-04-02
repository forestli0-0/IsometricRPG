#include "IsometricAbilities/GameplayAbilities/HeroAbilityTargetDataHelper.h"

#include "GameFramework/Actor.h"

const FGameplayAbilityTargetData* FHeroAbilityTargetDataHelper::GetPrimaryTargetData(
    const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
    return TargetDataHandle.Get(0);
}

AActor* FHeroAbilityTargetDataHelper::GetPrimaryActor(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
    const FGameplayAbilityTargetData* TargetData = GetPrimaryTargetData(TargetDataHandle);
    return TargetData ? GetPrimaryActor(*TargetData) : nullptr;
}

AActor* FHeroAbilityTargetDataHelper::GetPrimaryActor(const FGameplayAbilityTargetData& TargetData)
{
    if (TargetData.HasHitResult() && TargetData.GetHitResult())
    {
        if (AActor* HitActor = TargetData.GetHitResult()->GetActor())
        {
            return HitActor;
        }
    }

    const TArray<TWeakObjectPtr<AActor>> Actors = TargetData.GetActors();
    return (Actors.Num() > 0 && Actors[0].IsValid()) ? Actors[0].Get() : nullptr;
}

bool FHeroAbilityTargetDataHelper::TryGetTargetLocation(
    const FGameplayAbilityTargetDataHandle& TargetDataHandle,
    FVector& OutLocation)
{
    const FGameplayAbilityTargetData* TargetData = GetPrimaryTargetData(TargetDataHandle);
    return TargetData ? TryGetTargetLocation(*TargetData, OutLocation) : false;
}

bool FHeroAbilityTargetDataHelper::TryGetTargetLocation(
    const FGameplayAbilityTargetData& TargetData,
    FVector& OutLocation)
{
    if (TargetData.HasHitResult() && TargetData.GetHitResult())
    {
        OutLocation = TargetData.GetHitResult()->Location;
        return true;
    }

    if (AActor* TargetActor = GetPrimaryActor(TargetData))
    {
        OutLocation = TargetActor->GetActorLocation();
        return true;
    }

    return false;
}
