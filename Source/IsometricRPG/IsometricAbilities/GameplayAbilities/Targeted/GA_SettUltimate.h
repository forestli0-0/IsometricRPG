#pragma once

#include "CoreMinimal.h"
#include "GA_TargetedAbility.h"
#include "GameplayEffect.h"
#include "Components/TimelineComponent.h"
#include "GA_SettUltimate.generated.h"

/**
 * 腕豪大招 - 叹为观止 (The Show Stopper)
 * 参考英雄联盟腕豪的R技能
 * 
 * 技能流程：
 * 1. 选择目标敌方英雄
 * 2. 抓取目标并跳到空中
 * 3. 重重摔下，对主目标造成大量伤害
 * 4. 对落地区域的其他敌人造成AOE伤害
 * 5. 根据主目标的最大生命值造成额外伤害
 */
UCLASS()
class ISOMETRICRPG_API UGA_SettUltimate : public UGA_TargetedAbility
{
	GENERATED_BODY()

public:
	UGA_SettUltimate();

protected:
	// ==================== 基础伤害配置 ====================
	// 基础伤害
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Damage")
	float BaseDamage = 100.0f;

	// 基于目标最大生命值的伤害百分比
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Damage")
	float MaxHealthDamagePercent = 0.15f; // 15%最大生命值

	// AOE范围内的伤害衰减
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Damage")
	float AOEDamageReduction = 0.6f; // AOE伤害为主目标的60%

	// ==================== 技能范围配置 ====================
	// AOE伤害范围
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Range")
	float AOERadius = 300.0f;

	// 抓取范围（比普通攻击稍远）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Range")
	float GrabRange = 175.0f;

	// 添加两个新属性来控制大招的距离和高度
	UPROPERTY(EditDefaultsOnly, Category = "Ultimate|Movement")
	float JumpDistance = 1200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ultimate|Movement")
	float JumpArcHeight = 800.0f;

	// ==================== 时间配置 ====================
	// 抓取动画时间
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Timing")
	float GrabDuration = 0.5f;

	// 跳跃在空中的时间
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Timing")
	float JumpUpDuration = 0.8f;

	// 落地后的硬直时间
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Timing")
	float LandingStunDuration = 0.3f;

	// ==================== 游戏效果 ====================
	// 主目标伤害效果
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Effects")
	TSubclassOf<UGameplayEffect> PrimaryDamageEffectClass;

	// AOE伤害效果
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Effects")
	TSubclassOf<UGameplayEffect> AOEDamageEffectClass;

	// 眩晕效果（对主目标）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Effects")
	TSubclassOf<UGameplayEffect> StunEffectClass;

	// 击退效果（对AOE范围内的敌人）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Effects")
	TSubclassOf<UGameplayEffect> KnockbackEffectClass;

	// ==================== 视觉和音效 ====================
	// 抓取特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|VFX")
	TSubclassOf<class ANiagaraActor> GrabEffectClass;

	// 跳跃轨迹特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|VFX")
	TSubclassOf<class ANiagaraActor> JumpTrailEffectClass;

	// 落地爆炸特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|VFX")
	TSubclassOf<class ANiagaraActor> LandingExplosionEffectClass;

	// AOE范围预警特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|VFX")
	TSubclassOf<class ANiagaraActor> AOEWarningEffectClass;

	// 抓取音效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Audio")
	class USoundBase* GrabSound;

	// 跳跃音效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Audio")
	class USoundBase* JumpSound;

	// 落地音效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Audio")
	class USoundBase* LandingSound;

	// 腕豪台词音效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Audio")
	class USoundBase* SettVoiceLineSound;

	// ==================== 动画配置 ====================
	// 抓取动画
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Animation")
	UAnimMontage* GrabMontage;

	// 跳跃动画
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Animation")
	UAnimMontage* JumpMontage;

	// 落地动画
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Animation")
	UAnimMontage* LandingMontage;

	// ==================== 技能属性 ====================
	// 是否可以被打断
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Properties")
	bool bCanBeInterrupted = false;

	// 是否对友军造成伤害
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Properties")
	bool bDamageAllies = false;

