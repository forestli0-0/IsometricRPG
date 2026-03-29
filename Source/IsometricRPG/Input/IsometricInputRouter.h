#pragma once

#include "Input/IsometricInputTypes.h"

class FIsometricInputRouter
{
public:
    FPlayerInputCommand RoutePointerInput(const FCursorInputSnapshot& Snapshot) const;
    FPlayerInputCommand RouteAbilityInput(const FCursorInputSnapshot& Snapshot, const FAbilityInputPolicy& Policy) const;

private:
    FPlayerInputCommand BuildAbilityCommand(
        EPlayerInputCommandType CommandType,
        const FCursorInputSnapshot& Snapshot,
        const FAbilityInputPolicy& Policy) const;
};
