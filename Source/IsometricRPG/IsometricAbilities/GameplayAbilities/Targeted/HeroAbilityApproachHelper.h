#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "IsometricAbilities/AbilityTasks/AbilityTask_WaitMoveToLocation.h"

class APawn;
class AActor;
class ACharacter;
class UGA_TargetedAbility;
struct FGameplayAbilityActorInfo;

class ISOMETRICRPG_API FHeroAbilityApproachHelper
{
public:
    static void CachePrimaryTargetData(
        const FGameplayAbilityTargetDataHandle& TargetDataHandle,
        TWeakObjectPtr<AActor>& OutTargetActor,
        FVector& OutTargetLocation);

    static bool TryResolveTargetOrCached(
        const FGameplayAbilityTargetDataHandle& TargetDataHandle,
        const TWeakObjectPtr<AActor>& CachedTargetActor,
        const FVector& CachedTargetLocation,
        AActor*& OutTargetActor,
        FVector& OutTargetLocation);

    static bool TryResolveCommitTarget(
        const FGameplayAbilityTargetDataHandle& TargetDataHandle,
        APawn*& OutTargetActor,
        FVector& OutTargetLocation);

    static bool IsTargetWithinRange(
        const FGameplayAbilityActorInfo* ActorInfo,
        const FVector& TargetLocation,
        float RangeToApply,
        float& OutDistance);

    static float CalculateEffectiveAcceptanceRadius(
        const ACharacter* SelfCharacter,
        const AActor* TargetActor,
        float SkillRange,
        float Buffer = 10.0f);

    static bool TryRestoreTargetDataFromCache(
        FGameplayAbilityTargetDataHandle& InOutTargetDataHandle,
        const TWeakObjectPtr<AActor>& CachedTargetActor,
        const FVector& CachedTargetLocation);

    static void StartMoveToTarget(UGA_TargetedAbility& Ability, APawn& TargetActor, float AcceptanceRadius);

    template <typename TAbility>
    static void StartMoveToActorOrLocation(
        TAbility& Ability,
        AActor* TargetActor,
        const FVector& TargetLocation,
        float AcceptanceRadius)
    {
        const AActor* Avatar = Ability.GetAvatarActorFromActorInfo();
        const bool bIsServer = Avatar && Avatar->HasAuthority();
        const bool bIsLocallyControlled = Ability.GetCurrentActorInfo() && Ability.GetCurrentActorInfo()->IsLocallyControlled();
        auto* ThisAbility = static_cast<UGameplayAbility*>(&Ability);

        auto CreateMoveTask = [ThisAbility, TargetActor, TargetLocation, AcceptanceRadius]()
        {
            return TargetActor
                ? UAbilityTask_WaitMoveToLocation::WaitMoveToActor(ThisAbility, TargetActor, AcceptanceRadius)
                : UAbilityTask_WaitMoveToLocation::WaitMoveToLocation(ThisAbility, TargetLocation, AcceptanceRadius);
        };

        if (bIsServer)
        {
            if (UAbilityTask_WaitMoveToLocation* MoveTask = CreateMoveTask())
            {
                MoveTask->OnMoveFinished.AddDynamic(&Ability, &TAbility::OnReachedTarget);
                MoveTask->OnMoveFailed.AddDynamic(&Ability, &TAbility::OnFailedToTarget);
                MoveTask->ReadyForActivation();
            }
        }

        if (!bIsServer && bIsLocallyControlled)
        {
            if (UAbilityTask_WaitMoveToLocation* ClientMoveTask = CreateMoveTask())
            {
                ClientMoveTask->OnMoveFinished.AddDynamic(&Ability, &TAbility::OnReachedTarget);
                ClientMoveTask->OnMoveFailed.AddDynamic(&Ability, &TAbility::OnFailedToTarget);
                ClientMoveTask->ReadyForActivation();
            }
        }
    }
};
