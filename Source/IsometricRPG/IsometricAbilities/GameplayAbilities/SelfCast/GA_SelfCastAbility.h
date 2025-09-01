// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GA_SelfCastAbility.h
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GameplayAbilities/GA_HeroBaseAbility.h"
#include "GA_SelfCastAbility.generated.h"

/**
 * 自我施放型技能基类
 * 这种技能直接作用于施放者自身，不需要选择目标
 */
UCLASS()
class ISOMETRICRPG_API UGA_SelfCastAbility : public UGA_HeroBaseAbility
{
	GENERATED_BODY()

public:
	UGA_SelfCastAbility();

protected:
	// 自我增益效果
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SelfCast")
	TSubclassOf<class UGameplayEffect> SelfGameplayEffect;

	// 效果持续时间
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SelfCast")
	float EffectDuration = 5.0f;


	virtual void ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	// 自我施放技能不需要目标数据
	virtual bool RequiresTargetData_Implementation() const override;
	
	// 应用自我效果的辅助方法
	virtual void ApplySelfEffect(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

};
