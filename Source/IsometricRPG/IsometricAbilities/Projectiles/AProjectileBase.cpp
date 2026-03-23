#include "AProjectileBase.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectTypes.h"
#include "Components/AudioComponent.h"
#include "Net/UnrealNetwork.h"

AProjectileBase::AProjectileBase()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics; // 物理前先 Tick 以更新飞行距离

    bReplicates = true;
    SetReplicateMovement(true);

    CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    CollisionComp->InitSphereRadius(15.0f);
    CollisionComp->SetCollisionProfileName("Projectile");
    CollisionComp->OnComponentHit.AddDynamic(this, &AProjectileBase::OnHit);
    CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AProjectileBase::OnBeginOverlap);
    RootComponent = CollisionComp;

    VisualEffectComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("VisualEffectComp"));
    VisualEffectComp->SetupAttachment(RootComponent);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComp"));
    ProjectileMovement->UpdatedComponent = CollisionComp;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false; // 默认不弹跳，由InitData控制
    ProjectileMovement->ProjectileGravityScale = 0.0f; // 默认无重力，由InitData控制

    FlyingSoundComp = CreateDefaultSubobject<UAudioComponent>(TEXT("FlyingSoundComp"));
    FlyingSoundComp->SetupAttachment(RootComponent);
    FlyingSoundComp->bAutoActivate = false; // 不自动播放，由代码控制

    TravelDistance = 0.0f;
    ProjectileOwner = nullptr;
    ProjectileInstigator = nullptr;
    SourceAbilitySystemComponent = nullptr;
}

void AProjectileBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AProjectileBase, InitData);
}

void AProjectileBase::OnRep_InitData()
{
    // 当客户端收到 InitData 更新时，刷新视觉效果和移动参数
    if (ProjectileMovement)
    {
        ProjectileMovement->InitialSpeed = InitData.InitialSpeed;
        ProjectileMovement->MaxSpeed = InitData.MaxSpeed > 0 ? InitData.MaxSpeed : InitData.InitialSpeed;
        ProjectileMovement->ProjectileGravityScale = InitData.GravityScale;
        ProjectileMovement->bShouldBounce = InitData.bShouldBounce;
    }

    ApplyHomingConfig();

    // 生成视觉特效
    if (InitData.VisualEffect)
    {
        // 清理旧的（如果有）- 这里简单起见直接生成新的，通常 InitData 只会设置一次
        if (VisualEffectComp)
        {
             VisualEffectComp->Deactivate();
        }
        
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            InitData.VisualEffect,
            RootComponent,
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset,
            true // 自动销毁
        );
    }


    // 播放飞行音效
    if (FlyingSoundComp && InitData.FlyingSound)
    {
        FlyingSoundComp->SetSound(InitData.FlyingSound);
        FlyingSoundComp->Play();
    }
}

void AProjectileBase::InitializeProjectile(const UGameplayAbility* InSourceAbility, const FProjectileInitializationData& Data, AActor* InOwner, APawn* InInstigator, UAbilitySystemComponent* InSourceASC)
{
    InitData = Data;
    ProjectileOwner = InOwner;
    ProjectileInstigator = InInstigator;
    SourceAbilitySystemComponent = InSourceASC;
    SourceAbility = const_cast<UGameplayAbility*>(InSourceAbility);

    if (ProjectileMovement)
    {
        ProjectileMovement->InitialSpeed = InitData.InitialSpeed;
        ProjectileMovement->MaxSpeed = InitData.MaxSpeed > 0 ? InitData.MaxSpeed : InitData.InitialSpeed;
        ProjectileMovement->ProjectileGravityScale = InitData.GravityScale;
        ProjectileMovement->bShouldBounce = InitData.bShouldBounce;

        // 初始速度/方向
        if (ProjectileMovement->Velocity.IsZero() && ProjectileMovement->InitialSpeed > 0.f)
        {
            const FRotator CurrentActorRotation = GetActorRotation();
            ProjectileMovement->Velocity = CurrentActorRotation.Vector() * ProjectileMovement->InitialSpeed;
        }
    }

    bIsReturning = false;
    HitActorsForward.Reset();
    HitActorsReturn.Reset();

    ApplyHomingConfig();

    if (InitData.VisualEffect)
    {
        if (VisualEffectComp)
        {
            VisualEffectComp->Deactivate();
        }

        UNiagaraFunctionLibrary::SpawnSystemAttached(
            InitData.VisualEffect,
            RootComponent,
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset,
            true // 自动销毁
        );
    }
    else if (VisualEffectComp)
    {
        VisualEffectComp->Deactivate();
    }
    if (FlyingSoundComp && InitData.FlyingSound)
    {
        FlyingSoundComp->SetSound(InitData.FlyingSound);
        FlyingSoundComp->Play();

    }

    // 设置生命周期
    if (InitData.Lifespan > 0.0f)
    {
        GetWorldTimerManager().SetTimer(TimerHandle_Lifespan, this, &AProjectileBase::OnLifespanExpired, InitData.Lifespan, false);
    }

    // 忽略Owner的碰撞
    if (InOwner)
    {
        CollisionComp->IgnoreActorWhenMoving(InOwner, true);
    }
}

