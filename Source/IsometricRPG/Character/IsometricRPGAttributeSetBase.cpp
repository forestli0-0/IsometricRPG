// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/IsometricRPGAttributeSetBase.h"
#include "GameplayEffectExtension.h"
#include "IsometricAbilities/GameplayAbilities/Death/GA_Death.h"
#include "Gameframework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "IsoPlayerState.h"
#include "AbilitySystemBlueprintLibrary.h"
#include <LevelUp/LevelUpData.h>
UIsometricRPGAttributeSetBase::UIsometricRPGAttributeSetBase()
{

}

TSubclassOf<UGameplayEffect> UIsometricRPGAttributeSetBase::GetDefaultInitGE()
{
    static ConstructorHelpers::FClassFinder<UGameplayEffect> GEClass(TEXT("/Game/Blueprint/GameEffects/GE_InitializeAttributes"));
    return GEClass.Class;
}

void UIsometricRPGAttributeSetBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
   Super::PostGameplayEffectExecute(Data);
   // 添加一个日志，打印出每次被修改的属性
   //UE_LOG(LogTemp, Warning, TEXT("PostGameplayEffectExecute fired for attribute: %s"), *Data.EvaluatedData.Attribute.GetName());
   // 获取效果上下文，这是找到击杀者的关键
   FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
   UAbilitySystemComponent* SourceASC = Context.GetOriginalInstigatorAbilitySystemComponent();
   AActor* SourceActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;

   // 处理生命值变化
   if (Data.EvaluatedData.Attribute == GetHealthAttribute())
   {
       SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));

       if (GetHealth() <= 0.0f)
       {
           UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent();
           if (TargetASC)
           {
               // 1. 定义事件标签
               FGameplayTag DeathEventTag = FGameplayTag::RequestGameplayTag(FName("Event.Character.Death"));
               // 2. 创建事件数据 "包裹"
               FGameplayEventData Payload;
               Payload.EventTag = DeathEventTag;
               Payload.Instigator = SourceActor;
               Payload.Target = GetOwningActor(); // 死亡技能的目标是自己
               Payload.EventMagnitude = Data.EvaluatedData.Magnitude;
               // 3. 发送事件给目标ASC (也就是死者自己)
               // HandleGameplayEvent 会自动查找并激活监听此Tag的技能
               TargetASC->HandleGameplayEvent(DeathEventTag, &Payload);
           }
       }
       // 触发生命值变化事件
       OnHealthChanged.Broadcast(this, GetHealth());
   }
    // 处理法力值变化
    if (Data.EvaluatedData.Attribute == GetManaAttribute())
    {
        float NewMana = GetMana();
        // Clamp Mana 在 0 到 MaxMana 之间
        SetMana(FMath::Clamp(NewMana, 0.0f, GetMaxMana()));
        // 触发法力值变化事件
        OnManaChanged.Broadcast(this, GetMana());
    }
    // 处理经验值变化
    if (Data.EvaluatedData.Attribute == GetExperienceAttribute())
    {
        // 确保经验值不为负
        SetExperience(FMath::Max(0.f, GetExperience()));

        // 广播经验变化事件
        OnExperienceChanged.Broadcast(this, GetExperience(), GetExperienceToNextLevel());

        // 检查是否可以升级 (只在服务器上执行)
        AActor* OwningActor = GetOwningActor();
        if (OwningActor && OwningActor->GetLocalRole() == ROLE_Authority)
        {
            while (GetExperience() >= GetExperienceToNextLevel() && GetExperienceToNextLevel() > 0)
            {
                // 1. 升级！
                const float OldLevel = GetLevel();
                const float NewLevel = OldLevel + 1.f;
                SetLevel(NewLevel);

                // 2. 将溢出经验带到下一级
                const float ExpOver = GetExperience() - GetExperienceToNextLevel();



                SetExperience(ExpOver);

                // 3. 从曲线表计算新的升级所需经验
                if (ExperienceCurve)
                {
                    // 直接从一个曲线中，根据等级(NewLevel)作为Time，来评估(Eval)出对应的经验值(Value)
                    const float NewExpToNextLevel = ExperienceCurve->GetFloatValue(NewLevel);
                    SetExperienceToNextLevel(NewExpToNextLevel);
                }

                // 4. 应用升级属性增益 GameplayEffect


                UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
                if (ASC)
                {
                        if (LevelUpTable)
                        {
                            // 定义ContextString
                            static const FString ContextString(TEXT("IsometricRPGAttributeSetBase::LevelUp"));

                            FString RowName = FString::FromInt(NewLevel);
                            // FindRow模板参数写法修正，且ContextString已定义
                            FLevelUpData* RowData = LevelUpTable->FindRow<FLevelUpData>(FName(*RowName), ContextString);
                            if (RowData)
                            {
                                if (LevelUpEffectClass)
                                {
                                    FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
                                    ContextHandle.AddSourceObject(OwningActor);
                                    FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(LevelUpEffectClass, NewLevel, ContextHandle);
                            
                                    // 4.4 使用SetByCaller将表格中的数据填入GE Spec
                                    SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.LevelUp.MaxHealth")), RowData->MaxHealthGain);
                                    SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.LevelUp.MaxMana")), RowData->MaxManaGain);
                                    SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.LevelUp.AttackDamage")), RowData->AttackDamageGain);
                                    SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.LevelUp.PhysicalDefense")), RowData->PhysicalDefenseGain);
                                    SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.LevelUp.MagicDefense")), RowData->MagicDefenseGain);
                            
                                    // 应用这个填满了数据的GE Spec
                                    ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
                            
                                    UE_LOG(LogTemp, Warning, TEXT("Level Up to %d! Health Gain: %f, Damage Gain: %f"), (int32)NewLevel, RowData->MaxHealthGain, RowData->AttackDamageGain);
                            
                                    // 升级后自动回满血蓝
                                    SetHealth(GetMaxHealth());
                                    SetMana(GetMaxMana());
                                }
                            }
                    }
                }
                // 5. 广播等级变化事件
                OnLevelChanged.Broadcast(this, NewLevel);

                // 广播经验变化事件 (因为最大值和当前值都变了)
                OnExperienceChanged.Broadcast(this, GetExperience(), GetExperienceToNextLevel());
            }
        }
    }
}

