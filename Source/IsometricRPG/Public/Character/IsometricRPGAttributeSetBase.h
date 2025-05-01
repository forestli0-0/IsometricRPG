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

/**
* 
*/
UCLASS()
class ISOMETRICRPG_API UIsometricRPGAttributeSetBase : public UAttributeSet
{
GENERATED_BODY()

public:
UIsometricRPGAttributeSetBase();

UPROPERTY(EditDefaultsOnly, Category = "GAS")
TSubclassOf<UGameplayEffect> DefaultAttributes; // 现在采用的是静态函数的方式来为Set初始化，这个是用不上了

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
FGameplayAttributeData Health;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, Health);

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
FGameplayAttributeData MaxHealth;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, MaxHealth);

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
FGameplayAttributeData HealthRegenRate;
ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, HealthRegenRate);

UPROPERTY(BlueprintAssignable)
FHealthChangedEvent OnHealthChanged;


public:
    static TSubclassOf<UGameplayEffect> GetDefaultInitGE();

    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

};