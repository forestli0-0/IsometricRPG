// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\RPGGameplayAbility_Attack.cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "RPGGameplayAbility_Attack.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "GameFramework/Character.h" // Add this include to define PlayAnimMontage
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h" 
#include "Character/IsometricRPGCharacter.h"
#include "IsometricComponents/ActionQueueComponent.h"
URPGGameplayAbility_Attack::URPGGameplayAbility_Attack()
{
	// 设定为立即生效的技能
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 用蓝图路径初始化GE
	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectObj(TEXT("/Game/Blueprint/GameEffects/GE_AttackDamage"));
	if (DamageEffectObj.Succeeded())
	{
		DamageEffect = DamageEffectObj.Class;

	}
	// 初始化攻击蒙太奇路径
	static ConstructorHelpers::FObjectFinder<UAnimMontage> AttackMontageObj(TEXT("/Game/Characters/Animations/AM_Attack_Melee")); 
	if (AttackMontageObj.Succeeded())
	{
		AttackMontage = AttackMontageObj.Object;
	}
	// 设置触发条件为接收 GameplayEvent，监听 Tag 为 Ability.MeleeAttack
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.MeleeAttack"));

	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.MeleeAttack"));

	// 设置触发事件
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag("Ability.MeleeAttack");
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}


void URPGGameplayAbility_Attack::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) return;
    // 保存真正的攻击目标
    if (TriggerEventData && TriggerEventData->Target && TriggerEventData->Instigator)
    {
        // Modify the assignment to cast the const AActor* to AActor*  
        AttackTarget = const_cast<AActor*>(TriggerEventData->Target.Get());
        Attacker = const_cast<AActor*>(TriggerEventData->Instigator.Get());
    }

    // 播放攻击动画
    if (AttackMontage)
    {
        UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this,
            NAME_None,
            AttackMontage,
            1.0f, // 播放速率
            FName("DefaultSlot"), // 使用默认的蒙太奇片段名称
            false // 不中断当前播放的蒙太奇
        );

        if (Task)
        {
            // 绑定完成和中断事件
            Task->OnCompleted.AddDynamic(this, &URPGGameplayAbility_Attack::OnMontageCompleted);
            Task->OnInterrupted.AddDynamic(this, &URPGGameplayAbility_Attack::OnMontageInterrupted);
            Task->OnCancelled.AddDynamic(this, &URPGGameplayAbility_Attack::OnMontageCancelled);

            // 激活任务
            Task->ReadyForActivation();

        }
    }
}


// 在 OnMontageCompleted 回调中添加
void URPGGameplayAbility_Attack::OnMontageCompleted()
{
    // 结束能力
    if (CurrentSpecHandle.IsValid() && CurrentActorInfo)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }

}


void URPGGameplayAbility_Attack::OnMontageInterrupted()
{
    // 在动画被中断时结束能力
    if (CurrentSpecHandle.IsValid() && CurrentActorInfo)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

void URPGGameplayAbility_Attack::OnMontageCancelled()
{
    // 在动画被取消时结束能力
    if (CurrentSpecHandle.IsValid() && CurrentActorInfo)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}
