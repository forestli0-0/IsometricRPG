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
    PrimaryActorTick.TickGroup = TG_PrePhysics; // Tick before physics to update distance

    bReplicates = true;
    SetReplicateMovement(true);

    CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    CollisionComp->InitSphereRadius(15.0f);
    CollisionComp->SetCollisionProfileName("Projectile"); // 确保在项目中定义了这个碰撞配置
    CollisionComp->OnComponentHit.AddDynamic(this, &AProjectileBase::OnHit);
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
            true // autoDestroy
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
        ProjectileMovement->MaxSpeed = InitData.MaxSpeed > 0 ? InitData.MaxSpeed : InitData.InitialSpeed; // MaxSpeed为0则使用InitialSpeed
        ProjectileMovement->ProjectileGravityScale = InitData.GravityScale;
        ProjectileMovement->bShouldBounce = InitData.bShouldBounce;
        if (ProjectileMovement->Velocity.IsZero() && ProjectileMovement->InitialSpeed > 0.f)
        {
            // GetActorRotation() 应该能拿到生成时设置的 SpawnRotation
            FRotator CurrentActorRotation = GetActorRotation();
            ProjectileMovement->Velocity = CurrentActorRotation.Vector() * ProjectileMovement->InitialSpeed;
            UE_LOG(LogTemp, Warning, TEXT("AProjectileBase::InitializeProjectile - Manually set Velocity to: %s (Speed: %f) using ActorRotation: %s"),
                *ProjectileMovement->Velocity.ToString(),
                ProjectileMovement->Velocity.Size(),
                *CurrentActorRotation.ToString()
            );
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AProjectileBase::InitializeProjectile - Velocity was NOT zero (%s) or InitialSpeed was not positive (%f). Not setting velocity manually."),
                *ProjectileMovement->Velocity.ToString(),
                ProjectileMovement->InitialSpeed
            );
        }
    }


    if (VisualEffectComp && InitData.VisualEffect)
    {
        //if (UNiagaraComponent* NiagaraComp = Cast<UNiagaraComponent>(VisualEffectComp))
        //{
        //    NiagaraComp->SetAsset(InitData.VisualEffect);
        //    NiagaraComp->Activate();
        //}

    }
    else if (VisualEffectComp)
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
        true // autoDestroy
    );
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
    if (!OtherActor || OtherActor == ProjectileOwner || OtherActor == this)
    {
        // 忽略对自身或发射者的碰撞，除非特定逻辑允许 (例如，某些技能可能希望投射物能与发射者交互)
        return;
    }

    // 播放击中效果
    PlayImpactEffects(Hit.ImpactPoint);

    // 应用直接伤害
    ApplyDamageEffects(OtherActor, Hit);

    // 处理溅射伤害
    if (InitData.SplashRadius > 0.0f)
    {
        HandleSplashDamage(Hit.ImpactPoint, OtherActor);
    }

    // 销毁投射物
    // 可以根据InitData.bShouldBounce和其他逻辑决定是否立即销毁
    if (!InitData.bShouldBounce) // 假设ProjectileMovement有类似MaxBounceAmountBeforeDeath的属性
    {
        Destroy();
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
            nullptr, // Actor class filter
            TArray<AActor*>({ProjectileOwner, DirectHitActor}), // Actors to ignore (owner and direct hit target)
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
    if (InitData.MaxFlyDistance > 0.0f && TravelDistance >= InitData.MaxFlyDistance)
    {
        PlayImpactEffects(GetActorLocation()); // 在最大距离处播放效果（可选）
        Destroy();
    }
}

void AProjectileBase::OnLifespanExpired()
{
    PlayImpactEffects(GetActorLocation()); // 在生命周期结束时播放效果（可选）
    Destroy();
}
