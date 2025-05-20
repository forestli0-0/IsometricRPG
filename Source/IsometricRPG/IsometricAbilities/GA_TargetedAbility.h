// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_HeroBaseAbility.h"
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

protected:
	virtual bool CanActivateSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData, FGameplayTag& OutFailureTag) override;
	virtual void ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
