// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricAbilities/Projectile_Fireball.h"
// 基础Actor功能
#include "GameFramework/Actor.h"

// 碰撞组件
#include "Components/SphereComponent.h"

// 粒子系统（Cascade）
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"

// 运动组件
#include "GameFramework/ProjectileMovementComponent.h"

// 播放音效
#include "Kismet/GameplayStatics.h"

// GAS 能力系统相关
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"

// 你的自定义类（例如火球Ability引用）
#include "IsometricAbilities/GA_FireballAbility.h"
// Sets default values
AProjectile_Fireball::AProjectile_Fireball()
{
    // 设置此actor的tick
    PrimaryActorTick.bCanEverTick = true;

    // 使用球体作为简单的碰撞表示
    CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    CollisionComp->InitSphereRadius(15.0f);
    CollisionComp->SetCollisionProfileName("Projectile");
    CollisionComp->OnComponentHit.AddDynamic(this, &AProjectile_Fireball::OnHit);

    // 将碰撞组件设置为根
    RootComponent = CollisionComp;

    // 创建火球特效组件
    FireballEffectComp = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FireballEffect"));
    FireballEffectComp->SetupAttachment(RootComponent);

    // 使用ProjectileMovementComponent来管理运动
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
    ProjectileMovement->UpdatedComponent = CollisionComp;
    ProjectileMovement->InitialSpeed = 1500.f;
    ProjectileMovement->MaxSpeed = 1500.f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;
    ProjectileMovement->ProjectileGravityScale = 0.0f;

    // 3秒后自动销毁
    //InitialLifeSpan = 10.0f;

    // 默认值
    SplashRadius = 200.0f;
    DamageMultiplier = 1.0f;
    SpellPower = 50.0f;
}

// Called when the game starts or when spawned
void AProjectile_Fireball::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner())
	{
		CollisionComp->IgnoreActorWhenMoving(GetOwner(), true);
	}
	
}

// Called every frame
void AProjectile_Fireball::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//DrawDebugSphere(GetWorld(), GetActorLocation(), 15.f, 12, FColor::Red, false, -1.0f, 0, 1.0f); // 可选：调试可视化
	//UE_LOG(LogTemp, Warning, TEXT("Projectile_Fireball Tick: Actor %s, Current LifeSpan: %f, InitialLifeSpan set in C++ by Ability: %f"), *GetName(), GetLifeSpan(), InitialLifeSpan);
}

void AProjectile_Fireball::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	UE_LOG(LogTemp, Warning, TEXT("Projectile_Fireball %s EndPlay called. Reason: %s"), *GetName(), *UEnum::GetValueAsString(EndPlayReason));
}

void AProjectile_Fireball::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    // 获取碰撞位置
    FVector ImpactLocation = GetActorLocation();

    // 播放爆炸特效
    if (ExplosionEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, ImpactLocation, FRotator::ZeroRotator, true);
    }

    // 播放爆炸声音
    if (ExplosionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, ImpactLocation);
    }

    // 通知能力类处理伤害
    if (SourceAbility)
    {
        // 如果击中了有效目标，处理直接命中伤害
        if (OtherActor && OtherActor != this && OtherActor != GetOwner())
        {
            // 对直接命中的目标造成100%伤害
            SourceAbility->ApplyDamageToTarget(OtherActor, 1.0f);
        }

        // 处理溅射伤害（传入直接命中的目标，以避免重复伤害）
        SourceAbility->ApplySplashDamage(ImpactLocation, OtherActor);
    }

    // 销毁投射物
    Destroy();
}


void AProjectile_Fireball::SetFireballEffect(UParticleSystem* InFireballEffect)
{
    if (InFireballEffect && FireballEffectComp)
    {
        FireballEffectComp->SetTemplate(InFireballEffect);
    }
}

