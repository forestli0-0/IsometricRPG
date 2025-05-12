#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "Projectile_Fireball.generated.h"

class UGA_FireballAbility;
UCLASS()
class ISOMETRICRPG_API AProjectile_Fireball : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectile_Fireball();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// Called when the actor is being removed from play
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
    // 当投射物击中物体时调用
    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    // 设置火球效果
    void SetFireballEffect(UParticleSystem* InFireballEffect);

    // 投射物组件
    UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
    class USphereComponent* CollisionComp;

    // 投射物移动组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement)
    class UProjectileMovementComponent* ProjectileMovement;

    // 火球特效组件
    UPROPERTY(VisibleDefaultsOnly, Category = Effects)
    class UParticleSystemComponent* FireballEffectComp;

    // 火球技能引用
    UPROPERTY()
    class UGA_FireballAbility* SourceAbility;

    // 源能力系统组件
    UPROPERTY()
    class UAbilitySystemComponent* SourceASC;

    // 目标位置
    UPROPERTY()
    FVector TargetLocation;

    // 火球伤害效果
    UPROPERTY()
    TSubclassOf<UGameplayEffect> FireDamageEffect;

    // 灼烧效果
    UPROPERTY()
    TSubclassOf<UGameplayEffect> BurnEffect;

    // 爆炸效果
    UPROPERTY()
    UParticleSystem* ExplosionEffect;

    // 爆炸声音
    UPROPERTY()
    USoundBase* ExplosionSound;

    // 溅射半径
    UPROPERTY()
    float SplashRadius;

    // 伤害系数
    UPROPERTY()
    float DamageMultiplier;

    // 法术强度
    UPROPERTY()
    float SpellPower;
};
