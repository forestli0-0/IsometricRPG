// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricPlayerController.h"
#include "IsometricCameraManager.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Character/IsometricRPGCharacter.h"
#include "IsometricComponents/IsometricInputComponent.h"
#include "Blueprint/UserWidget.h"
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

	if (IsLocalController() && PlayerHUDClass)
	{
		PlayerHUD = CreateWidget<UUserWidget>(this, PlayerHUDClass);
		if (PlayerHUD)
		{
			PlayerHUD->AddToViewport();
		}
	}
}
// 【新增】实现PlayerTick函数
void AIsometricPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    // 如果右键被按住，则持续处理
    if (bIsRightMouseDown)
    {
        FHitResult HitResult;
        GetHitResultUnderCursor(ECC_Visibility, true, HitResult);

        if (AIsometricRPGCharacter* MyChar = Cast<AIsometricRPGCharacter>(GetPawn()))
        {
            if (UIsometricInputComponent* InputComp = MyChar->FindComponentByClass<UIsometricInputComponent>())
            {
                // 持续调用Triggered函数
                InputComp->HandleRightClickTriggered(HitResult, LastHitActor);
                LastHitActor = HitResult.GetActor(); // 更新上一个目标
            }
        }
    }
}
void AIsometricPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EIC->BindAction(Action_LeftClick, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleLeftClickInput);
		// 【修改】绑定右键的Started, Triggered, Completed事件
		EIC->BindAction(Action_RightClick, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleRightClickStarted);
		EIC->BindAction(Action_RightClick, ETriggerEvent::Completed, this, &AIsometricPlayerController::HandleRightClickCompleted);
		
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

void AIsometricPlayerController::HandleRightClickStarted(const FInputActionValue& Value)
{
	bIsRightMouseDown = true; // 设置右键按下状态
    LastHitActor = nullptr; // 重置上一个目标

	FHitResult HitResult;
	GetHitResultUnderCursor(ECC_Visibility, true, HitResult);
	
	if (AIsometricRPGCharacter* MyChar = Cast<AIsometricRPGCharacter>(GetPawn()))
	{
		if (UIsometricInputComponent* InputComp = MyChar->FindComponentByClass<UIsometricInputComponent>())
		{
            // 第一次点击时，就直接触发一次逻辑
			InputComp->HandleRightClickTriggered(HitResult, LastHitActor);
            LastHitActor = HitResult.GetActor();
		}

	}
}
// 【新增】处理右键松开
void AIsometricPlayerController::HandleRightClickCompleted(const FInputActionValue& Value)
{
    bIsRightMouseDown = false; // 重置右键按下状态
    LastHitActor = nullptr; // 清空缓存
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

