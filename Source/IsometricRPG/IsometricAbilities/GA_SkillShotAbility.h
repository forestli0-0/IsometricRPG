// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_HeroBaseAbility.h"
#include "GA_SkillShotAbility.generated.h"

/**
 * Ability that is aimed and fired in a direction (skill shot).
 */
UCLASS()
class ISOMETRICRPG_API UGA_SkillShotAbility : public UGA_HeroBaseAbility
{
	GENERATED_BODY()

public:
	UGA_SkillShotAbility();

protected:
	// Optional: Projectile class if it's a projectile-based skill shot
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillShot")
	// TSubclassOf<class AYourBaseProjectile> SkillShotProjectileClass;

	virtual bool CanActivateSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData, FGameplayTag& OutFailureTag) override;
	virtual void ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
