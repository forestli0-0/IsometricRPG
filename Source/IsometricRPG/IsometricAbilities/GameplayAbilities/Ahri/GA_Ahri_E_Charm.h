#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GameplayAbilities/Targeted/GA_TargetedProjectileAbility.h"
#include "GA_Ahri_E_Charm.generated.h"

/**
 * 阿狸E技能 - 魅惑
 * 
 * 技能描述：
 * 阿狸吹出一个吻，向目标方向发射一个爱心投射物。
 * 命中的第一个敌人会被魅惑，被迫朝阿狸移动一段时间，并受到魔法伤害。
 * 被魅惑的敌人移动速度会逐渐减慢，直到停止。
 * 
 * 技能类型：目标指向投射物 (Targeted Projectile)
 * 冷却时间：12秒
 * 魔法消耗：85点魔法值
 */
UCLASS(BlueprintType)
class ISOMETRICRPG_API UGA_Ahri_E_Charm : public UGA_TargetedProjectileAbility
{
	GENERATED_BODY()

public:
	UGA_Ahri_E_Charm();

protected:
	// 魅惑持续时间
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charm")
	float CharmDuration = 1.75f;

	// 投射物飞行速度
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charm")
	float ProjectileSpeed = 1550.0f;

	// 投射物宽度
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charm")
	float ProjectileWidth = 60.0f;

	// 基础魔法伤害
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charm")
	float BaseMagicDamage = 60.0f;

	// 每级技能伤害增长
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charm")
	float DamagePerLevel = 35.0f;

	// AP加成系数
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charm")
	float APRatio = 0.5f;

	// 魅惑效果的移动速度减少百分比
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charm")
	float MovementSpeedReduction = 65.0f;

	// 魅惑效果期间敌人受到的伤害增加百分比
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charm")
	float DamageAmplification = 20.0f;

	// 投射物特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charm|VFX")
	class UNiagaraSystem* ProjectileVFX;

	// 魅惑命中特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charm|VFX")
	class UNiagaraSystem* CharmHitVFX;

	// 魅惑状态特效（在被魅惑敌人身上显示）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charm|VFX")
	class UNiagaraSystem* CharmStatusVFX;

	// 音效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charm|Audio")
	class USoundBase* CastSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charm|Audio")
	class USoundBase* HitSound;

	// 魅惑的GameplayEffect类（用于应用魅惑状态）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charm|GameplayEffect")
	TSubclassOf<class UGameplayEffect> CharmEffectClass;

	// 伤害增幅的GameplayEffect类
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charm|GameplayEffect")
	TSubclassOf<class UGameplayEffect> DamageAmplificationEffectClass;

public:
	// 重写投射物执行逻辑
	virtual void ExecuteTargetedProjectile(AActor* Target, const FVector& TargetLocation);

	// 计算技能伤害
	UFUNCTION(BlueprintCallable, Category = "Charm")
	float CalculateDamage() const;

	// 投射物命中目标时的处理
	UFUNCTION(BlueprintCallable, Category = "Charm")
	void OnCharmProjectileHit(AActor* HitActor);

	// 应用魅惑效果
	UFUNCTION(BlueprintCallable, Category = "Charm")
	void ApplyCharmEffect(AActor* Target);

	// 魅惑效果结束时的处理
	UFUNCTION(BlueprintCallable, Category = "Charm")
	void OnCharmEffectEnd(AActor* Target);

private:
	// 当前投射物的引用
	UPROPERTY()
	TObjectPtr<class AProjectileBase> CurrentProjectile;

	// 被魅惑的目标
	UPROPERTY()
	TObjectPtr<AActor> CharmedTarget;

	// 魅惑效果的句柄
	FActiveGameplayEffectHandle CharmEffectHandle;

	// 伤害增幅效果的句柄
	FActiveGameplayEffectHandle DamageAmplificationHandle;
}; 