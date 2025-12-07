// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\AN_PlayMeleeAttackMontageNotify.cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "AN_PlayMeleeAttackMontageNotify.h"
#include "Character/IsometricRPGCharacter.h"
#include "AbilitySystemInterface.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "GameplayEffectTypes.h"
void UAN_PlayMeleeAttackMontageNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    // 早期验证关键前提条件
    if (!MeshComp || !MeshComp->GetOwner())
    {
        UE_LOG(LogTemp, Warning, TEXT("Notify: Invalid MeshComp or Owner"));
        return;
    }

    AActor* Owner = MeshComp->GetOwner();
    AIsometricRPGCharacter* IsoCharacter = Cast<AIsometricRPGCharacter>(Owner);

    if (!IsoCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("Notify: Owner is not AIsometricRPGCharacter"));
        return;
    }

    // 检查关键组件和资源是否有效
    if (!MeleeAttackEffectClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Notify: MeleeAttackEffectClass is null"));
        return;
    }

    // 获取攻击者的能力系统组件
    UAbilitySystemComponent* SourceASC = IsoCharacter->GetAbilitySystemComponent();
    if (!SourceASC)
    {
        UE_LOG(LogTemp, Error, TEXT("Notify: Source AbilitySystemComponent not found"));
        return;
    }

    if (IsoCharacter->CurrentAbilityTargets.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Notify: No targets available for melee attack"));
        return;
	}
    TargetActor = Cast<AActor>(IsoCharacter->CurrentAbilityTargets[0].Get());
    // 验证目标是否有效
    if (!TargetActor)
    {
        UE_LOG(LogTemp, Error, TEXT("Notify: TargetActor is null"));
        return;
    }


    // 获取目标的能力系统组件
    UAbilitySystemComponent* TargetASC = nullptr;
    if (TargetActor->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
    {
        IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(TargetActor);
        if (AbilityInterface)
        {
            TargetASC = AbilityInterface->GetAbilitySystemComponent();
        }
    }
    if (!TargetASC)
    {
        TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
    }
    if (!TargetASC)
    {
        UE_LOG(LogTemp, Error, TEXT("Notify: Target AbilitySystemComponent not found on %s"), *TargetActor->GetName());
        return;
    }

    // 创建效果上下文
    FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
    ContextHandle.AddSourceObject(this); // 记录这个 Notify 作为来源

    // 应用伤害效果 (使用新的包含属性计算的方法)
    ApplyEffectToTarget(SourceASC, TargetASC, MeleeAttackEffectClass, EffectLevel, ContextHandle);

    // 应用击退效果 (如果有的话)
    if (PunchEffectClass)
    {
        // 对于击退效果，我们可以不计算伤害，直接应用
        FGameplayEffectSpecHandle PunchSpecHandle = SourceASC->MakeOutgoingSpec(PunchEffectClass, EffectLevel, ContextHandle);
        if (PunchSpecHandle.IsValid())
        {
            // 设置击退力度参数，解决 "magnitude had not yet been set by caller" 错误
            // 并允许通过 Notify 配置不同的击退力度
            FGameplayTag KnockbackForceTag = FGameplayTag::RequestGameplayTag(FName("Data.Ability.KnockBack.Force"));
            PunchSpecHandle.Data->SetSetByCallerMagnitude(KnockbackForceTag, KnockbackForce);

            SourceASC->ApplyGameplayEffectSpecToTarget(*PunchSpecHandle.Data.Get(), TargetASC);
        }
    }
}


