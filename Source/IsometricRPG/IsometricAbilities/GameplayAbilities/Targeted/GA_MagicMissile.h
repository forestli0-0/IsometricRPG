// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GameplayAbilities/Targeted/GA_TargetedProjectileAbility.h"
#include "GA_MagicMissile.generated.h"

/**
 * 魔法飞弹技能 - Targeted类型的投射物技能
 * 选择敌人目标，发射追踪飞弹
 */
UCLASS()
class ISOMETRICRPG_API UGA_MagicMissile : public UGA_TargetedProjectileAbility
{
	GENERATED_BODY()

public:
	UGA_MagicMissile();
};
