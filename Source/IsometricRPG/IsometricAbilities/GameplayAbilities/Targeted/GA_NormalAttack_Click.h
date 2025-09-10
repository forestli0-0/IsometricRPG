// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GameplayAbilities/Targeted/GA_TargetedAbility.h"
#include "GA_NormalAttack_Click.generated.h"

/**
 * 
 */
UCLASS()
class ISOMETRICRPG_API UGA_NormalAttack_Click : public UGA_TargetedAbility
{
	GENERATED_BODY()
public:
	UGA_NormalAttack_Click();
protected:
	// 仅该技能内使用的缓存：跨越移动阶段后重建目标数据
	UPROPERTY()
	TWeakObjectPtr<AActor> CachedTargetActor;
	UPROPERTY()
	FVector CachedTargetLocation = FVector::ZeroVector;

	// 缓存目标数据并进入基类流程

	virtual void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data) override;

	// 使用“扩展到达半径”与移动任务；不调用父类该检查
	virtual bool OtherCheckBeforeCommit() override;

	// 到达后若发现当前 TargetData 失效则用缓存重建，再交给父类提交/执行

	void OnReachedTarget();


	void OnFailedToTarget();

	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual void ExecuteSkill(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo);
};
