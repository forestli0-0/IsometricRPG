// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GA_TargetedAbility.h
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GameplayAbilities/GA_HeroBaseAbility.h"
#include "Engine/DecalActor.h"
#include "NiagaraActor.h" 
#include "GA_TargetedAbility.generated.h"

/**
 * Ability that targets a specific actor.
 */
UCLASS()
class ISOMETRICRPG_API UGA_TargetedAbility : public UGA_HeroBaseAbility
{
	GENERATED_BODY()

public:
	UGA_TargetedAbility();

	virtual void StartTargetSelection(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnReachedTarget();
	UFUNCTION()
	void OnFailedToTarget();
protected:
	virtual bool OtherCheckBeforeCommit(const FGameplayAbilityTargetDataHandle& Data) override;
	virtual bool OtherCheckBeforeCommit(const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;
public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	TSubclassOf<ANiagaraActor> RangeIndicatorNiagaraActorClass;
	UPROPERTY()
	TObjectPtr<ANiagaraActor> ActiveRangeIndicatorNiagaraActor;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	UMaterialInterface* RangeIndicatorMaterial; // 在蓝图中设置你的圆形材质

	virtual void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data) override;
};