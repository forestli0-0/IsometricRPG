#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "AProjectileBase.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

// 用于初始化投射物的数据结构
USTRUCT(BlueprintType)
struct FProjectileInitializationData
{
    GENERATED_BODY()

    // 投射物初始速度
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    float InitialSpeed = 1000.0f;

    // 投射物最大速度
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    float MaxSpeed = 1000.0f;
    
    // 投射物最大飞行距离 (0 表示无限远或由生命周期控制)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    float MaxFlyDistance = 0.0f;

    // 投射物生命周期 (0 表示无限生命周期或由其他逻辑销毁)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    float Lifespan = 3.0f;

    // 投射物碰撞后应用的主要伤害效果 (可选)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    TSubclassOf<UGameplayEffect> DamageEffect;

	// 投射物碰撞后应用的主要伤害数值 (可选)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
	float DamageAmount = 10.0f;
    // 投射物视觉特效 (可选)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    UParticleSystem* VisualEffect;

    // 投射物击中时的爆炸/冲击特效 (可选)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    UParticleSystem* ImpactEffect;
    
    // 投射物飞行音效 (可选)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    USoundBase* FlyingSound;

    // 投射物击中音效 (可选)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    USoundBase* ImpactSound;

    // 溅射半径 (0 表示无溅射)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    float SplashRadius = 50.0f;

    // 溅射伤害效果 (可选, 仅当SplashRadius > 0时有效)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    TSubclassOf<UGameplayEffect> SplashDamageEffect;

    // 投射物是否受重力影响
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    float GravityScale = 0.0f;

    // 投射物是否应该弹跳
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    bool bShouldBounce = false;

    // 构造函数提供默认值
    FProjectileInitializationData() {}
};


UCLASS(Abstract) // 标记为抽象类，因为我们希望具体投射物继承它
class ISOMETRICRPG_API AProjectileBase : public AActor
{
    GENERATED_BODY()

public:
    AProjectileBase();

    // 用于从Ability初始化投射物
    UFUNCTION(BlueprintCallable, Category = "Projectile")
    virtual void InitializeProjectile(const UGameplayAbility* SourceAbility, const FProjectileInitializationData& Data, AActor* InOwner, APawn* InInstigator, UAbilitySystemComponent* InSourceASC);

protected:
    // 组件
    UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
    class USphereComponent* CollisionComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement)
    class UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY(VisibleDefaultsOnly, Category = Effects)
    class UParticleSystemComponent* VisualEffectComp;
    
    UPROPERTY(VisibleDefaultsOnly, Category = Audio)
    class UAudioComponent* FlyingSoundComp;

    // 属性
    UPROPERTY(BlueprintReadOnly, Category = "Projectile")
    FProjectileInitializationData InitData;
    
    UPROPERTY(BlueprintReadOnly, Category = "Projectile")
    TObjectPtr<AActor> ProjectileOwner;

    UPROPERTY(BlueprintReadOnly, Category = "Projectile")
    TObjectPtr<APawn> ProjectileInstigator;
    
    UPROPERTY(BlueprintReadOnly, Category = "Projectile")
    TObjectPtr<UAbilitySystemComponent> SourceAbilitySystemComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UGameplayAbility> SourceAbility;

    float TravelDistance;
    FTimerHandle TimerHandle_Lifespan;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // 碰撞处理
    UFUNCTION()
    virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    // 应用伤害效果
    virtual void ApplyDamageEffects(AActor* TargetActor, const FHitResult& HitResult);
    
    // 处理溅射伤害
    virtual void HandleSplashDamage(const FVector& ImpactLocation, AActor* DirectHitActor);

    // 播放击中效果
    virtual void PlayImpactEffects(const FVector& ImpactLocation);
    
    // 检查是否超出最大飞行距离
    virtual void CheckMaxFlyDistance();

    // 生命周期结束处理
    virtual void OnLifespanExpired();
    
public:
    FORCEINLINE class USphereComponent* GetCollisionComp() const { return CollisionComp; }
    FORCEINLINE class UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }
};
