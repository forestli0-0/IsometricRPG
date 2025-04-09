// Fill out your copyright notice in the Description page of Project Settings.

#include "IsometricComponents/IsometricInputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInput/Public/InputMappingContext.h"
#include "EnhancedInput/Public/InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/IsometricRPGCharacter.h" // Add this include to define AIsometricRPGCharacter
#include "AIController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

// Sets default values for this component's properties
UIsometricInputComponent::UIsometricInputComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

// Called when the game starts
void UIsometricInputComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}

// Called every frame
void UIsometricInputComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UIsometricInputComponent::SetupInput(UEnhancedInputComponent* InputComponent, APlayerController* PlayerController)
{
	if (!MappingContext || !MoveAction) return;

	// 添加映射上下文
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(MappingContext, 0);
	}

	// 绑定输入
	InputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &UIsometricInputComponent::Move);
}

void UIsometricInputComponent::Move(const FInputActionValue& Value)
{
    FVector2D Input = Value.Get<FVector2D>();
    if (Input.IsNearlyZero()) return;

    // 获取宿主角色
    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar) return;

    // 设置控制器旋转而不是直接旋转角色
    // 这会间接触发CharacterMovement的bOrientRotationToMovement
    FRotator YawRotation(0, OwnerChar->GetControlRotation().Yaw, 0);
    const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    // 计算移动方向
    FVector MoveDirection = Forward * Input.Y + Right * Input.X;

    // 正常化并应用移动输入
    if (!MoveDirection.IsNearlyZero())
    {
        MoveDirection.Normalize();
        OwnerChar->AddMovementInput(MoveDirection);
    }
}


 // Add this include to resolve the AIBlueprintHelperLibrary reference

void UIsometricInputComponent::HandleClick()
{
   FHitResult Hit;
   ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
   if (!OwnerCharacter) return;

   APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
   if (!PC) return;

   PC->GetHitResultUnderCursor(ECC_Visibility, true, Hit);
   if (Hit.bBlockingHit)
   {
       // 检查是否点击了敌人
       AActor* HitActor = Hit.GetActor();
       if (HitActor && HitActor->ActorHasTag("Enemy"))
       {
           // TODO: 攻击逻辑
                    // GAS 攻击逻辑
           //if (AIsometricRPGCharacter* MyChar = Cast<AIsometricRPGCharacter>(GetOwner()))
           //{
           //    MyChar->GetAbilitySystemComponent()->TryActivateAbilityByClass(URPGGameplayAbility_Attack::StaticClass());
           //}
       }
       else
       {
           // 移动逻辑
           UAIBlueprintHelperLibrary::SimpleMoveToLocation(PC, Hit.ImpactPoint); // Corrected namespace and class name
           
       }
   }
}


