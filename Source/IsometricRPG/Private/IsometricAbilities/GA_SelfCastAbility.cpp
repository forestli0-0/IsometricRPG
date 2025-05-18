// Copyright Epic Games, Inc. All Rights Reserved.

#include "IsometricAbilities/GA_SelfCastAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h" // Required for FGameplayTag

UGA_SelfCastAbility::UGA_SelfCastAbility()
{
	// Constructor logic if needed
}

bool UGA_SelfCastAbility::CanActivateSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData, FGameplayTag& OutFailureTag)
{
	if (!Super::CanActivateSkill(Handle, ActorInfo, ActivationInfo, TriggerEventData, OutFailureTag))
	{
		return false;
	}

	// Self-cast abilities usually don't have external targets or range checks beyond cooldown/cost.
	// Any specific conditions for self-cast can be added here.
	AActor* SelfActor = ActorInfo->AvatarActor.Get();
	if (!SelfActor)
	{
		OutFailureTag = FGameplayTag::RequestGameplayTag(FName("Ability.Failure.InvalidSelf"));
		return false;
	}

	return true;
}

void UGA_SelfCastAbility::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ExecuteSkill(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* SelfActor = ActorInfo->AvatarActor.Get();
	if (!SelfActor)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: Cannot execute self-cast skill - SelfActor is invalid."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // Cancel
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("%s: Executing self-cast ability on %s."), *GetName(), *SelfActor->GetName());

	// Implement self-cast logic here, e.g., apply a gameplay effect to self.
	// Example:
	// if (SelfGameplayEffect)
	// {
	// 	FGameplayEffectContextHandle ContextHandle = ActorInfo->AbilitySystemComponent->MakeEffectContext();
	// 	ContextHandle.AddSourceObject(this);
	// 	FGameplayEffectSpecHandle SpecHandle = ActorInfo->AbilitySystemComponent->MakeOutgoingSpec(SelfGameplayEffect, GetAbilityLevel(), ContextHandle);
	// 	if (SpecHandle.IsValid())
	// 	{
	// 		ApplyGameplayEffectSpecToSelf(Handle, ActorInfo, ActivationInfo, SpecHandle);
	// 	}
	// }

	// If this ability is instant (no montage) and should end immediately after execution.
	if (!MontageToPlay)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}
