// Fill out your copyright notice in the Description page of Project Settings.

#include "GA_MagicMissile.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h"

UGA_MagicMissile::UGA_MagicMissile()
{
	// 设置技能类型为Targeted
	AbilityType = EHeroAbilityType::Targeted;
	
	// 设置通用投射物类（你可以创建专门的魔法飞弹类）
	ProjectileClass = AProjectileBase::StaticClass();
	
	// 配置投射物数据
	ProjectileData.MaxFlyDistance = 800.0f;
	ProjectileData.InitialSpeed = 1000.0f; // 快速飞行
	ProjectileData.MaxSpeed = 1000.0f; // 最大速度
	ProjectileData.DamageAmount = 80.0f;
	ProjectileData.SplashRadius = 0.0f; // 单体伤害，无爆炸
	ProjectileData.Lifespan = 2.0f;

	// 设置施法范围
	RangeToApply = 600.0f;
	
	// 需要目标选择
	bRequiresTargetData = true;
}
