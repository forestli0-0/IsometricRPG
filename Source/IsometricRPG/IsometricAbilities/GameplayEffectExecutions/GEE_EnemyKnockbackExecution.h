// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GEE_EnemyKnockbackExecution.h
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GEE_EnemyKnockbackExecution.generated.h"

/**
* 
*/
UCLASS()
class ISOMETRICRPG_API UGEE_EnemyKnockbackExecution : public UGameplayEffectExecutionCalculation
{
GENERATED_BODY()

public:
// Corrected the function declaration to match Unreal Engine's naming conventions

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
