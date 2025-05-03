// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricAbilities/GA_HeroBaseAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h" 
UGA_HeroBaseAbility::UGA_HeroBaseAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_HeroBaseAbility::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (bFaceTarget && ActorInfo->AvatarActor.IsValid())
    {
        // 可以在此执行面向目标逻辑（略）
    }

    if (MontageToPlay && ActorInfo->GetAnimInstance())
    {
        MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, MontageToPlay);

        MontageTask->OnCompleted.AddDynamic(this, &UGA_HeroBaseAbility::K2_EndAbility);
        MontageTask->OnInterrupted.AddDynamic(this, &UGA_HeroBaseAbility::K2_EndAbility);
        MontageTask->OnCancelled.AddDynamic(this, &UGA_HeroBaseAbility::K2_EndAbility);
        MontageTask->ReadyForActivation();
    }
    else
    {
        OnAbilityTriggered();
        EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
    }
}

void UGA_HeroBaseAbility::OnAbilityTriggered_Implementation()
{
    switch (SkillType)
    {
    case EHeroSkillType::Targeted:
        ExecuteTargeted();
        break;
    case EHeroSkillType::Projectile:
        ExecuteProjectile();
        break;
    case EHeroSkillType::Area:
        ExecuteArea();
        break;
    case EHeroSkillType::SkillShot:
        ExecuteSkillShot();
        break;
    case EHeroSkillType::SelfCast:
        ExecuteSelfCast();
        break;
    }
}

void UGA_HeroBaseAbility::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (MontageTask)
    {
        MontageTask->EndTask();
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, true, bWasCancelled);
}

// 各种技能类型的默认实现（可重写）
void UGA_HeroBaseAbility::ExecuteTargeted() {}
void UGA_HeroBaseAbility::ExecuteProjectile() {}
void UGA_HeroBaseAbility::ExecuteArea() {}
void UGA_HeroBaseAbility::ExecuteSkillShot() {}
void UGA_HeroBaseAbility::ExecuteSelfCast() {}