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
	UInputAction* ClickAction;
	UPROPERTY(EditDefaultsOnly)
	UInputAction* Action_Q;
	UPROPERTY(EditDefaultsOnly)
	UInputAction* Action_E;
	UPROPERTY(EditDefaultsOnly)
	UInputAction* Action_R;
	UPROPERTY(EditDefaultsOnly)
	UInputAction* Action_C;
	UPROPERTY(EditDefaultsOnly)
	UInputAction* Action_Summoner1;
	UPROPERTY(EditDefaultsOnly)
	UInputAction* Action_Summoner2;

	UFUNCTION()
	void HandleClickInput(const FInputActionValue& Value);
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
