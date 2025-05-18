// Copyright Epic Games, Inc. All Rights Reserved.

#include "IsometricAbilities/GA_SkillShotAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h" // Required for FGameplayTag

UGA_SkillShotAbility::UGA_SkillShotAbility()
{
	// Constructor logic if needed
}

bool UGA_SkillShotAbility::CanActivateSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData, FGameplayTag& OutFailureTag)
{
	if (!Super::CanActivateSkill(Handle, ActorInfo, ActivationInfo, TriggerEventData, OutFailureTag))
	{
		return false;
	}

	// Skill shots might need a target point for aiming, similar to projectile abilities.
	// Range checks could be against this target point.
	AActor* SelfActor = ActorInfo->AvatarActor.Get();
	if (!SelfActor)
	{
		OutFailureTag = FGameplayTag::RequestGameplayTag(FName("Ability.Failure.InvalidSelf"));
		return false;
	}

	if (TriggerEventData && TriggerEventData->TargetData.Num() > 0)
    {
        const FGameplayAbilityTargetData* TargetData = TriggerEventData->TargetData.Data[0].Get();
        if (TargetData)
        {
			FVector TargetLocation = TargetData->GetHitResult() ? FVector(TargetData->GetHitResult()->Location) : FVector(TargetData->GetEndPoint());

            if (TargetLocation != FVector::ZeroVector)
            {
                float Distance = FVector::Dist(TargetLocation, SelfActor->GetActorLocation());
                if (Distance > RangeToApply) // Using RangeToApply as a general max range for aiming
                {
                    OutFailureTag = FGameplayTag::RequestGameplayTag(FName("Ability.Failure.OutOfRange"));
                    UE_LOG(LogTemp, Warning, TEXT("%s: Failed CanActivateSkill - Target aim point out of range (%.2f > %.2f)"), *GetName(), Distance, RangeToApply);
                    return false;
                }
            }
        }
    }

	return true;
}

void UGA_SkillShotAbility::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ExecuteSkill(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogTemp, Log, TEXT("%s: Executing skill shot ability."), *GetName());

	// Implement skill shot logic here.
	// This could involve spawning a projectile (if SkillShotProjectileClass is set),
	// performing a line trace, or other custom logic.

	// Example: If it spawns a projectile (similar to UGA_ProjectileAbility)
	// AActor* SelfActor = ActorInfo->AvatarActor.Get();
	// if (SelfActor && SkillShotProjectileClass) { ... spawn logic ... }

	// If this ability is instant (no montage) and should end immediately after execution.
	if (!MontageToPlay)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}
