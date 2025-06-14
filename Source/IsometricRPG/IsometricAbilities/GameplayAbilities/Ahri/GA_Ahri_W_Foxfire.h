#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GameplayAbilities/SelfCast/GA_SelfCastAbility.h"
#include "GA_Ahri_W_Foxfire.generated.h"

/**
 * 阿狸W技能 - 狐火
 * 
 * 技能描述：
 * 阿狸释放三个狐火球环绕在自己周围，每个狐火球会自动寻找附近的敌人并造成魔法伤害。
 * 狐火球优先攻击阿狸当前攻击的目标，如果没有目标则寻找最近的敌人。
 * 同一个敌人只能被狐火球命中一次。
 * 
 * 技能类型：自我施放 (SelfCast)
 * 冷却时间：9秒
 * 魔法消耗：55点魔法值
 */
UCLASS(BlueprintType)
class ISOMETRICRPG_API UGA_Ahri_W_Foxfire : public UGA_SelfCastAbility
{
	GENERATED_BODY()

public:
	UGA_Ahri_W_Foxfire();

protected:
	// 狐火球的数量
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	int32 FoxfireCount = 3;

	// 狐火球的搜索范围
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float SearchRange = 550.0f;

	// 狐火球的飞行速度
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float FoxfireSpeed = 1500.0f;

	// 狐火球环绕阿狸的半径
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float OrbitRadius = 150.0f;

	// 狐火球环绕的角速度（弧度/秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float OrbitSpeed = 3.0f;

	// 基础魔法伤害（每个狐火球）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float BaseMagicDamage = 40.0f;

	// 每级技能伤害增长
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float DamagePerLevel = 25.0f;

	// AP加成系数
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float APRatio = 0.3f;

	// 狐火球持续时间（如果没有找到目标）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float FoxfireDuration = 5.0f;

	// 狐火球发射间隔
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float FireInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|Projectile")
	TSubclassOf<class AProjectileBase> SkillShotProjectileClass;

	// 狐火球的特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|VFX")
	class UNiagaraSystem* FoxfireVFX;

	// 狐火球命中敌人的特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|VFX")
	class UNiagaraSystem* HitVFX;

	// 狐火球环绕特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|VFX")
	class UNiagaraSystem* OrbitVFX;

	// 狐火球的音效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|Audio")
	class USoundBase* CastSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|Audio")
	class USoundBase* FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|Audio")
	class USoundBase* HitSound;

public:
	// 重写技能执行逻辑
	virtual void ExecuteSelfCast();

	// 计算技能伤害
	UFUNCTION(BlueprintCallable, Category = "Foxfire")
	float CalculateDamage() const;

	// 创建狐火球
	UFUNCTION(BlueprintCallable, Category = "Foxfire")
	void CreateFoxfires();

	// 发射一个狐火球
	UFUNCTION(BlueprintCallable, Category = "Foxfire")
	void FireFoxfire(int32 FoxfireIndex);

	// 寻找狐火球的目标
	UFUNCTION(BlueprintCallable, Category = "Foxfire")
	AActor* FindFoxfireTarget() const;

	// 狐火球命中敌人时的处理
	UFUNCTION(BlueprintCallable, Category = "Foxfire")
	void OnFoxfireHitTarget(AActor* HitActor, int32 FoxfireIndex);

	// 所有狐火球完成后的处理
	UFUNCTION(BlueprintCallable, Category = "Foxfire")
	void OnAllFoxfiresComplete();

private:
	// 当前活跃的狐火球
	UPROPERTY()
	TArray<TObjectPtr<class AProjectileBase>> ActiveFoxfires;

	// 已经被命中的敌人列表（避免重复伤害）
	UPROPERTY()
	TSet<TObjectPtr<AActor>> HitActors;

	// 当前已发射的狐火球数量
	int32 FiredFoxfireCount = 0;

	// 狐火球发射定时器
	FTimerHandle FireTimerHandle;

	// 环绕特效的组件
	UPROPERTY()
	TObjectPtr<class UNiagaraComponent> OrbitEffectComponent;
}; 