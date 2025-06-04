// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h"
#include "ProjectileFireball.generated.h"

/**
 * 
 */
UCLASS()
class ISOMETRICRPG_API AProjectileFireball : public AProjectileBase
{
	GENERATED_BODY()
	
public:
	AProjectileFireball();
	
	// 火球灼烧效果 (持续伤害)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects")
	TSubclassOf<class UGameplayEffect> BurnEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects")
	float BurnDamagePerTick = 2.f;
	// 应用伤害效果
	virtual void ApplyDamageEffects(AActor* TargetActor, const FHitResult& HitResult) override;

	// 处理溅射伤害
	virtual void HandleSplashDamage(const FVector& ImpactLocation, AActor* DirectHitActor) override;
};
