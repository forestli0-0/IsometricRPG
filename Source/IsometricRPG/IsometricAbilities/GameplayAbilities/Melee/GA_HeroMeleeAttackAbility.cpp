// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GA_HeroMeleeAttackAbility.cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_HeroMeleeAttackAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h" 
#include "Character/IsometricRPGAttributeSetBase.h"
#include "IsometricAbilities/TargetTrace/GATA_CursorTrace.h"
#include "FX/NA_NiagaraActorBase.h"
#include "IsometricAbilities/GameplayAbilities/HeroAbilityCommitHelper.h"
#include "IsometricAbilities/GameplayAbilities/Targeted/HeroAbilityApproachHelper.h"
UGA_HeroMeleeAttackAbility::UGA_HeroMeleeAttackAbility()
{
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    // 近战 A 键：保留交互式选目标流程（显示范围圈与选取目标）。
    SetUsesInteractiveTargeting(true);
    // 默认目标选择器（若蓝图未配置时的兜底），使用鼠标光标追踪
    TargetActorClass = AGATA_CursorTrace::StaticClass();
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
    ConfigureAbilityIdentityTag(FGameplayTag::RequestGameplayTag("Ability.Player.BasicAttack"));
    AddBlockedTagIfMissing(FGameplayTag::RequestGameplayTag("Cooldown.Ability.MeleeAttack"));

    // 范围圈 Niagara 指示器（可在蓝图覆盖），提供一个合理默认
    RangeIndicatorNiagaraActorClass = ANA_NiagaraActorBase::StaticClass();

}

void UGA_HeroMeleeAttackAbility::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
    // 打印调试信息
    UE_LOG(LogTemp, Warning, TEXT("使用A键普攻"));

    AActor* OwnerActor = GetAvatarActorFromActorInfo();

    if (!OwnerActor) return;

    // 通过TriggerEventData获取目标，再次确认朝向正确
    if (CurrentTargetDataHandle.Num() > 0)
    {
        const FGameplayAbilityTargetDataHandle& TargetDataHandle = CurrentTargetDataHandle;
        const FGameplayAbilityTargetData* TargetData = TargetDataHandle.Get(0); // 获取第一个目标数据

        if (TargetData)
        {

            const FHitResult* HitResult = TargetData->GetHitResult();
            if (HitResult)
            {
                const TWeakObjectPtr<UObject> TargetObject = HitResult->GetActor();
                if (TargetObject.IsValid())
                {
                    AActor* TargetActor = Cast<AActor>(TargetObject.Get());
                    if (TargetActor)
                    {
                        // 打印目标和自身位置，用于调试
                        UE_LOG(LogTemp, Verbose, TEXT("攻击目标位置: %s, 自身位置: %s"),
                            *TargetActor->GetActorLocation().ToString(),
                            *OwnerActor->GetActorLocation().ToString());
                    }
                }
            }
        }
    }

    // 注意：此处不需要自己结束技能，因为我们使用了蒙太奇，让蒙太奇完成时自动结束技能
    // 如果这是一个即时技能(没有蒙太奇)，则需要手动结束
    if (!MontageToPlay)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}

bool UGA_HeroMeleeAttackAbility::CheckCost(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}

bool UGA_HeroMeleeAttackAbility::OtherCheckBeforeCommit()
{
    ACharacter* SelfChar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!SelfChar)
    {
        return false;
    }

    const AActor* TargetActor = CachedTargetActor.Get();
    const float EffectiveAcceptance = FHeroAbilityApproachHelper::CalculateEffectiveAcceptanceRadius(SelfChar, TargetActor, RangeToApply);
    return CheckCachedTargetInRangeOrStartApproach(EffectiveAcceptance);
}

void UGA_HeroMeleeAttackAbility::ApplyCooldown(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo) const
{
    float AppliedCooldownDuration = 0.0f;
    FHeroAbilityCommitHelper::ApplyAttackSpeedCooldown(
        *this,
        ActorInfo,
        CooldownGameplayEffectClass,
        AttributeSet,
        GetAbilityLevel(Handle, ActorInfo),
        AppliedCooldownDuration);
}
