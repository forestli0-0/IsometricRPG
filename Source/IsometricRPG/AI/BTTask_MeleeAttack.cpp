// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_MeleeAttack.h"
#include "AIController.h"
UBTTask_MeleeAttack::UBTTask_MeleeAttack()
{
    // 默认情况下，我们假设任务是脚本驱动的，可能包含异步逻辑。
    // 因此，我们设置为InProgress，并通过脚本调用FinishExecute来结束任务。
    // 如果你的TS任务总是同步瞬间完成，可以在TS的`ReceiveExecuteAI`中直接调用`FinishExecute`。
    bNotifyTick = true; // 如果你的任务需要Tick，请保留此项
    bNotifyTaskFinished = true; // 如果你需要处理任务中断逻辑，请保留
}

EBTNodeResult::Type UBTTask_MeleeAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 调用脚本层的 ReceiveExecuteAI
    ReceiveExecuteAI(OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr);

    // 返回InProgress，意味着任务正在执行，等待脚本逻辑调用`FinishExecute`来决定最终结果。
    return EBTNodeResult::InProgress;
}

void UBTTask_MeleeAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    // 调用脚本层的 ReceiveTickAI
    ReceiveTickAI(OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr, DeltaSeconds);
}

void UBTTask_MeleeAttack::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    // 调用脚本层的 ReceiveFinishAI，用于处理任务被中断等情况
    ReceiveFinishAI(OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr, TaskResult);
}
