// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GA_HeroBaseAbility.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "GA_HeroBaseAbility.generated.h"

/**
 * 所有英雄技能的基类。用于抽象。
 */
UCLASS(Abstract) // 标记为抽象类
class ISOMETRICRPG_API UGA_HeroBaseAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_HeroBaseAbility();

protected:
    // 技能动画
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Animation")
    UAnimMontage* MontageToPlay;

    // 技能是否朝向目标
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Animation")
    bool bFaceTarget = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Attribute")
    float RangeToApply = 100.f;

    //技能都应该在激活前获取自身的属性集
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Ability|Attribute") // 标记为瞬态
        UIsometricRPGAttributeSetBase* AttributeSet;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooldown")
    float CooldownDuration = 1.f;

    // 技能激活时调用
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    // 技能结束
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
    /**
     * 在ActivateAbility期间调用，用于检查技能是否可以激活。
     * 子类应重写此方法以实现特定的激活检查（如目标校验、距离等）。
     * @param OutFailureTag 如果返回false，可以通过该标签指示失败原因。
     * @return 如果技能可以激活返回true，否则返回false。
     */
    virtual bool CanActivateSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData, FGameplayTag& OutFailureTag);

    /**
     * 在ActivateAbility期间调用，用于执行技能的核心逻辑。
     * 子类应重写此方法以实现具体效果。
     * 如果技能为瞬发且不依赖Montage来判断结束，则此函数应调用EndAbility。
     */
    virtual void ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData);

    // 动画播放任务引用
    UPROPERTY()
    class UAbilityTask_PlayMontageAndWait* MontageTask;
};
