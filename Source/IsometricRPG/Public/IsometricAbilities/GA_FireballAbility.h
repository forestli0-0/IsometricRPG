#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GA_HeroBaseAbility.h"
#include "GA_FireballAbility.generated.h"


class AProjectile_Fireball;
/**
 * 
 */
UCLASS()
class ISOMETRICRPG_API UGA_FireballAbility : public UGA_HeroBaseAbility
{
	GENERATED_BODY()

public:
    UGA_FireballAbility();

    // 火球击中目标后产生的溅射效果（修改参数以包含直接命中的目标）
    void ApplySplashDamage(const FVector& ImpactLocation, AActor* DirectHitActor = nullptr);

    // 对单个目标应用伤害的方法
    void ApplyDamageToTarget(AActor* TargetActor, float DamageMultiplier);
protected:
    // 重写投射物攻击方法
    virtual void ExecuteProjectile() override;
    FGameplayAbilityTargetData* GetCurrentAbilityTargetData() const;
    // 火球投射物类
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Projectile")
    TSubclassOf<class AProjectile_Fireball> ProjectileClass;

    // 火球溅射范围
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Projectile")
    float SplashRadius = 200.0f;

    // 火球主要伤害效果
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects")
    TSubclassOf<class UGameplayEffect> FireDamageEffect;

    // 火球灼烧效果 (持续伤害)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects")
    TSubclassOf<class UGameplayEffect> BurnEffect;

    // 火球溅射伤害系数 (相对于主要伤害)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects")
    float SplashDamageMultiplier = 0.5f;

    // 火球飞行速度
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Projectile")
    float ProjectileSpeed = 1000.0f;

    // 火球特效
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects")
    UParticleSystem* FireballEffect;

    // 火球爆炸特效
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects")
    UParticleSystem* ExplosionEffect;

    // 火球音效
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Sound")
    USoundBase* FireballSound;

    // 火球爆炸音效
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Sound")
    USoundBase* ExplosionSound;
};
