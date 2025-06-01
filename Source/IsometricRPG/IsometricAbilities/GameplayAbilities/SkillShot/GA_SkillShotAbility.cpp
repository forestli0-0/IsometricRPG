// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GA_SkillShotAbility.cpp
// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_SkillShotAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h" // Required for FGameplayTag

UGA_SkillShotAbility::UGA_SkillShotAbility()
{
	// Constructor logic if needed
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
