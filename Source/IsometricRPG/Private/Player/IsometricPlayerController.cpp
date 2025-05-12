// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/IsometricPlayerController.h"
#include "Player/IsometricCameraManager.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include <Character/IsometricRPGCharacter.h>

AIsometricPlayerController::AIsometricPlayerController()
{
    // 设置使用自定义的PlayerCameraManager
    PlayerCameraManagerClass = AIsometricCameraManager::StaticClass();
}
void AIsometricPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		Subsystem->AddMappingContext(ClickMappingContext, 0);
	}
	// 显示鼠标
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AIsometricPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EIC->BindAction(ClickAction, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleClickInput);

		// 绑定技能按键
		EIC->BindAction(Action_Q, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillInput, 1);
		EIC->BindAction(Action_E, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillInput, 2);
		EIC->BindAction(Action_R, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillInput, 3);
		EIC->BindAction(Action_C, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillInput, 4);
		EIC->BindAction(Action_Summoner1, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillInput, 5);
		EIC->BindAction(Action_Summoner2, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillInput, 6);
	}
}

void AIsometricPlayerController::HandleClickInput(const FInputActionValue& Value)
{
	// 找到控制的角色
	if (AIsometricRPGCharacter* MyChar = Cast<AIsometricRPGCharacter>(GetPawn()))
	{
		if (UIsometricInputComponent* InputComp = MyChar->FindComponentByClass<UIsometricInputComponent>())
		{
			InputComp->HandleClick(); // 交给组件做
		}

	}

}

void AIsometricPlayerController::SetTargetActor(AActor* NewTargetActor)
{
	TargetActor = NewTargetActor;
}

void AIsometricPlayerController::HandleSkillInput(const FInputActionInstance& Instance, int SkillIndex)
{
	const UInputAction* TriggeredAction = Instance.GetSourceAction();

   // 找到控制的角色  
   if (AIsometricRPGCharacter* MyChar = Cast<AIsometricRPGCharacter>(GetPawn()))  
   {  
       if (UIsometricInputComponent* InputComp = MyChar->FindComponentByClass<UIsometricInputComponent>())  
       {  
           InputComp->HandleSkillInput(SkillIndex);
       }  
   }  
}