void AProjectileBase::BeginPlay()
{
    Super::BeginPlay();
}

void AProjectileBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 更新飞行距离
    if (ProjectileMovement && !ProjectileMovement->Velocity.IsNearlyZero())
    {
        TravelDistance += ProjectileMovement->Velocity.Size() * DeltaTime;
        CheckMaxFlyDistance();
    }

    // 返航投射物：持续朝 Owner 当前坐标调整速度，保证追随实时位置
    if (bIsReturning && ProjectileOwner && ProjectileMovement)
    {
        const FVector Dir = (ProjectileOwner->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        const float Speed = InitData.InitialSpeed * FMath::Max(0.1f, InitData.ReturnSpeedMultiplier);
        ProjectileMovement->Velocity = Dir * Speed;

        // 接近 Owner 后销毁
        const float DistSq = FVector::DistSquared(GetActorLocation(), ProjectileOwner->GetActorLocation());
        if (DistSq <= FMath::Square(60.0f))
        {
            Destroy();
        }
    }

}

void AProjectileBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(TimerHandle_Lifespan);
    if (FlyingSoundComp && FlyingSoundComp->IsPlaying())
    {
        FlyingSoundComp->Stop();
    }
    Super::EndPlay(EndPlayReason);
}

void AProjectileBase::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    HandleImpactActor(OtherActor, Hit);

    // 对于阻挡型命中，如果不是往返/穿透，并且不弹跳，则销毁
    if (!InitData.bReturnToOwner && !InitData.bShouldBounce)
    {
        Destroy();
    }
}

void AProjectileBase::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    HandleImpactActor(OtherActor, SweepResult);

    // overlap 默认认为是“穿透”，不在这里销毁；若需要非穿透投射物，可通过 bReturnToOwner/bShouldBounce 控制 OnHit 的销毁
}

void AProjectileBase::HandleImpactActor(AActor* OtherActor, const FHitResult& Hit)
{
    if (!OtherActor || OtherActor == ProjectileOwner || OtherActor == this)
    {
        return;
    }

    // 往返投射物：允许同一目标去程/回程各命中一次，但同一阶段只命中一次
    if (InitData.bReturnToOwner)
    {
        TSet<TWeakObjectPtr<AActor>>& HitSet = bIsReturning ? HitActorsReturn : HitActorsForward;
        if (HitSet.Contains(OtherActor))
        {
            return;
        }
        HitSet.Add(OtherActor);
    }

    OnProjectileHitActor.Broadcast(OtherActor, Hit);

    PlayImpactEffects(Hit.ImpactPoint);
    ApplyDamageEffects(OtherActor, Hit);

    if (InitData.SplashRadius > 0.0f)
    {
        HandleSplashDamage(Hit.ImpactPoint, OtherActor);
    }
}

void AProjectileBase::ApplyDamageEffects(AActor* TargetActor, const FHitResult& HitResult)
{
    if (SourceAbilitySystemComponent && InitData.DamageEffect && TargetActor)
    {
        UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
        if (TargetASC)
        {
            FGameplayEffectContextHandle EffectContext = SourceAbilitySystemComponent->MakeEffectContext();
            EffectContext.AddSourceObject(this); // 投射物作为效果源对象
            EffectContext.AddInstigator(ProjectileInstigator, ProjectileOwner); // 设置Instigator和Causer
            EffectContext.AddHitResult(HitResult);

            FGameplayEffectSpecHandle SpecHandle = SourceAbilitySystemComponent->MakeOutgoingSpec(InitData.DamageEffect, 1.0f, EffectContext);
			SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), -InitData.DamageAmount); // 设置伤害量
            if (SpecHandle.IsValid())
            {
                SourceAbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
            }
        }
    }
}

