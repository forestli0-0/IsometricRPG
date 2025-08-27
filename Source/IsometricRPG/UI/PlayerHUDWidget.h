// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillBarWidget.h"
#include "PlayerHUDWidget.generated.h"

/**
 * 
 */
UCLASS()
class ISOMETRICRPG_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()
public:
    // 使用 meta = (BindWidget) 来绑定一个子控件
    // 变量名 "SkillBar" 必须和你在WBP_PlayerHUD设计器里给WBP_SkillBar控件起的名字完全一样！
    UPROPERTY(meta = (BindWidget))
    USkillBarWidget* SkillBar;
};
