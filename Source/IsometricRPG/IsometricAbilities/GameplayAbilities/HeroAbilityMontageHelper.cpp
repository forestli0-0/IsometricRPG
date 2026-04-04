#include "IsometricAbilities/GameplayAbilities/HeroAbilityMontageHelper.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "IsometricAbilities/GameplayAbilities/GA_HeroBaseAbility.h"
#include "IsometricAbilities/GameplayAbilities/HeroAbilityTargetDataHelper.h"

namespace
{
bool TryResolveTargetDirectionFromTargetData(
    const FGameplayAbilityTargetDataHandle& TargetDataHandle,
    const FVector& CharacterLocation,
    FVector& OutDirection)
{
    FVector TargetLocation = FVector::ZeroVector;
    if (!FHeroAbilityTargetDataHelper::TryGetTargetLocation(TargetDataHandle, TargetLocation))
    {
        return false;
    }

    OutDirection = TargetLocation - CharacterLocation;
    return true;
}
}

float FHeroAbilityMontageHelper::ResolvePlayRate(const UAnimMontage* MontageToPlay, const float CooldownDuration)
{
    if (!MontageToPlay || CooldownDuration <= 0.f || MontageToPlay->GetPlayLength() <= 0.f)
    {
        return 1.0f;
    }

    const float DesiredRate = MontageToPlay->GetPlayLength() / CooldownDuration;
    return FMath::Clamp(DesiredRate, 1.f, 3.f);
}

bool FHeroAbilityMontageHelper::TryFaceCharacterToCurrentTarget(
    ACharacter& Character,
    const FVector& CurrentAimDirection,
    const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
    FVector DirectionToTargetHorizontal = CurrentAimDirection;
    bool bHasTarget = !DirectionToTargetHorizontal.IsNearlyZero();

    if (!bHasTarget)
    {
        bHasTarget = TryResolveTargetDirectionFromTargetData(
            TargetDataHandle,
            Character.GetActorLocation(),
            DirectionToTargetHorizontal);
    }

    if (!bHasTarget)
    {
        return false;
    }

    DirectionToTargetHorizontal.Z = 0.0f;
    if (DirectionToTargetHorizontal.IsNearlyZero())
    {
        return false;
    }

    Character.SetActorRotation(DirectionToTargetHorizontal.Rotation(), ETeleportType::None);
    return true;
}

UAbilityTask_PlayMontageAndWait* FHeroAbilityMontageHelper::PlayAbilityMontage(
    UGA_HeroBaseAbility& Ability,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityTargetDataHandle& TargetDataHandle,
    const FVector& CurrentAimDirection,
    UAnimMontage* MontageToPlay,
    const float CooldownDuration,
    const bool bFaceTarget)
{
    UAnimInstance* AnimInstance = Ability.GetCurrentActorInfo() ? Ability.GetCurrentActorInfo()->GetAnimInstance() : nullptr;
    if (!MontageToPlay || !IsValid(AnimInstance))
    {
        return nullptr;
    }

    const float PlayRate = ResolvePlayRate(MontageToPlay, CooldownDuration);
    if (bFaceTarget)
    {
        if (ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr)
        {
            if (!TryFaceCharacterToCurrentTarget(*Character, CurrentAimDirection, TargetDataHandle))
            {
                UE_LOG(LogTemp, Verbose, TEXT("%s: No target data to face when playing montage."), *Ability.GetName());
            }
        }
    }

    UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        &Ability,
        NAME_None,
        MontageToPlay,
        PlayRate);
    if (!MontageTask)
    {
        return nullptr;
    }

    UE_LOG(LogTemp, Verbose, TEXT("%s: PlayMontage %s at rate %.2f"), *Ability.GetName(), *MontageToPlay->GetName(), PlayRate);
    return MontageTask;
}

void FHeroAbilityMontageHelper::BindBaseMontageTaskCallbacks(
    UGA_HeroBaseAbility& Ability,
    UAbilityTask_PlayMontageAndWait& MontageTask,
    const bool bEndAbilityWhenBaseMontageEnds,
    const bool bCancelAbilityWhenBaseMontageInterrupted)
{
    if (bEndAbilityWhenBaseMontageEnds)
    {
        MontageTask.OnCompleted.AddDynamic(&Ability, &UGA_HeroBaseAbility::HandleMontageCompleted);
        MontageTask.OnBlendOut.AddDynamic(&Ability, &UGA_HeroBaseAbility::HandleMontageCompleted);
    }

    if (bCancelAbilityWhenBaseMontageInterrupted)
    {
        MontageTask.OnInterrupted.AddDynamic(&Ability, &UGA_HeroBaseAbility::HandleMontageInterruptedOrCancelled);
        MontageTask.OnCancelled.AddDynamic(&Ability, &UGA_HeroBaseAbility::HandleMontageInterruptedOrCancelled);
    }
}
