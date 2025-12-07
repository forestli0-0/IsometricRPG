// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "IsometricAbilities/Types/IsoDamageType.h"
#include "IsometricRPGAttributeSetBase.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHealthChangedEvent, UIsometricRPGAttributeSetBase*, AttributeSet, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FManaChangedEvent, UIsometricRPGAttributeSetBase*, AttributeSet, float, NewMana);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FExperienceChangedEvent, UIsometricRPGAttributeSetBase*, AttributeSet, float, NewExperience, float, NewMaxExperience);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLevelChangedEvent, UIsometricRPGAttributeSetBase*, AttributeSet, float, NewLevel);
/**
* 
*/
UCLASS()
class ISOMETRICRPG_API UIsometricRPGAttributeSetBase : public UAttributeSet
{
	GENERATED_BODY()

public:
	UIsometricRPGAttributeSetBase();

	// 核心属性
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Attributes|Core")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, MaxHealth);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Attributes|Core")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, Health);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_HealthRegenRate, Category = "Attributes|Core")
	FGameplayAttributeData HealthRegenRate;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, HealthRegenRate);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana, Category = "Attributes|Core")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, MaxMana);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category = "Attributes|Core")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, Mana);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ManaRegenRate, Category = "Attributes|Core")
	FGameplayAttributeData ManaRegenRate;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, ManaRegenRate);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "Attributes|Core")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, MoveSpeed);

	// 攻击属性
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_AttackRange, Category = "Attributes|Attack")
	FGameplayAttributeData AttackRange;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, AttackRange);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_AttackDamage, Category = "Attributes|Attack")
	FGameplayAttributeData AttackDamage;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, AttackDamage);

	/** Ability Power / Magic Damage scaling */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_AbilityPower, Category = "Attributes|Attack")
	FGameplayAttributeData AbilityPower;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, AbilityPower);

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_AttackSpeed, Category = "Attributes|Attack")
        FGameplayAttributeData AttackSpeed;
        ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, AttackSpeed);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalChance, Category = "Attributes|Attack")
	FGameplayAttributeData CriticalChance;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, CriticalChance);

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalDamage, Category = "Attributes|Attack")
        FGameplayAttributeData CriticalDamage;
        ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, CriticalDamage);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ArmorPenetration, Category = "Attributes|Attack")
	FGameplayAttributeData ArmorPenetration;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, ArmorPenetration);

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_MagicPenetration, Category = "Attributes|Attack")
        FGameplayAttributeData MagicPenetration;
        ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, MagicPenetration);

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ElementalPenetration, Category = "Attributes|Attack")
        FGameplayAttributeData ElementalPenetration;
        ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, ElementalPenetration);

        // 防御属性
        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalDefense, Category = "Attributes|Defense")
        FGameplayAttributeData PhysicalDefense;
        ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, PhysicalDefense);

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_MagicDefense, Category = "Attributes|Defense")
        FGameplayAttributeData MagicDefense;
        ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, MagicDefense);

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_LifeSteal, Category = "Attributes|Defense")
        FGameplayAttributeData LifeSteal;
        ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, LifeSteal);

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ManaLeech, Category = "Attributes|Defense")
        FGameplayAttributeData ManaLeech;
        ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, ManaLeech);

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_BlockChance, Category = "Attributes|Defense")
        FGameplayAttributeData BlockChance;
        ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, BlockChance);

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_BlockDamageReduction, Category = "Attributes|Defense")
        FGameplayAttributeData BlockDamageReduction;
        ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, BlockDamageReduction);

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalResistance, Category = "Attributes|Defense")
        FGameplayAttributeData PhysicalResistance;
        ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, PhysicalResistance);

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_FireResistance, Category = "Attributes|Defense")
        FGameplayAttributeData FireResistance;
        ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, FireResistance);

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_IceResistance, Category = "Attributes|Defense")
        FGameplayAttributeData IceResistance;
        ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, IceResistance);

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_LightningResistance, Category = "Attributes|Defense")
        FGameplayAttributeData LightningResistance;
        ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, LightningResistance);

        // 经验与等级
        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Level, Category = "Attributes|Experience")
	FGameplayAttributeData Level;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, Level);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Experience, Category = "Attributes|Experience")
	FGameplayAttributeData Experience;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, Experience);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ExperienceToNextLevel, Category = "Attributes|Experience")
	FGameplayAttributeData ExperienceToNextLevel;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, ExperienceToNextLevel);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ExperienceBounty, Category = "Attributes|Experience")
	FGameplayAttributeData ExperienceBounty;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, ExperienceBounty);
	// 经验曲线
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Experience")
	TObjectPtr<class UCurveFloat> ExperienceCurve;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Experience")
	TSubclassOf<UGameplayEffect> LevelUpEffectClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Experience")
	TObjectPtr<UDataTable> LevelUpTable;

	//~====================================================================================
	//~ 技能点
	//~====================================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_TotalSkillPoint, Category = "Attributes|SkillPoints")
	FGameplayAttributeData TotalSkillPoint;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, TotalSkillPoint);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_UnUsedSkillPoint, Category = "Attributes|SkillPoints")
	FGameplayAttributeData UnUsedSkillPoint;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, UnUsedSkillPoint);
