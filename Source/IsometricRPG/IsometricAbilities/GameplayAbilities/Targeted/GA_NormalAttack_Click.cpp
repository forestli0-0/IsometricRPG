// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricAbilities/GameplayAbilities/Targeted/GA_NormalAttack_Click.h"
#include "GA_NormalAttack_Click.h"

UGA_NormalAttack_Click::UGA_NormalAttack_Click()
{
    bRequiresTargetData = false;
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
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Player.BasicAttack"));

    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Player.BasicAttack"));

    // 设置触发事件
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag("Ability.Player.BasicAttack");
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
    // 设定冷却阻断标签
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Cooldown.Ability.MeleeAttack"));

}
void UGA_NormalAttack_Click::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    // 打印调试信息
    UE_LOG(LogTemp, Warning, TEXT("使用普攻技能"));

    AActor* OwnerActor = GetAvatarActorFromActorInfo();

    if (!OwnerActor) return;

    // 通过TriggerEventData获取目标，再次确认朝向正确
    if (TriggerEventData && (TriggerEventData->TargetData.Num() > 0))
    {
        const FGameplayAbilityTargetDataHandle& TargetDataHandle = TriggerEventData->TargetData;
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
void UGA_NormalAttack_Click::ApplyCooldown(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo) const
{
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
            float ThisCooldownDuration = 1.0 / AttackSpeed; // 使用局部变量代替类成员变量
            // 设置 Set by Caller 的值
            GESpec.SetSetByCallerMagnitude(CooldownDurationTag, ThisCooldownDuration);

            UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
            ASC->ApplyGameplayEffectSpecToSelf(GESpec);
        }
    }
}