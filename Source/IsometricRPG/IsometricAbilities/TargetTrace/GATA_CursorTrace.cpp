// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricAbilities/TargetTrace/GATA_CursorTrace.h"
#include "Abilities/GameplayAbility.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h" // 可选，用于调试绘图

AGATA_CursorTrace::AGATA_CursorTrace()
{
    // 设置默认值
    TraceChannel = UEngineTypes::ConvertToTraceType(ECC_Visibility); // 默认使用 Visibility 通道
    bTraceComplex = false;
    bDestroyOnConfirmation = true; // 通常在确认后销毁 GATA
    PrimaryActorTick.bCanEverTick = false; // 对于简单的点击目标，通常不需要 Tick
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("目标选择器已经构建"));
}

void AGATA_CursorTrace::StartTargeting(UGameplayAbility* Ability)
{
    Super::StartTargeting(Ability);
    // 你可以在这里获取 PlayerController，但通常在 ConfirmTargetingAndContinue 中获取更安全
    // 因为 Ability 可能在 GATA 激活前发生变化
}

void AGATA_CursorTrace::ConfirmTargetingAndContinue()
{
    // 确保我们有一个有效的主控 GameplayAbility
    check(OwningAbility);

    APlayerController* PC = OwningAbility->GetCurrentActorInfo()->PlayerController.Get();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("GATA_CursorTrace: PlayerController is null."));
        // 创建一个空的 TargetDataHandle 来取消或通知失败
        FGameplayAbilityTargetDataHandle Handle;
        TargetDataReadyDelegate.Broadcast(Handle);
        return;
    }

    FHitResult HitResult;
    bool bHit = PC->GetHitResultUnderCursorByChannel(TraceChannel, bTraceComplex, HitResult);

    // (可选) 绘制调试射线
#if ENABLE_DRAW_DEBUG
    if (bHit)
    {
        DrawDebugLine(GetWorld(), HitResult.TraceStart, HitResult.ImpactPoint, FColor::Green, false, 2.0f, 0, 1.0f);
        DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 16.0f, 12, FColor::Green, false, 2.0f, 0, 1.0f);
    }
    else
    {
        // 如果没有命中，获取光标下的一个远点用于绘制射线
        FVector CursorWorldLocation, CursorWorldDirection;
        PC->DeprojectMousePositionToWorld(CursorWorldLocation, CursorWorldDirection);
        DrawDebugLine(GetWorld(), CursorWorldLocation, CursorWorldLocation + CursorWorldDirection * 10000.f, FColor::Red, false, 2.0f, 0, 1.0f);
    }
#endif // ENABLE_DRAW_DEBUG

    FGameplayAbilityTargetDataHandle TargetDataHandle;

    if (bHit && HitResult.GetActor()) // 确保击中了 Actor
    {
        // 将 FHitResult 包装成 FGameplayAbilityTargetData_SingleTargetHit
        // 注意：这里需要 new 一个，因为 FGameplayAbilityTargetDataHandle 会管理它的生命周期
        FGameplayAbilityTargetData_SingleTargetHit* NewTargetData = new FGameplayAbilityTargetData_SingleTargetHit();
        NewTargetData->HitResult = HitResult;
        TargetDataHandle.Add(NewTargetData);
    }
    // 即使没有命中，或者命中了但不是 Actor，我们仍然广播一个 (可能是空的) TargetDataHandle
    // 这样 Ability 的回调会被触发，它可以决定如何处理“没有有效目标”的情况

    // 广播 TargetDataReadyDelegate，将数据传回给 UAbilityTask_WaitTargetData
    TargetDataReadyDelegate.Broadcast(TargetDataHandle);

    // 如果 bDestroyOnConfirmation 为 true (默认是 true)，这个 Actor 将在广播后被销毁
}