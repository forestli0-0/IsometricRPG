// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlueprintBase.h"
#include "BTTask_MeleeAttack.generated.h"

/**
 * 
 */
UCLASS()
class ISOMETRICRPG_API UBTTask_MeleeAttack : public UBTTask_BlueprintBase
{
	GENERATED_BODY()

public:
    UBTTask_MeleeAttack();

protected:
    //~ Begin UBTTaskNode Interface
    /** 行为树执行此任务的入口 */
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    /** 如果任务不是瞬间完成的，会在此处Tick */
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    /** 任务中断时的处理 */
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
    //~ End UBTTaskNode Interface

public:
    /**
     * 在蓝图/脚本中声明的入口事件。
     * Puerts会自动将TS中的`ReceiveExecuteAI`函数绑定到这里。
     */

    void ReceiveExecuteAI(AAIController* OwnerController, APawn* ControlledPawn);

    /**
     * 在蓝图/脚本中声明的Tick事件。
     * Puerts会自动将TS中的`ReceiveTickAI`函数绑定到这里。
     */

    void ReceiveTickAI(AAIController* OwnerController, APawn* ControlledPawn, float DeltaSeconds);

    /**
     * 在蓝图/脚本中声明的结束事件。
     * Puerts会自动将TS中的`ReceiveFinishAI`函数绑定到这里。
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI|Puerts")
    void ReceiveFinishAI(AAIController* OwnerController, APawn* ControlledPawn, EBTNodeResult::Type TaskResult);

};
