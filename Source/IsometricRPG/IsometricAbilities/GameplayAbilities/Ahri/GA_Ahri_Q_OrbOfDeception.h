#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GameplayAbilities/SkillShot/GA_SkillShotProjectileAbility.h"
#include "GA_Ahri_Q_OrbOfDeception.generated.h"

/**
 * 阿狸Q技能 - 欺诈宝珠
 * 
 * 技能描述：
 * 阿狸投掷出一个魔法宝珠，向目标方向飞行并穿透敌人造成伤害。
 * 宝珠在达到最大距离后会返回阿狸身边，返回途中再次对敌人造成伤害。
 * 
 * 技能类型：方向性技能 (SkillShot)
 * 冷却时间：7秒
 * 魔法消耗：65点魔法值
 */
UCLASS(BlueprintType)
class ISOMETRICRPG_API UGA_Ahri_Q_OrbOfDeception : public UGA_SkillShotProjectileAbility
{
	GENERATED_BODY()

public:
	UGA_Ahri_Q_OrbOfDeception();

protected:
	// 基础魔法伤害
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb of Deception")
	float BaseMagicDamage = 40.0f;

	// 每级技能伤害增长
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb of Deception")
	float DamagePerLevel = 40.0f;

	// AP加成系数
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb of Deception")
	float APRatio = 0.35f;

	// 宝珠返回时的伤害系数（通常返回伤害更高）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb of Deception")
	float ReturnDamageMultiplier = 1.0f;

	// 宝珠的特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb of Deception|VFX")
	class UNiagaraSystem* OrbVFX;

	// 宝珠命中敌人的特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb of Deception|VFX")
	class UNiagaraSystem* HitVFX;

	// 宝珠的音效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb of Deception|Audio")
	class USoundBase* CastSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb of Deception|Audio")
	class USoundBase* HitSound;

public:
	// 重写技能执行逻辑，该函数体将为空，因为所有逻辑都在基类中
	virtual void ExecuteSkillShot(const FVector& Direction, const FVector& StartLocation) override;

	// 计算技能伤害
	UFUNCTION(BlueprintCallable, Category = "Orb of Deception")
	float CalculateDamage() const;
}; 