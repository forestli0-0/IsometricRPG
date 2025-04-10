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

/**
 * 
 */
UCLASS()
class ISOMETRICRPG_API UIsometricRPGAttributeSetBase : public UAttributeSet
{
	GENERATED_BODY()

public:
	UIsometricRPGAttributeSetBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, Health);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, MaxHealth);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData HealthRegenRate;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, HealthRegenRate);
};