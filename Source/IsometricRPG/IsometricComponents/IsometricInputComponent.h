#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Engine/HitResult.h"
#include "IsometricRPG/IsometricAbilities/Types/HeroAbilityTypes.h"
// 前向声明
class UAbilitySystemComponent;
class AIsometricRPGCharacter;
class APlayerController;

#include "IsometricInputComponent.generated.h"



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ISOMETRICRPG_API UIsometricInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIsometricInputComponent();

protected:
	virtual void BeginPlay() override;

public:
	// 这些方法由 AIsometricPlayerController（或 AI 控制器）调用
	void HandleLeftClick(const FHitResult& HitResult);

    void HandleRightClickTriggered(const FHitResult& HitResult, bool bIsHeldInput = false);

	void HandleSkillPressed(EAbilityInputID InputID, const FHitResult& TargetData);
	void HandleSkillReleased(EAbilityInputID InputID);


	// 游戏行为请求，可由该组件内部或 AI 调用
	void RequestMoveToLocation(const FVector& TargetLocation, bool bIsHeldInput = false);
	void RequestBasicAttack(AActor* TargetActor, bool bUseUnreliableRemoteUpdate = false);

private:
	UPROPERTY()
	AIsometricRPGCharacter* OwnerCharacter;

	UPROPERTY()
	APlayerController* CachedPlayerController;

	UPROPERTY()
	UAbilitySystemComponent* OwnerASC;

	TWeakObjectPtr<AActor> CurrentSelectedTargetForUI;

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnTargetSelectedForUI(AActor* Target);
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnTargetClearedForUI();

private:
	// 辅助方法：发送 GAS 的确认/取消 输入
	void SendConfirmTargetInput();
	void SendCancelTargetInput();

	// --- 服务端 RPC（用于网络动作） ---
public:
	UFUNCTION(Server, Reliable)
	void Server_RequestMoveToLocation(const FVector& TargetLocation);

	UFUNCTION(Server, Reliable)
	void Server_RequestBasicAttack(AActor* TargetActor);

	UFUNCTION(Server, Unreliable)
	void Server_UpdateBasicAttack(AActor* TargetActor);

private:
	void HandleMoveIntentOnAuthority();
	void StopPredictedClickMove();
};
