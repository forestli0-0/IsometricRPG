// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricPlayerController.h"
#include "IsometricCameraManager.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Character/IsometricRPGCharacter.h"
#include "Character/IsoPlayerState.h"
#include "IsometricComponents/IsometricInputComponent.h"
#include "UI/HUD/HUDPresentationBuilder.h"
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

    EnsureHUDCreated();
    BindHUDRefreshSource(GetPlayerState<AIsoPlayerState>());
}

void AIsometricPlayerController::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    BindHUDRefreshSource(GetPlayerState<AIsoPlayerState>());
}

void AIsometricPlayerController::NotifyAbilityCooldownTriggered_Implementation(
    const FGameplayAbilitySpecHandle& SpecHandle,
    float DurationSeconds)
{
    if (!SpecHandle.IsValid() || DurationSeconds <= 0.f)
    {
        return;
    }

    FHeroHUDRefreshRequest Request;
    Request.Kind = EHeroHUDRefreshKind::Cooldown;
    Request.SpecHandle = SpecHandle;
    Request.DurationSeconds = DurationSeconds;
    RefreshHUD(Request);
}

void AIsometricPlayerController::EnsureHUDCreated()
{
    if (!IsLocalController() || PlayerHUDInstance || !PlayerHUDClass)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[PC] Creating PlayerHUD on %s"), *GetName());
    PlayerHUDInstance = CreateWidget<UHUDRootWidget>(this, PlayerHUDClass);
    if (PlayerHUDInstance)
    {
        PlayerHUDInstance->AddToViewport();
    }
}

void AIsometricPlayerController::BindHUDRefreshSource(AIsoPlayerState* InPlayerState)
{
    if (!IsLocalController())
    {
        return;
    }

    EnsureHUDCreated();

    if (BoundHUDRefreshSource.Get() == InPlayerState && HUDRefreshRequestedHandle.IsValid())
    {
        RefreshHUD(FHeroHUDRefreshRequest{ EHeroHUDRefreshKind::Full });
        return;
    }

    UnbindHUDRefreshSource();
    if (!InPlayerState)
    {
        return;
    }

    BoundHUDRefreshSource = InPlayerState;
    HUDRefreshRequestedHandle = InPlayerState->OnHUDRefreshRequested().AddUObject(this, &AIsometricPlayerController::HandleHUDRefreshRequested);
    RefreshHUD(FHeroHUDRefreshRequest{ EHeroHUDRefreshKind::Full });
}

void AIsometricPlayerController::UnbindHUDRefreshSource()
{
    if (AIsoPlayerState* CurrentSource = BoundHUDRefreshSource.Get())
    {
        if (HUDRefreshRequestedHandle.IsValid())
        {
            CurrentSource->OnHUDRefreshRequested().Remove(HUDRefreshRequestedHandle);
        }
    }

    HUDRefreshRequestedHandle.Reset();
    BoundHUDRefreshSource.Reset();
}

void AIsometricPlayerController::HandleHUDRefreshRequested(const FHeroHUDRefreshRequest& Request)
{
    RefreshHUD(Request);
}

