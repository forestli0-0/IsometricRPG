// Fill out your copyright notice in the Description page of Project Settings.

#include "IsometricComponents/IsometricInputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInput/Public/InputMappingContext.h"
#include "EnhancedInput/Public/InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/IsometricRPGCharacter.h"
#include "AIController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "IsometricAbilities/RPGGameplayAbility_Attack.h"
#include <Player/IsometricPlayerController.h>
#include <AbilitySystemBlueprintLibrary.h>
#include "GameplayTagContainer.h"
#include "IsometricComponents/ActionQueueComponent.h"
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
    // 取消当前的鼠标命令
    auto OwnerAQC = OwnerChar->FindComponentByClass<UActionQueueComponent>();
	if (OwnerAQC)
	{
        OwnerAQC->ClearCommand();
        AController* Controller = OwnerChar->GetController();
        if (Controller)
        {
            Controller->StopMovement();
        }
        if (USkeletalMeshComponent* Mesh = OwnerChar->GetMesh())
        {
            if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
            {
                AnimInstance->Montage_Stop(0.1f); // 0.2秒的混合时间，可以根据需要调整  
            }
        }
	}
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
        AActor* HitActor = Hit.GetActor();
        FInputCommand Command;

        // 如果已选择了技能且点击了有效目标
        if (CurrentSelectedSkillIndex > 0)
        {
            // 如果选择了敌人作为目标
			if (HitActor && HitActor->ActorHasTag("Enemy"))
			{
				// 设置为技能命令
				Command.CommandType = EInputCommandType::UseSkill;
				Command.SkillIndex = CurrentSelectedSkillIndex;
				Command.TargetLocation = Hit.ImpactPoint;
				Command.TargetActor = HitActor;
				// 使用后清除当前选择的技能
				CurrentSelectedSkillIndex = -1;
				// 查找对应的技能标签
				if (const FGameplayTag* FoundTag = SkillMappings.Find(Command.SkillIndex))
				{
					Command.AbilityTag = *FoundTag;
					UE_LOG(LogTemp, Warning, TEXT("使用技能 %d, %s"), Command.SkillIndex, *Command.AbilityTag.GetTagName().ToString());
				}
			}
			else
			{
				// 如果点击了地面或其他无效目标
                // 向目标位置发射技能
				Command.CommandType = EInputCommandType::UseSkill;
				Command.SkillIndex = CurrentSelectedSkillIndex;
				Command.TargetLocation = Hit.ImpactPoint;
				Command.TargetActor = nullptr; // 没有目标Actor
				// 使用后清除当前选择的技能
				CurrentSelectedSkillIndex = -1;
				// 查找对应的技能标签
				if (const FGameplayTag* FoundTag = SkillMappings.Find(Command.SkillIndex))
				{
					Command.AbilityTag = *FoundTag;
					UE_LOG(LogTemp, Warning, TEXT("使用技能 %d, %s"), Command.SkillIndex, *Command.AbilityTag.GetTagName().ToString());
				}
			}
        }
        else if (HitActor && HitActor->ActorHasTag("Enemy"))
        {
            // 如果点击的是敌人，设置为普通攻击命令
            CurrentSelectedSkillIndex = 0;
            Command.CommandType = EInputCommandType::BasicAttack;
            Command.TargetActor = HitActor;
            Command.TargetLocation = Hit.ImpactPoint;
            Command.SkillIndex = CurrentSelectedSkillIndex;
            CurrentSelectedSkillIndex = -1;
            // 查找对应的技能标签
            if (const FGameplayTag* FoundTag = SkillMappings.Find(Command.SkillIndex))
            {
                Command.AbilityTag = *FoundTag;

                UE_LOG(LogTemp, Warning, TEXT("使用普攻 %d, %s"), Command.SkillIndex, *Command.AbilityTag.GetTagName().ToString());
            }
        }
        else
        {
            // 否则设置为移动命令
            Command.CommandType = EInputCommandType::Movement;
            Command.TargetLocation = Hit.ImpactPoint;
        }

        // 处理命令
        ProcessInputCommand(Command);
    }
}

void UIsometricInputComponent::HandleSkillInput(int32 SkillIndex)
{
    if (SkillIndex >= 0 && SkillMappings.Contains(SkillIndex))
    {
        // 选择技能，等待点击选择目标
        CurrentSelectedSkillIndex = SkillIndex;

        UE_LOG(LogTemp, Display, TEXT("技能 %d 已选择，等待选择目标"), SkillIndex);
    }
}

void UIsometricInputComponent::ProcessInputCommand(const FInputCommand& Command)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    UActionQueueComponent* ActionQueue = OwnerCharacter->FindComponentByClass<UActionQueueComponent>();
    if (!ActionQueue) return;

    // 根据命令类型执行不同的操作
    switch (Command.CommandType)
    {
    case EInputCommandType::BasicAttack:
        if (Command.TargetActor.IsValid())
        {
            ActionQueue->SetCommand_AttackTarget(Command.AbilityTag, Command.TargetLocation, Command.TargetActor.Get());
        }
        break;

    case EInputCommandType::UseSkill:
    {
            ActionQueue->SetCommand_UseSkill(Command.AbilityTag, Command.TargetLocation, Command.TargetActor.Get());
    }
    break;

    case EInputCommandType::Movement:
        ActionQueue->SetCommand_MoveTo(Command.TargetLocation);
        break;
    }
}

AActor* UIsometricInputComponent::GetTargetUnderCursor() const
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return nullptr;

    APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
    if (!PC) return nullptr;

    FHitResult Hit;
    PC->GetHitResultUnderCursor(ECC_Visibility, true, Hit);

    return Hit.GetActor();
}

FVector UIsometricInputComponent::GetLocationUnderCursor() const
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return FVector::ZeroVector;

    APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
    if (!PC) return FVector::ZeroVector;

    FHitResult Hit;
    PC->GetHitResultUnderCursor(ECC_Visibility, true, Hit);

    return Hit.ImpactPoint;
}


