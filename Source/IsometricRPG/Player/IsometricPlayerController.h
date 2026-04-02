#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Interfaces/HeroAbilityNotificationReceiver.h"
#include "Input/IsometricInputTypes.h"
#include "IsometricRPG/IsometricAbilities/Types/HeroAbilityTypes.h"
#include "IsometricRPG/UI/HUD/HUDRootWidget.h"
#include "IsometricRPG/UI/HUD/HUDRefreshTypes.h"
#include "IsometricPlayerController.generated.h"


class UInputMappingContext;
class UInputAction;
class UIsometricInputComponent;
class AIsoPlayerState;
UCLASS()
class ISOMETRICRPG_API AIsometricPlayerController : public APlayerController, public IHeroAbilityNotificationReceiver
{
	GENERATED_BODY()
public:
	AIsometricPlayerController();
    virtual void NotifyAbilityCooldownTriggered_Implementation(const FGameplayAbilitySpecHandle& SpecHandle, float DurationSeconds) override;

protected:
	virtual void BeginPlay() override;
    virtual void OnRep_PlayerState() override;
	virtual void SetupInputComponent() override;
    // 添加Tick函数来处理按住右键的逻辑
	virtual void PlayerTick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* ClickMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_LeftClick;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_RightClick;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_A;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_Q;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_W;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_E;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_R;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_Summoner1;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_Summoner2;

	// 处理鼠标左键点击: 选择目标，可以是敌人也可以是友方
	UFUNCTION()
	void HandleLeftClickInput(const FInputActionValue& Value);
	// 处理鼠标右键点击: 移动或者攻击目标
	UFUNCTION()
	void HandleRightClickStarted(const FInputActionValue& Value);
    UFUNCTION()
    void HandleRightClickCompleted(const FInputActionValue& Value);

protected:
	// 处理技能输入
	void HandleSkillInput(EAbilityInputID InputID);

	// 处理技能输入（新：Pressed/Released 语义，用于蓄力/引导/持续施法）
	void HandleSkillPressed(EAbilityInputID InputID);
	void HandleSkillHeld(EAbilityInputID InputID);
	void HandleSkillReleased(EAbilityInputID InputID);
	void BuildCursorInputSnapshot(
		EPlayerInputSourceKind SourceKind,
		EInputEventPhase Phase,
		FCursorInputSnapshot& OutSnapshot,
		EAbilityInputID InputID = EAbilityInputID::None,
		float HeldDuration = 0.0f) const;
	void DispatchInputSnapshot(const FCursorInputSnapshot& Snapshot);
	float GetAbilityHeldDuration(EAbilityInputID InputID) const;
	UIsometricInputComponent* ResolveInputComponent() const;

	UPROPERTY(EditDefaultsOnly, Category = "Input|RightClick")
	float HeldMoveRepathInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Input|RightClick")
	float HeldMoveRepathDistance = 75.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Input|RightClick")
	float HeldAttackRetryInterval = 0.25f;

protected:
	// 添加一个属性来存储我们的主HUD类
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UHUDRootWidget> PlayerHUDClass;
public:
	// 添加一个变量来持有创建的HUD实例
	UPROPERTY()
	TObjectPtr<UHUDRootWidget> PlayerHUDInstance;
private:
    void EnsureHUDCreated();
    void BindHUDRefreshSource(AIsoPlayerState* InPlayerState);
    void UnbindHUDRefreshSource();
    void HandleHUDRefreshRequested(const FHeroHUDRefreshRequest& Request);
    void RefreshHUD(const FHeroHUDRefreshRequest& Request);

    // 【新增】用于追踪右键是否被按下的状态
    bool bIsRightMouseDown = false;
    // 【新增】用于缓存上一帧的目标Actor，以判断目标是否变化
    TWeakObjectPtr<AActor> LastHitActor;
    FVector LastHeldMoveLocation = FVector::ZeroVector;
    float NextHeldMoveCommandTime = 0.0f;
    float NextHeldAttackCommandTime = 0.0f;
    bool bHasLastHeldMoveLocation = false;
	TMap<EAbilityInputID, float> AbilityPressedAtTime;
    TWeakObjectPtr<AIsoPlayerState> BoundHUDRefreshSource;
    FDelegateHandle HUDRefreshRequestedHandle;

    bool GetHitResultUnderCursorSafe(ECollisionChannel TraceChannel, bool bTraceComplex, FHitResult& HitResult) const;
};
