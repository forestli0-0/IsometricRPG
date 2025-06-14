// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GA_SkillShotAbility.h
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GameplayAbilities/GA_HeroBaseAbility.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h"
#include "GA_SkillShotAbility.generated.h"

/**
 * Ability that is aimed and fired in a direction (skill shot).
 * This type of ability targets a DIRECTION rather than a specific actor.
 * Examples: Linear projectiles, beam attacks, cone attacks
 */
UCLASS()
class ISOMETRICRPG_API UGA_SkillShotAbility : public UGA_HeroBaseAbility
{
	GENERATED_BODY()

public:
	UGA_SkillShotAbility();

protected:
	// Width/radius of the skill shot effect
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillShot")
	float SkillShotWidth = 100.0f;

	// Optional: Projectile class if it's a projectile-based skill shot
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillShot")
	TSubclassOf<class AProjectileBase> SkillShotProjectileClass;

	// Visual indicator for aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillShot|Visual")
	TSubclassOf<class ANiagaraActor> AimIndicatorClass;

	UPROPERTY()
	TObjectPtr<class ANiagaraActor> ActiveAimIndicator;

	virtual void ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/* 关于这个SkillShot类，对应的应该是Targeted,也就是说，要么是指向目标，要么是指向方向。
		那么这里的目标选择应该是方向，但是，如果是按基类的流程，又要左键确认目标，暂时没想好快捷施法，
		因此这里这个先留着，设置为不需要目标，这样走的就是直接执行的流程*/
	virtual void StartTargetSelection(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// Get the direction from cursor/input
	virtual FVector GetSkillShotDirection(const FGameplayEventData* TriggerEventData) const;

	// Execute the skill shot in the given direction
	virtual void ExecuteSkillShot(const FVector& Direction, const FVector& StartLocation);
};
