// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_SelfCastAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h" // 需要使用 FGameplayTag

UGA_SelfCastAbility::UGA_SelfCastAbility()
{
	// 如果需要，在这里添加构造函数逻辑
}

bool UGA_SelfCastAbility::CanActivateSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData, FGameplayTag& OutFailureTag)
{	if (!Super::CanActivateSkill(Handle, ActorInfo, ActivationInfo, TriggerEventData, OutFailureTag))
	{
		return false;
	}

	// 自我施放技能通常不需要外部目标或超出冷却/消耗之外的距离检查
	// 可以在此处添加自我施放的特定条件
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
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // 取消技能
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("%s: Executing self-cast ability on %s."), *GetName(), *SelfActor->GetName());

	// 在此处实现自我施放逻辑，例如，对自身应用游戏效果
	// 示例：
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

	// 如果这个技能是即时的（没有动作蒙太奇）并且应该在执行后立即结束
	if (!MontageToPlay)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}
