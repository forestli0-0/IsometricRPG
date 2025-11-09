// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlueprintBase.h"
#include "BTTask_MeleeAttack.generated.h"

/**
 * Melee attack task that delegates behavior to Blueprint/TS via base events
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
    /**任务结束/中断时的处理 */
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
    //~ End UBTTaskNode Interface
};
