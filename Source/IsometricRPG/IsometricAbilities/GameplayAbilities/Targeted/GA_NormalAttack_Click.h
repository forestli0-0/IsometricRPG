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
	// 缓存目标数据并进入基类流程

	virtual void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data) override;

	// 使用“扩展到达半径”与移动任务；不调用父类该检查
	virtual bool OtherCheckBeforeCommit() override;

	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual void ExecuteSkill(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo);
};
