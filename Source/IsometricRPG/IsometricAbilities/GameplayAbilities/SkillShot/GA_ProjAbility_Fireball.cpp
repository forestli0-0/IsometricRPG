// Fill out your copyright notice in the Description page of Project Settings.

#include "IsometricAbilities/GameplayAbilities/SkillShot/GA_ProjAbility_Fireball.h"
#include "IsometricAbilities/Projectiles/ProjectileFireball.h"

UGA_ProjAbility_Fireball::UGA_ProjAbility_Fireball()
{
	// 设置技能类型为SkillShot
	AbilityType = EHeroAbilityType::SkillShot;
	
	// 设置火球投射物类
	ProjectileClass = AProjectileFireball::StaticClass();
	
	// 配置技能射击相关属性
	RangeToApply = 1200.0f; // 火球最大射程
	SkillShotWidth = 80.0f; // 火球宽度
	
	// 配置投射物数据
	ProjectileData.MaxFlyDistance = RangeToApply;
	ProjectileData.InitialSpeed = 800.0f; // 火球飞行速度
	ProjectileData.MaxSpeed = 800.0f; // 火球最大速度
	ProjectileData.DamageAmount = 100.0f;
	ProjectileData.SplashRadius = 150.0f; // 爆炸范围
	ProjectileData.Lifespan = 3.0f;
	
	// Fireball 直接消费当前瞄准方向，不进入交互式选目标流程。
	SetUsesInteractiveTargeting(false);

	// Fireball 走按下瞄准、释放提交的试点流程。
	InputPolicy.InputMode = EAbilityInputMode::PressAndRelease;
	InputPolicy.bUpdateTargetWhileHeld = true;
	InputPolicy.bAllowInputBuffer = true;
	InputPolicy.MaxBufferWindow = 0.3f;
}




