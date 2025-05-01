// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ABP_MyCharacterAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class ISOMETRICRPG_API UABP_MyCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
    // 这个变量可以在蓝图中读写，也可以在C++中读写
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character State")
    bool bIsDead;

    // 或者，更推荐的方式是使用函数来设置状态，封装性更好
    UFUNCTION(BlueprintCallable, Category = "Character State")
    void SetIsDead(bool NewValue);


};
