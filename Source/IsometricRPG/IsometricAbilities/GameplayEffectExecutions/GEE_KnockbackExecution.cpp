// Fill out your copyright notice in the Description page of Project Settings.

#include "GEE_KnockbackExecution.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/IsometricRPGAttributeSetBase.h"

// 定义我们要捕获的属性
struct FKnockbackStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);

	FKnockbackStatics()
	{
		// 捕获目标的生命值属性（可用于计算击飞强度）
		DEFINE_ATTRIBUTE_CAPTUREDEF(UIsometricRPGAttributeSetBase, Health, Target, false);
	}
};

static const FKnockbackStatics& KnockbackStatics()
{
	static FKnockbackStatics Statics;
	return Statics;
}

UGEE_KnockbackExecution::UGEE_KnockbackExecution()
{
	// 设置默认值
	KnockbackForce = 800.0f;
	KnockbackDuration = 1.0f;

	// 定义我们要捕获的属性
	RelevantAttributesToCapture.Add(KnockbackStatics().HealthDef);
}

void UGEE_KnockbackExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent();
	UAbilitySystemComponent* SourceAbilitySystemComponent = ExecutionParams.GetSourceAbilitySystemComponent();

	AActor* SourceActor = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->GetAvatarActor() : nullptr;
	AActor* TargetActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetAvatarActor() : nullptr;

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// 从规格中获取标签和数值
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	if (!TargetActor || !SourceActor)
	{
		return;
	}

	// 检查目标是否是角色
	ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
	if (!TargetCharacter)
	{
		return;
	}

	// 获取角色移动组件
	UCharacterMovementComponent* MovementComponent = TargetCharacter->GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	// 计算击飞方向
	FVector KnockbackDirection = (TargetActor->GetActorLocation() - SourceActor->GetActorLocation()).GetSafeNormal();
	
	// 添加向上的分量，使击飞更明显
	KnockbackDirection.Z = 0.5f;
	KnockbackDirection = KnockbackDirection.GetSafeNormal();

	// 计算击飞向量
	FVector KnockbackVelocity = KnockbackDirection * KnockbackForce;

	// 应用击飞
	MovementComponent->Launch(KnockbackVelocity);

	// 添加击飞状态标签
	FGameplayTagContainer KnockbackTags;
	KnockbackTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Knockback")));
	
	// 这个标签会阻止角色移动一段时间
	TargetAbilitySystemComponent->AddLooseGameplayTags(KnockbackTags);

	// 设置定时器来移除击飞状态
	if (TargetActor->GetWorld())
	{
		FTimerHandle KnockbackTimerHandle;
		TargetActor->GetWorld()->GetTimerManager().SetTimer(
			KnockbackTimerHandle,
			FTimerDelegate::CreateLambda([=]()
				{
					if (TargetAbilitySystemComponent)
					{
						TargetAbilitySystemComponent->RemoveLooseGameplayTags(KnockbackTags);
					}
				}),
			KnockbackDuration,
			false
		);
	}
	UE_LOG(LogTemp, Log, TEXT("Applied knockback to %s with force %f"), *TargetActor->GetName(), KnockbackForce);
}

void UGEE_KnockbackExecution::RemoveKnockbackState(UAbilitySystemComponent* TargetASC, FGameplayTagContainer TagsToRemove) const
{
	if (TargetASC)
	{
		TargetASC->RemoveLooseGameplayTags(TagsToRemove);
		UE_LOG(LogTemp, Log, TEXT("Removed knockback state from target"));
	}
}
