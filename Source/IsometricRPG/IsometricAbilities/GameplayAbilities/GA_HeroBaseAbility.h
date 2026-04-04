#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "GameplayTagContainer.h"
#include "IsometricAbilities/GameplayAbilities/HeroAbilityTargetingHelper.h"
#include "Input/IsometricInputTypes.h"
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
    friend class FHeroAbilityTargetingHelper;
    friend class FHeroAbilityFlowHelper;

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

    // 设计时可配置的技能显示名称，供 UI 使用
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    FText AbilityDisplayName;

    /** Returns the localized display name, falling back to the class name when unset. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability")
    FText GetAbilityDisplayNameText() const;

    /** Returns the configured resource cost for UI or external systems. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability|Cost")
    float GetResourceCost() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability|Type")
    EHeroAbilityType GetHeroAbilityType() const { return AbilityType; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability|Targeting")
    bool ExpectsActorTargetData() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability|Targeting")
    bool ExpectsLocationTargetData() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability|Targeting")
    bool ExpectsPreparedTargetData() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability|Targeting")
    bool UsesInteractiveTargeting() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ability|Input")
    FAbilityInputPolicy GetAbilityInputPolicy() const;
    virtual FAbilityInputPolicy GetAbilityInputPolicy_Implementation() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability|Input")
    float GetCurrentHeldDuration() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability|Input")
    EInputEventPhase GetCurrentInputTriggerPhase() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability|Input")
    FVector GetCurrentAimPoint() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability|Input")
    FVector GetCurrentAimDirection() const;
protected:
    // 技能类型
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Type")
    EHeroAbilityType AbilityType;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Input")
    FAbilityInputPolicy InputPolicy;

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

    // 是否走基类的交互式选目标流程：已有合法目标数据就直接复用，否则进入 WaitTargetData 确认。
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Targeting")
    bool bUseInteractiveTargeting = true;

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

    /** Builds the cost GE spec and injects project-specific SetByCaller values. */
    FGameplayEffectSpecHandle MakeCostGameplayEffectSpecHandle(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo) const;

    /** Resolves mana cost from a built GE spec for pre-commit checks. */
    float ResolveManaCostFromSpec(const FGameplayEffectSpec& CostSpec) const;

    /** Zero-cost abilities should not depend on a Cost GameplayEffect asset. */
    bool HasMeaningfulCost() const;

    //=========================================
    // 辅助方法和任务回调
    //=========================================
    
      // 开始进行目标选择流程
    virtual void StartTargetSelection(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo);
    virtual void ConfigureTargetSelectionActor(AGameplayAbilityTargetActor& TargetActor);
    virtual void BeginTargetSelectionPresentation();
      // 直接执行技能（跳过目标选择）
    virtual void DirectExecuteAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo);

    virtual bool OtherCheckBeforeCommit();

    /**
     * 服务端对客户端传来的目标数据进行合法性校验。
     * 基类实现：检查目标 Actor 是否存活、是否在最大允许距离内、是否有视线。
     * 子类可覆写以添加阵营、状态等额外校验。
     * 仅在服务端 ActivateAbility 时调用；客户端直接返回 true 以保持预测流畅。
     */
    virtual bool ServerValidateTargetData(const FGameplayAbilityTargetDataHandle& TargetData,
                                          const FGameplayAbilityActorInfo* ActorInfo) const;

    /** 服务端校验使用的最大容忍距离（超出此距离的目标视为非法）。0 表示不做距离限制。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability|Validation")
    float ServerMaxTargetDistance = 0.f;

    /**
     * 调试用：对 LocalPredicted 技能强制制造一次“客户端先播，本地预测成功，服务端随后拒绝”的场景。
     * 适合练习预测失败时的日志、表现收口和状态回退。
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability|Validation|Debug")
    bool bForcePredictionFailureForTests = false;

    // 播放技能动画
    virtual void PlayAbilityMontage(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo);

    /** Pushes cooldown info to the owning player state for HUD updates. */
    void NotifyCooldownTriggered(const FGameplayAbilitySpecHandle& Handle, const FGameplayAbilityActorInfo* ActorInfo) const;

    /** Logs detailed state when CommitAbility fails so runtime blockers are visible in logs. */
    void LogCommitFailureDiagnostics(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const TCHAR* Phase) const;
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
    FHeroAbilityTargetingPolicy BuildTargetingPolicy() const;
    bool InitializeActivationRuntimeState(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo);
    // 动画播放任务引用
    UPROPERTY()
    class UAbilityTask_PlayMontageAndWait* MontageTask;
    void ResetRuntimeAbilityState();
    void EndTargetDataTask();
    void EndMontageTask();
    void BindMontageTaskCallbacks(UAbilityTask_PlayMontageAndWait& InMontageTask);
    bool TryCommitAndExecuteAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const TCHAR* Phase,
        const TCHAR* OtherCheckFailureReason);
    void ContinueAbilityActivation(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo);
    bool ShouldWaitForReplicatedTargetData(
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo& ActivationInfo) const;
    void SendInitialTargetDataToServer(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo& ActivationInfo);
    void WaitForReplicatedTargetData(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo& ActivationInfo);
    void CleanupReplicatedTargetDataDelegates();
    void HandleReplicatedTargetDataReceived(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ActivationTag);
    void HandleReplicatedTargetDataCancelled();
    bool HasRequiredExecutionTargetData(const FGameplayAbilityTargetDataHandle& TargetData) const;
    bool ValidateAbilityConfiguration() const;
    bool ValidateExecutionTargetData(const FGameplayAbilityTargetDataHandle& TargetData, const TCHAR* Phase) const;
    void SetUsesInteractiveTargeting(bool bEnabled);
    void AddAssetTagIfMissing(const FGameplayTag& Tag);
    void AddOwnedTagIfMissing(const FGameplayTag& Tag);
    void AddBlockedTagIfMissing(const FGameplayTag& Tag);
    void SetGameplayEventTriggerTag(const FGameplayTag& Tag);
    void ConfigureAbilityIdentityTag(
        const FGameplayTag& AbilityTag,
        bool bAddOwnedTag = true,
        bool bUseAsGameplayEventTrigger = true);

    FGameplayAbilityActivationInfo CurrentActivationInfo;
    FPendingAbilityActivationContext CurrentInputContext;

    FGameplayAbilityTargetDataHandle CurrentTargetDataHandle;
    FDelegateHandle ReplicatedTargetDataDelegateHandle;
    FDelegateHandle ReplicatedTargetDataCancelledDelegateHandle;
protected:
        virtual void PostInitProperties() override;


};
