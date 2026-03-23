// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricPlayerController.h"
#include "IsometricCameraManager.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Character/IsometricRPGCharacter.h"
#include "Character/IsoPlayerState.h"
#include "IsometricComponents/IsometricInputComponent.h"
#include "UI/HUD/HUDRootWidget.h"

namespace
{
	bool IsEnemyUnderCursor(const AActor* HitActor, const AActor* OwnerActor)
	{
		return HitActor && HitActor != OwnerActor && HitActor->ActorHasTag(FName("Enemy"));
	}
}

AIsometricPlayerController::AIsometricPlayerController()
{
    // 设置使用自定义的PlayerCameraManager
    PlayerCameraManagerClass = AIsometricCameraManager::StaticClass();
}

void AIsometricPlayerController::BeginPlay()
{
	Super::BeginPlay();
	// 统一使用 Game + UI，确保鼠标位置与命中测试一致
	FInputModeGameAndUI InputModeData;
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputModeData.SetHideCursorDuringCapture(false);
	SetInputMode(InputModeData);
	UE_LOG(LogTemp, Log, TEXT("[PC] InputMode: GameAndUI on %s (Auth=%d)"), *GetName(), HasAuthority()?1:0);
	if (IsLocalController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (!ClickMappingContext)
			{
				UE_LOG(LogTemp, Warning, TEXT("[PC] ClickMappingContext is NOT set on %s"), *GetName());
			}
			else
			{
				Subsystem->AddMappingContext(ClickMappingContext, 0);
				UE_LOG(LogTemp, Log, TEXT("[PC] MappingContext added on %s"), *GetName());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[PC] EnhancedInputLocalPlayerSubsystem not found on %s"), *GetName());
		}
	}

	// 显示鼠标
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	if (IsLocalController() && PlayerHUDClass)
	{
		UE_LOG(LogTemp, Log, TEXT("[PC] Creating PlayerHUD on %s"), *GetName());
		PlayerHUDInstance = CreateWidget<UHUDRootWidget>(this, PlayerHUDClass);
		if (PlayerHUDInstance)
		{
			PlayerHUDInstance->AddToViewport();
			
			// 通知PlayerState UI已经初始化完成
			if (AIsoPlayerState* IsoPlayerState = GetPlayerState<AIsoPlayerState>())
			{
				IsoPlayerState->OnUIInitialized();
			}
		}
	}
}

