#include "Input/IsometricInputRouter.h"

FPlayerInputCommand FIsometricInputRouter::RoutePointerInput(const FCursorInputSnapshot& Snapshot) const
{
    FPlayerInputCommand Command;
    Command.TriggerPhase = Snapshot.Phase;
    Command.HitResult = Snapshot.HitResult;
    Command.TargetActor = Snapshot.HitActor;
    Command.TargetLocation = Snapshot.HitResult.ImpactPoint;
    Command.WorldTime = Snapshot.WorldTime;
    Command.HeldDuration = Snapshot.HeldDuration;

    switch (Snapshot.SourceKind)
    {
    case EPlayerInputSourceKind::LeftMouse:
        if (Snapshot.Phase != EInputEventPhase::Pressed)
        {
            return Command;
        }

        Command.Type = Snapshot.HitActor ? EPlayerInputCommandType::SelectTarget : EPlayerInputCommandType::ClearSelection;
        return Command;

    case EPlayerInputSourceKind::RightMouse:
        if (Snapshot.Phase != EInputEventPhase::Pressed && Snapshot.Phase != EInputEventPhase::Held)
        {
            return Command;
        }

        if (Snapshot.bHitEnemy && Snapshot.HitActor)
        {
            Command.Type = EPlayerInputCommandType::BasicAttackTarget;
            return Command;
        }

        if (Snapshot.bBlockingHit)
        {
            Command.Type = EPlayerInputCommandType::MoveToLocation;
            return Command;
        }

        return Command;

    case EPlayerInputSourceKind::AbilitySlot:
    default:
        return Command;
    }
}

FPlayerInputCommand FIsometricInputRouter::RouteAbilityInput(const FCursorInputSnapshot& Snapshot, const FAbilityInputPolicy& Policy) const
{
    switch (Snapshot.Phase)
    {
    case EInputEventPhase::Pressed:
        if (Policy.InputMode == EAbilityInputMode::Instant)
        {
            return BuildAbilityCommand(EPlayerInputCommandType::CommitAbilityInput, Snapshot, Policy);
        }

        return BuildAbilityCommand(EPlayerInputCommandType::BeginAbilityInput, Snapshot, Policy);

    case EInputEventPhase::Held:
        if (Policy.InputMode == EAbilityInputMode::Instant)
        {
            return FPlayerInputCommand();
        }

        if (Policy.bUpdateTargetWhileHeld
            || Policy.InputMode == EAbilityInputMode::ChargeOnHold
            || Policy.InputMode == EAbilityInputMode::ChannelOnHold)
        {
            return BuildAbilityCommand(EPlayerInputCommandType::UpdateAbilityInput, Snapshot, Policy);
        }

        return FPlayerInputCommand();

    case EInputEventPhase::Released:
        if (Policy.InputMode == EAbilityInputMode::Instant)
        {
            return FPlayerInputCommand();
        }

        return BuildAbilityCommand(EPlayerInputCommandType::CommitAbilityInput, Snapshot, Policy);

    case EInputEventPhase::Cancelled:
        return BuildAbilityCommand(EPlayerInputCommandType::CancelAbilityInput, Snapshot, Policy);

    default:
        return FPlayerInputCommand();
    }
}

FPlayerInputCommand FIsometricInputRouter::BuildAbilityCommand(
    EPlayerInputCommandType CommandType,
    const FCursorInputSnapshot& Snapshot,
    const FAbilityInputPolicy& Policy) const
{
    FPlayerInputCommand Command;
    Command.Type = CommandType;
    Command.AbilityInputID = Snapshot.AbilityInputID;
    Command.TriggerPhase = Snapshot.Phase;
    Command.HitResult = Snapshot.HitResult;
    Command.TargetActor = Snapshot.HitActor;
    Command.TargetLocation = Snapshot.HitResult.ImpactPoint;
    Command.WorldTime = Snapshot.WorldTime;
    Command.HeldDuration = Snapshot.HeldDuration;
    Command.bAllowBuffering = Policy.bAllowInputBuffer;
    return Command;
}
