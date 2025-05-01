// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/IsometricRPGAttributeSetBase.h"
#include "GameplayEffectExtension.h"
#include "IsometricAbilities/GA_Death.h"
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
   // 处理生命值变化
   if (Data.EvaluatedData.Attribute == GetHealthAttribute())
   {
       float NewHealth = GetHealth();
       float maxHealth = GetMaxHealth();
       // Clamp Health 在 0 到 MaxHealth 之间
       SetHealth(FMath::Clamp(NewHealth, 0.0f, GetMaxHealth()));
       auto temp = GetHealth();
       // 可以在这里添加血量为0时的逻辑（比如广播事件）
       if (GetHealth() <= 0.0f)
       {
           if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
           {
               ASC->TryActivateAbilityByClass(UGA_Death::StaticClass());
           }
       }
       // 触发生命值变化事件
       OnHealthChanged.Broadcast(this, GetHealth());
   }
}


