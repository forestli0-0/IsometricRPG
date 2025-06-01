// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GA_HeroMeleeAttackAbility.h
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GameplayAbilities/Targeted/GA_TargetedAbility.h"
#include "GA_HeroMeleeAttackAbility.generated.h"

class UIsometricRPGAttributeSetBase;
/**
 * 
 */
UCLASS()
class ISOMETRICRPG_API UGA_HeroMeleeAttackAbility : public UGA_TargetedAbility
{
	GENERATED_BODY()
public:
	UGA_HeroMeleeAttackAbility();
	bool hello() const{ return true; }
protected:
	// 重写目标型攻击
	virtual void ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual bool CheckCost(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

};
	
