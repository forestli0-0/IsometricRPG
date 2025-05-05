// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "GA_HeroBaseAbility.generated.h"

UENUM(BlueprintType)
enum class EHeroSkillType : uint8
{
    Targeted,     // 指向性技能
    Projectile,   // 弹道体技能
    Area,         // 范围伤害
    SkillShot,    // 技能射线/方向发射
    SelfCast      // 自身施放型
};
/**
 * 
 */
UCLASS()
class ISOMETRICRPG_API UGA_HeroBaseAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
    UGA_HeroBaseAbility();

protected:
    // 技能类型
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    EHeroSkillType SkillType = EHeroSkillType::Targeted;

    // 技能动画
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Animation")
    UAnimMontage* MontageToPlay;

    // 技能是否朝向目标
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Animation")
    bool bFaceTarget = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Attribute")\
    float RangeToApply = 100.f;

    //技能都应该在激活前获取自身的属性集
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Attribute")
	UIsometricRPGAttributeSetBase* AttributeSet;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooldown")
    float CooldownDuration = 1.f;

    // 技能激活时调用
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    // 技能命中回调（由蒙太奇通知或时机触发）
    UFUNCTION(BlueprintNativeEvent, Category = "Ability")
    void OnAbilityTriggered();
    virtual void OnAbilityTriggered_Implementation();

    // 技能结束
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
    // 各种类型技能逻辑实现
    virtual void ExecuteTargeted();
    virtual void ExecuteProjectile();
    virtual void ExecuteArea();
    virtual void ExecuteSkillShot();
    virtual void ExecuteSelfCast();

    // 动画播放任务引用
    UPROPERTY()
    class UAbilityTask_PlayMontageAndWait* MontageTask;
};
