#include "Player/HeroCursorInputRouter.h"

#include "Character/IsometricRPGCharacter.h"
#include "IsometricComponents/IsometricInputComponent.h"
#include "Player/IsometricPlayerController.h"

bool FHeroCursorInputRouter::IsEnemyUnderCursor(const AActor* HitActor, const AActor* OwnerActor)
{
    return HitActor && HitActor != OwnerActor && HitActor->ActorHasTag(FName("Enemy"));
}

void FHeroCursorInputRouter::Tick(
    AIsometricPlayerController& Controller,
    const FHeroCursorInputRouterConfig& Config,
    float DeltaTime)
{
    (void)DeltaTime;

    if (!bIsRightMouseDown)
    {
        return;
    }

    AIsometricRPGCharacter* Character = Cast<AIsometricRPGCharacter>(Controller.GetPawn());
    if (!Character)
    {
        return;
    }

    if (!Controller.ResolveInputComponent())
    {
        return;
    }

    FCursorInputSnapshot Snapshot;
    Controller.BuildCursorInputSnapshot(EPlayerInputSourceKind::RightMouse, EInputEventPhase::Held, Snapshot);

    const float WorldTime = Controller.GetWorld() ? Controller.GetWorld()->GetTimeSeconds() : 0.0f;
    AActor* CurrentHitActor = Snapshot.HitActor;
    if (IsEnemyUnderCursor(CurrentHitActor, Character))
    {
        const bool bTargetChanged = CurrentHitActor != LastHitActor.Get();
        const bool bRetryAttack = !bTargetChanged && WorldTime >= NextHeldAttackCommandTime;
        if (bTargetChanged || bRetryAttack)
        {
            Controller.DispatchInputSnapshot(Snapshot);
            LastHitActor = CurrentHitActor;
            NextHeldAttackCommandTime = WorldTime + Config.HeldAttackRetryInterval;
            bHasLastHeldMoveLocation = false;
        }
        return;
    }

    if (!Snapshot.bBlockingHit)
    {
        return;
    }

    const bool bSwitchedFromActorTarget = LastHitActor.IsValid();
    const FVector2D CurrentMovePoint(Snapshot.HitResult.ImpactPoint.X, Snapshot.HitResult.ImpactPoint.Y);
    const FVector2D PreviousMovePoint(LastHeldMoveLocation.X, LastHeldMoveLocation.Y);
    const bool bMovePointChanged = !bHasLastHeldMoveLocation
        || (CurrentMovePoint - PreviousMovePoint).SizeSquared() >= FMath::Square(Config.HeldMoveRepathDistance);
    const bool bRepathIntervalElapsed = WorldTime >= NextHeldMoveCommandTime;

    if (bSwitchedFromActorTarget || (bMovePointChanged && bRepathIntervalElapsed))
    {
        Controller.DispatchInputSnapshot(Snapshot);
        LastHitActor = nullptr;
        LastHeldMoveLocation = Snapshot.HitResult.ImpactPoint;
        NextHeldMoveCommandTime = WorldTime + Config.HeldMoveRepathInterval;
        bHasLastHeldMoveLocation = true;
    }
}

void FHeroCursorInputRouter::HandleLeftClick(AIsometricPlayerController& Controller)
{
    UE_LOG(LogTemp, Log, TEXT("[PC] LeftClick Started on %s (Auth=%d)"), *Controller.GetName(), Controller.HasAuthority() ? 1 : 0);

    FCursorInputSnapshot Snapshot;
    Controller.BuildCursorInputSnapshot(EPlayerInputSourceKind::LeftMouse, EInputEventPhase::Pressed, Snapshot);
    UE_LOG(
        LogTemp,
        Log,
        TEXT("[PC] LeftClick Hit: Block=%d Loc=%s Actor=%s"),
        Snapshot.bBlockingHit ? 1 : 0,
        *Snapshot.HitResult.ImpactPoint.ToString(),
        Snapshot.HitActor ? *Snapshot.HitActor->GetName() : TEXT("None"));

    Controller.DispatchInputSnapshot(Snapshot);
}

