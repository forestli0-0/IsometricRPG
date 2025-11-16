// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GA_HealRegenAbility.cpp
#include "GA_HealRegenAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Kismet/KismetSystemLibrary.h"

UGA_HealRegenAbility::UGA_HealRegenAbility()
{

}

void UGA_HealRegenAbility::ApplySelfEffect(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	const FString AvatarName = (ActorInfo && ActorInfo->AvatarActor.IsValid())
		? ActorInfo->AvatarActor->GetName()
		: TEXT("<None>");
	UE_LOG(LogTemp, Log, TEXT("[PassiveDebug][RegenAbility] ApplySelfEffect on %s (Authority=%d)"),
		*AvatarName,
		ActorInfo ? ActorInfo->IsNetAuthority() : 0);
	if (HealGameplayEffectClass)
	{
		auto ASC = ActorInfo->AbilitySystemComponent.Get();
		auto context = FGameplayEffectContextHandle(ASC->MakeEffectContext());
		context.AddSourceObject(this);
		FGameplayEffectSpecHandle specHandle = ASC->MakeOutgoingSpec(HealGameplayEffectClass, GetAbilityLevel(), context);
		if (specHandle.IsValid())
		{
			specHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Ability.Heal")), HealPercent);
			ASC->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
		}
	}
	if (SpeedGameplayEffectClass)
	{
		auto ASC = ActorInfo->AbilitySystemComponent.Get();
		auto context = FGameplayEffectContextHandle(ASC->MakeEffectContext());
		context.AddSourceObject(this);
		FGameplayEffectSpecHandle specHandle = ASC->MakeOutgoingSpec(SpeedGameplayEffectClass, GetAbilityLevel(), context);
		if (specHandle.IsValid())
		{
			//specHandle.Data->SetDuration(BuffDuration, false);
			specHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Ability.Duration")), BuffDuration);
			specHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Ability.SpeedUp")), MoveSpeedBonusPercent);
			ASC->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
		}
	}
}


