#pragma once

#include "CoreMinimal.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectPtr.h"
#include "HAL/Platform.h"
#include "Internationalization/Text.h"
// #include "Templates/ObjectPtr.h"

class UTexture2D;

/** HUD 左侧面板展示的综合属性。 */
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

/** 物品/装备面板的单格视图模型。 */
struct FHUDItemSlotViewModel
{
    FText HotkeyLabel = FText::GetEmpty();
    TObjectPtr<UTexture2D> Icon = nullptr;
    int32 StackCount = 0;
    bool bIsActive = false;
};

/** 技能栏上方单个增益/减益图标的轻量视图模型。 */
struct FHUDBuffIconViewModel
{
    FName TagName = NAME_None;          // 来源的 Gameplay Tag（用于调试或作为叠层键）
    TObjectPtr<UTexture2D> Icon = nullptr; // 要显示的图标
    int32 StackCount = 0;               // 可选的层数
    bool bIsDebuff = false;             // 需要时用于区分染色
    float TimeRemaining = -1.f;         // 小于 0 表示隐藏计时文本
    float TotalDuration = -1.f;         // 若使用径向计时器则为其持续时间
};
