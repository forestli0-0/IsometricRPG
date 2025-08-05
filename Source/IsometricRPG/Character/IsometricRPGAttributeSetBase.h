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

	//~====================================================================================
	//~ 核心属性
	//~====================================================================================

	// 最大生命值
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes|Core")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, MaxHealth);

	// 当前生命值
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes|Core")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, Health);

	// 生命值恢复速率
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes|Core")
	FGameplayAttributeData HealthRegenRate;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, HealthRegenRate);

	// 最大法力值
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Core")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, MaxMana);

	// 当前法力值
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Core")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, Mana);

	// 法力值恢复速率
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Core")
	FGameplayAttributeData ManaRegenRate;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, ManaRegenRate);
	
	// 移动速度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Core")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, MoveSpeed);

	//~====================================================================================
	//~ 攻击属性
	//~====================================================================================

	// 攻击范围
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Attack")
	FGameplayAttributeData AttackRange;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, AttackRange);

	// 攻击力
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Attack")
	FGameplayAttributeData AttackDamage;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, AttackDamage);

	// 攻击速度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Attack")
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, AttackSpeed);

	// 暴击几率
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Attack")
	FGameplayAttributeData CriticalChance;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, CriticalChance);

	// 暴击伤害
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Attack")
	FGameplayAttributeData CriticalDamage;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, CriticalDamage);

	// 物理穿透
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Attack")
	FGameplayAttributeData ArmorPenetration;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, ArmorPenetration);

	// 魔法穿透
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Attack")
	FGameplayAttributeData MagicPenetration;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, MagicPenetration);

	//~====================================================================================
	//~ 防御属性
	//~====================================================================================
	
	// 物理防御
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Defense")
	FGameplayAttributeData PhysicalDefense;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, PhysicalDefense);

	// 魔法防御
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Defense")
	FGameplayAttributeData MagicDefense;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, MagicDefense);

	// 生命偷取
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Defense")
	FGameplayAttributeData LifeSteal;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, LifeSteal);

	//~====================================================================================
	//~ 经验与等级
	//~====================================================================================
	// 当前经验值
	// 当前等级
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Level, Category = "Attributes|Experience")
	FGameplayAttributeData Level;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, Level);

	// 当前经验值
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Experience, Category = "Attributes|Experience")
	FGameplayAttributeData Experience;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, Experience);

	// 升级所需经验
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ExperienceToNextLevel, Category = "Attributes|Experience")
	FGameplayAttributeData ExperienceToNextLevel;
	ATTRIBUTE_ACCESSORS(UIsometricRPGAttributeSetBase, ExperienceToNextLevel);

	// 击杀该单位可获得的经验奖励
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Experience")
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|SkillPoints")
	float TotalSkillPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|SkillPoints")
	float UnUsedSkillPoint;
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
    UFUNCTION()
    virtual void OnRep_Level(const FGameplayAttributeData& OldLevel);

    UFUNCTION()
    virtual void OnRep_Experience(const FGameplayAttributeData& OldExperience);

    UFUNCTION()
    virtual void OnRep_ExperienceToNextLevel(const FGameplayAttributeData& OldExperienceToNextLevel);

};