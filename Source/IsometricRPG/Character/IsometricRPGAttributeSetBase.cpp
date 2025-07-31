// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/IsometricRPGAttributeSetBase.h"
#include "GameplayEffectExtension.h"
#include "IsometricAbilities/GameplayAbilities/Death/GA_Death.h"
#include "Gameframework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "IsoPlayerState.h"
#include "AbilitySystemBlueprintLibrary.h"
UIsometricRPGAttributeSetBase::UIsometricRPGAttributeSetBase()
{
    //Health.SetBaseValue(100.0f);  // 设置基础生命值为 100.0
    //MaxHealth.SetBaseValue(100.0f);  // 设置最大生命值为 100.0
    //HealthRegenRate.SetBaseValue(1.0f);  // 设置生命恢复速率为 1.0
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
   UE_LOG(LogTemp, Warning, TEXT("PostGameplayEffectExecute fired for attribute: %s"), *Data.EvaluatedData.Attribute.GetName());
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
               Payload.Instigator = SourceActor; // <-- 将击杀者放入包裹
               Payload.Target = GetOwningActor(); // 死者是自己
               // 你还可以放入其他数据，比如伤害量、伤害来源等
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

                // 4. 应用升级属性增益 GameplayEffect (需要在蓝图中创建)
                // 这个 GE 应该包含对 MaxHealth, MaxMana, AttackDamage 等属性的永久性提升
                AIsoPlayerState* PS = Cast<AIsoPlayerState>(OwningActor);
                if (PS)
                {
                    // 假设你在PlayerState中定义了一个LevelUpEffect
                    // TSubclassOf<UGameplayEffect> LevelUpEffect = PS->LevelUpEffect;
                    // if (LevelUpEffect)
                    // {
                    //     FGameplayEffectSpecHandle SpecHandle = GetOwningAbilitySystemComponent()->MakeOutgoingSpec(LevelUpEffect, NewLevel, Context);
                    //     GetOwningAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
                    // }
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
        // 属性集现在在PlayerState上，所以我们需要通过它来获取Character。
        // GetOwningAbilitySystemComponent() -> GetAvatarActor() 是获取与此ASC关联的Character的可靠方法。
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

    // 为新属性添加复制规则
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, Level, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, Experience, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UIsometricRPGAttributeSetBase, ExperienceToNextLevel, COND_None, REPNOTIFY_Always);
}


// 实现 OnRep 函数
void UIsometricRPGAttributeSetBase::OnRep_Level(const FGameplayAttributeData& OldLevel)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, Level, OldLevel);
    OnLevelChanged.Broadcast(this, GetLevel());
}

void UIsometricRPGAttributeSetBase::OnRep_Experience(const FGameplayAttributeData& OldExperience)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, Experience, OldExperience);
    OnExperienceChanged.Broadcast(this, GetExperience(), GetExperienceToNextLevel());
}

void UIsometricRPGAttributeSetBase::OnRep_ExperienceToNextLevel(const FGameplayAttributeData& OldExperienceToNextLevel)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UIsometricRPGAttributeSetBase, ExperienceToNextLevel, OldExperienceToNextLevel);
    OnExperienceChanged.Broadcast(this, GetExperience(), GetExperienceToNextLevel());
}


