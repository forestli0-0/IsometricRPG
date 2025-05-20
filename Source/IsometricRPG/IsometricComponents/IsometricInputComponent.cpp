// Fill out your copyright notice in the Description page of Project Settings.

#include "IsometricInputComponent.h"
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
	if (!MappingContext) return;

	// 添加映射上下文
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(MappingContext, 0);
	}
}

void UIsometricInputComponent::HandleLeftClick()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;
    APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
    if (!PC) return;

    FHitResult Hit;
    PC->GetHitResultUnderCursor(ECC_Visibility, true, Hit);
    AActor* HitActor = Hit.GetActor();

    if (CurrentSelectedSkillIndex >= 0 && SkillMappings.Contains(CurrentSelectedSkillIndex))
    {
        // 已选择技能，左键释放技能
        FInputCommand Command;
        Command.CommandType = EInputCommandType::UseSkill;
        Command.SkillIndex = CurrentSelectedSkillIndex;
        Command.TargetLocation = Hit.ImpactPoint;
        Command.TargetActor = HitActor;
        if (const FGameplayTag* FoundTag = SkillMappings.Find(Command.SkillIndex))
        {
            Command.AbilityTag = *FoundTag;
        }
        CurrentSelectedSkillIndex = -1;
        // 技能释放后清除目标选中
        CurrentSelectedTarget = nullptr;
        OnTargetCleared();
        ProcessInputCommand(Command);
        return;
    }

    // 未选择技能，左键只做目标选中
    if (HitActor && HitActor != OwnerCharacter)
    {
        CurrentSelectedTarget = HitActor;
        OnTargetSelected(HitActor); // 通知UI
    }
    else
    {
        // 没有选中目标，清除
        CurrentSelectedTarget = nullptr;
        OnTargetCleared();
    }
}

void UIsometricInputComponent::HandleRightClick()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;
    APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
    if (!PC) return;

    FHitResult Hit;
    PC->GetHitResultUnderCursor(ECC_Visibility, true, Hit);
    AActor* HitActor = Hit.GetActor();

    // 右键点敌人：普通攻击
    if (HitActor && HitActor->ActorHasTag("Enemy"))
    {
        FInputCommand Command;
        Command.CommandType = EInputCommandType::BasicAttack;
        Command.TargetActor = HitActor;
        Command.TargetLocation = Hit.ImpactPoint;
        Command.SkillIndex = 0;
        if (const FGameplayTag* FoundTag = SkillMappings.Find(Command.SkillIndex))
        {
            Command.AbilityTag = *FoundTag;
        }
        // 清除目标和技能选择
        CurrentSelectedTarget = nullptr;
        CurrentSelectedSkillIndex = -1;
        OnTargetCleared();
        ProcessInputCommand(Command);
        return;
    }
    // 右键点地面：移动
    if (Hit.bBlockingHit)
    {
        FInputCommand Command;
        Command.CommandType = EInputCommandType::Movement;
        Command.TargetLocation = Hit.ImpactPoint;
        // 清除目标和技能选择
        CurrentSelectedTarget = nullptr;
        CurrentSelectedSkillIndex = -1;
        OnTargetCleared();
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