void FHeroCursorInputRouter::HandleRightClickStarted(
    AIsometricPlayerController& Controller,
    const FHeroCursorInputRouterConfig& Config)
{
    bIsRightMouseDown = true;
    LastHitActor = nullptr;
    bHasLastHeldMoveLocation = false;
    NextHeldMoveCommandTime = 0.0f;
    NextHeldAttackCommandTime = 0.0f;

    UE_LOG(LogTemp, Log, TEXT("[PC] RightClick Started on %s (Auth=%d)"), *Controller.GetName(), Controller.HasAuthority() ? 1 : 0);

    FCursorInputSnapshot Snapshot;
    Controller.BuildCursorInputSnapshot(EPlayerInputSourceKind::RightMouse, EInputEventPhase::Pressed, Snapshot);
    UE_LOG(
        LogTemp,
        Log,
        TEXT("[PC] RightClick Hit: Block=%d Loc=%s Actor=%s"),
        Snapshot.bBlockingHit ? 1 : 0,
        *Snapshot.HitResult.ImpactPoint.ToString(),
        Snapshot.HitActor ? *Snapshot.HitActor->GetName() : TEXT("None"));

    Controller.DispatchInputSnapshot(Snapshot);

    const float WorldTime = Controller.GetWorld() ? Controller.GetWorld()->GetTimeSeconds() : 0.0f;
    if (AIsometricRPGCharacter* Character = Cast<AIsometricRPGCharacter>(Controller.GetPawn()))
    {
        if (IsEnemyUnderCursor(Snapshot.HitActor, Character))
        {
            LastHitActor = Snapshot.HitActor;
            NextHeldAttackCommandTime = WorldTime + Config.HeldAttackRetryInterval;
        }
        else if (Snapshot.bBlockingHit)
        {
            LastHeldMoveLocation = Snapshot.HitResult.ImpactPoint;
            NextHeldMoveCommandTime = WorldTime + Config.HeldMoveRepathInterval;
            bHasLastHeldMoveLocation = true;
        }
    }
}

void FHeroCursorInputRouter::HandleRightClickCompleted(AIsometricPlayerController& Controller)
{
    bIsRightMouseDown = false;
    LastHitActor = nullptr;
    bHasLastHeldMoveLocation = false;
    NextHeldMoveCommandTime = 0.0f;
    NextHeldAttackCommandTime = 0.0f;
    UE_LOG(LogTemp, Log, TEXT("[PC] RightClick Completed on %s (Auth=%d)"), *Controller.GetName(), Controller.HasAuthority() ? 1 : 0);
}

void FHeroCursorInputRouter::HandleSkillPressed(AIsometricPlayerController& Controller, const EAbilityInputID InputID)
{
    const float WorldTime = Controller.GetWorld() ? Controller.GetWorld()->GetTimeSeconds() : 0.0f;
    AbilityPressedAtTime.Add(InputID, WorldTime);

    FCursorInputSnapshot Snapshot;
    Controller.BuildCursorInputSnapshot(EPlayerInputSourceKind::AbilitySlot, EInputEventPhase::Pressed, Snapshot, InputID, 0.0f);
    Controller.DispatchInputSnapshot(Snapshot);
}

void FHeroCursorInputRouter::HandleSkillHeld(AIsometricPlayerController& Controller, const EAbilityInputID InputID)
{
    FCursorInputSnapshot Snapshot;
    Controller.BuildCursorInputSnapshot(
        EPlayerInputSourceKind::AbilitySlot,
        EInputEventPhase::Held,
        Snapshot,
        InputID,
        Controller.GetAbilityHeldDuration(AbilityPressedAtTime, InputID));
    Controller.DispatchInputSnapshot(Snapshot);
}

void FHeroCursorInputRouter::HandleSkillReleased(AIsometricPlayerController& Controller, const EAbilityInputID InputID)
{
    FCursorInputSnapshot Snapshot;
    Controller.BuildCursorInputSnapshot(
        EPlayerInputSourceKind::AbilitySlot,
        EInputEventPhase::Released,
        Snapshot,
        InputID,
        Controller.GetAbilityHeldDuration(AbilityPressedAtTime, InputID));
    AbilityPressedAtTime.Remove(InputID);
    Controller.DispatchInputSnapshot(Snapshot);
}
