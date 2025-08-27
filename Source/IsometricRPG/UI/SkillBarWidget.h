// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillSlotWidget.h"
#include "SkillBarWidget.generated.h"

/**
 * 
 */
UCLASS()
class ISOMETRICRPG_API USkillBarWidget : public UUserWidget
{
	GENERATED_BODY()
public:
    // 创建一个数组来存放所有的技能槽引用，设置为BlueprintReadOnly以便蓝图可以填充它
    UPROPERTY(BlueprintReadOnly, Category = "SkillBar")
    TArray<TObjectPtr<USkillSlotWidget>> SkillSlots;
};
