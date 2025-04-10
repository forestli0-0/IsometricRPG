// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricAbilities/RPGGameplayAbility_Attack.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "Character/IsometricRPGCharacter.h"
URPGGameplayAbility_Attack::URPGGameplayAbility_Attack()
{
	// 设定为立即生效的技能
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 用蓝图路径初始化
	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectObj(TEXT("/Game/Blueprint/GameEffects/GE_AttackDamage"));
	if (DamageEffectObj.Succeeded())
	{
		DamageEffect = DamageEffectObj.Class;

	}
}

void URPGGameplayAbility_Attack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Blue, FString::Printf(TEXT("激活攻击技能")));
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) return;

    if (DamageEffect)
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());

		// 获取目标Actor的AbilitySystemComponent
        UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(TriggerEventData->Target.Get()));
		if (TargetASC)
		{
			auto c = TargetASC->GetOwner();
			GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Blue, FString::Printf(TEXT("目标是%s"), *c->GetName()));
			// 获取目标Actor的AttributeSet
			AIsometricRPGCharacter* TargetCharacter = Cast<AIsometricRPGCharacter>(c);
            UIsometricRPGAttributeSetBase* TargetAttributeSet = const_cast<UIsometricRPGAttributeSetBase*>(Cast<const UIsometricRPGAttributeSetBase>(TargetCharacter->GetAbilitySystemComponent()->GetAttributeSet(UIsometricRPGAttributeSetBase::StaticClass())));
			// 获取目标Actor的Health属性
            GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Blue, FString::Printf(TEXT("Health: %f"), TargetAttributeSet->GetHealth()));
			GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
   
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
