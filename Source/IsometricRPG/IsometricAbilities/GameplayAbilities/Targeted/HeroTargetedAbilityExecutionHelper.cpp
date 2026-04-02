#include "IsometricAbilities/GameplayAbilities/Targeted/HeroTargetedAbilityExecutionHelper.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Components/CapsuleComponent.h"
#include "IsometricAbilities/GameplayAbilities/HeroAbilityTargetDataHelper.h"
#include "IsometricAbilities/GameplayAbilities/Targeted/GA_TargetedAbility.h"

void FHeroTargetedAbilityExecutionHelper::CachePrimaryTargetData(
    const FGameplayAbilityTargetDataHandle& TargetDataHandle,
    TWeakObjectPtr<AActor>& OutTargetActor,
    FVector& OutTargetLocation)
{
    OutTargetActor = nullptr;
    OutTargetLocation = FVector::ZeroVector;

    if (const FGameplayAbilityTargetData* TargetData = FHeroAbilityTargetDataHelper::GetPrimaryTargetData(TargetDataHandle))
    {
        if (!FHeroAbilityTargetDataHelper::TryGetTargetLocation(*TargetData, OutTargetLocation))
        {
            OutTargetLocation = FVector::ZeroVector;
        }

        OutTargetActor = FHeroAbilityTargetDataHelper::GetPrimaryActor(*TargetData);
    }
}

bool FHeroTargetedAbilityExecutionHelper::TryResolveTargetOrCached(
    const FGameplayAbilityTargetDataHandle& TargetDataHandle,
    const TWeakObjectPtr<AActor>& CachedTargetActor,
    const FVector& CachedTargetLocation,
    AActor*& OutTargetActor,
    FVector& OutTargetLocation)
{
    OutTargetActor = nullptr;
    OutTargetLocation = FVector::ZeroVector;

    if (const FGameplayAbilityTargetData* TargetData = FHeroAbilityTargetDataHelper::GetPrimaryTargetData(TargetDataHandle))
    {
        if (!FHeroAbilityTargetDataHelper::TryGetTargetLocation(*TargetData, OutTargetLocation))
        {
            OutTargetLocation = FVector::ZeroVector;
        }

        OutTargetActor = FHeroAbilityTargetDataHelper::GetPrimaryActor(*TargetData);
    }

    if (!OutTargetActor && CachedTargetActor.IsValid())
    {
        OutTargetActor = CachedTargetActor.Get();
    }

    if (OutTargetLocation.IsNearlyZero() && !CachedTargetLocation.IsNearlyZero())
    {
        OutTargetLocation = CachedTargetLocation;
    }

    return OutTargetActor != nullptr || !OutTargetLocation.IsNearlyZero();
}

bool FHeroTargetedAbilityExecutionHelper::TryResolveCommitTarget(
    const FGameplayAbilityTargetDataHandle& TargetDataHandle,
    APawn*& OutTargetActor,
    FVector& OutTargetLocation)
{
    OutTargetActor = nullptr;
    OutTargetLocation = FVector::ZeroVector;

    if (!FHeroAbilityTargetDataHelper::TryGetTargetLocation(TargetDataHandle, OutTargetLocation))
    {
        return false;
    }

    OutTargetActor = Cast<APawn>(FHeroAbilityTargetDataHelper::GetPrimaryActor(TargetDataHandle));
    return true;
}

bool FHeroTargetedAbilityExecutionHelper::IsTargetWithinRange(
    const FGameplayAbilityActorInfo* ActorInfo,
    const FVector& TargetLocation,
    const float RangeToApply,
    float& OutDistance)
{
    OutDistance = 0.0f;

    const AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
    if (!AvatarActor)
    {
        return false;
    }

    OutDistance = FVector::Distance(TargetLocation, AvatarActor->GetActorLocation());
    return OutDistance <= RangeToApply;
}

