// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GEE_KnockbackExecution.generated.h"

/**
 * 击飞效果执行计算
 * 用于处理突进技能的击飞控制效果
 */
UCLASS()
class ISOMETRICRPG_API UGEE_KnockbackExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UGEE_KnockbackExecution();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

	// 移除击飞状态的函数
	UFUNCTION()
	void RemoveKnockbackState(UAbilitySystemComponent* TargetASC, FGameplayTagContainer TagsToRemove) const;

protected:
	// 击飞力度
	UPROPERTY()
	float KnockbackForce;

	// 击飞持续时间
	UPROPERTY()
	float KnockbackDuration;
};
