// Fill out your copyright notice in the Description page of Project Settings.


//#include "IsometricAbilities/GameplayAbilities/Targeted/GA_NormalAttack_Click.h"
#include "GA_NormalAttack_Click.h"
#include "IsometricAbilities/TargetTrace/GATA_CursorTrace.h"
#include "Character/IsometricRPGCharacter.h"
#include "GameFramework/Character.h"
#include "IsometricAbilities/GameplayAbilities/HeroAbilityCommitHelper.h"
#include "IsometricAbilities/GameplayAbilities/Targeted/HeroAbilityApproachHelper.h"

UGA_NormalAttack_Click::UGA_NormalAttack_Click()
{
    // 按标准 Targeted 流程处理：若已有目标就直接用，否则进入交互式确认。
    SetUsesInteractiveTargeting(true);
    TargetActorClass = AGATA_CursorTrace::StaticClass();
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
    ConfigureAbilityIdentityTag(FGameplayTag::RequestGameplayTag("Ability.Player.DirBasicAttack"));
    AddBlockedTagIfMissing(FGameplayTag::RequestGameplayTag("Cooldown.Ability.MeleeAttack"));

}
void UGA_NormalAttack_Click::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	// 普攻的命中绑定在动画里，用动画通知触发
    // 打印调试信息
    UE_LOG(LogTemp, Warning, TEXT("使用右键普攻"));

    AActor* OwnerActor = GetAvatarActorFromActorInfo();

    if (!OwnerActor) return;

    // 使用缓存的目标数据进行调试打印，避免重复解析
    if (CachedTargetActor.IsValid())
    {
        UE_LOG(LogTemp, Verbose, TEXT("攻击目标位置: %s, 自身位置: %s"),
            *CachedTargetActor->GetActorLocation().ToString(),
            *OwnerActor->GetActorLocation().ToString());
    }
    else if (!CachedTargetLocation.IsNearlyZero())
    {
        UE_LOG(LogTemp, Verbose, TEXT("攻击目标位置: %s, 自身位置: %s"),
            *CachedTargetLocation.ToString(),
            *OwnerActor->GetActorLocation().ToString());
    }

    if (!MontageToPlay)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}
void UGA_NormalAttack_Click::ApplyCooldown(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo) const
{
    float AppliedCooldownDuration = 0.0f;
    if (FHeroAbilityCommitHelper::ApplyAttackSpeedCooldown(
        *this,
        ActorInfo,
        CooldownGameplayEffectClass,
        AttributeSet,
        GetAbilityLevel(Handle, ActorInfo),
        AppliedCooldownDuration))
    {
        FHeroAbilityCommitHelper::NotifyCooldownTriggered(Handle, ActorInfo, AppliedCooldownDuration);
    }
}

void UGA_NormalAttack_Click::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
    CacheCurrentTargetDataForApproach(Data);

    if (AIsometricRPGCharacter* SourceCharacter = Cast<AIsometricRPGCharacter>(GetAvatarActorFromActorInfo()))
    {
        SourceCharacter->RecordRecentBasicAttackTarget(CachedTargetActor.Get(), GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f);
    }

    // 交回基类，继续原有流程（会调用 OtherCheckBeforeCommit → Commit/移动任务）
    Super::OnTargetDataReady(Data);
}

bool UGA_NormalAttack_Click::OtherCheckBeforeCommit()
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