	// 最大生命值伤害是否为真实伤害
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SettUlt|Properties")
	bool bMaxHealthDamageIsTrueDamage = true;

	// ==================== 核心方法重写 ====================
	virtual void ExecuteSkill(const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo
	) override;


	// ==================== 技能阶段方法 ====================
	// 第一阶段：抓取目标
	UFUNCTION()
	virtual void StartGrabPhase();

	// 第二阶段：跳跃到空中
	UFUNCTION()
	virtual void StartJumpPhase();



	// 第四阶段：落地爆炸
	UFUNCTION()
	virtual void StartLandingPhase();

	// 技能结束清理
	UFUNCTION()
	virtual void FinishUltimate();

	// ==================== 伤害计算方法 ====================
	// 计算对主目标的总伤害
	virtual float CalculatePrimaryTargetDamage(AActor* Target, AActor* Caster) const;

	// 计算AOE伤害
	virtual float CalculateAOEDamage(AActor* Target, AActor* Caster) const;

	// 应用主目标伤害
	virtual void ApplyPrimaryTargetDamage(AActor* Target, AActor* Caster);

	// 应用AOE伤害
	virtual void ApplyAOEDamage(const FVector& CenterLocation, AActor* Caster);

	// ==================== 辅助方法 ====================
	// 获取AOE范围内的敌人
	virtual TArray<AActor*> GetEnemiesInAOE(const FVector& CenterLocation, AActor* Caster) const;

	// 播放阶段特效
	virtual void PlayGrabEffects(AActor* Target);
	virtual void PlayJumpEffects(const FVector& StartLocation, const FVector& EndLocation);
	virtual void PlayLandingEffects(const FVector& LandingLocation);

	// 检查目标是否为有效的英雄单位
	virtual bool IsValidHeroTarget(AActor* Target, AActor* Caster) const;



	// ==================== 状态变量 ====================
private:
	// 当前技能阶段
	UPROPERTY()
	int32 CurrentPhase = 0;

	// 主要目标
	UPROPERTY()
	AActor* PrimaryTarget = nullptr;

	// 施法者
	UPROPERTY()
	AActor* Caster = nullptr;

	// 起始位置
	FVector StartLocation;

	// 落地位置
	FVector LandingLocation;

	// 阶段定时器
	FTimerHandle GrabTimer;
	FTimerHandle JumpTimer;
	FTimerHandle SlamTimer;
	FTimerHandle LandingTimer;

	// 特效引用
	UPROPERTY()
	class ANiagaraActor* ActiveGrabEffect = nullptr;

	UPROPERTY()
	class ANiagaraActor* ActiveJumpTrailEffect = nullptr;

	UPROPERTY()
	class ANiagaraActor* ActiveLandingEffect = nullptr;

	UPROPERTY()
	class ANiagaraActor* ActiveAOEWarningEffect = nullptr;

	// 动画任务引用
	UPROPERTY()
	class UAbilityTask_PlayMontageAndWait* CurrentMontageTask = nullptr;
	// 跳跃曲线
	UPROPERTY(EditDefaultsOnly, Category = "Ultimate|Movement")
	UCurveFloat* JumpArcCurve;
// 解决落地弹跳问题
private:
		// 保存施法者和目标的原始移动模式
		UPROPERTY()
		TEnumAsByte<ECollisionEnabled::Type> CasterOriginalCollision;
		UPROPERTY()
		TEnumAsByte<EMovementMode> CasterOriginalMode;

		UPROPERTY()
		TEnumAsByte<ECollisionEnabled::Type> TargetOriginalCollision;
		UPROPERTY()
		TEnumAsByte<EMovementMode> TargetOriginalMode;

		// 用于弱引用，防止角色中途死亡导致崩溃
		UPROPERTY()
		TWeakObjectPtr<ACharacter> CasterCharacter;

		UPROPERTY()
		TWeakObjectPtr<ACharacter> TargetCharacter;

		bool bOriginalUseControllerDesiredRotation;

		bool bOriginalOrientRotationToMovement;
}; 