// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricPlayerController.h"
#include "IsometricCameraManager.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Character/IsometricRPGCharacter.h"
#include "IsometricComponents/IsometricInputComponent.h" // Added include for UIsometricInputComponent

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
		EIC->BindAction(Action_LeftClick, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleLeftClickInput);
		EIC->BindAction(Action_RightClick, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleRightClickInput);
		
		// 绑定技能按键, 使用 EAbilityInputID 枚举替换魔术数字
		EIC->BindAction(Action_A, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillInput, EAbilityInputID::Ability_A);
		EIC->BindAction(Action_Q, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillInput, EAbilityInputID::Ability_Q);
		EIC->BindAction(Action_W, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillInput, EAbilityInputID::Ability_W);
		EIC->BindAction(Action_E, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillInput, EAbilityInputID::Ability_E);
		EIC->BindAction(Action_R, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillInput, EAbilityInputID::Ability_R);
		EIC->BindAction(Action_Summoner1, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillInput, EAbilityInputID::Ability_Summoner1);
		EIC->BindAction(Action_Summoner2, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillInput, EAbilityInputID::Ability_Summoner2);
	}
}

void AIsometricPlayerController::HandleRightClickInput(const FInputActionValue& Value)
{
	FHitResult HitResult;
	GetHitResultUnderCursor(ECC_Visibility, true, HitResult);
	
	if (AIsometricRPGCharacter* MyChar = Cast<AIsometricRPGCharacter>(GetPawn()))
	{
		if (UIsometricInputComponent* InputComp = MyChar->FindComponentByClass<UIsometricInputComponent>())
		{
			InputComp->HandleRightClick(HitResult); // Pass HitResult
		}

	}
}
void AIsometricPlayerController::HandleLeftClickInput(const FInputActionValue& Value)
{
	FHitResult HitResult;
	GetHitResultUnderCursor(ECC_Visibility, true, HitResult);

	if (AIsometricRPGCharacter* MyChar = Cast<AIsometricRPGCharacter>(GetPawn()))
	{
		if (UIsometricInputComponent* InputComp = MyChar->FindComponentByClass<UIsometricInputComponent>())
		{
			InputComp->HandleLeftClick(HitResult); // Pass HitResult
		}
	}
}


void AIsometricPlayerController::HandleSkillInput(EAbilityInputID InputID)
{
	FHitResult HitResult; 
	GetHitResultUnderCursor(ECC_Visibility, true, HitResult);

   if (AIsometricRPGCharacter* MyChar = Cast<AIsometricRPGCharacter>(GetPawn()))  
   {  
       if (UIsometricInputComponent* InputComp = MyChar->FindComponentByClass<UIsometricInputComponent>())  
       {  
           InputComp->HandleSkillInput(InputID, HitResult); // 将枚举传递下去
       }  
   }  
}

