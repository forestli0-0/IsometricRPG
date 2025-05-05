// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricAbilities/GA_HeroMeleeAttackAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "AbilitySystemGlobals.h"
#include "IsometricComponents/ActionQueueComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h" 
#include "Character/IsometricRPGAttributeSetBase.h"
UGA_HeroMeleeAttackAbility::UGA_HeroMeleeAttackAbility()
{
    SkillType = EHeroSkillType::Targeted;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    // 初始化攻击蒙太奇路径
    static ConstructorHelpers::FObjectFinder<UAnimMontage> AttackMontageObj(TEXT("/Game/Characters/Animations/AM_Attack_Melee"));
    if (AttackMontageObj.Succeeded())
    {
        MontageToPlay = AttackMontageObj.Object;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load AttackMontage! Check path validity."));
        // 可考虑添加调试断言
        checkNoEntry();
    }
    // 设置触发条件为接收 GameplayEvent，监听 Tag 为 Ability.MeleeAttack
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.MeleeAttack"));

    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.MeleeAttack"));

    // 设置触发事件
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag("Ability.MeleeAttack");
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
    // 设定冷却阻断标签
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Cooldown.Ability.MeleeAttack"));

    static ConstructorHelpers::FClassFinder<UGameplayEffect> CooldownEffectObj(TEXT("/Game/Blueprint/GameEffects/GE_Cooldown_MeleeAttack"));
	if (CooldownEffectObj.Succeeded())
	{
        CooldownGameplayEffectClass = CooldownEffectObj.Class;
	}
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load CooldownEffect! Check path validity."));
        // 可考虑添加调试断言
        checkNoEntry();
    }
}

void UGA_HeroMeleeAttackAbility::ExecuteTargeted()
{
  // 打印调试信息
  UE_LOG(LogTemp, Warning, TEXT("使用普攻技能"));
  // 获取目标位置

  AActor* OwnerActor = GetAvatarActorFromActorInfo();
  if (!OwnerActor) return;

  // 定义 ActorInfo 和 ActivationInfo
  const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
  const FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();
  FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();

  // 应用冷却效果
  if (CooldownGameplayEffectClass)
  {
      FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownGameplayEffectClass, GetAbilityLevel());
      if (SpecHandle.Data.IsValid())
      {
          FGameplayEffectSpec& GESpec = *SpecHandle.Data.Get();

          // 获取你在 GE 中配置的 Data Tag
          FGameplayTag CooldownDurationTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Cooldown.Duration")); // !!! 确保这里的 Tag 字符串与 GE 中配置的一致 !!!
		  auto AttackSpeed = AttributeSet->GetAttackSpeed();
		  if (AttackSpeed <= 0.0f)
		  {
			  AttackSpeed = 1.0f; // 防止除以零
		  }
          CooldownDuration = 1.0 / AttackSpeed;
          // 设置 Set by Caller 的值
          GESpec.SetSetByCallerMagnitude(CooldownDurationTag, CooldownDuration);

          UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
          ASC->ApplyGameplayEffectSpecToSelf(GESpec);
      }
  }
}


