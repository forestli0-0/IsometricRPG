// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "SkillSlotData.generated.h"

// Forward declaration
class UTexture2D;
class UGameplayAbility;

USTRUCT(BlueprintType)
struct FSkillSlotData
{
    GENERATED_BODY()

public:
    FSkillSlotData()
        : AbilityClass(nullptr)
        , Icon(nullptr)
        , CooldownRemaining(0.f)
        , CooldownDuration(0.f)
        , InputHint(FText::GetEmpty())
    {
    }

    // 技能类（GAS）
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UGameplayAbility> AbilityClass;

    // 技能图标
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* Icon;

    // 冷却剩余时间（实时更新）
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CooldownRemaining;

    // 冷却总时长
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CooldownDuration;

    // 对应输入提示（可用于 UI 显示快捷键）
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText InputHint;
};

