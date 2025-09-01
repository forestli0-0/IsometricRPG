// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GA_SkillShotAbility.cpp
// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_SkillShotAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "NiagaraActor.h"

UGA_SkillShotAbility::UGA_SkillShotAbility()
{
	AbilityType = EHeroAbilityType::SkillShot;
	bRequiresTargetData = false;
}



void UGA_SkillShotAbility::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{

	UE_LOG(LogTemp, Log, TEXT("%s: Executing skill shot ability."), *GetName());

	// Get the direction from the trigger event data (should contain target location)
	FVector Direction = GetSkillShotDirection();
	FVector StartLocation = GetAvatarActorFromActorInfo()->GetActorLocation();

	// Execute the skill shot
	ExecuteSkillShot(Direction, StartLocation);

	// If this ability is instant (no montage) and should end immediately after execution.
	if (!MontageToPlay)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_SkillShotAbility::StartTargetSelection(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	// For skill shots, we need a position/direction target, not an actor target
	// This could use a line trace or cursor position target actor
	if (!TargetActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("'%s': TargetActorClass is NOT SET! Cancelling."), *GetName());
		CancelAbility(Handle, ActorInfo, ActivationInfo, true);
		return;
	}

	// Show aim indicator if available
	if (AimIndicatorClass)
	{
		AActor* Avatar = GetAvatarActorFromActorInfo();
		if (Avatar)
		{
			FVector SpawnLocation = Avatar->GetActorLocation();
			ActiveAimIndicator = GetWorld()->SpawnActor<ANiagaraActor>(
				AimIndicatorClass,
				SpawnLocation,
				FRotator::ZeroRotator
			);
		}
	}

	// Start targeting for direction/position
	Super::StartTargetSelection(Handle, ActorInfo, ActivationInfo);
}


FVector UGA_SkillShotAbility::GetSkillShotDirection() const
{
	if (CurrentTargetDataHandle.Num() > 0)
	{
		// Get direction from current location to target location
		const FHitResult* HitResult = CurrentTargetDataHandle.Get(0)->GetHitResult();
		if (HitResult)
		{
			FVector StartLocation = GetAvatarActorFromActorInfo()->GetActorLocation();
			FVector TargetLocation = HitResult->Location;
			auto Direction = (TargetLocation - StartLocation).GetSafeNormal();
			Direction.Z = 0;
			return Direction;
		}
	}

	// Default to forward direction if no target data
	return GetAvatarActorFromActorInfo()->GetActorForwardVector();
}

void UGA_SkillShotAbility::ExecuteSkillShot(const FVector& Direction, const FVector& StartLocation)
{
	// Clean up aim indicator
	if (ActiveAimIndicator)
	{
		ActiveAimIndicator->Destroy();

		ActiveAimIndicator = nullptr;
	}

	// If we have a projectile class, spawn the projectile
	if (SkillShotProjectileClass)
	{
		FRotator SpawnRotation = Direction.Rotation();
		
		// You would implement projectile spawning here
		// Similar to GA_ProjectileAbility but with direction instead of specific target
		UE_LOG(LogTemp, Log, TEXT("Spawning skill shot projectile in direction: %s"), *Direction.ToString());
	}
	else
	{
		// Implement line trace or instant hit logic here
		FVector EndLocation = StartLocation + (Direction * RangeToApply);
		
		// Perform line trace
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(GetAvatarActorFromActorInfo());
		
		FHitResult HitResult;
		bool bHit = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			StartLocation,
			EndLocation,
			ECC_Pawn,
			QueryParams
		);

		if (bHit && HitResult.GetActor())
		{
			UE_LOG(LogTemp, Log, TEXT("射线检测到: %s"), *HitResult.GetActor()->GetName());
			// Apply effects to hit actor
		}
	}
}
