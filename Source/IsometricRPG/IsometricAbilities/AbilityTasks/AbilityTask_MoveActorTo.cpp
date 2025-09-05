// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricAbilities/AbilityTasks/AbilityTask_MoveActorTo.h"
#include "GameFramework/Actor.h"
#include "Curves/CurveFloat.h"
#include "AbilitySystemComponent.h"


UAbilityTask_MoveActorTo* UAbilityTask_MoveActorTo::MoveActorTo(UGameplayAbility* OwningAbility, FName TaskInstanceName, AActor* TargetActor, FVector TargetLocation, float Duration, EMoveInterpMethod InterpMethod, float ParabolicArcHeight, UCurveFloat* MovementCurve)

{
	// 创建任务实例
	UAbilityTask_MoveActorTo* MyTask = NewAbilityTask<UAbilityTask_MoveActorTo>(OwningAbility, TaskInstanceName);
	if (MyTask)
	{
		MyTask->ActorToMove = TargetActor;
		MyTask->TargetLocation = TargetLocation;
		MyTask->Duration = FMath::Max(Duration, 0.01f);
		MyTask->MovementCurve = MovementCurve;
		// 保存新参数
		MyTask->InterpMethod = InterpMethod;
		MyTask->ParabolicArcHeight = ParabolicArcHeight;
	}
	return MyTask;
}

void UAbilityTask_MoveActorTo::Activate()
{
	Super::Activate();

	if (!ActorToMove.IsValid())
	{
		EndTask();
		return;
	}

	// 记录起始位置和时间
	StartLocation = ActorToMove->GetActorLocation();
	TimeElapsed = 0.0f;
	// 确保任务在AbilitySystem中注册Tick（兼容服务端）
	SetTickingTask(true);
	UE_LOG(LogTemp, Verbose, TEXT("AbilityTask_MoveActorTo Activate: Actor=%s Target=%s Duration=%.2f Interp=%d (Role=%d)"),
		*ActorToMove->GetName(), *TargetLocation.ToString(), Duration, (int32)InterpMethod,
		ActorToMove->GetLocalRole());
}

void UAbilityTask_MoveActorTo::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (!ActorToMove.IsValid())
	{
		EndTask();
		return;
	}

	TimeElapsed += DeltaTime;

	// 计算插值 Alpha (0.0 to 1.0)
	float Alpha = FMath::Clamp(TimeElapsed / Duration, 0.0f, 1.0f);

	// 如果提供了曲线，则使用曲线的值作为 Alpha
	if (MovementCurve)
	{
		Alpha = MovementCurve->GetFloatValue(Alpha);
	}

	// 使用 Lerp (线性插值) 计算当前帧的位置
	FVector NewLocation;

	switch (InterpMethod)
	{
	case EMoveInterpMethod::Parabolic:
	{
		// 水平方向 (X, Y) 仍然是线性插值
		FVector HorizontalLerp = FMath::Lerp(StartLocation, TargetLocation, Alpha);

		// 垂直方向 (Z) 我们要手动计算出一个抛物线
		// 使用 sin(Alpha * PI) 可以得到一个从 0 -> 1 -> 0 的平滑曲线
		float Z_Offset = FMath::Sin(Alpha * PI) * ParabolicArcHeight;

		NewLocation.X = HorizontalLerp.X;
		NewLocation.Y = HorizontalLerp.Y;
		NewLocation.Z = HorizontalLerp.Z + Z_Offset; // 在线性插值的Z值基础上增加抛物线高度
		break;
	}
	case EMoveInterpMethod::Linear:
	default:
	{
		// 保持原有的线性插值逻辑
		NewLocation = FMath::Lerp(StartLocation, TargetLocation, Alpha);
		break;
	}
	}

	ActorToMove->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);

	if (TimeElapsed >= Duration)
	{
		ActorToMove->SetActorLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
	UE_LOG(LogTemp, Verbose, TEXT("AbilityTask_MoveActorTo Finished: Actor=%s Reached=%s"), *ActorToMove->GetName(), *TargetLocation.ToString());
		OnFinish.Broadcast();
		EndTask();
	}
}


void UAbilityTask_MoveActorTo::SetTickingTask(bool bShouldTick)
{
	// 兼容UE4/UE5的AbilityTask Tick注册方式
	bTickingTask = bShouldTick;
}