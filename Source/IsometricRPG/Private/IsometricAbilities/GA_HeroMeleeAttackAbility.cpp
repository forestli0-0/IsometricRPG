// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricAbilities/GA_HeroMeleeAttackAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "AbilitySystemGlobals.h"
#include "IsometricComponents/ActionQueueComponent.h"
UGA_HeroMeleeAttackAbility::UGA_HeroMeleeAttackAbility()
{
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.MeleeAttack"));
    SkillType = EHeroSkillType::Targeted;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_HeroMeleeAttackAbility::ExecuteTargeted()
{
    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    if (!AvatarActor) return;

    AActor* OwnerActor = nullptr;

    // 示例：你可以从组件获取目标引用，也可以走 Trace 或 Tag
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        OwnerActor = ASC->GetAvatarActor(); // 这里要换成你的获取逻辑
    }

    if (!OwnerActor || !OwnerActor->IsValidLowLevel()) return;

    const float Distance = FVector::Dist(AvatarActor->GetActorLocation(), OwnerActor->GetActorLocation());
    auto TargetAQC = OwnerActor->FindComponentByClass<UActionQueueComponent>();
	if (!TargetAQC) return;
	auto TargetActor = TargetAQC->GetCommand().TargetActor;
    if (Distance > AttackRange)
    {
        // 向目标移动

    }

    if (DamageEffect)
    {
        FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());
        GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(),
            UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor));
    }
}
