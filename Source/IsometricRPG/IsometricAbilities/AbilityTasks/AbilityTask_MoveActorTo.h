#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_MoveActorTo.generated.h"

// 声明一个委托，当移动完成时会调用它
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMoveActorToDelegate);

// 新增一个枚举来选择插值方式
UENUM()
enum class EMoveInterpMethod : uint8
{
	Linear,
	Parabolic
};

UCLASS()
class ISOMETRICRPG_API UAbilityTask_MoveActorTo : public UAbilityTask
{
	GENERATED_BODY()
	
public:
	// 当移动完成时广播
	UPROPERTY(BlueprintAssignable)
	FMoveActorToDelegate OnFinish;
	
	/**
	 * 在指定时间内将一个Actor平滑移动到目标位置。
	 * @param InterpMethod 插值方式 (线性 或 抛物线)
	 * @param ParabolicArcHeight 抛物线的最高点高度 (仅在抛物线模式下有效)
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_MoveActorTo* MoveActorTo(UGameplayAbility* OwningAbility, FName TaskInstanceName, AActor* TargetActor, FVector TargetLocation, float Duration, EMoveInterpMethod InterpMethod = EMoveInterpMethod::Linear, float ParabolicArcHeight = 500.0f, UCurveFloat* MovementCurve = nullptr);


	// 任务被激活时调用
	virtual void Activate() override;
	// 每帧调用
	virtual void TickTask(float DeltaTime) override;

private:
	TWeakObjectPtr<AActor> ActorToMove;
	FVector StartLocation;
	FVector TargetLocation;
	float Duration;
	float TimeElapsed;

	UPROPERTY()
	UCurveFloat* MovementCurve;

	// 新增的成员变量
	EMoveInterpMethod InterpMethod;
	float ParabolicArcHeight;
protected:
	/** 设置该任务是否需要Tick（用于AbilitySystem的任务调度） */
	void SetTickingTask(bool bShouldTick);// 在cpp文件中实现SetTickingTask方法
};
