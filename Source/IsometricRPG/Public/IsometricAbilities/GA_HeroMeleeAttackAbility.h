// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GA_HeroBaseAbility.h"
#include "GA_HeroMeleeAttackAbility.generated.h"

class UIsometricRPGAttributeSetBase;
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
};
