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
};
