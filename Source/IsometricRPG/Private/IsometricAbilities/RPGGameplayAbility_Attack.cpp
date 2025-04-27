// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricAbilities/RPGGameplayAbility_Attack.h"
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
    GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Blue, FString::Printf(TEXT("普通攻击")));
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) return;

    // 播放攻击动画
    if (AttackMontage)
    {
        UE_LOG(LogTemp, Warning, TEXT("准备播放蒙太奇: %s"), *GetNameSafe(AttackMontage));

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

            GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Green, FString::Printf(TEXT("播放攻击动画")));
        }
    }
}


// 在 OnMontageCompleted 回调中添加
void URPGGameplayAbility_Attack::OnMontageCompleted()
{
    GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Green, FString::Printf(TEXT("攻击动画完成")));

    if (DamageEffect)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Yellow, FString::Printf(TEXT("准备应用伤害效果: %s"), *GetNameSafe(DamageEffect)));

        // 创建 GameplayEffectSpec
        FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());

        if (SpecHandle.IsValid())
        {
            GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Yellow, FString::Printf(TEXT("伤害效果Spec创建成功，等级: %d"), GetAbilityLevel()));
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, FString::Printf(TEXT("伤害效果Spec创建失败")));
        }

        // 获取目标的 AbilitySystemComponent
        if (CurrentActorInfo)
        {
            UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(CurrentActorInfo->AvatarActor.Get()));
            if (TargetASC)
            {
                // 应用伤害效果
                FActiveGameplayEffectHandle AppliedHandle = GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

                if (AppliedHandle.WasSuccessfullyApplied())
                {
                    GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Green, FString::Printf(TEXT("伤害效果应用成功!")));
                }
                else
                {
                    GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, FString::Printf(TEXT("伤害效果应用失败")));
                }
            }
            else
            {
                GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, FString::Printf(TEXT("无法获取目标的AbilitySystemComponent")));
            }
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, FString::Printf(TEXT("CurrentActorInfo为空，无法应用伤害效果")));
        }
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, FString::Printf(TEXT("DamageEffect未设置，无法应用伤害")));
    }

    // 结束能力
    if (CurrentSpecHandle.IsValid() && CurrentActorInfo)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Green, FString::Printf(TEXT("攻击技能完成，结束能力")));
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, FString::Printf(TEXT("无法正常结束能力: SpecHandle有效: %d, ActorInfo有效: %d"),
            CurrentSpecHandle.IsValid(), CurrentActorInfo != nullptr));
    }
}


void URPGGameplayAbility_Attack::OnMontageInterrupted()
{
    GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, FString::Printf(TEXT("攻击动画被中断")));

    // 在动画被中断时结束能力
    if (CurrentSpecHandle.IsValid() && CurrentActorInfo)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

void URPGGameplayAbility_Attack::OnMontageCancelled()
{
    GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, FString::Printf(TEXT("攻击动画被取消")));

    // 在动画被取消时结束能力
    if (CurrentSpecHandle.IsValid() && CurrentActorInfo)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