// 【新增】实现PlayerTick函数
void AIsometricPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    // “按住右键”不等于“每帧都发命令”。
    // 这里只负责把输入节流成少量语义化事件：换目标、重试攻击、重定路径。
    if (!bIsRightMouseDown)
    {
        return;
    }

    FHitResult HitResult;
    if (!GetHitResultUnderCursorSafe(ECC_Visibility, true, HitResult))
    {
        return;
    }

    AIsometricRPGCharacter* MyChar = Cast<AIsometricRPGCharacter>(GetPawn());
    if (!MyChar)
    {
        return;
    }

    UIsometricInputComponent* InputComp = MyChar->FindComponentByClass<UIsometricInputComponent>();
    if (!InputComp)
    {
        return;
    }

    const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    AActor* CurrentHitActor = HitResult.GetActor();
    if (IsEnemyUnderCursor(CurrentHitActor, MyChar))
    {
        // 敌人目标：只在目标变化，或者到了重试时间时重新发一次攻击请求。
        const bool bTargetChanged = CurrentHitActor != LastHitActor.Get();
        const bool bRetryAttack = !bTargetChanged && WorldTime >= NextHeldAttackCommandTime;
        if (bTargetChanged || bRetryAttack)
        {
            InputComp->HandleRightClickTriggered(HitResult, true);
            LastHitActor = CurrentHitActor;
            NextHeldAttackCommandTime = WorldTime + HeldAttackRetryInterval;
            bHasLastHeldMoveLocation = false;
        }
        return;
    }

    if (!HitResult.bBlockingHit)
    {
        return;
    }

    const bool bSwitchedFromActorTarget = LastHitActor.IsValid();
    const FVector2D CurrentMovePoint(HitResult.ImpactPoint.X, HitResult.ImpactPoint.Y);
    const FVector2D PreviousMovePoint(LastHeldMoveLocation.X, LastHeldMoveLocation.Y);
    const bool bMovePointChanged = !bHasLastHeldMoveLocation
        || (CurrentMovePoint - PreviousMovePoint).SizeSquared() >= FMath::Square(HeldMoveRepathDistance);
    const bool bRepathIntervalElapsed = WorldTime >= NextHeldMoveCommandTime;

    // 地面移动：只有目标点变化足够大并且节流时间到了，才重新发起一次 move intent。
    if (bSwitchedFromActorTarget || (bMovePointChanged && bRepathIntervalElapsed))
    {
        InputComp->HandleRightClickTriggered(HitResult, true);
        LastHitActor = nullptr;
        LastHeldMoveLocation = HitResult.ImpactPoint;
        NextHeldMoveCommandTime = WorldTime + HeldMoveRepathInterval;
        bHasLastHeldMoveLocation = true;
    }
}
void AIsometricPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EIC->BindAction(Action_LeftClick, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleLeftClickInput);
		EIC->BindAction(Action_RightClick, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleRightClickStarted);
		EIC->BindAction(Action_RightClick, ETriggerEvent::Completed, this, &AIsometricPlayerController::HandleRightClickCompleted);
		
		// 技能按键：绑定 Press/Release 语义
		EIC->BindAction(Action_A, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillPressed, EAbilityInputID::Ability_A);
		EIC->BindAction(Action_Q, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillPressed, EAbilityInputID::Ability_Q);
		EIC->BindAction(Action_W, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillPressed, EAbilityInputID::Ability_W);
		EIC->BindAction(Action_E, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillPressed, EAbilityInputID::Ability_E);
		EIC->BindAction(Action_R, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillPressed, EAbilityInputID::Ability_R);
		EIC->BindAction(Action_Summoner1, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillPressed, EAbilityInputID::Ability_Summoner1);
		EIC->BindAction(Action_Summoner2, ETriggerEvent::Started, this, &AIsometricPlayerController::HandleSkillPressed, EAbilityInputID::Ability_Summoner2);

		EIC->BindAction(Action_A, ETriggerEvent::Completed, this, &AIsometricPlayerController::HandleSkillReleased, EAbilityInputID::Ability_A);
		EIC->BindAction(Action_Q, ETriggerEvent::Completed, this, &AIsometricPlayerController::HandleSkillReleased, EAbilityInputID::Ability_Q);
		EIC->BindAction(Action_W, ETriggerEvent::Completed, this, &AIsometricPlayerController::HandleSkillReleased, EAbilityInputID::Ability_W);
		EIC->BindAction(Action_E, ETriggerEvent::Completed, this, &AIsometricPlayerController::HandleSkillReleased, EAbilityInputID::Ability_E);
		EIC->BindAction(Action_R, ETriggerEvent::Completed, this, &AIsometricPlayerController::HandleSkillReleased, EAbilityInputID::Ability_R);
		EIC->BindAction(Action_Summoner1, ETriggerEvent::Completed, this, &AIsometricPlayerController::HandleSkillReleased, EAbilityInputID::Ability_Summoner1);
		EIC->BindAction(Action_Summoner2, ETriggerEvent::Completed, this, &AIsometricPlayerController::HandleSkillReleased, EAbilityInputID::Ability_Summoner2);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[PC] SetupInputComponent: EnhancedInputComponent cast FAILED on %s"), *GetName());
	}
}

