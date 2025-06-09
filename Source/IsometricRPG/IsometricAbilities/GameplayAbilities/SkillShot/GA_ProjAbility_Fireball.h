// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GameplayAbilities/SkillShot/GA_SkillShotProjectileAbility.h"
#include "GA_ProjAbility_Fireball.generated.h"

/**
 * 火球术技能 - SkillShot类型的投射物技能
 * 朝鼠标方向发射火球，可以击中路径上的敌人
 */
UCLASS()
class ISOMETRICRPG_API UGA_ProjAbility_Fireball : public UGA_SkillShotProjectileAbility
{
	GENERATED_BODY()

public:
	UGA_ProjAbility_Fireball();
};
