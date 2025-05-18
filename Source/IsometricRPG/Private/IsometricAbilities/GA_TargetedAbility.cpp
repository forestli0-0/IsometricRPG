// Copyright Epic Games, Inc. All Rights Reserved.

#include "IsometricAbilities/GA_TargetedAbility.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayTagContainer.h" // Required for FGameplayTag

UGA_TargetedAbility::UGA_TargetedAbility()
{
	// Constructor logic if needed
}

bool UGA_TargetedAbility::CanActivateSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData, FGameplayTag& OutFailureTag)
{
	if (!Super::CanActivateSkill(Handle, ActorInfo, ActivationInfo, TriggerEventData, OutFailureTag))
	{
		return false;
	}

	if (!TriggerEventData || !IsValid(TriggerEventData->Target))
	{
		OutFailureTag = FGameplayTag::RequestGameplayTag(FName("Ability.Failure.InvalidTarget"));
		UE_LOG(LogTemp, Warning, TEXT("%s: Failed CanActivateSkill - No valid target in TriggerEventData."), *GetName());
		return false;
        // Fix the error by using a const_cast to remove the const qualifier from the pointer.  
	} 

    AActor* TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
	// Optionally, check if the target is a Pawn or a specific type
	// if (!Cast<APawn>(TargetActor))
	// {
	// 	OutFailureTag = FGameplayTag::RequestGameplayTag(FName("Ability.Failure.TargetNotPawn"));
	//  UE_LOG(LogTemp, Warning, TEXT("%s: Failed CanActivateSkill - Target is not a Pawn."), *GetName());
	// 	return false;
	// }

	AActor* SelfActor = ActorInfo->AvatarActor.Get();
	if (!SelfActor)
	{
		OutFailureTag = FGameplayTag::RequestGameplayTag(FName("Ability.Failure.InvalidSelf"));
		UE_LOG(LogTemp, Warning, TEXT("%s: Failed CanActivateSkill - SelfActor is invalid."), *GetName());
		return false;
	}

	float Distance = FVector::Dist(TargetActor->GetActorLocation(), SelfActor->GetActorLocation());
	if (Distance > RangeToApply) // RangeToApply is inherited from UGA_HeroBaseAbility
	{
		OutFailureTag = FGameplayTag::RequestGameplayTag(FName("Ability.Failure.OutOfRange"));
		UE_LOG(LogTemp, Warning, TEXT("%s: Failed CanActivateSkill - Target out of range (%.2f > %.2f)"), *GetName(), Distance, RangeToApply);
		return false;
	}
	
	// Example: Stop movement if needed (original code had this commented out)
	// APawn* SelfPawn = Cast<APawn>(SelfActor);
	// if (SelfPawn)
	// {
	//     UPawnMovementComponent* MovementComponent = SelfPawn->GetMovementComponent();
	//     if (MovementComponent && MovementComponent->IsMovingOnGround()) // Check if actually moving
	//     {
	//         MovementComponent->StopMovementImmediately();
	//     }
	// }

	return true;
}

void UGA_TargetedAbility::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ExecuteSkill(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (TriggerEventData && IsValid(TriggerEventData->Target))
	{
		UE_LOG(LogTemp, Log, TEXT("%s: Executing targeted skill on %s."), *GetName(), *TriggerEventData->Target.Get()->GetName());
		
		// 确保在技能执行时朝向目标
		AActor* SelfActor = ActorInfo->AvatarActor.Get();
		AActor* TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
		
		if (SelfActor && TargetActor)
		{
			// 获取目标当前的实时位置
			FVector TargetLocation = TargetActor->GetActorLocation();
			FVector SelfLocation = SelfActor->GetActorLocation();
			
			// 计算朝向目标的方向
			FVector Direction = (TargetLocation - SelfLocation).GetSafeNormal();
			if (!Direction.IsZero())
			{
				// 生成新的朝向旋转
				FRotator NewRotation = FRotationMatrix::MakeFromX(Direction).Rotator();
				// 仅保留Yaw角度，不更改Pitch和Roll
				NewRotation.Pitch = 0.0f;
				NewRotation.Roll = 0.0f;
				// 应用旋转到角色
				SelfActor->SetActorRotation(NewRotation);
				
				UE_LOG(LogTemp, Verbose, TEXT("%s: Facing towards target at location: %s"), 
					*GetName(), *TargetLocation.ToString());
			}
		}
		
		// Implement actual skill logic here, e.g., apply gameplay effect to TriggerEventData->Target
		// Example:
		// FGameplayEffectContextHandle ContextHandle = ActorInfo->AbilitySystemComponent->MakeEffectContext();
		// ContextHandle.AddSourceObject(this);
		// FGameplayEffectSpecHandle SpecHandle = ActorInfo->AbilitySystemComponent->MakeOutgoingSpec(MyTargetedEffectGE, GetAbilityLevel(), ContextHandle);
		// if (SpecHandle.IsValid())
		// {
		//    ApplyGameplayEffectSpecToTarget(Handle, ActorInfo, ActivationInfo, SpecHandle, TriggerEventData->TargetData);
		// }
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: ExecuteSkill called without a valid target."), *GetName());
	}

	// If this ability is instant (no montage) and should end immediately after execution.
	if (!MontageToPlay)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}