void UIsometricRPGAttributeSetBase::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    if (Attribute == GetMoveSpeedAttribute())
    {
        if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
        {
            if (ACharacter* OwnerCharacter = Cast<ACharacter>(ASC->GetAvatarActor()))
            {
                if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
                {
                    MovementComponent->MaxWalkSpeed = NewValue;
                }
            }
        }
    }
}
// 添加网络复制规则
void UIsometricRPGAttributeSetBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Core
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, HealthRegenRate, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, MaxMana, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, Mana, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, ManaRegenRate, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, MoveSpeed, COND_None, REPNOTIFY_Always);

    // Attack
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, AttackRange, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, AttackDamage, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, AttackSpeed, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, CriticalChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, CriticalDamage, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, ArmorPenetration, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, MagicPenetration, COND_None, REPNOTIFY_Always);

    // Defense
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, PhysicalDefense, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, MagicDefense, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, LifeSteal, COND_None, REPNOTIFY_Always);

    // Level/Experience（已存在）
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, Level, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, Experience, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, ExperienceToNextLevel, COND_None, REPNOTIFY_Always);

    // Bounty
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, ExperienceBounty, COND_None, REPNOTIFY_Always);
}

// OnRep 实现（统一使用宏）
void UIsometricRPGAttributeSetBase::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, MaxHealth, OldValue); }
void UIsometricRPGAttributeSetBase::OnRep_Health(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, Health, OldValue);
    OnHealthChanged.Broadcast(this, GetHealth());
}
void UIsometricRPGAttributeSetBase::OnRep_HealthRegenRate(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, HealthRegenRate, OldValue); }
void UIsometricRPGAttributeSetBase::OnRep_MaxMana(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, MaxMana, OldValue); }
void UIsometricRPGAttributeSetBase::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, Mana, OldValue);
    OnManaChanged.Broadcast(this, GetMana());
}
void UIsometricRPGAttributeSetBase::OnRep_ManaRegenRate(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, ManaRegenRate, OldValue); }
void UIsometricRPGAttributeSetBase::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, MoveSpeed, OldValue); }

void UIsometricRPGAttributeSetBase::OnRep_AttackRange(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, AttackRange, OldValue); }
void UIsometricRPGAttributeSetBase::OnRep_AttackDamage(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, AttackDamage, OldValue); }
void UIsometricRPGAttributeSetBase::OnRep_AttackSpeed(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, AttackSpeed, OldValue); }
void UIsometricRPGAttributeSetBase::OnRep_CriticalChance(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, CriticalChance, OldValue); }
void UIsometricRPGAttributeSetBase::OnRep_CriticalDamage(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, CriticalDamage, OldValue); }
void UIsometricRPGAttributeSetBase::OnRep_ArmorPenetration(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, ArmorPenetration, OldValue); }
void UIsometricRPGAttributeSetBase::OnRep_MagicPenetration(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, MagicPenetration, OldValue); }

void UIsometricRPGAttributeSetBase::OnRep_PhysicalDefense(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, PhysicalDefense, OldValue); }
void UIsometricRPGAttributeSetBase::OnRep_MagicDefense(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, MagicDefense, OldValue); }
void UIsometricRPGAttributeSetBase::OnRep_LifeSteal(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, LifeSteal, OldValue); }

void UIsometricRPGAttributeSetBase::OnRep_ExperienceBounty(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, ExperienceBounty, OldValue); }
void UIsometricRPGAttributeSetBase::OnRep_Level(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, Level, OldValue);
    OnLevelChanged.Broadcast(this, GetLevel());
}

void UIsometricRPGAttributeSetBase::OnRep_Experience(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, Experience, OldValue);
    OnExperienceChanged.Broadcast(this, GetExperience(), GetExperienceToNextLevel());
}

void UIsometricRPGAttributeSetBase::OnRep_ExperienceToNextLevel(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, ExperienceToNextLevel, OldValue);

}
void UIsometricRPGAttributeSetBase::OnRep_TotalSkillPoint(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, TotalSkillPoint, OldValue); }
void UIsometricRPGAttributeSetBase::OnRep_UnUsedSkillPoint(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, UnUsedSkillPoint, OldValue); }


