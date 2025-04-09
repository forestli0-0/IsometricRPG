// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricAbilities/RPGGameplayAbility_Attack.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
URPGGameplayAbility_Attack::URPGGameplayAbility_Attack()
{
	// 设定为立即生效的技能
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
}

void URPGGameplayAbility_Attack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) return;

	// 应用一个伤害 GE
    // Replace the problematic line with the following code to fix the error:
    if (TriggerEventData && TriggerEventData->Target.Get() && DamageEffect)
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());

		// 获取目标Actor的AbilitySystemComponent
        // Modify the line causing the error by casting the const AActor* to AActor*  
        UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(TriggerEventData->Target.Get()));
		if (TargetASC)
		{
			GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
