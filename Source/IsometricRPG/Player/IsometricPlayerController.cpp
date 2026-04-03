// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricPlayerController.h"
#include "IsometricCameraManager.h"
#include "Character/IsometricRPGCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Character/IsoPlayerState.h"
#include "IsometricComponents/IsometricInputComponent.h"
#include "UI/HUD/HUDRootWidget.h"

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

    HUDCoordinator.Initialize(*this);
    HUDCoordinator.HandlePlayerStateChanged(GetPlayerState<AIsoPlayerState>());
}

void AIsometricPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    HUDCoordinator.Reset();
    Super::EndPlay(EndPlayReason);
}

void AIsometricPlayerController::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    HUDCoordinator.HandlePlayerStateChanged(GetPlayerState<AIsoPlayerState>());
}

void AIsometricPlayerController::NotifyAbilityCooldownTriggered_Implementation(
    const FGameplayAbilitySpecHandle& SpecHandle,
    float DurationSeconds)
{
    HUDCoordinator.NotifyCooldownTriggered(SpecHandle, DurationSeconds);
}

// 【新增】实现PlayerTick函数
void AIsometricPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    CursorInputRouter.Tick(*this, MakeCursorInputRouterConfig(), DeltaTime);
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

		EIC->BindAction(Action_A, ETriggerEvent::Ongoing, this, &AIsometricPlayerController::HandleSkillHeld, EAbilityInputID::Ability_A);
		EIC->BindAction(Action_Q, ETriggerEvent::Ongoing, this, &AIsometricPlayerController::HandleSkillHeld, EAbilityInputID::Ability_Q);
		EIC->BindAction(Action_W, ETriggerEvent::Ongoing, this, &AIsometricPlayerController::HandleSkillHeld, EAbilityInputID::Ability_W);
		EIC->BindAction(Action_E, ETriggerEvent::Ongoing, this, &AIsometricPlayerController::HandleSkillHeld, EAbilityInputID::Ability_E);
		EIC->BindAction(Action_R, ETriggerEvent::Ongoing, this, &AIsometricPlayerController::HandleSkillHeld, EAbilityInputID::Ability_R);
		EIC->BindAction(Action_Summoner1, ETriggerEvent::Ongoing, this, &AIsometricPlayerController::HandleSkillHeld, EAbilityInputID::Ability_Summoner1);
		EIC->BindAction(Action_Summoner2, ETriggerEvent::Ongoing, this, &AIsometricPlayerController::HandleSkillHeld, EAbilityInputID::Ability_Summoner2);

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
    CursorInputRouter.HandleRightClickStarted(*this, MakeCursorInputRouterConfig());
}

void AIsometricPlayerController::HandleRightClickCompleted(const FInputActionValue& Value)
{
    CursorInputRouter.HandleRightClickCompleted(*this);
}
void AIsometricPlayerController::HandleLeftClickInput(const FInputActionValue& Value)
{
    CursorInputRouter.HandleLeftClick(*this);
}


void AIsometricPlayerController::HandleSkillInput(EAbilityInputID InputID)
{
    CursorInputRouter.HandleSkillPressed(*this, InputID);
}

void AIsometricPlayerController::HandleSkillPressed(EAbilityInputID InputID)
{
    CursorInputRouter.HandleSkillPressed(*this, InputID);
}

void AIsometricPlayerController::HandleSkillHeld(EAbilityInputID InputID)
{
    CursorInputRouter.HandleSkillHeld(*this, InputID);
}

void AIsometricPlayerController::HandleSkillReleased(EAbilityInputID InputID)
{
    CursorInputRouter.HandleSkillReleased(*this, InputID);
}

void AIsometricPlayerController::BuildCursorInputSnapshot(
	EPlayerInputSourceKind SourceKind,
	EInputEventPhase Phase,
	FCursorInputSnapshot& OutSnapshot,
	EAbilityInputID InputID,
	float HeldDuration) const
{
	OutSnapshot = FCursorInputSnapshot();
	OutSnapshot.SourceKind = SourceKind;
	OutSnapshot.Phase = Phase;
	OutSnapshot.AbilityInputID = InputID;
	OutSnapshot.WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	OutSnapshot.HeldDuration = HeldDuration;

	FHitResult HitResult;
	if (GetHitResultUnderCursorSafe(ECC_Visibility, true, HitResult))
	{
		OutSnapshot.HitResult = HitResult;
		OutSnapshot.HitActor = HitResult.GetActor();
		OutSnapshot.bBlockingHit = HitResult.bBlockingHit;
		OutSnapshot.bHitEnemy = HitResult.GetActor()
            && HitResult.GetActor() != GetPawn()
            && HitResult.GetActor()->ActorHasTag(FName("Enemy"));
	}
}

void AIsometricPlayerController::DispatchInputSnapshot(const FCursorInputSnapshot& Snapshot)
{
	if (UIsometricInputComponent* InputComp = ResolveInputComponent())
	{
		InputComp->ProcessInputSnapshot(Snapshot);
	}
}

float AIsometricPlayerController::GetAbilityHeldDuration(const TMap<EAbilityInputID, float>& PressedTimes, EAbilityInputID InputID) const
{
	const float* PressedTime = PressedTimes.Find(InputID);
	if (!PressedTime || !GetWorld())
	{
		return 0.0f;
	}

	return FMath::Max(GetWorld()->GetTimeSeconds() - *PressedTime, 0.0f);
}

FHeroCursorInputRouterConfig AIsometricPlayerController::MakeCursorInputRouterConfig() const
{
    FHeroCursorInputRouterConfig Config;
    Config.HeldMoveRepathInterval = HeldMoveRepathInterval;
    Config.HeldMoveRepathDistance = HeldMoveRepathDistance;
    Config.HeldAttackRetryInterval = HeldAttackRetryInterval;
    return Config;
}

UIsometricInputComponent* AIsometricPlayerController::ResolveInputComponent() const
{
	if (const AIsometricRPGCharacter* MyChar = Cast<AIsometricRPGCharacter>(GetPawn()))
	{
		return MyChar->FindComponentByClass<UIsometricInputComponent>();
	}

	return nullptr;
}

bool AIsometricPlayerController::GetHitResultUnderCursorSafe(ECollisionChannel TraceChannel, bool bTraceComplex, FHitResult& HitResult) const
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
