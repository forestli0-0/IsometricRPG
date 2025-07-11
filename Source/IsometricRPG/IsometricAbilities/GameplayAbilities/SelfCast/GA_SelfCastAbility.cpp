// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GA_SelfCastAbility.cpp
// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_SelfCastAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

UGA_SelfCastAbility::UGA_SelfCastAbility()
{
    // 设置技能类型为自我施放
    AbilityType = EHeroAbilityType::SelfCast;
    
    // 自我施放技能默认不需要目标选择
    bRequiresTargetData = false;
    
    // 自我施放技能默认不需要朝向目标
    bFaceTarget = false;
}

bool UGA_SelfCastAbility::RequiresTargetData_Implementation() const
{
    // 自我施放技能不需要目标数据
    return false;
}

void UGA_SelfCastAbility::ApplySelfEffect(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
    // 如果有设置自我增益效果，则应用它
    if (SelfGameplayEffect)
    {
        UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
        if (ASC)
        {
            FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
            ContextHandle.AddSourceObject(this);

            FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
                SelfGameplayEffect,
                GetAbilityLevel(),
                ContextHandle);

            if (SpecHandle.IsValid())
            {
                // 设置持续时间
                if (EffectDuration > 0.0f)
                {
                    SpecHandle.Data->SetDuration(EffectDuration, true);
                }

                // 应用效果到自身
                FActiveGameplayEffectHandle ActiveGEHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

                if (ActiveGEHandle.IsValid())
                {
                    UE_LOG(LogTemp, Log, TEXT("%s: Successfully applied self effect."), *GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("%s: Failed to apply self effect."), *GetName());
                }
            }
        }
    }
}



void UGA_SelfCastAbility::ExecuteSkill(
    const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, 
    const FGameplayEventData* TriggerEventData)
{
    AActor* SelfActor = GetAvatarActorFromActorInfo();
    if (!SelfActor)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Cannot execute self-cast skill - SelfActor is invalid."), *GetName());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // 取消技能
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("%s: Executing self-cast ability on %s."), *GetName(), *SelfActor->GetName());

    // 应用自我效果
    ApplySelfEffect(Handle, ActorInfo, ActivationInfo);

    // 如果没有动画蒙太奇，则立即结束技能
    // 如果有蒙太奇，则由蒙太奇完成回调结束技能
    if (!MontageToPlay)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}

