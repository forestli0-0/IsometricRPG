// Fill out your copyright notice in the Description page of Project Settings.
#include "IsometricAbilities/GA_FireballAbility.h"
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Engine/CollisionProfile.h"
#include "DrawDebugHelpers.h"

#include "Character/IsometricRPGAttributeSetBase.h" // 自定义属性集
#include "IsometricAbilities/Projectile_Fireball.h" // 自定义投射物类
#include "IsometricComponents/ActionQueueComponent.h" // 自定义动作队列组件


UGA_FireballAbility::UGA_FireballAbility()
{
    // 设置技能类型为投射物
    SkillType = EHeroSkillType::Projectile;

    // 设置技能CD
    CooldownDuration = 8.0f;

    // 设置技能范围
    RangeToApply = 500.0f;

    // 设置是否朝向目标
    bFaceTarget = true;

    // 加载施法动画蒙太奇
    static ConstructorHelpers::FObjectFinder<UAnimMontage> CastingMontageObj(TEXT("/Game/Characters/Animations/AM_CastFireball"));
    if (CastingMontageObj.Succeeded())
    {
        MontageToPlay = CastingMontageObj.Object;
    }

    // 加载火球投射物类蓝图
    static ConstructorHelpers::FClassFinder<AProjectile_Fireball> ProjectileClassObj(TEXT("/Game/Blueprint/Abilities/BP_Projectile_Fireball"));
    if (ProjectileClassObj.Succeeded())
    {
        ProjectileClass = ProjectileClassObj.Class;
    }

    // 加载火球伤害效果
    static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectObj(TEXT("/Game/Blueprint/GameEffects/GE_FireballDamage"));
    if (DamageEffectObj.Succeeded())
    {
        FireDamageEffect = DamageEffectObj.Class;
    }

    // 加载火球灼烧效果
    static ConstructorHelpers::FClassFinder<UGameplayEffect> BurnEffectObj(TEXT("/Game/Blueprint/GameEffects/GE_BurnDOT"));
    if (BurnEffectObj.Succeeded())
    {
        BurnEffect = BurnEffectObj.Class;
    }

    // 加载火球特效
    //static ConstructorHelpers::FObjectFinder<UParticleSystem> FireballVFXObj(TEXT("/Game/Effects/Particles/P_FireballTrail"));
    //if (FireballVFXObj.Succeeded())
    //{
    //    FireballEffect = FireballVFXObj.Object;
    //}

    // 加载爆炸特效
    static ConstructorHelpers::FObjectFinder<UParticleSystem> ExplosionVFXObj(TEXT("/Game/FX/Particles/P_Fireball_HitCharacter"));
    if (ExplosionVFXObj.Succeeded())
    {
        ExplosionEffect = ExplosionVFXObj.Object;
    }

    // 加载火球音效
    //static ConstructorHelpers::FObjectFinder<USoundBase> FireSoundObj(TEXT("/Game/Audio/S_FireballCast"));
    //if (FireSoundObj.Succeeded())
    //{
    //    FireballSound = FireSoundObj.Object;
    //}

    // 加载爆炸音效
    //static ConstructorHelpers::FObjectFinder<USoundBase> ExplosionSoundObj(TEXT("/Game/Audio/S_FireballExplosion"));
    //if (ExplosionSoundObj.Succeeded())
    //{
    //    ExplosionSound = ExplosionSoundObj.Object;
    //}

    // 设置技能的GameplayTag
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Q"));
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Q"));

    // 设置技能触发事件
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag("Ability.Q");
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);

    // 设定冷却阻断标签
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Cooldown.Ability.Q"));

    static ConstructorHelpers::FClassFinder<UGameplayEffect> CooldownEffectObj(TEXT("/Game/Blueprint/GameEffects/GE_Cooldown_Q"));
    if (CooldownEffectObj.Succeeded())
    {
        CooldownGameplayEffectClass = CooldownEffectObj.Class;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load CooldownEffect! Check path validity."));
        // 可考虑添加调试断言
        checkNoEntry();
    }
	CooldownDuration = 5.0f; // 设置冷却时间
}

