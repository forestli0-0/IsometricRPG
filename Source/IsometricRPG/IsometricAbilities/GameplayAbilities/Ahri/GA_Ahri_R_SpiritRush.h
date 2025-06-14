#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GameplayAbilities/SelfCast/GA_SelfCastAbility.h"
#include "GA_Ahri_R_SpiritRush.generated.h"

/**
 * 阿狸R技能 - 狐狸大招 (Spirit Rush)
 * 
 * 技能描述：
 * 阿狸向鼠标指针方向瞬移一段距离，并发射三道狐灵火球攻击附近的敌人。
 * 该技能可以在10秒内连续使用3次，每次使用都会重置冷却时间。
 * 狐灵火球会优先攻击阿狸当前的目标，然后是附近的敌人。
 * 
 * 技能类型：自我施放 + 位移 (SelfCast + Dash)
 * 冷却时间：100秒
 * 魔法消耗：100点魔法值
 * 使用次数：3次
 */
UCLASS(BlueprintType)
class ISOMETRICRPG_API UGA_Ahri_R_SpiritRush : public UGA_SelfCastAbility
{
	GENERATED_BODY()

public:
	UGA_Ahri_R_SpiritRush();

protected:
	// 最大使用次数
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush")
	int32 MaxCharges = 3;

	// 瞬移距离
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush")
	float DashDistance = 450.0f;

	// 瞬移速度
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush")
	float DashSpeed = 2200.0f;

	// 狐灵火球的数量（每次使用）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush")
	int32 SpiritOrbCount = 3;

	// 狐灵火球的搜索范围
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush")
	float SpiritOrbRange = 550.0f;

	// 狐灵火球的飞行速度
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush")
	float SpiritOrbSpeed = 1600.0f;

	// 基础魔法伤害（每个狐灵火球）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush")
	float BaseMagicDamage = 60.0f;

	// 每级技能伤害增长
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush")
	float DamagePerLevel = 45.0f;

	// AP加成系数
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush")
	float APRatio = 0.35f;

	// 多次使用的时间窗口
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush")
	float ChargeWindowDuration = 10.0f;

	// 同一个敌人被多个狐灵火球命中的伤害递减
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush")
	float DamageReductionPerHit = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<class AProjectileBase> SkillShotProjectileClass;

	// 瞬移特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush|VFX")
	class UNiagaraSystem* DashStartVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush|VFX")
	class UNiagaraSystem* DashEndVFX;

	// 狐灵火球特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush|VFX")
	class UNiagaraSystem* SpiritOrbVFX;

	// 狐灵火球命中特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush|VFX")
	class UNiagaraSystem* SpiritOrbHitVFX;

	// 音效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush|Audio")
	class USoundBase* CastSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush|Audio")
	class USoundBase* DashSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush|Audio")
	class USoundBase* SpiritOrbFireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spirit Rush|Audio")
	class USoundBase* SpiritOrbHitSound;

public:
	// 重写技能执行逻辑
	virtual void ExecuteSelfCast();

	// 重写技能激活逻辑以支持多次使用
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 计算技能伤害
	UFUNCTION(BlueprintCallable, Category = "Spirit Rush")
	float CalculateDamage(int32 HitCount = 0) const;

	// 执行瞬移
	UFUNCTION(BlueprintCallable, Category = "Spirit Rush")
	void ExecuteDash();

	// 获取瞬移目标位置
	UFUNCTION(BlueprintCallable, Category = "Spirit Rush")
	FVector GetDashTargetLocation() const;

	// 瞬移完成后发射狐灵火球
	UFUNCTION(BlueprintCallable, Category = "Spirit Rush")
	void FireSpiritOrbs();

	// 寻找狐灵火球的目标
	UFUNCTION(BlueprintCallable, Category = "Spirit Rush")
	TArray<AActor*> FindSpiritOrbTargets() const;

	// 狐灵火球命中敌人时的处理
	UFUNCTION(BlueprintCallable, Category = "Spirit Rush")
	void OnSpiritOrbHitTarget(AActor* HitActor, int32 OrbIndex);

	// 检查是否可以再次使用
	UFUNCTION(BlueprintCallable, Category = "Spirit Rush")
	bool CanUseAgain() const;

	// 重置技能状态
	UFUNCTION(BlueprintCallable, Category = "Spirit Rush")
	void ResetAbilityState();

	// 技能窗口期结束的处理
	UFUNCTION(BlueprintCallable, Category = "Spirit Rush")
	void OnChargeWindowExpired();

private:
	// 当前已使用次数
	int32 CurrentCharges = 0;

	// 充能窗口定时器
	FTimerHandle ChargeWindowTimerHandle;

	// 每个敌人被命中的次数（用于计算伤害递减）
	UPROPERTY()
	TMap<TObjectPtr<AActor>, int32> EnemyHitCounts;

	// 当前激活的狐灵火球
	UPROPERTY()
	TArray<TObjectPtr<class AProjectileBase>> ActiveSpiritOrbs;

	// 是否在充能窗口期内
	bool bInChargeWindow = false;

	// 瞬移任务
	UPROPERTY()
	TObjectPtr<class UAbilityTask> DashTask;
}; 