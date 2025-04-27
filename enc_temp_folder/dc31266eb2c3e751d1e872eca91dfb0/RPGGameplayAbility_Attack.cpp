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
		// 播放的主体是 TriggerEventData->Target

		const ACharacter* CharacterToPlay = Cast<ACharacter>(TriggerEventData->Instigator);
        if (CharacterToPlay) {
			USkeletalMeshComponent* Mesh = CharacterToPlay->GetMesh();
			if (Mesh)
			{
				UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
                // 创建播放蒙太奇的任务
                FName AniName = FName(*AnimInstance->GetName());
                FName MontageStartSection = FName("DefaultSlot");

                UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
                    this,
                    AniName,
                    AttackMontage,
                    1.0f, // 播放速率
                    MontageStartSection, // 使用默认的蒙太奇片段名称
                    false // 不中断当前播放的蒙太奇
                );

                if (Task)
                {
                    // 绑定完成和中断事件（可选）
                    Task->OnCompleted.AddDynamic(this, &URPGGameplayAbility_Attack::OnMontageCompleted);
                    Task->OnInterrupted.AddDynamic(this, &URPGGameplayAbility_Attack::OnMontageInterrupted);
                    Task->OnCancelled.AddDynamic(this, &URPGGameplayAbility_Attack::OnMontageCancelled);

                    // 激活任务
                    Task->ReadyForActivation();

                    GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Green, FString::Printf(TEXT("播放攻击动画")));
                }
            }
    }
    }

    if (DamageEffect)
    {
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());

		// 获取目标Actor的AbilitySystemComponent
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(TriggerEventData->Target.Get()));
		if (TargetASC)
		{
			// ---------打印调试信息----------
			auto c = TargetASC->GetOwner();
			GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Blue, FString::Printf(TEXT("目标是%s"), *c->GetName()));
			// 获取目标Actor的AttributeSet
			AIsometricRPGCharacter* TargetCharacter = Cast<AIsometricRPGCharacter>(c);
			UIsometricRPGAttributeSetBase* TargetAttributeSet = const_cast<UIsometricRPGAttributeSetBase*>(Cast<const UIsometricRPGAttributeSetBase>(TargetCharacter->GetAbilitySystemComponent()->GetAttributeSet(UIsometricRPGAttributeSetBase::StaticClass())));
			// 获取目标Actor的Health属性
			GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Blue, FString::Printf(TEXT("攻击前目标血量: %f"), TargetAttributeSet->GetHealth()));
			// ----------结束调试-------------

			GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
    }

    // 注意：如果您想等待动画完成，不要在这里立即结束能力
    // 而是在动画完成回调中结束能力
    // EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// 在 OnMontageCompleted 回调中添加
void URPGGameplayAbility_Attack::OnMontageCompleted()
{
    GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Green, FString::Printf(TEXT("攻击动画完成")));

    // 获取角色和动画实例状态
    if (CurrentActorInfo)
    {
        ACharacter* Character = Cast<ACharacter>(CurrentActorInfo->AvatarActor.Get());
        if (Character)
        {
            USkeletalMeshComponent* Mesh = Character->GetMesh();
            if (Mesh)
            {
                UAnimInstance* AnimInst = Mesh->GetAnimInstance();
                if (AnimInst)
                {
                    bool bIsPlaying = AnimInst->IsAnyMontagePlaying();
                    GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Cyan,
                        FString::Printf(TEXT("角色: %s, 是否有蒙太奇正在播放: %s"),
                            *Character->GetName(), bIsPlaying ? TEXT("是") : TEXT("否")));
                }
                else
                {
                    GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, TEXT("AnimInstance 无效"));
                }
            }
            else
            {
                GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, TEXT("Mesh 无效"));
            }
        }
    }

    if (CurrentSpecHandle.IsValid() && CurrentActorInfo)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
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

