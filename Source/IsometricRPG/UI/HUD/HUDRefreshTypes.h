#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"

enum class EHeroHUDRefreshKind : uint8
{
    Full,
    Vitals,
    Experience,
    ChampionStats,
    GameplayPresentation,
    ActionBar,
    Cooldown
};

struct FHeroHUDRefreshRequest
{
    EHeroHUDRefreshKind Kind = EHeroHUDRefreshKind::Full;
    FGameplayAbilitySpecHandle SpecHandle;
    float DurationSeconds = 0.f;
};
