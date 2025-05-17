// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "IsometricPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;

UCLASS()
class ISOMETRICRPG_API AIsometricPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AIsometricPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly)
	UInputMappingContext* ClickMappingContext;

	UPROPERTY(EditDefaultsOnly)
	UInputAction* Action_LeftClick;
	UPROPERTY(EditDefaultsOnly)
	UInputAction* Action_RightClick;
	UPROPERTY(EditDefaultsOnly)
	UInputAction* Action_Q;
	UPROPERTY(EditDefaultsOnly)
	UInputAction* Action_W;
	UPROPERTY(EditDefaultsOnly)
	UInputAction* Action_E;
	UPROPERTY(EditDefaultsOnly)
	UInputAction* Action_R;
	UPROPERTY(EditDefaultsOnly)
	UInputAction* Action_Summoner1;
	UPROPERTY(EditDefaultsOnly)
	UInputAction* Action_Summoner2;
	// 处理鼠标左键点击: 选择目标，可以是敌人也可以是友方
	UFUNCTION()
	void HandleLeftClickInput(const FInputActionValue& Value);
	// 处理鼠标右键点击: 移动或者攻击目标
	UFUNCTION()
	void HandleRightClickInput(const FInputActionValue& Value);

protected:
	// 处理技能输入
	UFUNCTION()
	void HandleSkillInput(const FInputActionInstance& Instance, int SkillIndex);
public:
	UPROPERTY(EditDefaultsOnly)
	AActor* TargetActor;

	UFUNCTION(BlueprintCallable)
	void SetTargetActor(AActor* NewTargetActor);

};
