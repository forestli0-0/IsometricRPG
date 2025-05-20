// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_HeroBaseAbility.h"
#include "GA_SelfCastAbility.generated.h"

/**
 * Ability that is cast on self.
 */
UCLASS()
class ISOMETRICRPG_API UGA_SelfCastAbility : public UGA_HeroBaseAbility
{
	GENERATED_BODY()

public:
	UGA_SelfCastAbility();

protected:
	// Optional: Gameplay Effect to apply to self
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SelfCast")
	// TSubclassOf<class UGameplayEffect> SelfGameplayEffect;

	virtual bool CanActivateSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData, FGameplayTag& OutFailureTag) override;
	virtual void ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
