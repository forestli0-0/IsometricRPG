// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "IsometricRPGAttributeSetBase.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

// Fix: Correct the type of the first parameter in the delegate declaration
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHealthChangedEvent, UIsometricRPGAttributeSetBase*, AttributeSet, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FManaChangedEvent, UIsometricRPGAttributeSetBase*, AttributeSet, float, NewMana);
/**
* 
*/
UCLASS()
class ISOMETRICRPG_API UIsometricRPGAttributeSetBase : public UAttributeSet
{
GENERATED_BODY()

public:
UIsometricRPGAttributeSetBase();

/** ----- 核心属性 ----- */

// 最大生命值
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
FGameplayAttributeData MaxHealth;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, MaxHealth);
// 当前生命值
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
FGameplayAttributeData Health;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, Health);
// 生命值恢复速率
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
FGameplayAttributeData HealthRegenRate;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, HealthRegenRate);

// 最大法力值
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
FGameplayAttributeData MaxMana;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, MaxMana);

// 当前法力值
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
FGameplayAttributeData Mana;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, Mana);

// 法力值恢复速率
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
FGameplayAttributeData ManaRegenRate;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, ManaRegenRate);

/** ----- 攻击属性 ----- */

// 攻击力
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
FGameplayAttributeData AttackDamage;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, AttackDamage);
// 攻击速度
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
FGameplayAttributeData AttackSpeed;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, AttackSpeed);
// 暴击几率
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
FGameplayAttributeData CriticalChance;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, CriticalChance);
// 暴击伤害
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
FGameplayAttributeData CriticalDamage;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, CriticalDamage);
// 物理穿透
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
FGameplayAttributeData ArmorPenetration;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, ArmorPenetration);
// 魔法穿透
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
FGameplayAttributeData MagicPenetration;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, MagicPenetration);

// 移动速度
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
FGameplayAttributeData MoveSpeed;

/** ----- 防御属性 ----- */
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, MoveSpeed);
// 物理防御
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
FGameplayAttributeData PhysicalDefense;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, PhysicalDefense);
// 魔法防御
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
FGameplayAttributeData MagicDefense;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, MagicDefense);
// 生命偷取
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
FGameplayAttributeData LifeSteal;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, LifeSteal);

/** ----- 经验与等级 ----- */

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
FGameplayAttributeData Experience;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, Experience);

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
FGameplayAttributeData Level;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, Level);


public:
    static TSubclassOf<UGameplayEffect> GetDefaultInitGE();

    UPROPERTY(BlueprintAssignable)
    FHealthChangedEvent OnHealthChanged;
    UPROPERTY(BlueprintAssignable)
    FManaChangedEvent OnManaChanged;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
};