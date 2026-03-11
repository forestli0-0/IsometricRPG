#include "IsometricAbilities/AbilityTasks/AbilityTask_WaitMoveToLocation.h"

#include "Abilities/GameplayAbility.h"
#include "GameFramework/Character.h"
#include "IsometricComponents/IsometricPathFollowingComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

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
        if (!Task->SourceCharacter.IsValid())
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
        if (!Task->SourceCharacter.IsValid())
        {
            return nullptr;
        }
    }
    return Task;
}

void UAbilityTask_WaitMoveToLocation::Activate()
{
    if (!(Ability && SourceCharacter.IsValid()))
    {
        FailMove();
        return;
    }

    const bool bHasValidGoal = TargetActor.IsValid() || !TargetLocation.IsZero();
    if (!bHasValidGoal)
    {
        UE_LOG(LogTemp, Warning, TEXT("Target为空，无法向目标移动"));
        FailMove();
        return;
    }

    PathFollowingComponent = SourceCharacter->FindComponentByClass<UIsometricPathFollowingComponent>();
    if (PathFollowingComponent.IsValid())
    {
        PathFollowingFinishedHandle = PathFollowingComponent->OnMoveFinished.AddUObject(
            this,
            &UAbilityTask_WaitMoveToLocation::HandlePathFollowingFinished);
    }

    if (HasReachedDestination())
    {
        FinishMoveSuccessfully();
        return;
    }

    if (CanDrivePathFollowing())
    {
        if (!PathFollowingComponent.IsValid())
        {
            FailMove();
            return;
        }

        const bool bStartedMove = TargetActor.IsValid()
            ? PathFollowingComponent->RequestMoveToActor(TargetActor.Get(), AcceptanceRadius)
            : PathFollowingComponent->RequestMoveToLocation(TargetLocation, AcceptanceRadius);

        if (!bStartedMove)
        {
            if (HasReachedDestination())
            {
                FinishMoveSuccessfully();
            }
            else
            {
                FailMove();
            }
            return;
        }

        bIssuedMoveRequest = true;
    }

    // 远端玩家在服务端那份 ability task 不主动驱动位移，只轮询何时真正进入施法距离。
    if (UWorld* World = SourceCharacter->GetWorld())
    {
        World->GetTimerManager().SetTimer(
            ProgressTimerHandle,
            this,
            &UAbilityTask_WaitMoveToLocation::PollMoveProgress,
            ProgressPollInterval,
            true);
    }

    PollMoveProgress();
}

void UAbilityTask_WaitMoveToLocation::OnDestroy(bool bInOwnerFinished)
{
    CleanupTaskState();
    Super::OnDestroy(bInOwnerFinished);
}

bool UAbilityTask_WaitMoveToLocation::CanDrivePathFollowing() const
{
    if (!(SourceCharacter.IsValid() && SourceCharacter->GetController()))
    {
        return false;
    }

    if (SourceCharacter->IsPlayerControlled())
    {
        return SourceCharacter->IsLocallyControlled();
    }

    return SourceCharacter->HasAuthority();
}

bool UAbilityTask_WaitMoveToLocation::HasReachedDestination() const
{
    if (!SourceCharacter.IsValid())
    {
        return false;
    }

    FVector GoalLocation = FVector::ZeroVector;
    if (TargetActor.IsValid())
    {
        GoalLocation = TargetActor->GetActorLocation();
    }
    else if (!TargetLocation.IsZero())
    {
        GoalLocation = TargetLocation;
    }
    else
    {
        return false;
    }

    const FVector Delta = GoalLocation - SourceCharacter->GetActorLocation();
    return FVector2D(Delta.X, Delta.Y).SizeSquared() <= FMath::Square(AcceptanceRadius);
}

void UAbilityTask_WaitMoveToLocation::PollMoveProgress()
{
    if (!SourceCharacter.IsValid())
    {
        FailMove();
        return;
    }

    if (!TargetActor.IsValid() && !TargetLocation.IsZero())
    {
        // 静态目标点不需要额外校验。
    }
    else if (!TargetActor.IsValid() && TargetLocation.IsZero())
    {
        FailMove();
        return;
    }

    if (HasReachedDestination())
    {
        if (bIssuedMoveRequest && PathFollowingComponent.IsValid() && PathFollowingComponent->HasActiveMove())
        {
            // 让 PathFollower 统一结束并分发结果，避免任务和组件各自收尾。
            PathFollowingComponent->StopMove(EIsometricPathFollowResult::Succeeded);
            return;
        }

        FinishMoveSuccessfully();
        return;
    }

    if (CanDrivePathFollowing() && bIssuedMoveRequest)
    {
        if (!(PathFollowingComponent.IsValid() && PathFollowingComponent->HasActiveMove()))
        {
            FailMove();
        }
    }
}

void UAbilityTask_WaitMoveToLocation::HandlePathFollowingFinished(EIsometricPathFollowResult Result)
{
    if (Result == EIsometricPathFollowResult::Succeeded || HasReachedDestination())
    {
        FinishMoveSuccessfully();
        return;
    }

    FailMove();
}

void UAbilityTask_WaitMoveToLocation::CleanupTaskState()
{
    if (SourceCharacter.IsValid())
    {
        SourceCharacter->GetWorldTimerManager().ClearTimer(ProgressTimerHandle);
    }

    if (PathFollowingComponent.IsValid() && PathFollowingFinishedHandle.IsValid())
    {
        PathFollowingComponent->OnMoveFinished.Remove(PathFollowingFinishedHandle);
    }

    PathFollowingFinishedHandle.Reset();
    bIssuedMoveRequest = false;
}

void UAbilityTask_WaitMoveToLocation::FinishMoveSuccessfully()
{
    CleanupTaskState();

    if (ShouldBroadcastAbilityTaskDelegates())
    {
        OnMoveFinished.Broadcast();
    }

    EndTask();
}

void UAbilityTask_WaitMoveToLocation::FailMove()
{
    CleanupTaskState();

    if (ShouldBroadcastAbilityTaskDelegates())
    {
        OnMoveFailed.Broadcast();
    }

    EndTask();
}


