#pragma once

#include "CoreMinimal.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectPtr.h"
#include "HAL/Platform.h"
#include "Internationalization/Text.h"
// #include "Templates/ObjectPtr.h"

class UTexture2D;

/** Aggregate stats shown on the far-left panel of the HUD. */
struct FHUDChampionStatsViewModel
{
    float AttackDamage = 0.f;
    float AbilityPower = 0.f;
    float Armor = 0.f;
    float MagicResist = 0.f;
    float AttackSpeed = 0.f;
    float CritChance = 0.f;
    float MoveSpeed = 0.f;
};

/** Simple per-slot view model for the item/equipment panel. */
struct FHUDItemSlotViewModel
{
    FText HotkeyLabel = FText::GetEmpty();
    TObjectPtr<UTexture2D> Icon = nullptr;
    int32 StackCount = 0;
    bool bIsActive = false;
};

/** Lightweight view model for a single buff/debuff icon above the action bar. */
struct FHUDBuffIconViewModel
{
    FName TagName = NAME_None;          // Source gameplay tag (for debugging/stacking key)
    TObjectPtr<UTexture2D> Icon = nullptr; // Icon to display
    int32 StackCount = 0;               // Optional stacks
    bool bIsDebuff = false;             // For tinting if needed
    float TimeRemaining = -1.f;         // <0 means hide timer text
    float TotalDuration = -1.f;         // For radial timer if desired
};
