// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\RPGGameplayAbility_Attack.h
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "RPGGameplayAbility_Attack.generated.h"

/**
 * 
 */
UCLASS()
class ISOMETRICRPG_API URPGGameplayAbility_Attack : public UGameplayAbility
{
	GENERATED_BODY()
public:
	URPGGameplayAbility_Attack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<class UGameplayEffect> DamageEffect;
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* AttackMontage;
	
	// 保存攻击目标
	UPROPERTY()
	AActor* AttackTarget;
	// 攻击者自己
	UPROPERTY()
	AActor* Attacker;
protected:
	// 蒙太奇完成回调
	UFUNCTION()
	void OnMontageCompleted();

	// 蒙太奇中断回调
	UFUNCTION()
	void OnMontageInterrupted();

	// 蒙太奇取消回调
	UFUNCTION()
	void OnMontageCancelled();
};