float FHeroTargetedAbilityExecutionHelper::CalculateEffectiveAcceptanceRadius(
    const ACharacter* SelfCharacter,
    const AActor* TargetActor,
    const float SkillRange,
    const float Buffer)
{
    float SelfCapsuleRadius = 0.0f;
    if (SelfCharacter && SelfCharacter->GetCapsuleComponent())
    {
        SelfCapsuleRadius = SelfCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
    }

    float TargetCapsuleRadius = 0.0f;
    if (const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
    {
        if (TargetCharacter->GetCapsuleComponent())
        {
            TargetCapsuleRadius = TargetCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
        }
    }

    return SkillRange + SelfCapsuleRadius + TargetCapsuleRadius + Buffer;
}

bool FHeroTargetedAbilityExecutionHelper::TryRestoreTargetDataFromCache(
    FGameplayAbilityTargetDataHandle& InOutTargetDataHandle,
    const TWeakObjectPtr<AActor>& CachedTargetActor,
    const FVector& CachedTargetLocation)
{
    if (InOutTargetDataHandle.IsValid(0))
    {
        return true;
    }

    FGameplayAbilityTargetDataHandle RebuiltTargetData;
    if (CachedTargetActor.IsValid())
    {
        FGameplayAbilityTargetData_ActorArray* ActorArrayData = new FGameplayAbilityTargetData_ActorArray();
        ActorArrayData->TargetActorArray.Add(CachedTargetActor);
        RebuiltTargetData.Add(ActorArrayData);
    }
    else if (!CachedTargetLocation.IsNearlyZero())
    {
        FHitResult HitResult;
        HitResult.Location = CachedTargetLocation;
        FGameplayAbilityTargetData_SingleTargetHit* HitTargetData = new FGameplayAbilityTargetData_SingleTargetHit(HitResult);
        RebuiltTargetData.Add(HitTargetData);
    }

    if (!RebuiltTargetData.IsValid(0))
    {
        return false;
    }

    InOutTargetDataHandle = RebuiltTargetData;
    return true;
}

void FHeroTargetedAbilityExecutionHelper::StartMoveToTarget(
    UGA_TargetedAbility& Ability,
    APawn& TargetActor,
    const float AcceptanceRadius)
{
    const AActor* Avatar = Ability.GetAvatarActorFromActorInfo();
    const bool bIsServer = Avatar && Avatar->HasAuthority();
    const bool bIsLocallyControlled = Ability.GetCurrentActorInfo() && Ability.GetCurrentActorInfo()->IsLocallyControlled();
    auto* ThisAbility = static_cast<UGameplayAbility*>(&Ability);

    if (bIsServer)
    {
        if (UAbilityTask_WaitMoveToLocation* MoveTask = UAbilityTask_WaitMoveToLocation::WaitMoveToActor(ThisAbility, &TargetActor, AcceptanceRadius))
        {
            MoveTask->OnMoveFinished.AddDynamic(&Ability, &UGA_TargetedAbility::OnReachedTarget);
            MoveTask->OnMoveFailed.AddDynamic(&Ability, &UGA_TargetedAbility::OnFailedToTarget);
            MoveTask->ReadyForActivation();
            UE_LOG(LogTemp, Verbose, TEXT("%s: Server started WaitMoveToActor (Acceptance=%.1f)."), *Ability.GetName(), AcceptanceRadius);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Failed to create WaitMoveToActor on server."), *Ability.GetName());
        }
    }

    if (!bIsServer && bIsLocallyControlled)
    {
        if (UAbilityTask_WaitMoveToLocation* ClientMoveTask = UAbilityTask_WaitMoveToLocation::WaitMoveToActor(ThisAbility, &TargetActor, AcceptanceRadius))
        {
            ClientMoveTask->OnMoveFinished.AddDynamic(&Ability, &UGA_TargetedAbility::OnReachedTarget);
            ClientMoveTask->OnMoveFailed.AddDynamic(&Ability, &UGA_TargetedAbility::OnFailedToTarget);
            ClientMoveTask->ReadyForActivation();
            UE_LOG(LogTemp, Verbose, TEXT("%s: Client mirror WaitMoveToActor started for prediction (Acceptance=%.1f)."), *Ability.GetName(), AcceptanceRadius);
        }
    }
}
