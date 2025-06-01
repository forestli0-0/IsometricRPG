// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricAbilities/AbilityTasks/AbilityTask_WaitMoveToLocation.h"

#include "AIController.h"
#include "GameFramework/Character.h"
#include "NavigationSystem.h"
#include "Abilities/GameplayAbility.h"
#include "TimerManager.h"
#include <Blueprint/AIBlueprintHelperLibrary.h>


UAbilityTask_WaitMoveToLocation* UAbilityTask_WaitMoveToLocation::WaitMoveToLocation(
    UGameplayAbility* OwningAbility,
    FVector InTargetLocation,
    float InAcceptanceRadius)
{
    UAbilityTask_WaitMoveToLocation* Task = NewAbilityTask<UAbilityTask_WaitMoveToLocation>(OwningAbility);
    Task->TargetLocation = InTargetLocation;
    Task->AcceptanceRadius = InAcceptanceRadius;
    Task->TargetActor = nullptr;

    if (OwningAbility)
    {
        Task->SourceCharacter = Cast<ACharacter>(OwningAbility->GetCurrentActorInfo()->AvatarActor.Get());
        if (!Task->SourceCharacter)
        {
            return nullptr;
        }
    }

    return Task;
}

UAbilityTask_WaitMoveToLocation* UAbilityTask_WaitMoveToLocation::WaitMoveToActor(
    UGameplayAbility* OwningAbility,
    AActor* InTargetActor,
    float InAcceptanceRadius)
{
    UAbilityTask_WaitMoveToLocation* Task = NewAbilityTask<UAbilityTask_WaitMoveToLocation>(OwningAbility);
    Task->TargetActor = InTargetActor;
    Task->AcceptanceRadius = InAcceptanceRadius;

    if (OwningAbility)
    {
        Task->SourceCharacter = Cast<ACharacter>(OwningAbility->GetCurrentActorInfo()->AvatarActor.Get());
        if (!Task->SourceCharacter)
        {
            return nullptr;
        }
    }
    return Task;
}

void UAbilityTask_WaitMoveToLocation::Activate()  
{  
    if (!Ability)  
    {  
        EndTask();  
        return;  
    }  

    if (TargetActor)  
    {  
        // 动态目标：定时重启 MoveTo 以追踪目标  
        UpdateMoveToActor();  
        SourceCharacter->GetWorldTimerManager().SetTimer(  
            RepathTimerHandle, this,  
            &UAbilityTask_WaitMoveToLocation::UpdateMoveToActor,  
            0.5f, true); // 每0.5秒更新目标位置  
    }  
    else  
    {  
        if (!TargetLocation.IsZero()) // 修复：检查 TargetLocation 是否为零向量  
        {  
            UpdateMoveToLocation();
        }  
        else  
        {  
            UE_LOG(LogTemp, Warning, TEXT("Target为空，无法向目标移动"));  
            OnMoveFailed.Broadcast();  
            EndTask();  
        }  
    }  
}

void UAbilityTask_WaitMoveToLocation::UpdateMoveToActor()
{
    if (!TargetActor)
    {
        OnMoveFailed.Broadcast();
        EndTask();
        return;
    }

    float Distance = FVector::Dist(SourceCharacter->GetActorLocation(), TargetActor->GetActorLocation());
    if (Distance <= AcceptanceRadius)
    {
        SourceCharacter->GetController()->StopMovement();
        OnMoveFinished.Broadcast();
        EndTask();
        return;
    }

    UAIBlueprintHelperLibrary::SimpleMoveToActor(SourceCharacter->GetController(), TargetActor);
}

void UAbilityTask_WaitMoveToLocation::UpdateMoveToLocation()
{
    if (TargetLocation.IsZero())
    {
        OnMoveFailed.Broadcast();
        EndTask();
        return;
    }

    float Distance = FVector::Dist(SourceCharacter->GetActorLocation(), TargetLocation);
    if (Distance <= AcceptanceRadius)
    {
        AIController->StopMovement();
        OnMoveFinished.Broadcast();
        EndTask();
        return;
    }
    UAIBlueprintHelperLibrary::SimpleMoveToLocation(SourceCharacter->GetController(), TargetLocation);
}

void UAbilityTask_WaitMoveToLocation::OnDestroy(bool bInOwnerFinished)
{
    if (Ability && SourceCharacter && SourceCharacter->IsValidLowLevel())
    {
        SourceCharacter->GetWorldTimerManager().ClearTimer(RepathTimerHandle);
    }

    Super::OnDestroy(bInOwnerFinished);
}


