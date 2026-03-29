#include "Input/IsometricInputSessionManager.h"

const FAbilityInputSession* FIsometricInputSessionManager::FindSession(EAbilityInputID InputID) const
{
    return ActiveSessions.Find(InputID);
}

bool FIsometricInputSessionManager::HasActiveSession(EAbilityInputID InputID) const
{
    const FAbilityInputSession* Session = ActiveSessions.Find(InputID);
    return Session && Session->bIsActive;
}

bool FIsometricInputSessionManager::HasBlockingSession(EAbilityInputID InputID) const
{
    for (const TPair<EAbilityInputID, FAbilityInputSession>& Pair : ActiveSessions)
    {
        if (Pair.Key != InputID && Pair.Value.bIsActive)
        {
            return true;
        }
    }

    return false;
}

bool FIsometricInputSessionManager::HasAnyActiveSession() const
{
    for (const TPair<EAbilityInputID, FAbilityInputSession>& Pair : ActiveSessions)
    {
        if (Pair.Value.bIsActive)
        {
            return true;
        }
    }

    return false;
}

void FIsometricInputSessionManager::BeginSession(const FPlayerInputCommand& Command, const FAbilityInputPolicy& Policy)
{
    FAbilityInputSession& Session = ActiveSessions.FindOrAdd(Command.AbilityInputID);
    Session.AbilityInputID = Command.AbilityInputID;
    Session.InputMode = Policy.InputMode;
    Session.StartWorldTime = Command.WorldTime;
    Session.LastUpdateWorldTime = Command.WorldTime;
    Session.HeldDuration = 0.0f;
    Session.LatestHitResult = Command.HitResult;
    Session.LatestTargetActor = Command.TargetActor;
    Session.bIsActive = true;
}

bool FIsometricInputSessionManager::UpdateSession(
    const FPlayerInputCommand& Command,
    const FAbilityInputPolicy& Policy,
    FAbilityInputSession& OutSession)
{
    FAbilityInputSession* Session = ActiveSessions.Find(Command.AbilityInputID);
    if (!Session || !Session->bIsActive)
    {
        return false;
    }

    Session->InputMode = Policy.InputMode;
    Session->LastUpdateWorldTime = Command.WorldTime;
    Session->HeldDuration = FMath::Max(Command.WorldTime - Session->StartWorldTime, Command.HeldDuration);
    Session->LatestHitResult = Command.HitResult;
    Session->LatestTargetActor = Command.TargetActor;
    OutSession = *Session;
    return true;
}

bool FIsometricInputSessionManager::CommitSession(
    const FPlayerInputCommand& Command,
    const FAbilityInputPolicy& Policy,
    FAbilityInputSession& OutSession)
{
    if (FAbilityInputSession* Session = ActiveSessions.Find(Command.AbilityInputID))
    {
        Session->InputMode = Policy.InputMode;
        Session->LastUpdateWorldTime = Command.WorldTime;
        Session->HeldDuration = FMath::Max(Command.WorldTime - Session->StartWorldTime, Command.HeldDuration);
        Session->LatestHitResult = Command.HitResult;
        Session->LatestTargetActor = Command.TargetActor;
        OutSession = *Session;
        ActiveSessions.Remove(Command.AbilityInputID);
        return true;
    }

    OutSession.AbilityInputID = Command.AbilityInputID;
    OutSession.InputMode = Policy.InputMode;
    OutSession.StartWorldTime = Command.WorldTime;
    OutSession.LastUpdateWorldTime = Command.WorldTime;
    OutSession.HeldDuration = Command.HeldDuration;
    OutSession.LatestHitResult = Command.HitResult;
    OutSession.LatestTargetActor = Command.TargetActor;
    OutSession.bIsActive = false;
    return false;
}

bool FIsometricInputSessionManager::CancelSession(EAbilityInputID InputID, FAbilityInputSession& OutSession)
{
    if (FAbilityInputSession* Session = ActiveSessions.Find(InputID))
    {
        OutSession = *Session;
        ActiveSessions.Remove(InputID);
        return true;
    }

    return false;
}

void FIsometricInputSessionManager::BufferCommand(const FPlayerInputCommand& Command, const FAbilityInputPolicy& Policy)
{
    if (!Policy.bAllowInputBuffer)
    {
        return;
    }

    BufferedCommand.Command = Command;
    BufferedCommand.ExpireWorldTime = Command.WorldTime + FMath::Max(Policy.MaxBufferWindow, 0.0f);
    BufferedCommand.bOccupied = true;
}

bool FIsometricInputSessionManager::HasBufferedCommand() const
{
    return BufferedCommand.bOccupied;
}

bool FIsometricInputSessionManager::ConsumeBufferedCommand(float CurrentTime, FPlayerInputCommand& OutCommand)
{
    if (!BufferedCommand.bOccupied)
    {
        return false;
    }

    if (CurrentTime > BufferedCommand.ExpireWorldTime)
    {
        ClearBufferedCommand();
        return false;
    }

    OutCommand = BufferedCommand.Command;
    ClearBufferedCommand();
    return true;
}

void FIsometricInputSessionManager::ClearBufferedCommand()
{
    BufferedCommand = FBufferedPlayerInputCommand();
}
