#include "IsometricAbilities/GEE_EnemyKnockbackExecution.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"

void UGEE_EnemyKnockbackExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    // 获取目标和来源的 Ability System Component
    UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
    UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();

    if (!TargetASC || !SourceASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("UGEE_EnemyKnockbackExecution: Invalid ASCs!"));
        return;
    }

    // 获取目标和来源的拥有者
    AActor* TargetActor = TargetASC->GetOwner();
    AActor* SourceActor = SourceASC->GetOwner();

    if (!TargetActor || !SourceActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("UGEE_EnemyKnockbackExecution: Invalid Actors!"));
        return;
    }

    // 计算击退方向
    FVector KnockbackDirection = (TargetActor->GetActorLocation() - SourceActor->GetActorLocation()).GetSafeNormal();
    float KnockbackStrength = 500.0f;

    // 应用击退效果
    if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
    {
        // 使用 LaunchCharacter 实现击退
        TargetCharacter->LaunchCharacter(KnockbackDirection * KnockbackStrength, true, true);
    }
    else if (UPrimitiveComponent* TargetComp = TargetActor->FindComponentByClass<UPrimitiveComponent>())
    {
        // 如果目标不是 Character，尝试使用 AddImpulse
        TargetComp->AddImpulse(KnockbackDirection * KnockbackStrength, NAME_None, true);
    }
}