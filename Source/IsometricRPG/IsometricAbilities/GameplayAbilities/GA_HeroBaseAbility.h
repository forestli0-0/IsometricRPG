#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "GameplayTagContainer.h"
#include "GA_HeroBaseAbility.generated.h"

// 技能类型枚举，用于区分不同种类的技能
// 基于英雄联盟的技能分类系统
UENUM(BlueprintType)
enum class EHeroAbilityType : uint8
{
    SelfCast     UMETA(DisplayName = "自我施放"),   // 作用于自身，如加速、治疗
    Targeted     UMETA(DisplayName = "目标指向"),   // 选择具体敌人，如单体攻击
    SkillShot    UMETA(DisplayName = "方向技能"),   // 朝方向发射，如火球术
    AreaEffect   UMETA(DisplayName = "区域效果"),   // 选择地面位置，如AOE技能
    Passive      UMETA(DisplayName = "被动技能")    // 被动效果
};

/**
 * 所有英雄技能的基类。用于抽象。
 */
UCLASS(Abstract) // 标记为抽象类
class ISOMETRICRPG_API UGA_HeroBaseAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_HeroBaseAbility();


    // 技能使用的目标Actor类，可以在蓝图中指定
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Targeting")
    TSubclassOf<AGameplayAbilityTargetActor> TargetActorClass;

    // 目标选择任务
    UPROPERTY()
    class UAbilityTask_WaitTargetData* TargetDataTask;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    FGameplayTag TriggerTag;
    // 冷却时间
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability|Cooldown")
    float CooldownDuration = 1.f;
protected:
    // 技能类型
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Type")
    EHeroAbilityType AbilityType;

    // 技能动画
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability|Animation")
    UAnimMontage* MontageToPlay;
    
    // 是否使用基类的“播放蒙太奇并在结束时收尾”的通用流程
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability|Animation")
    bool bUseBaseMontageFlow = true;

    // 基类蒙太奇完成/BlendOut后，是否自动 EndAbility
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability|Animation")
    bool bEndAbilityWhenBaseMontageEnds = true;

    // 基类蒙太奇被中断/取消时，是否自动 CancelAbility
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability|Animation")
    bool bCancelAbilityWhenBaseMontageInterrupted = true;
    
    // 施法范围
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability|Attribute")
    float RangeToApply = 100.f;
	
    // 是否面向目标
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability|Attribute")
    bool bFaceTarget = true;

    // 是否需要进行目标选择 - 子类可重写该属性
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Targeting")
    bool bRequiresTargetData = true;

    // 技能都应该在激活前获取自身的属性集
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Ability|Attribute") 
    UIsometricRPGAttributeSetBase* AttributeSet;
    
    // 是否在技能结束后自动应用冷却
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability|Cooldown")
    bool bAutoApplyCooldown = true;

	// 消耗资源
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability|Cost")
	float CostMagnitude = 1.f;

    //=========================================
    // 关键的覆盖方法和新增虚函数
    //=========================================

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
    
    // 技能结束
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;
        
    virtual void CancelAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateCancelAbility) override;
        
        
    // 子类应该实现的技能执行逻辑    
    virtual void ExecuteSkill(
        const FGameplayAbilitySpecHandle Handle, 
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo);
    
    // 子类可以覆盖的方法：该技能是否需要目标数据
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ability")
    bool RequiresTargetData() const;
    virtual bool RequiresTargetData_Implementation() const;
    
    // 冷却处理 - 提供统一的冷却管理
    virtual void ApplyCooldown(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo) const override;

    // 消耗检查 - 提供统一的消耗检查
    virtual bool CheckCost(
        const FGameplayAbilitySpecHandle Handle, 
        const FGameplayAbilityActorInfo* ActorInfo, 
        OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
        
    // 应用消耗 - 提供统一的消耗应用
    virtual void ApplyCost(
        const FGameplayAbilitySpecHandle Handle, 
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo) const override;

    //=========================================
    // 辅助方法和任务回调
    //=========================================
    
      // 开始进行目标选择流程
    virtual void StartTargetSelection(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo);
      // 直接执行技能（跳过目标选择）
    virtual void DirectExecuteAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo);

    virtual bool OtherCheckBeforeCommit();
    // 播放技能动画
    virtual void PlayAbilityMontage(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo);
public:
    // WaitTargetData 任务的委托回调
    UFUNCTION()
    virtual void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data);
    UFUNCTION()
    virtual void OnTargetingCancelled(const FGameplayAbilityTargetDataHandle& Data);
    
    // 动画相关回调
    UFUNCTION()
    virtual void HandleMontageCompleted();
    
    UFUNCTION()
    virtual void HandleMontageInterruptedOrCancelled();

protected:
    // 动画播放任务引用
    UPROPERTY()
    class UAbilityTask_PlayMontageAndWait* MontageTask;
    void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    FGameplayAbilityActivationInfo CurrentActivationInfo;

    FGameplayAbilityTargetDataHandle CurrentTargetDataHandle;
protected:
        virtual void PostInitProperties() override;


};
