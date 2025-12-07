#pragma once

#include "CoreMinimal.h"
#include "IsoDamageType.generated.h"

/**
 * Unified elemental/physical damage channels for attribute-driven calculations.
 */
UENUM(BlueprintType)
enum class EIsoDamageType : uint8
{
    Physical UMETA(DisplayName = "Physical"),
    Magic UMETA(DisplayName = "Magic"),
    Fire UMETA(DisplayName = "Fire"),
    Ice UMETA(DisplayName = "Ice"),
    Lightning UMETA(DisplayName = "Lightning")
};

