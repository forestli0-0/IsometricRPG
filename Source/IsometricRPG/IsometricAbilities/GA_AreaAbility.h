// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_HeroBaseAbility.h"
#include "GA_AreaAbility.generated.h"

/**
 * Ability that affects an area.
 */
UCLASS()
class ISOMETRICRPG_API UGA_AreaAbility : public UGA_HeroBaseAbility
{
	GENERATED_BODY()

public:
	UGA_AreaAbility();

protected:
	// Radius of the area of effect
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Area")
	float AreaRadius = 300.f;

	// Optional: Gameplay Effect to apply to targets in the area
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Area")
	// TSubclassOf<class UGameplayEffect> AreaGameplayEffect;

	virtual bool CanActivateSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData, FGameplayTag& OutFailureTag) override;
	virtual void ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
