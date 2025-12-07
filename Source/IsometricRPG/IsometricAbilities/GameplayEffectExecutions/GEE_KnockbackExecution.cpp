// Fill out your copyright notice in the Description page of Project Settings.

#include "GEE_KnockbackExecution.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"

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


	if (SourceActor)
	{
		if (APlayerState* PS = Cast<APlayerState>(SourceActor))
		{
			if (PS->GetPawn())
			{
				SourceActor = PS->GetPawn();
			}
		}
		else if (AController* Controller = Cast<AController>(SourceActor))
		{
			if (Controller->GetPawn())
			{
				SourceActor = Controller->GetPawn();
			}
		}
	}

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// 从规格中获取标签和数值
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	// 尝试从 SetByCaller 获取击退力度，如果没设置则使用默认值
	float ForceMagnitude = KnockbackForce;
	FGameplayTag KnockbackForceTag = FGameplayTag::RequestGameplayTag(FName("Data.Ability.KnockBack.Force"));
	float SetByCallerMagnitude = Spec.GetSetByCallerMagnitude(KnockbackForceTag, false, -1.0f);
	
	if (SetByCallerMagnitude > 0.0f)
	{
		ForceMagnitude = SetByCallerMagnitude;
	}

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

    // Debug Log: 打印击退计算的关键信息
	UE_LOG(LogTemp, Warning, TEXT("击退效果执行: 来源Actor=[%s] 坐标=[%s], 目标Actor=[%s] 坐标=[%s], 最终击退力度=[%f] (SetByCaller原始值=[%f])"), 
		SourceActor ? *SourceActor->GetName() : TEXT("NULL"), 
		SourceActor ? *SourceActor->GetActorLocation().ToString() : TEXT("N/A"),
		TargetActor ? *TargetActor->GetName() : TEXT("NULL"), 
		TargetActor ? *TargetActor->GetActorLocation().ToString() : TEXT("N/A"),
		ForceMagnitude,
		SetByCallerMagnitude
	);

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
	FVector KnockbackVelocity = KnockbackDirection * ForceMagnitude;

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
