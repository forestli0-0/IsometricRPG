// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IsometricAbilities/GA_HeroBaseAbility.h"
#include "GA_ProjectileAbility.generated.h"

/**
 * Ability that launches a projectile.
 */
UCLASS()
class ISOMETRICRPG_API UGA_ProjectileAbility : public UGA_HeroBaseAbility
{
	GENERATED_BODY()

public:
	UGA_ProjectileAbility();

protected:
	// Projectile class to spawn
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<class AProjectile_Fireball> ProjectileClass; // Example, replace with a base projectile if you have one

	// Socket or location from which to spawn the projectile
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FName MuzzleSocketName = NAME_None;

	virtual bool CanActivateSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData, FGameplayTag& OutFailureTag) override;
	virtual void ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
