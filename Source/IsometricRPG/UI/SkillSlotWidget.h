// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillSlotData.h"
#include "SkillSlotWidget.generated.h"

/**
 * 
 */
UCLASS()
class ISOMETRICRPG_API USkillSlotWidget : public UUserWidget
{
	GENERATED_BODY()
public:
    // BlueprintImplementableEvent: 声明一个可在蓝图中实现的函数
    // C++可以调用它，但它的功能逻辑完全在WBP_SkillSlot的事件图表中定义
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "SkillSlot")
    void UpdateSlot(const FSkillSlotData& SkillData);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "SkillSlot")
    void StartCooldown(float CooldownDuration);
};
