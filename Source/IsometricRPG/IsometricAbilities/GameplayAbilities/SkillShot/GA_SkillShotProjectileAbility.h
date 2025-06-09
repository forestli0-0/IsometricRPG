// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GameplayAbilities/SkillShot/GA_SkillShotAbility.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h"
#include "GA_SkillShotProjectileAbility.generated.h"

/**
 * 方向投射物技能类
 * 朝指定方向发射投射物，可以击中路径上的目标
 * 示例：EZ的Q技能、火球术、冰箭等
 */
UCLASS()
class ISOMETRICRPG_API UGA_SkillShotProjectileAbility : public UGA_SkillShotAbility
{
	GENERATED_BODY()

public:
	UGA_SkillShotProjectileAbility();

protected:
	// 要生成的投射物类
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillShot|Projectile")
	TSubclassOf<class AProjectileBase> ProjectileClass;

	// 用于初始化投射物的数据
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillShot|Projectile", meta = (ExposeOnSpawn = true))
	FProjectileInitializationData ProjectileData;

	// 投射物生成的插槽/位置
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SkillShot|Projectile")
	FName MuzzleSocketName = FName("Muzzle");

	// 重写技能执行逻辑，使用投射物
	virtual void ExecuteSkillShot(const FVector& Direction, const FVector& StartLocation) override;

	// 获取发射位置和旋转
	virtual void GetLaunchTransform(const FVector& Direction, const AActor* SourceActor, FVector& OutLocation, FRotator& OutRotation) const;
	
	// 生成并初始化投射物
	virtual class AProjectileBase* SpawnProjectile(
		const FVector& SpawnLocation, 
		const FRotator& SpawnRotation, 
		AActor* SourceActor, 
		APawn* SourcePawn,
		UAbilitySystemComponent* SourceASC);
};
