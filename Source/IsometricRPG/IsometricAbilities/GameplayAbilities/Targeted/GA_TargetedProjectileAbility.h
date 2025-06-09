// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GameplayAbilities/Targeted/GA_TargetedAbility.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h"
#include "GA_TargetedProjectileAbility.generated.h"

/**
 * 指向投射物技能类
 * 选择敌人后发射追踪投射物
 * 示例：安妮的Q技能、导弹类技能等
 */
UCLASS()
class ISOMETRICRPG_API UGA_TargetedProjectileAbility : public UGA_TargetedAbility
{
	GENERATED_BODY()

public:
	UGA_TargetedProjectileAbility();

protected:
	// 要生成的投射物类
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeted|Projectile")
	TSubclassOf<class AProjectileBase> ProjectileClass;

	// 用于初始化投射物的数据
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeted|Projectile", meta = (ExposeOnSpawn = true))
	FProjectileInitializationData ProjectileData;

	// 投射物生成的插槽/位置
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Targeted|Projectile")
	FName MuzzleSocketName = FName("Muzzle");

	// 重写技能执行逻辑，使用投射物
	virtual void ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// 获取发射位置和旋转（朝向目标）
	virtual void GetLaunchTransform(const FGameplayEventData* TriggerEventData, const AActor* SourceActor, FVector& OutLocation, FRotator& OutRotation) const;
	
	// 生成并初始化投射物
	virtual class AProjectileBase* SpawnProjectile(
		const FVector& SpawnLocation, 
		const FRotator& SpawnRotation, 
		AActor* SourceActor, 
		APawn* SourcePawn,
		UAbilitySystemComponent* SourceASC,
		const FGameplayEventData* TriggerEventData);
};
