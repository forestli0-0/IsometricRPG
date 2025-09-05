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
    // 从 TriggerEventData 或 Avatar 角色的暂存 TargetData 中解析攻击目标与施法者
    AttackTarget = nullptr;
    Attacker = nullptr;

    // 优先使用 TriggerEventData（若是 GameplayEvent 激活）
    if (TriggerEventData)
    {
        if (TriggerEventData->Target)
        {
            AttackTarget = const_cast<AActor*>(TriggerEventData->Target.Get());
        }
        if (TriggerEventData->Instigator)
        {
            Attacker = const_cast<AActor*>(TriggerEventData->Instigator.Get());
        }
    }

    // 若未通过事件获得，则尝试从角色的 StoredTargetData 获取
    if (!AttackTarget || !Attacker)
    {
        AIsometricRPGCharacter* RPGChar = Cast<AIsometricRPGCharacter>(GetAvatarActorFromActorInfo());
        if (RPGChar)
        {
            if (!Attacker)
            {
                Attacker = RPGChar; // 默认施法者为自身
            }
            const FGameplayAbilityTargetDataHandle DataHandle = RPGChar->GetAbilityTargetData();
            if (DataHandle.Num() > 0 && DataHandle.Data[0].IsValid())
            {
                const TSharedPtr<FGameplayAbilityTargetData> DataPtr = DataHandle.Data[0];
                if (DataPtr->HasHitResult() && DataPtr->GetHitResult())
                {
                    AttackTarget = DataPtr->GetHitResult()->GetActor();
                }
                if (!AttackTarget && DataPtr->GetActors().Num() > 0 && DataPtr->GetActors()[0].IsValid())
                {
                    AttackTarget = DataPtr->GetActors()[0].Get();
                }
            }
        }
    }

    // 播放攻击动画
    if (AttackMontage)
    {
        // 注意：CreatePlayMontageAndWaitProxy 的第5个参数是 StartSectionName（片段名），不是 Slot 名。
        // 传入不存在的 Section 名可能导致立刻完成/中断，从而表现为“抽动一下就结束”。这里改为不指定片段。
        UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this,
            NAME_None,
            AttackMontage,
            1.0f, // 播放速率
            NAME_None, // 不指定起始片段
            true // 当能力结束时停止蒙太奇
        );

        if (Task)
        {
            // 绑定完成和中断事件
            Task->OnCompleted.AddDynamic(this, &URPGGameplayAbility_Attack::OnMontageCompleted);
            Task->OnBlendOut.AddDynamic(this, &URPGGameplayAbility_Attack::OnMontageCompleted);
            Task->OnInterrupted.AddDynamic(this, &URPGGameplayAbility_Attack::OnMontageInterrupted);
            Task->OnCancelled.AddDynamic(this, &URPGGameplayAbility_Attack::OnMontageCancelled);

            // 激活任务
            Task->ReadyForActivation();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Failed to create montage task for AttackMontage=%s"), *GetName(), *AttackMontage->GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: AttackMontage is null; ending ability immediately."), *GetName());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
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
