// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\AN_PlayMeleeAttackMontageNotify.h
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimNotifies/AnimNotify_PlayMontageNotify.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "AN_PlayMeleeAttackMontageNotify.generated.h"

/**
* 
*/
UCLASS()
class ISOMETRICRPG_API UAN_PlayMeleeAttackMontageNotify : public UAnimNotify_PlayMontageNotify
{
GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayEffect")
    TSubclassOf<class UGameplayEffect> MeleeAttackEffectClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayEffect")
    TSubclassOf<class UGameplayEffect> PunchEffectClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayEffect")
    float EffectLevel = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayEffect")
    float KnockbackForce = 800.0f;

    AActor* TargetActor;
private:
    void ApplyEffectToTarget(
        UAbilitySystemComponent* SourceASC,
        UAbilitySystemComponent* TargetASC,
        TSubclassOf<UGameplayEffect> EffectClass,
        float Level,
        FGameplayEffectContextHandle& ContextHandle);
};
