#pragma once

#include "CoreMinimal.h"
#include "Input/IsometricInputTypes.h"
#include "IsometricRPG/IsometricAbilities/Types/HeroAbilityTypes.h"

class AActor;
class AIsometricPlayerController;

struct FHeroCursorInputRouterConfig
{
    float HeldMoveRepathInterval = 0.1f;
    float HeldMoveRepathDistance = 75.0f;
    float HeldAttackRetryInterval = 0.25f;
};

class ISOMETRICRPG_API FHeroCursorInputRouter
{
public:
    void Tick(
        AIsometricPlayerController& Controller,
        const FHeroCursorInputRouterConfig& Config,
        float DeltaTime);
    void HandleLeftClick(AIsometricPlayerController& Controller);
    void HandleRightClickStarted(
        AIsometricPlayerController& Controller,
        const FHeroCursorInputRouterConfig& Config);
    void HandleRightClickCompleted(AIsometricPlayerController& Controller);
    void HandleSkillPressed(AIsometricPlayerController& Controller, EAbilityInputID InputID);
    void HandleSkillHeld(AIsometricPlayerController& Controller, EAbilityInputID InputID);
    void HandleSkillReleased(AIsometricPlayerController& Controller, EAbilityInputID InputID);

private:
    static bool IsEnemyUnderCursor(const AActor* HitActor, const AActor* OwnerActor);

    bool bIsRightMouseDown = false;
    TWeakObjectPtr<AActor> LastHitActor;
    FVector LastHeldMoveLocation = FVector::ZeroVector;
    float NextHeldMoveCommandTime = 0.0f;
    float NextHeldAttackCommandTime = 0.0f;
    bool bHasLastHeldMoveLocation = false;
    TMap<EAbilityInputID, float> AbilityPressedAtTime;
};
