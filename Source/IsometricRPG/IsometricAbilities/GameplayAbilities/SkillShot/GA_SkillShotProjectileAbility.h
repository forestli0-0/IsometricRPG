// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GameplayAbilities/SkillShot/GA_SkillShotAbility.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h"
#include "Misc/DataValidation.h"
#include "GA_SkillShotProjectileAbility.generated.h"

struct FPropertyChangedEvent;

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
	// 历史兼容字段。旧蓝图可能仍然把投射物类配在这里，加载后会迁移到 SkillShotProjectileClass。
	UPROPERTY(EditDefaultsOnly, Category = "Deprecated", meta = (DeprecatedProperty, DeprecationMessage = "Use SkillShotProjectileClass on the base skill-shot ability instead.", EditCondition = "false", EditConditionHides))
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

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	/** 统一读取当前技能实际使用的投射物类。 */
	TSubclassOf<class AProjectileBase> GetConfiguredProjectileClass() const;

	/** 将旧蓝图里的 ProjectileClass 迁移到 SkillShotProjectileClass。 */
	void SynchronizeProjectileClassFields();
};