void UGA_FireballAbility::ExecuteProjectile()
{
    // 获取施法者和目标位置
    AActor* SourceActor = GetAvatarActorFromActorInfo();
    if (!SourceActor)
        return;
    // 停止移动
	ACharacter* Character = Cast<ACharacter>(SourceActor);
	if (Character)
	{
        Character->GetController()->StopMovement();
	}


    AActor* TargetActor = nullptr;
    FVector TargetLocation;

    // 从 TriggerEventData 中获取目标位置
    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
    if (!ActorInfo)
        return;

    // 从ASC中获取AttributeSet
    if (!AttributeSet)
    {
        UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
        if (ASC)
        {
            AttributeSet = const_cast<UIsometricRPGAttributeSetBase*>(Cast<UIsometricRPGAttributeSetBase>(ASC->GetAttributeSet(UIsometricRPGAttributeSetBase::StaticClass())));
        }

        if (!AttributeSet)
            return;
    }

    // 获取施法位置 (角色位置 + 高度偏移)
    FVector SpawnLocation = SourceActor->GetActorLocation() + FVector(0, 0, 90.0f);


    if (Character)
    {
        UActionQueueComponent* ActionQueue = Character->FindComponentByClass<UActionQueueComponent>();
        if (ActionQueue)
        {
            FQueuedCommand CurrentCommand = ActionQueue->GetCommand();
            TargetLocation = CurrentCommand.TargetLocation;
            TargetActor = CurrentCommand.TargetActor.Get();
            // 清除技能命令
			ActionQueue->ClearCommand();
        }
    }

    if (TargetLocation.IsZero())
        return;

    // 计算发射方向
    FVector Direction = (TargetLocation - SpawnLocation).GetSafeNormal();

    // 播放施法音效
    if (FireballSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FireballSound, SpawnLocation);
    }

    // 生成火球投射物
    if (ProjectileClass)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        SpawnParams.Owner = SourceActor;
        SpawnParams.Instigator = Cast<APawn>(SourceActor);

        // 设置投射物生成变换
        FRotator ProjectileRotation = Direction.Rotation();

        // 生成投射物
        AProjectile_Fireball* FireballProjectile = GetWorld()->SpawnActor<AProjectile_Fireball>(
            ProjectileClass,
            SpawnLocation,
            ProjectileRotation,
            SpawnParams
        );

        if (FireballProjectile)
        {
            // 配置火球投射物
            FireballProjectile->SetOwner(SourceActor);
            FireballProjectile->SourceAbility = this;
            FireballProjectile->SourceASC = GetAbilitySystemComponentFromActorInfo();
            FireballProjectile->TargetLocation = TargetLocation;
            FireballProjectile->FireDamageEffect = FireDamageEffect;
            FireballProjectile->BurnEffect = BurnEffect;
            FireballProjectile->ExplosionEffect = ExplosionEffect;
            FireballProjectile->ExplosionSound = ExplosionSound;
            FireballProjectile->SplashRadius = SplashRadius;
            FireballProjectile->DamageMultiplier = 1.0f; // 主要伤害为100%

            // 从属性集获取法术伤害
            float SpellPower = AttributeSet->GetAttackDamage(); // 假设使用攻击力作为法术强度
            FireballProjectile->SpellPower = SpellPower;

            // 初始化投射物
            UProjectileMovementComponent* ProjectileMovement = FireballProjectile->FindComponentByClass<UProjectileMovementComponent>();
            if (ProjectileMovement)
            {
                ProjectileMovement->InitialSpeed = ProjectileSpeed;
                ProjectileMovement->MaxSpeed = ProjectileSpeed;
                ProjectileMovement->ProjectileGravityScale = 0.0f; // 无重力
            }

            // 添加火球轨迹特效
            if (FireballEffect)
            {
                FireballProjectile->SetFireballEffect(FireballEffect);
            }
        }
    }
}

