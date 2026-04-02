#include "IsometricAbilities/GameplayAbilities/HeroAbilityMontageHelper.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
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
