// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GA_ProjectileAbility.cpp
// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_ProjectileAbility.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameplayTagContainer.h" // Required for FGameplayTag
#include "IsometricAbilities/Projectiles/Projectile_Fireball.h" // Make sure this path is correct

UGA_ProjectileAbility::UGA_ProjectileAbility()
{
	// Constructor logic if needed
	ProjectileClass = AProjectile_Fireball::StaticClass(); // Default, can be changed in BP
	MuzzleSocketName = FName("Muzzle"); // Example socket name, adjust as needed
}

bool UGA_ProjectileAbility::CanActivateSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData, FGameplayTag& OutFailureTag)
{
	if (!Super::CanActivateSkill(Handle, ActorInfo, ActivationInfo, TriggerEventData, OutFailureTag))
	{
		return false;
	}

	if (!ProjectileClass)
	{
		OutFailureTag = FGameplayTag::RequestGameplayTag(FName("Ability.Failure.InvalidProjectileClass"));
		UE_LOG(LogTemp, Error, TEXT("%s: Failed CanActivateSkill - ProjectileClass is not set."), *GetName());
		return false;
	}

	// For projectile abilities, a target location might still be relevant for aiming
	// but not strictly required for activation in the same way as a targeted ability.
	// Range checks might be against the target point if provided, or a general max range.
	AActor* SelfActor = ActorInfo->AvatarActor.Get();
	if (!SelfActor)
	{
		OutFailureTag = FGameplayTag::RequestGameplayTag(FName("Ability.Failure.InvalidSelf"));
		return false;
	}

	if (TriggerEventData && TriggerEventData->TargetData.Num() > 0)
    {
        const FGameplayAbilityTargetData* TargetData = TriggerEventData->TargetData.Data[0].Get();
        if (TargetData)
        {
			FVector TargetLocation = TargetData->GetHitResult() ? FVector(TargetData->GetHitResult()->Location) : FVector(TargetData->GetEndPoint());

            if (TargetLocation != FVector::ZeroVector)
            {
                float Distance = FVector::Dist(TargetLocation, SelfActor->GetActorLocation());
                if (Distance > RangeToApply) // Using RangeToApply as a general max range for aiming
                {
                    OutFailureTag = FGameplayTag::RequestGameplayTag(FName("Ability.Failure.OutOfRange"));
                    UE_LOG(LogTemp, Warning, TEXT("%s: Failed CanActivateSkill - Target aim point out of range (%.2f > %.2f)"), *GetName(), Distance, RangeToApply);
                    return false;
                }
            }
        }
    }

	return true;
}

void UGA_ProjectileAbility::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ExecuteSkill(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* SelfActor = ActorInfo->AvatarActor.Get();
	if (!SelfActor || !ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: Cannot execute skill - SelfActor or ProjectileClass is invalid."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // Cancel if prerequisites are missing
		return;
	}

	FVector SpawnLocation;
	FRotator SpawnRotation;

	USceneComponent* MuzzleComponent = nullptr;
	APawn* SelfPawn = Cast<APawn>(SelfActor);
	if (SelfPawn && MuzzleSocketName != NAME_None)
	{
		// Try to find the socket on the mesh
		TArray<USceneComponent*> Components;
		SelfPawn->GetComponents(Components);
		for (USceneComponent* Component : Components)
		{
			if (Component->DoesSocketExist(MuzzleSocketName))
			{
				MuzzleComponent = Component;
				break;
			}
		}
	}

	if (MuzzleComponent)
	{
		SpawnLocation = MuzzleComponent->GetSocketLocation(MuzzleSocketName);
	}
	else
	{
		// Fallback if socket not found or not specified
		FTransform MuzzleTransform = SelfPawn ? SelfPawn->GetActorTransform() : FTransform::Identity;
		SpawnLocation = MuzzleTransform.GetLocation() + SelfActor->GetActorForwardVector() * 100.0f; // Offset a bit forward
	}

	// Determine rotation
	FVector TargetLocation = FVector::ZeroVector;
	if (TriggerEventData && TriggerEventData->TargetData.Num() > 0)
	{
		const FGameplayAbilityTargetData* TargetData = TriggerEventData->TargetData.Data[0].Get();
		if (TargetData)
		{
			TargetLocation = TargetData->GetHitResult() ? FVector(TargetData->GetHitResult()->Location) : FVector(TargetData->GetEndPoint());

		}
	}

	if (TargetLocation != FVector::ZeroVector)
	{
		SpawnRotation = UKismetMathLibrary::FindLookAtRotation(SpawnLocation, TargetLocation);
	}
	else
	{
		SpawnRotation = SelfActor->GetActorRotation();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = SelfActor;
	SpawnParams.Instigator = SelfPawn;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProjectile_Fireball* SpawnedProjectile = GetWorld()->SpawnActor<AProjectile_Fireball>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (SpawnedProjectile)
	{
		UE_LOG(LogTemp, Log, TEXT("%s: Spawned projectile %s"), *GetName(), *SpawnedProjectile->GetName());
		// You might want to pass damage causer or other info to the projectile here
		// SpawnedProjectile->SetOwner(SelfActor);
		// SpawnedProjectile->DamageCauser = SelfActor; // If your projectile has such a property
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s: Failed to spawn projectile."), *GetName());
	}

	// Projectile abilities usually end after firing, unless they have a channel time or other mechanics.
	// If there's a montage for firing animation, it will handle ending the ability.
	if (!MontageToPlay) 
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}