public:
	//~====================================================================================
	//~ Public Methods
	//~====================================================================================
	
	static TSubclassOf<UGameplayEffect> GetDefaultInitGE();

	//~====================================================================================
	//~ Delegates
	//~====================================================================================

	UPROPERTY(BlueprintAssignable)
	FHealthChangedEvent OnHealthChanged;
	
	UPROPERTY(BlueprintAssignable)
	FManaChangedEvent OnManaChanged;

	UPROPERTY(BlueprintAssignable)
	FExperienceChangedEvent OnExperienceChanged;
	
	UPROPERTY(BlueprintAssignable)
	FLevelChangedEvent OnLevelChanged;
	//~====================================================================================
	//~ Overrides
	//~====================================================================================
	
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

protected:
    // 为新属性添加OnRep函数
    UFUNCTION() virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
	UFUNCTION() virtual void OnRep_Health(const FGameplayAttributeData& OldValue);
	UFUNCTION() virtual void OnRep_HealthRegenRate(const FGameplayAttributeData& OldValue);
	UFUNCTION() virtual void OnRep_MaxMana(const FGameplayAttributeData& OldValue);
	UFUNCTION() virtual void OnRep_Mana(const FGameplayAttributeData& OldValue);
	UFUNCTION() virtual void OnRep_ManaRegenRate(const FGameplayAttributeData& OldValue);
	UFUNCTION() virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION() virtual void OnRep_AttackRange(const FGameplayAttributeData& OldValue);
	UFUNCTION() virtual void OnRep_AttackDamage(const FGameplayAttributeData& OldValue);
	UFUNCTION() virtual void OnRep_AbilityPower(const FGameplayAttributeData& OldValue);
	UFUNCTION() virtual void OnRep_AttackSpeed(const FGameplayAttributeData& OldValue);
        UFUNCTION() virtual void OnRep_CriticalChance(const FGameplayAttributeData& OldValue);
        UFUNCTION() virtual void OnRep_CriticalDamage(const FGameplayAttributeData& OldValue);
        UFUNCTION() virtual void OnRep_ArmorPenetration(const FGameplayAttributeData& OldValue);
        UFUNCTION() virtual void OnRep_MagicPenetration(const FGameplayAttributeData& OldValue);
        UFUNCTION() virtual void OnRep_ElementalPenetration(const FGameplayAttributeData& OldValue);

        UFUNCTION() virtual void OnRep_PhysicalDefense(const FGameplayAttributeData& OldValue);
        UFUNCTION() virtual void OnRep_MagicDefense(const FGameplayAttributeData& OldValue);
        UFUNCTION() virtual void OnRep_LifeSteal(const FGameplayAttributeData& OldValue);
        UFUNCTION() virtual void OnRep_ManaLeech(const FGameplayAttributeData& OldValue);
        UFUNCTION() virtual void OnRep_BlockChance(const FGameplayAttributeData& OldValue);
        UFUNCTION() virtual void OnRep_BlockDamageReduction(const FGameplayAttributeData& OldValue);
        UFUNCTION() virtual void OnRep_PhysicalResistance(const FGameplayAttributeData& OldValue);
        UFUNCTION() virtual void OnRep_FireResistance(const FGameplayAttributeData& OldValue);
        UFUNCTION() virtual void OnRep_IceResistance(const FGameplayAttributeData& OldValue);
        UFUNCTION() virtual void OnRep_LightningResistance(const FGameplayAttributeData& OldValue);

	UFUNCTION() virtual void OnRep_ExperienceBounty(const FGameplayAttributeData& OldValue);
	UFUNCTION() virtual void OnRep_Level(const FGameplayAttributeData& OldValue);
	UFUNCTION() virtual void OnRep_Experience(const FGameplayAttributeData& OldValue);
	UFUNCTION() virtual void OnRep_ExperienceToNextLevel(const FGameplayAttributeData& OldValue);

	UFUNCTION() virtual void OnRep_TotalSkillPoint(const FGameplayAttributeData& OldValue);
	UFUNCTION() virtual void OnRep_UnUsedSkillPoint(const FGameplayAttributeData& OldValue);


};