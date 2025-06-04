// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GA_ProjectileAbility.h
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GameplayAbilities/GA_HeroBaseAbility.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h" // 包含新的投射物基类
#include "GA_ProjectileAbility.generated.h"

/**
 * 投射物类型技能基类
 * 这种技能会生成并发射一个投射物，需要目标位置
 */
UCLASS()
class ISOMETRICRPG_API UGA_ProjectileAbility : public UGA_HeroBaseAbility
{
	GENERATED_BODY()

public:
	UGA_ProjectileAbility();

protected:
	// 要生成的投射物类，应该是 AProjectileBase 的子类
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<class AProjectileBase> ProjectileClass;

	// 用于初始化投射物的数据，可以在每个技能蓝图中配置
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Data", meta = (ExposeOnSpawn = true))
	FProjectileInitializationData ProjectileData;

	// 投射物生成的插槽/位置
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
	FName MuzzleSocketName = NAME_None;

	virtual void ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    
    // 获取发射位置和旋转
    virtual void GetLaunchTransform(const FGameplayEventData* TriggerEventData, const AActor* SourceActor, FVector& OutLocation, FRotator& OutRotation) const;
    
    // 生成并初始化投射物
    virtual class AProjectileBase* SpawnProjectile(
        const FVector& SpawnLocation, 
        const FRotator& SpawnRotation, 
        AActor* SourceActor, 
        APawn* SourcePawn,
        UAbilitySystemComponent* SourceASC // 添加SourceASC参数
		);
};
