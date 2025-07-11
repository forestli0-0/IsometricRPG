// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\AN_PlayMeleeAttackMontageNotify.cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "AN_PlayMeleeAttackMontageNotify.h"
#include "Character/IsometricRPGCharacter.h"
#include "AbilitySystemInterface.h"
#include "Character/IsometricRPGAttributeSetBase.h"
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
    float AttackDamage = SourceASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetAttackDamageAttribute());
    float ArmorPenetration = SourceASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetArmorPenetrationAttribute());
    float CritChance = SourceASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetCriticalChanceAttribute());
    float CritDamage = SourceASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetCriticalDamageAttribute());

    // 2. 读取防御者的属性
    float TargetPhysicalDefense = TargetASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetPhysicalDefenseAttribute());

    // 3. 计算最终伤害
    float FinalDamage = AttackDamage;

    // 应用护甲穿透 (简单公式: 最终伤害 = 攻击力 * (1 - max(0, (防御力 - 护甲穿透)) / 100))
    float EffectiveDefense = FMath::Max(0.0f, TargetPhysicalDefense - ArmorPenetration);
    float DefenseFactor = 1.0f - (EffectiveDefense / 100.0f);  // 假设每100点防御减少100%伤害
    DefenseFactor = FMath::Clamp(DefenseFactor, 0.1f, 1.0f);  // 防御最多减少90%伤害
    FinalDamage *= DefenseFactor;

    // 应用暴击
    bool IsCritical = (FMath::FRand() <= CritChance);
    if (IsCritical)
    {
        FinalDamage *= (1.0f + CritDamage);
        FinalDamage = FinalDamage;
        UE_LOG(LogTemp, Warning, TEXT("暴击! 伤害: %f"), FinalDamage);
    }

    // 4. 创建GameplayEffectSpec并设置自定义伤害
    FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, Level, ContextHandle);
    if (SpecHandle.IsValid())
    {
        // 使用SetByCallerMagnitude设置自定义伤害值
        // 注意: 在GameplayEffect蓝图中需要有对应的SetByCaller Data Definition
        SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), -FinalDamage);

        // 5. 应用效果到目标
        FActiveGameplayEffectHandle ActiveGEHandle = SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

        // 6. 调试日志
        UE_LOG(LogTemp, Warning, TEXT("应用攻击效果 - 攻击力: %f, 护甲穿透: %f, 目标防御: %f, 最终伤害: %f"),
            AttackDamage, ArmorPenetration, TargetPhysicalDefense, FinalDamage);
    }
}
