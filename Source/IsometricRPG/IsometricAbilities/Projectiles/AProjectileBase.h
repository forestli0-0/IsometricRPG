#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "AProjectileBase.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class USphereComponent;
class UProjectileMovementComponent;
class UAudioComponent;

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
    UNiagaraSystem* VisualEffect;

    // 投射物击中时的爆炸/冲击特效 (可选)  
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")  
    UNiagaraSystem* ImpactEffect;

    // 投射物飞行音效 (可选)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    USoundBase* FlyingSound;

    // 投射物击中音效 (可选)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    USoundBase* ImpactSound;

    // 溅射伤害效果 (可选)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    TSubclassOf<UGameplayEffect> SplashDamageEffect;

    // 溅射半径
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    float SplashRadius = 0.0f;

    // 重力缩放
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    float GravityScale = 0.0f;

    // 是否反弹
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data")
    bool bShouldBounce = false;
};

UCLASS()
class ISOMETRICRPG_API AProjectileBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectileBase();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY()
    class UGameplayAbility* SourceAbility;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 用于从Ability初始化投射物
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	virtual void InitializeProjectile(const UGameplayAbility* InSourceAbility, const FProjectileInitializationData& Data, AActor* InOwner, APawn* InInstigator, UAbilitySystemComponent* InSourceASC);

	// 碰撞处理
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION()
    virtual void OnRep_InitData();

	UPROPERTY(ReplicatedUsing = OnRep_InitData, BlueprintReadOnly, Category = "Projectile Data")
	FProjectileInitializationData InitData;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile Data")
	AActor* ProjectileOwner;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile Data")
	APawn* ProjectileInstigator;
	
	UPROPERTY(BlueprintReadOnly, Category = "Projectile Data")
	UAbilitySystemComponent* SourceAbilitySystemComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile Data")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile Data")
	TSubclassOf<UGameplayEffect> SplashDamageEffect;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile Data")
	UNiagaraSystem* VisualEffect;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile Data")
	UNiagaraSystem* ImpactEffect;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile Data")
	USoundBase* FlyingSound;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile Data")
	USoundBase* ImpactSound;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile Data")
	float InitialSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile Data")
	float MaxSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile Data")
	float MaxFlyDistance;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile Data")
	float Lifespan;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile Data")
	float SplashRadius;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile Data")
	float GravityScale;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile Data")
	bool bShouldBounce;

private:
    UPROPERTY(VisibleDefaultsOnly, Category = "Projectile")
    class USphereComponent* CollisionComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    class UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    class UNiagaraComponent* VisualEffectComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    class UAudioComponent* FlyingSoundComp;

    float TravelDistance;
    FTimerHandle TimerHandle_Lifespan;

protected:
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
