// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricAbilities/AN_PlayMeleeAttackMontageNotify.h"
#include "Character/IsometricRPGCharacter.h"
#include "IsometricComponents/ActionQueueComponent.h"
#include "AbilitySystemInterface.h"
void UAN_PlayMeleeAttackMontageNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (MeshComp && MeshComp->GetOwner())
	{
		auto owner = MeshComp->GetOwner();

		auto IsoCharacter = Cast<AIsometricRPGCharacter>(owner);
		if (IsoCharacter)
		{
			auto  ActionQueue = IsoCharacter->FindComponentByClass<UActionQueueComponent>();
			// 攻击帧触发
			if (ActionQueue)
			{
				ActionQueue->bAttackInProgress = true;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Notify: Owner is not AIsometricRPGCharacter"));
            return;
		}
        if (!MeshComp || !MeleeAttackEffectClass) return;

        if (!owner) return;

        // 获取攻击者的 ASC
        UAbilitySystemComponent* SourceASC = IsoCharacter->GetAbilitySystemComponent();
        if (!SourceASC) return;
        auto ownerActionQueue = owner->FindComponentByClass<UActionQueueComponent>();

        // 修复 TWeakObjectPtr<AActor> 到 AActor* 的转换问题  
        if (ownerActionQueue)  
        {  
           TWeakObjectPtr<AActor> WeakTargetActor = ownerActionQueue->GetCommand().TargetActor;  
           if (WeakTargetActor.IsValid())  
           {  
               TargetActor = WeakTargetActor.Get();  
           }  
        }
        else
        {
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("没有找到目标动作队列组件"));
        }
	    auto TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();

        if (!TargetASC) return;

        // 创建 GameplayEffectSpec
        FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
        ContextHandle.AddSourceObject(this); // 可选：记录这个 Notify 作为来源

        FGameplayEffectSpecHandle HurtSpecHandle = SourceASC->MakeOutgoingSpec(MeleeAttackEffectClass, EffectLevel, ContextHandle);
        FGameplayEffectSpecHandle PunchSpecHandle = SourceASC->MakeOutgoingSpec(PunchEffectClass, EffectLevel, ContextHandle);
        if (HurtSpecHandle.IsValid())
        {
            SourceASC->ApplyGameplayEffectSpecToTarget(*HurtSpecHandle.Data.Get(), TargetASC);
        }
        if (PunchSpecHandle.IsValid())
        {
            SourceASC->ApplyGameplayEffectSpecToTarget(*PunchSpecHandle.Data.Get(), TargetASC);
        }
           

	}
}