void UAN_PlayMeleeAttackMontageNotify::ApplyEffectToTarget(
    UAbilitySystemComponent* SourceASC,
    UAbilitySystemComponent* TargetASC,
    TSubclassOf<UGameplayEffect> EffectClass,
    float Level,
    FGameplayEffectContextHandle& ContextHandle)
{
    if (!EffectClass || !SourceASC || !TargetASC)
    {
        return;
    }

    // 1. 读取攻击者的属性
    const float AttackDamage = SourceASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetAttackDamageAttribute());
    const float ArmorPenetration = SourceASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetArmorPenetrationAttribute());
    const float MagicPenetration = SourceASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetMagicPenetrationAttribute());
    const float ElementalPenetration = SourceASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetElementalPenetrationAttribute());
    const float CritChance = SourceASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetCriticalChanceAttribute());
    const float CritDamage = SourceASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetCriticalDamageAttribute());
    const float LifeStealPercent = SourceASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetLifeStealAttribute());
    const float ManaLeechPercent = SourceASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetManaLeechAttribute());

    // 2. 读取防御者的属性
    const float TargetPhysicalDefense = TargetASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetPhysicalDefenseAttribute());
    const float TargetMagicDefense = TargetASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetMagicDefenseAttribute());
    const float TargetPhysicalResist = TargetASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetPhysicalResistanceAttribute());
    const float TargetFireResist = TargetASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetFireResistanceAttribute());
    const float TargetIceResist = TargetASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetIceResistanceAttribute());
    const float TargetLightningResist = TargetASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetLightningResistanceAttribute());
    const float BlockChance = TargetASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetBlockChanceAttribute());
    const float BlockReduction = TargetASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetBlockDamageReductionAttribute());

    // 3. 计算最终伤害
    float FinalDamage = AttackDamage;

    auto ResolveResistance = [&](EIsoDamageType InType) -> float
    {
        switch (InType)
        {
        case EIsoDamageType::Magic:
            return TargetMagicDefense;
        case EIsoDamageType::Fire:
            return TargetFireResist;
        case EIsoDamageType::Ice:
            return TargetIceResist;
        case EIsoDamageType::Lightning:
            return TargetLightningResist;
        default:
            return TargetPhysicalResist > 0.f ? TargetPhysicalResist : TargetPhysicalDefense;
        }
    };

    auto ResolvePenetration = [&](EIsoDamageType InType) -> float
    {
        switch (InType)
        {
        case EIsoDamageType::Magic:
            return MagicPenetration;
        case EIsoDamageType::Fire:
        case EIsoDamageType::Ice:
        case EIsoDamageType::Lightning:
            return ElementalPenetration;
        default:
            return ArmorPenetration;
        }
    };

    const float EffectiveDefense = FMath::Max(0.0f, ResolveResistance(DamageType) - ResolvePenetration(DamageType));
    float DefenseFactor = 1.0f - (EffectiveDefense / 100.0f);  // 假设每100点防御减少100%伤害
    DefenseFactor = FMath::Clamp(DefenseFactor, 0.1f, 1.0f);  // 防御最多减少90%伤害
    FinalDamage *= DefenseFactor;

    // 4. 格挡：计算在暴击之前，格挡的减免效果
    const bool bBlocked = FMath::FRand() <= BlockChance;
    if (bBlocked)
    {
        const float BlockFactor = 1.0f - FMath::Clamp(BlockReduction, 0.0f, 0.9f);
        FinalDamage *= BlockFactor;
    }

    // 5. 应用暴击
    const bool IsCritical = (FMath::FRand() <= CritChance);
    if (IsCritical)
    {
        FinalDamage *= (1.0f + CritDamage);
        UE_LOG(LogTemp, Warning, TEXT("暴击! 伤害: %f"), FinalDamage);
    }
    FinalDamage = FMath::Max(0.f, FinalDamage);

    // 4. 创建GameplayEffectSpec并设置自定义伤害
    FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, Level, ContextHandle);
    if (SpecHandle.IsValid())
    {
        // 使用SetByCallerMagnitude设置自定义伤害值
        // 注意: 在GameplayEffect蓝图中需要有对应的SetByCaller Data Definition
        SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), -FinalDamage);

        // 5. 应用效果到目标
        FActiveGameplayEffectHandle ActiveGEHandle = SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

        // 6. 吸血 / 回蓝
        if (FinalDamage > 0.f)
        {
            if (LifeStealPercent > 0.f)
            {
                const float HealAmount = FinalDamage * LifeStealPercent;
                SourceASC->ApplyModToAttribute(UIsometricRPGAttributeSetBase::GetHealthAttribute(), EGameplayModOp::Additive, HealAmount);
            }

            if (ManaLeechPercent > 0.f)
            {
                const float ManaAmount = FinalDamage * ManaLeechPercent;
                SourceASC->ApplyModToAttribute(UIsometricRPGAttributeSetBase::GetManaAttribute(), EGameplayModOp::Additive, ManaAmount);
            }
        }

        // 7. 调试日志
        UE_LOG(LogTemp, Warning, TEXT("应用攻击效果 - 类型: %d 攻击力: %f, 穿透(物/魔/元): %f/%f/%f, 目标防御: %f, 最终伤害: %f, 格挡:%d"),
            static_cast<int32>(DamageType), AttackDamage, ArmorPenetration, MagicPenetration, ElementalPenetration, TargetPhysicalDefense, FinalDamage, bBlocked);
    }
}
