// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include <InputActionValue.h>
#include "EnhancedInput/Public/InputMappingContext.h" // Add this include
#include "GameFramework/Character.h"
#include "IsometricInputComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ISOMETRICRPG_API UIsometricInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UIsometricInputComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
public:
	void SetupInput(class UEnhancedInputComponent* InputComponent, class APlayerController* PlayerController);

	void HandleClick();
protected:
	void Move(const FInputActionValue& Value);
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* MappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* MoveAction;
	
private:
	void MoveToLocation(ACharacter* Character, const FVector& Destination)
	{
		if (!Character) return;
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			// 方法1：直接调用移动输入（推荐）
			FVector Direction = (Destination - Character->GetActorLocation()).GetSafeNormal();
			PC->GetPawn()->AddMovementInput(Direction, 1.0f);
		}
		
	}
};
