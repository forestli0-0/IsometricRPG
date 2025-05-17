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

    // 设置法力消耗的默认值
    ManaCost = 20.0f; 

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
	CooldownDuration = ThisCooldownDuration; // 设置冷却时间
	// 设置技能消耗效果类
    static ConstructorHelpers::FClassFinder<UGameplayEffect> CostEffectObj(TEXT("/Game/Blueprint/GameEffects/GE_FireballManaCost"));
    if (CostEffectObj.Succeeded())
    {
        CostGameplayEffectClass = CostEffectObj.Class;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load CostGameplayEffect! Check path validity."));
        // 可考虑添加调试断言
        checkNoEntry();
    }
}

// 实现 ApplyCost 方法以应用自定义法力消耗
void UGA_FireballAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{    // 检查蓝是否足够
    if (AttributeSet && AttributeSet->GetMana() < ManaCost)
    {
        // 蓝不足时，不应用消耗GE，也不调用父类
        UE_LOG(LogTemp, Warning, TEXT("UGA_FireballAbility::ApplyCost - Not enough mana, skipping cost application."));
        return;
    }
    if (CostGameplayEffectClass)
    {
        if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
        {
            UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();
            FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CostGameplayEffectClass, GetAbilityLevel());

            if (SpecHandle.Data.IsValid())
            {
                FGameplayEffectSpec& GESpec = *SpecHandle.Data.Get();

                // 这个 GameplayTag 必须与你的 CostGameplayEffectClass (例如 GE_FireballManaCost) 中的
                // SetByCaller_Magnitude 相匹配，该 GE 用于减少法力值。
                FGameplayTag ManaMagnitudeTag = FGameplayTag::RequestGameplayTag(FName("Data.Cost.Mana"));
                
                // 设置法力消耗的量。确保 ManaCost 是正值。
                GESpec.SetSetByCallerMagnitude(ManaMagnitudeTag, -ManaCost); 

                UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
                ASC->ApplyGameplayEffectSpecToSelf(GESpec);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UGA_FireballAbility::ApplyCost - ActorInfo or ASC is invalid."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UGA_FireballAbility::ApplyCost - CostGameplayEffectClass is not set. Calling Super::ApplyCost."));
        Super::ApplyCost(Handle, ActorInfo, ActivationInfo); // 如果没有指定消耗GE，则调用基类实现
    }
}

// 覆盖以实现自定义消耗检查
bool UGA_FireballAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    // 首先调用父类的CheckCost，确保其他潜在的消耗检查（如冷却）通过
    if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
    {
        return false;
    }

    // 确保ActorInfo和AbilitySystemComponent有效
    if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("UGA_FireballAbility::CheckCost - ActorInfo or ASC is invalid."));
        return false; 
    }

    const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    
    // 获取自定义属性集
    const UIsometricRPGAttributeSetBase* MyAttributeSet = Cast<const UIsometricRPGAttributeSetBase>(ASC->GetAttributeSet(UIsometricRPGAttributeSetBase::StaticClass()));

    if (!MyAttributeSet)
    {
        UE_LOG(LogTemp, Warning, TEXT("UGA_FireballAbility::CheckCost - UIsometricRPGAttributeSetBase not found on ASC. Cannot check mana cost."));
        // 根据游戏逻辑，如果找不到属性集，可能意味着无法执行消耗检查
        // 这里我们严格处理，如果无法检查则视为失败
        if (OptionalRelevantTags)
        {
            OptionalRelevantTags->AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Failed.InternalError.MissingAttributes")));
        }
        return false;
    }

    // 获取当前法力值 (假设您的属性集有 GetMana() 方法并且是const的)
    float CurrentMana = MyAttributeSet->GetMana(); 

    // 检查法力是否足够
    // ManaCost 是 UGA_FireballAbility 的成员变量
    if (CurrentMana < ManaCost)
    {
        if (OptionalRelevantTags)
        {
            // 添加一个通用的消耗失败标签和更具体的法力不足标签
            OptionalRelevantTags->AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Failed.Cost")));
            OptionalRelevantTags->AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Failed.NotEnoughMana")));
        }
        // 主动触发事件，通知ASC和动作队列（ActionQueueComponent）
        FGameplayEventData FailEventData;
        FailEventData.EventTag = FGameplayTag::RequestGameplayTag(FName("Ability.Failed.Cost"));
        FailEventData.Instigator = GetAvatarActorFromActorInfo();
        FailEventData.Target = GetAvatarActorFromActorInfo();
        UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
        if (MyASC)
        {
            MyASC->HandleGameplayEvent(FailEventData.EventTag, &FailEventData);
        }
        UE_LOG(LogTemp, Log, TEXT("%s CheckCost failed due to insufficient mana. Required: %.2f, Current: %.2f"), *GetName(), ManaCost, CurrentMana);
        return false;
    }

    // 如果所有检查都通过
    return true;
}

void UGA_FireballAbility::ExecuteProjectile()
{
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
    FVector AimDirectionTarget = TargetLocation;
    // 将目标点的Z轴调整为与生成点一致，以获得更水平的发射方向
    AimDirectionTarget.Z = SpawnLocation.Z; 

    FVector Direction = (AimDirectionTarget - SpawnLocation);

    // 如果目标点与生成点在XY平面上非常接近（例如，点击角色脚下），则默认向前发射
    if (Direction.SizeSquared2D() < 1.0f) // 使用一个小的阈值，例如1.0平方单位
    {
        Direction = SourceActor->GetActorForwardVector();
    }
    else
    {
        Direction = Direction.GetSafeNormal();
    }
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
            FireballProjectile->FireDamageEffect = FireDamageEffect;
            FireballProjectile->BurnEffect = BurnEffect;
            FireballProjectile->ExplosionEffect = ExplosionEffect;
            FireballProjectile->ExplosionSound = ExplosionSound;
            FireballProjectile->SplashRadius = SplashRadius;
            FireballProjectile->DamageMultiplier = 1.0f; // 主要伤害为100%
            // 火球的目标位置应该是施法距离加上火球的速度
            //FireballProjectile->TargetLocation = SpawnLocation + Direction * ProjectileFlyDistance;

            UE_LOG(LogTemp, Warning, TEXT("GA_FireballAbility: ProjectileFlyDistance = %f, ProjectileSpeed = %f"), ProjectileFlyDistance, ProjectileSpeed);
            float CalculatedLifeSpan = 0.0f;
            if (ProjectileSpeed == 0.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("GA_FireballAbility: ProjectileSpeed is ZERO! Projectile will use constructor default LifeSpan or BP override if any."));
            }
            else
            {
                CalculatedLifeSpan = ProjectileFlyDistance / ProjectileSpeed;
            }
			// 设置投射物的生命期
            FireballProjectile->SetLifeSpan(CalculatedLifeSpan); 
            
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

}