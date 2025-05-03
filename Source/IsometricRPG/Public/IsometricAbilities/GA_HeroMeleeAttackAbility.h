// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GA_HeroBaseAbility.h"
#include "GA_HeroMeleeAttackAbility.generated.h"

/**
 * 
 */
UCLASS()
class ISOMETRICRPG_API UGA_HeroMeleeAttackAbility : public UGA_HeroBaseAbility
{
	GENERATED_BODY()
public:
	UGA_HeroMeleeAttackAbility();
protected:
	// 重写目标型攻击
	virtual void ExecuteTargeted() override;

    // 攻击命中后使用的 GE
    UPROPERTY(EditDefaultsOnly, Category = "Ability")
    TSubclassOf<class UGameplayEffect> DamageEffect;

    // 命中范围（可选）
    UPROPERTY(EditDefaultsOnly, Category = "Ability")
    float AttackRange = 100.f;
};
