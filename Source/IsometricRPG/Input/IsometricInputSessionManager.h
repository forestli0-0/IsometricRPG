#pragma once

#include "Input/IsometricInputTypes.h"

class FIsometricInputSessionManager
{
public:
    const FAbilityInputSession* FindSession(EAbilityInputID InputID) const;
    bool HasActiveSession(EAbilityInputID InputID) const;
    bool HasBlockingSession(EAbilityInputID InputID) const;
    bool HasAnyActiveSession() const;

    void BeginSession(const FPlayerInputCommand& Command, const FAbilityInputPolicy& Policy);
    bool UpdateSession(const FPlayerInputCommand& Command, const FAbilityInputPolicy& Policy, FAbilityInputSession& OutSession);
    bool CommitSession(const FPlayerInputCommand& Command, const FAbilityInputPolicy& Policy, FAbilityInputSession& OutSession);
    bool CancelSession(EAbilityInputID InputID, FAbilityInputSession& OutSession);

    void BufferCommand(const FPlayerInputCommand& Command, const FAbilityInputPolicy& Policy);
    bool HasBufferedCommand() const;
    bool ConsumeBufferedCommand(float CurrentTime, FPlayerInputCommand& OutCommand);
    void ClearBufferedCommand();

private:
    TMap<EAbilityInputID, FAbilityInputSession> ActiveSessions;
    FBufferedPlayerInputCommand BufferedCommand;
};