void AIsometricPlayerController::HandleRightClickStarted(const FInputActionValue& Value)
{
	bIsRightMouseDown = true; // 设置右键按下状态
    LastHitActor = nullptr; // 重置上一个目标
    bHasLastHeldMoveLocation = false;
    NextHeldMoveCommandTime = 0.0f;
    NextHeldAttackCommandTime = 0.0f;
	UE_LOG(LogTemp, Log, TEXT("[PC] RightClick Started on %s (Auth=%d)"), *GetName(), HasAuthority()?1:0);

	FHitResult HitResult;
	GetHitResultUnderCursorSafe(ECC_Visibility, true, HitResult);
	UE_LOG(LogTemp, Log, TEXT("[PC] RightClick Hit: Block=%d Loc=%s Actor=%s"), HitResult.bBlockingHit?1:0, *HitResult.ImpactPoint.ToString(), HitResult.GetActor()?*HitResult.GetActor()->GetName():TEXT("None"));
	
	if (AIsometricRPGCharacter* MyChar = Cast<AIsometricRPGCharacter>(GetPawn()))
	{
		if (UIsometricInputComponent* InputComp = MyChar->FindComponentByClass<UIsometricInputComponent>())
		{
            // 第一次点击时，就直接触发一次逻辑
			InputComp->HandleRightClickTriggered(HitResult);

            const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
            if (IsEnemyUnderCursor(HitResult.GetActor(), MyChar))
            {
                LastHitActor = HitResult.GetActor();
                NextHeldAttackCommandTime = WorldTime + HeldAttackRetryInterval;
            }
            else if (HitResult.bBlockingHit)
            {
                LastHeldMoveLocation = HitResult.ImpactPoint;
                NextHeldMoveCommandTime = WorldTime + HeldMoveRepathInterval;
                bHasLastHeldMoveLocation = true;
            }
		}

	}
}

void AIsometricPlayerController::HandleRightClickCompleted(const FInputActionValue& Value)
{
    bIsRightMouseDown = false; // 重置右键按下状态
    LastHitActor = nullptr; // 清空缓存
    bHasLastHeldMoveLocation = false;
    NextHeldMoveCommandTime = 0.0f;
    NextHeldAttackCommandTime = 0.0f;
	UE_LOG(LogTemp, Log, TEXT("[PC] RightClick Completed on %s (Auth=%d)"), *GetName(), HasAuthority()?1:0);
}
void AIsometricPlayerController::HandleLeftClickInput(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Log, TEXT("[PC] LeftClick Started on %s (Auth=%d)"), *GetName(), HasAuthority()?1:0);
	FHitResult HitResult;
	GetHitResultUnderCursorSafe(ECC_Visibility, true, HitResult);
	UE_LOG(LogTemp, Log, TEXT("[PC] LeftClick Hit: Block=%d Loc=%s Actor=%s"), HitResult.bBlockingHit?1:0, *HitResult.ImpactPoint.ToString(), HitResult.GetActor()?*HitResult.GetActor()->GetName():TEXT("None"));

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
	HandleSkillPressed(InputID);
}

void AIsometricPlayerController::HandleSkillPressed(EAbilityInputID InputID)
{
	FHitResult HitResult;
	GetHitResultUnderCursorSafe(ECC_Visibility, true, HitResult);

	if (AIsometricRPGCharacter* MyChar = Cast<AIsometricRPGCharacter>(GetPawn()))
	{
		if (UIsometricInputComponent* InputComp = MyChar->FindComponentByClass<UIsometricInputComponent>())
		{
			InputComp->HandleSkillPressed(InputID, HitResult);
		}
	}
}

void AIsometricPlayerController::HandleSkillReleased(EAbilityInputID InputID)
{
	if (AIsometricRPGCharacter* MyChar = Cast<AIsometricRPGCharacter>(GetPawn()))
	{
		if (UIsometricInputComponent* InputComp = MyChar->FindComponentByClass<UIsometricInputComponent>())
		{
			InputComp->HandleSkillReleased(InputID);
		}
	}
}

bool AIsometricPlayerController::GetHitResultUnderCursorSafe(ECollisionChannel TraceChannel, bool bTraceComplex, FHitResult& HitResult)
{
    ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
    if (LocalPlayer && LocalPlayer->ViewportClient)
    {
        FVector WorldOrigin;
        FVector WorldDirection;
        if (DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
        {
            FCollisionQueryParams Params(NAME_None, bTraceComplex, this);
            if (GetPawn())
            {
                Params.AddIgnoredActor(GetPawn());
            }
            return GetWorld()->LineTraceSingleByChannel(HitResult, WorldOrigin, WorldOrigin + WorldDirection * HitResultTraceDistance, TraceChannel, Params);
        }
    }
    return false;
}