void AIsometricPlayerController::RefreshHUD(const FHeroHUDRefreshRequest& Request)
{
    if (!IsLocalController())
    {
        return;
    }

    EnsureHUDCreated();
    if (!PlayerHUDInstance)
    {
        return;
    }

    AIsoPlayerState* IsoPlayerState = BoundHUDRefreshSource.Get();
    if (!IsoPlayerState)
    {
        IsoPlayerState = GetPlayerState<AIsoPlayerState>();
        if (!IsoPlayerState)
        {
            return;
        }
    }

    const FHUDPresentationContext Context = IsoPlayerState->MakeHUDPresentationContext();

    switch (Request.Kind)
    {
    case EHeroHUDRefreshKind::Full:
        FHUDPresentationBuilder::RefreshEntireHUD(
            *PlayerHUDInstance,
            Context,
            [IsoPlayerState](const UGameplayAbility* AbilityCDO, float& OutDuration, float& OutRemaining)
            {
                return IsoPlayerState->QueryCooldownState(AbilityCDO, OutDuration, OutRemaining);
            });
        return;

    case EHeroHUDRefreshKind::Vitals:
        FHUDPresentationBuilder::RefreshVitals(*PlayerHUDInstance, Context.AttributeSet);
        return;

    case EHeroHUDRefreshKind::Experience:
        FHUDPresentationBuilder::RefreshExperience(*PlayerHUDInstance, Context.AttributeSet);
        return;

    case EHeroHUDRefreshKind::ChampionStats:
        FHUDPresentationBuilder::RefreshChampionStats(*PlayerHUDInstance, Context.AttributeSet);
        return;

    case EHeroHUDRefreshKind::GameplayPresentation:
        FHUDPresentationBuilder::RefreshGameplayTagPresentation(*PlayerHUDInstance, Context);
        return;

    case EHeroHUDRefreshKind::ActionBar:
        if (Context.EquippedAbilities)
        {
            FHUDPresentationBuilder::RefreshActionBar(
                *PlayerHUDInstance,
                *Context.EquippedAbilities,
                [IsoPlayerState](const UGameplayAbility* AbilityCDO, float& OutDuration, float& OutRemaining)
                {
                    return IsoPlayerState->QueryCooldownState(AbilityCDO, OutDuration, OutRemaining);
                });
        }
        return;

    case EHeroHUDRefreshKind::Cooldown:
        {
            FEquippedAbilityInfo EquippedInfo;
            if (!Request.SpecHandle.IsValid() || !IsoPlayerState->TryGetEquippedAbilityInfoByHandle(Request.SpecHandle, EquippedInfo))
            {
                return;
            }

            if (EquippedInfo.Slot == ESkillSlot::Invalid || EquippedInfo.Slot == ESkillSlot::MAX)
            {
                return;
            }

            float CooldownDuration = Request.DurationSeconds;
            float CooldownRemaining = Request.DurationSeconds;

            UClass* AbilityClass = EquippedInfo.AbilityClass.Get();
            if (!AbilityClass && !EquippedInfo.AbilityClass.ToSoftObjectPath().IsNull())
            {
                AbilityClass = EquippedInfo.AbilityClass.LoadSynchronous();
            }

            if (AbilityClass)
            {
                if (const UGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<UGameplayAbility>())
                {
                    IsoPlayerState->QueryCooldownState(AbilityCDO, CooldownDuration, CooldownRemaining);
                }
            }

            PlayerHUDInstance->UpdateAbilityCooldown(EquippedInfo.Slot, CooldownDuration, CooldownRemaining);
        }
        return;
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

    AIsometricRPGCharacter* MyChar = Cast<AIsometricRPGCharacter>(GetPawn());
    if (!MyChar)
    {
        return;
    }

    UIsometricInputComponent* InputComp = ResolveInputComponent();
    if (!InputComp)
    {
        return;
    }

    FCursorInputSnapshot Snapshot;
    BuildCursorInputSnapshot(EPlayerInputSourceKind::RightMouse, EInputEventPhase::Held, Snapshot);

    const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    AActor* CurrentHitActor = Snapshot.HitActor;
    if (IsEnemyUnderCursor(CurrentHitActor, MyChar))
    {
        // 敌人目标：只在目标变化，或者到了重试时间时重新发一次攻击请求。
        const bool bTargetChanged = CurrentHitActor != LastHitActor.Get();
        const bool bRetryAttack = !bTargetChanged && WorldTime >= NextHeldAttackCommandTime;
        if (bTargetChanged || bRetryAttack)
        {
            DispatchInputSnapshot(Snapshot);
            LastHitActor = CurrentHitActor;
            NextHeldAttackCommandTime = WorldTime + HeldAttackRetryInterval;
            bHasLastHeldMoveLocation = false;
        }
        return;
    }

    if (!Snapshot.bBlockingHit)
    {
        return;
    }

    const bool bSwitchedFromActorTarget = LastHitActor.IsValid();
    const FVector2D CurrentMovePoint(Snapshot.HitResult.ImpactPoint.X, Snapshot.HitResult.ImpactPoint.Y);
    const FVector2D PreviousMovePoint(LastHeldMoveLocation.X, LastHeldMoveLocation.Y);
    const bool bMovePointChanged = !bHasLastHeldMoveLocation
        || (CurrentMovePoint - PreviousMovePoint).SizeSquared() >= FMath::Square(HeldMoveRepathDistance);
    const bool bRepathIntervalElapsed = WorldTime >= NextHeldMoveCommandTime;

    // 地面移动：只有目标点变化足够大并且节流时间到了，才重新发起一次 move intent。
    if (bSwitchedFromActorTarget || (bMovePointChanged && bRepathIntervalElapsed))
    {
        DispatchInputSnapshot(Snapshot);
        LastHitActor = nullptr;
        LastHeldMoveLocation = Snapshot.HitResult.ImpactPoint;
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
	bIsRightMouseDown = true; // 设置右键按下状态
    LastHitActor = nullptr; // 重置上一个目标
    bHasLastHeldMoveLocation = false;
    NextHeldMoveCommandTime = 0.0f;
    NextHeldAttackCommandTime = 0.0f;
	UE_LOG(LogTemp, Log, TEXT("[PC] RightClick Started on %s (Auth=%d)"), *GetName(), HasAuthority()?1:0);

	FCursorInputSnapshot Snapshot;
	BuildCursorInputSnapshot(EPlayerInputSourceKind::RightMouse, EInputEventPhase::Pressed, Snapshot);
	UE_LOG(LogTemp, Log, TEXT("[PC] RightClick Hit: Block=%d Loc=%s Actor=%s"), Snapshot.bBlockingHit?1:0, *Snapshot.HitResult.ImpactPoint.ToString(), Snapshot.HitActor?*Snapshot.HitActor->GetName():TEXT("None"));

	DispatchInputSnapshot(Snapshot);

    const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    if (AIsometricRPGCharacter* MyChar = Cast<AIsometricRPGCharacter>(GetPawn()))
    {
        if (IsEnemyUnderCursor(Snapshot.HitActor, MyChar))
        {
            LastHitActor = Snapshot.HitActor;
            NextHeldAttackCommandTime = WorldTime + HeldAttackRetryInterval;
        }
        else if (Snapshot.bBlockingHit)
        {
            LastHeldMoveLocation = Snapshot.HitResult.ImpactPoint;
            NextHeldMoveCommandTime = WorldTime + HeldMoveRepathInterval;
            bHasLastHeldMoveLocation = true;
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
	FCursorInputSnapshot Snapshot;
	BuildCursorInputSnapshot(EPlayerInputSourceKind::LeftMouse, EInputEventPhase::Pressed, Snapshot);
	UE_LOG(LogTemp, Log, TEXT("[PC] LeftClick Hit: Block=%d Loc=%s Actor=%s"), Snapshot.bBlockingHit?1:0, *Snapshot.HitResult.ImpactPoint.ToString(), Snapshot.HitActor?*Snapshot.HitActor->GetName():TEXT("None"));
	DispatchInputSnapshot(Snapshot);
}


void AIsometricPlayerController::HandleSkillInput(EAbilityInputID InputID)
{
	HandleSkillPressed(InputID);
}

void AIsometricPlayerController::HandleSkillPressed(EAbilityInputID InputID)
{
	const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	AbilityPressedAtTime.Add(InputID, WorldTime);

	FCursorInputSnapshot Snapshot;
	BuildCursorInputSnapshot(EPlayerInputSourceKind::AbilitySlot, EInputEventPhase::Pressed, Snapshot, InputID, 0.0f);
	DispatchInputSnapshot(Snapshot);
}

void AIsometricPlayerController::HandleSkillHeld(EAbilityInputID InputID)
{
	FCursorInputSnapshot Snapshot;
	BuildCursorInputSnapshot(
		EPlayerInputSourceKind::AbilitySlot,
		EInputEventPhase::Held,
		Snapshot,
		InputID,
		GetAbilityHeldDuration(InputID));
	DispatchInputSnapshot(Snapshot);
}

void AIsometricPlayerController::HandleSkillReleased(EAbilityInputID InputID)
{
	FCursorInputSnapshot Snapshot;
	BuildCursorInputSnapshot(
		EPlayerInputSourceKind::AbilitySlot,
		EInputEventPhase::Released,
		Snapshot,
		InputID,
		GetAbilityHeldDuration(InputID));
	AbilityPressedAtTime.Remove(InputID);
	DispatchInputSnapshot(Snapshot);
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
		OutSnapshot.bHitEnemy = IsEnemyUnderCursor(HitResult.GetActor(), GetPawn());
	}
}

void AIsometricPlayerController::DispatchInputSnapshot(const FCursorInputSnapshot& Snapshot)
{
	if (UIsometricInputComponent* InputComp = ResolveInputComponent())
	{
		InputComp->ProcessInputSnapshot(Snapshot);
	}
}

float AIsometricPlayerController::GetAbilityHeldDuration(EAbilityInputID InputID) const
{
	const float* PressedTime = AbilityPressedAtTime.Find(InputID);
	if (!PressedTime || !GetWorld())
	{
		return 0.0f;
	}

	return FMath::Max(GetWorld()->GetTimeSeconds() - *PressedTime, 0.0f);
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
