// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h" // Add this include to fix the error
#include "IsometricPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;

UCLASS()
class ISOMETRICRPG_API AIsometricPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly)
	UInputMappingContext* ClickMappingContext;

	UPROPERTY(EditDefaultsOnly)
	UInputAction* ClickAction;

	UFUNCTION()
	void HandleClickInput(const FInputActionValue& Value);

public:
	UPROPERTY(EditDefaultsOnly)
	AActor* TargetActor;

	UFUNCTION(BlueprintCallable)
	void SetTargetActor(AActor* NewTargetActor);

};