void AProjectileBase::HandleSplashDamage(const FVector& ImpactLocation, AActor* DirectHitActor)
{
    if (SourceAbilitySystemComponent && InitData.SplashDamageEffect && InitData.SplashRadius > 0.0f)
    {
        TArray<AActor*> OverlappedActors;
        TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
        ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn)); // 可以根据需要配置检测的物体类型
        ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Destructible));

        UKismetSystemLibrary::SphereOverlapActors(
            GetWorld(),
            ImpactLocation,
            InitData.SplashRadius,
            ObjectTypes,
            nullptr, // 过滤的 Actor 类（可选）
            TArray<AActor*>({ProjectileOwner, DirectHitActor}), // 忽略的 Actor（发射者与直接命中目标）
            OverlappedActors
        );

        for (AActor* OverlappedActor : OverlappedActors)
        {
            if (OverlappedActor)
            {
                UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OverlappedActor);
                if (TargetASC)
                {
                    FGameplayEffectContextHandle EffectContext = SourceAbilitySystemComponent->MakeEffectContext();
                    EffectContext.AddSourceObject(this);
                    EffectContext.AddInstigator(ProjectileInstigator, ProjectileOwner);
                    // 对于溅射，HitResult可能不直接相关，或者可以创建一个新的HitResult指向溅射中心

                    FGameplayEffectSpecHandle SpecHandle = SourceAbilitySystemComponent->MakeOutgoingSpec(InitData.SplashDamageEffect, 1.0f, EffectContext);
                    float SplashDamage = -InitData.DamageAmount * 0.3;
                    SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), SplashDamage);
                    if (SpecHandle.IsValid())
                    {
                        SourceAbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
                    }
                }
            }
        }
    }
}

void AProjectileBase::PlayImpactEffects(const FVector& ImpactLocation)
{
    if (InitData.ImpactEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            InitData.ImpactEffect,
            ImpactLocation,
            GetActorRotation()
        );
    }
    if (InitData.ImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, InitData.ImpactSound, ImpactLocation);
    }
    if (FlyingSoundComp && FlyingSoundComp->IsPlaying())
    {
        FlyingSoundComp->Stop(); // 停止飞行音效
    }
}

void AProjectileBase::CheckMaxFlyDistance()
{
    if (InitData.MaxFlyDistance <= 0.0f)
    {
        return;
    }

    if (TravelDistance < InitData.MaxFlyDistance)
    {
        return;
    }

    if (InitData.bReturnToOwner && !bIsReturning)
    {
        StartReturnToOwner();
        return;
    }

    PlayImpactEffects(GetActorLocation());
    Destroy();
}

void AProjectileBase::OnLifespanExpired()
{
    PlayImpactEffects(GetActorLocation()); // 在生命周期结束时播放效果（可选）
    Destroy();
}

void AProjectileBase::ApplyHomingConfig()
{
    if (!ProjectileMovement)
    {
        return;
    }

    const AActor* HomingTarget = InitData.HomingTargetActor.Get();
    const bool bShouldHome = InitData.bEnableHoming && HomingTarget;

    ProjectileMovement->bIsHomingProjectile = bShouldHome;
    if (bShouldHome)
    {
        ProjectileMovement->HomingTargetComponent = HomingTarget->GetRootComponent();
        ProjectileMovement->HomingAccelerationMagnitude = InitData.HomingAcceleration;
    }
    else
    {
        ProjectileMovement->HomingTargetComponent = nullptr;
        ProjectileMovement->HomingAccelerationMagnitude = 0.f;
    }
}

void AProjectileBase::StartReturnToOwner()
{
    if (bIsReturning)
    {
        return;
    }

    if (!ProjectileOwner || !ProjectileMovement)
    {
        Destroy();
        return;
    }

    bIsReturning = true;
    TravelDistance = 0.0f;

    // 返航时强制朝向 Owner
    const FVector Dir = (ProjectileOwner->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    const float Speed = InitData.InitialSpeed * FMath::Max(0.1f, InitData.ReturnSpeedMultiplier);

    ProjectileMovement->Velocity = Dir * Speed;

    // 返航阶段一般不启用 homing（避免抖动）；如果需要也可以改成追踪 Owner
    ProjectileMovement->bIsHomingProjectile = false;
    ProjectileMovement->HomingTargetComponent = nullptr;
    ProjectileMovement->HomingAccelerationMagnitude = 0.f;
}
