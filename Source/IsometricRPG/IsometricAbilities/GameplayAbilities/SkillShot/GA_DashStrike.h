// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GameplayAbilities/SkillShot/GA_SkillShotAbility.h"
#include "GameplayEffect.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"
#include "GA_DashStrike.generated.h"

/**
 * 突进冲撞技能 - SkillShot类型
 * 朝指定方向快速突进，撞击路径上的敌人并造成击飞效果
 * 类似于英雄联盟中剑圣的阿尔法突击或盲僧的天音波
 */
UCLASS()
class ISOMETRICRPG_API UGA_DashStrike : public UGA_SkillShotAbility
{
	GENERATED_BODY()

public:
	UGA_DashStrike();

protected:
	// 突进距离
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float DashDistance = 600.0f;

	// 突进速度 (单位/秒)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float DashSpeed = 1200.0f;

	// 撞击伤害
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float DamageMagnitude = 10.0f;
	// 碰撞检测半径
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float CollisionRadius = 80.0f;

	// 对敌人造成的伤害效果
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TSubclassOf<UGameplayEffect> DamageEffect;

	// 击飞效果
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TSubclassOf<UGameplayEffect> KnockbackEffect;

	// 突进过程中的无敌效果（可选）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TSubclassOf<UGameplayEffect> InvulnerabilityEffect;

	// 突进特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	TSubclassOf<class ANiagaraActor> DashEffectClass;

	// 击中敌人的特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	TSubclassOf<class ANiagaraActor> HitEffectClass;

	// 突进声音
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	class USoundBase* DashSound;

	// 击中敌人的声音
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	class USoundBase* HitSound;

	virtual void ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

	// 执行突进
	UFUNCTION()
	virtual void ExecuteDash(const FVector& DashDirection, const FVector& StartLocation);

	// 突进过程中的碰撞检测
	UFUNCTION()
	virtual void PerformDashCollisionCheck(const FVector& CurrentLocation, const FVector& PreviousLocation);

	// 处理击中敌人
	UFUNCTION()
	virtual void OnHitEnemy(AActor* HitActor, const FVector& HitLocation);

	// 突进完成
	UFUNCTION()
	virtual void OnDashComplete();

	// 突进计时器
	UPROPERTY()
	FTimerHandle DashTimerHandle;

	// 存储突进相关数据
	UPROPERTY()
	FVector DashTargetLocation;

	UPROPERTY()
	FVector DashStartLocation;

	UPROPERTY()
	FVector CurrentDashDirection;

	UPROPERTY()
	TArray<AActor*> HitActors; // 防止重复击中同一个敌人

	UPROPERTY()
	class ANiagaraActor* ActiveDashEffect;

private:
	// 渐进式突进相关
	UPROPERTY()
	float DashProgress = 0.0f;

	UPROPERTY()
	FTimerHandle ProgressiveDashTimerHandle;

	// 验证突进目标位置是否可达
	UFUNCTION()
	FVector ValidateDashDestination(const FVector& StartLocation, const FVector& TargetLocation);

	// 应用无敌效果
	UFUNCTION()
	void ApplyInvulnerabilityEffect();

	// 移除无敌效果
	UFUNCTION()
	void RemoveInvulnerabilityEffect();

	// 开始渐进式突进
	UFUNCTION()
	void StartProgressiveDash();

	// 更新突进位置
	UFUNCTION()
	void UpdateDashPosition();

	// 判断是否是敌人
	UFUNCTION()
	bool IsEnemyActor(AActor* Actor) const;

	virtual void PlayAbilityMontage(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	// 重写获取技能方向的方法，确保突进距离固定
	virtual FVector GetSkillShotDirection() const override;
};
