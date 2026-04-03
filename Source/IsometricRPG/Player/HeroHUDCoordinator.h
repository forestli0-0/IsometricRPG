#pragma once

#include "CoreMinimal.h"
#include "UI/HUD/HUDRefreshTypes.h"

class AIsoPlayerState;
class AIsometricPlayerController;

class ISOMETRICRPG_API FHeroHUDCoordinator
{
public:
    void Initialize(AIsometricPlayerController& InController);
    void HandlePlayerStateChanged(AIsoPlayerState* InPlayerState);
    void NotifyCooldownTriggered(const FGameplayAbilitySpecHandle& SpecHandle, float DurationSeconds);
    void Reset();

private:
    void EnsureHUDCreated();
    void UnbindHUDRefreshSource();
    void HandleHUDRefreshRequested(const FHeroHUDRefreshRequest& Request);
    void RefreshHUD(const FHeroHUDRefreshRequest& Request);
    bool ResolveController(AIsometricPlayerController*& OutController) const;
    AIsoPlayerState* ResolvePlayerState(AIsometricPlayerController& Controller) const;

    TWeakObjectPtr<AIsometricPlayerController> OwningController;
    TWeakObjectPtr<AIsoPlayerState> BoundHUDRefreshSource;
    FDelegateHandle HUDRefreshRequestedHandle;
};