void UGA_FireballAbility::ApplySplashDamage(const FVector& ImpactLocation, AActor* DirectHitActor)
{

    // 获取施法者的ASC
    UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
    if (!SourceASC)
        return;

    // 获取溅射范围内的所有角色
    TArray<AActor*> IgnoreActors;
    IgnoreActors.Add(GetAvatarActorFromActorInfo());

    // 如果有直接命中的目标，也将其添加到忽略列表，因为它会单独处理
    if (DirectHitActor)
    {
        IgnoreActors.Add(DirectHitActor);
    }
    TArray<FHitResult> HitResults;
    bool bHit = UKismetSystemLibrary::SphereTraceMulti(
        GetWorld(),
        ImpactLocation,
        ImpactLocation,
        SplashRadius,
        UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Pawn),
        false,
        IgnoreActors,
        EDrawDebugTrace::None,
        HitResults,
        true
    );

    if (bHit)
    {
        TSet<AActor*> DamagedActors; // 用于跟踪已造成伤害的Actor
        // 处理溅射范围内的所有角色
        for (const FHitResult& Hit : HitResults)
        {
            AActor* HitActor = Hit.GetActor();
            // 确保Actor有效，不是施法者自己，不是直接命中目标，并且尚未对该Actor造成过伤害
            if (HitActor && HitActor != GetAvatarActorFromActorInfo() && HitActor != DirectHitActor && !DamagedActors.Contains(HitActor))
            {
                ApplyDamageToTarget(HitActor, SplashDamageMultiplier);
                DamagedActors.Add(HitActor); // 将Actor添加到已处理列表
            }
        }
    }
}
// 新添加一个处理对单个目标应用伤害的辅助方法
void UGA_FireballAbility::ApplyDamageToTarget(AActor* TargetActor, float DamageMultiplier)
{
    if (!TargetActor)
        return;

    // 获取施法者的ASC
    UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
    if (!SourceASC)
        return;

    // 获取目标的ASC
    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
    if (!TargetASC)
        return;

    // 从属性集获取法术伤害
    float SpellPower = AttributeSet ? AttributeSet->GetAttackDamage() : 50.0f;

    // 应用直接伤害效果
    if (FireDamageEffect)
    {
        FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
        ContextHandle.AddSourceObject(this);

        FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(FireDamageEffect, GetAbilityLevel(), ContextHandle);
        if (SpecHandle.IsValid())
        {
            // 计算实际伤害
            float ActualDamage = SpellPower * DamageMultiplier;

            // 设置伤害值
            SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), -ActualDamage);

            // 应用效果
            SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

            // 调试日志
            UE_LOG(LogTemp, Display, TEXT("对 %s 造成 %.1f 伤害 (系数: %.2f)"),
                *TargetActor->GetName(), ActualDamage, DamageMultiplier);
        }
    }

    // 应用灼烧效果 (DOT)
    if (BurnEffect)
    {
        FGameplayEffectContextHandle BurnContextHandle = SourceASC->MakeEffectContext();
        BurnContextHandle.AddSourceObject(this);

        FGameplayEffectSpecHandle BurnSpecHandle = SourceASC->MakeOutgoingSpec(BurnEffect, GetAbilityLevel(), BurnContextHandle);
        if (BurnSpecHandle.IsValid())
        {
            // 设置灼烧每跳伤害
            float BurnDamagePerTick = -SpellPower * 0.1f; // 每跳10%法术强度的伤害
            BurnSpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage.PerTick")), BurnDamagePerTick);

            // 应用灼烧效果
            SourceASC->ApplyGameplayEffectSpecToTarget(*BurnSpecHandle.Data.Get(), TargetASC);
        }
    }
    // 应用冷却效果
    if (CooldownGameplayEffectClass)
    {
        FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownGameplayEffectClass, GetAbilityLevel());
        if (SpecHandle.Data.IsValid())
        {
            FGameplayEffectSpec& GESpec = *SpecHandle.Data.Get();

            // 获取你在 GE 中配置的 Data Tag
            FGameplayTag CooldownDurationTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Cooldown.Duration"));

            // 设置 Set by Caller 的值
            GESpec.SetSetByCallerMagnitude(CooldownDurationTag, CooldownDuration);

            UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
            ASC->ApplyGameplayEffectSpecToSelf(GESpec);
        }
    }
}