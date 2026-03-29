#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Input/IsometricInputRouter.h"
#include "Input/IsometricInputSessionManager.h"

class UAbilitySystemComponent;
class AIsometricRPGCharacter;
class APlayerController;
class UGA_HeroBaseAbility;
enum class ESkillSlot : uint8;

#include "IsometricInputComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ISOMETRICRPG_API UIsometricInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIsometricInputComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	void ProcessInputSnapshot(const FCursorInputSnapshot& Snapshot);


	// 游戏行为请求，可由该组件内部或 AI 调用
	void RequestMoveToLocation(const FVector& TargetLocation, bool bIsHeldInput = false);
	void RequestBasicAttack(const FHitResult& HitResult, AActor* TargetActor, bool bUseUnreliableRemoteUpdate = false);

private:
	UPROPERTY()
	AIsometricRPGCharacter* OwnerCharacter;

	UPROPERTY()
	UAbilitySystemComponent* OwnerASC;

	TWeakObjectPtr<AActor> CurrentSelectedTargetForUI;
	FIsometricInputRouter InputRouter;
	FIsometricInputSessionManager InputSessionManager;

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
	void Server_RequestBasicAttack(const FHitResult& HitResult, AActor* TargetActor);

	UFUNCTION(Server, Unreliable)
	void Server_UpdateBasicAttack(const FHitResult& HitResult, AActor* TargetActor);

private:
	void ExecuteCommand(const FPlayerInputCommand& Command);
	void ApplySelectionCommand(const FPlayerInputCommand& Command);
	void ExecuteAbilityInputCommand(const FPlayerInputCommand& Command);
	bool ExecuteAbilityCommitCommand(const FPlayerInputCommand& Command, const FAbilityInputPolicy& Policy);
	const UGA_HeroBaseAbility* ResolveHeroAbility(EAbilityInputID InputID) const;
	bool ResolveAbilityInputPolicy(EAbilityInputID InputID, FAbilityInputPolicy& OutPolicy) const;
	void EnrichAbilityCommandTargetData(FPlayerInputCommand& Command) const;
	ESkillSlot ResolveSkillSlotFromInput(EAbilityInputID InputID) const;
	FPendingAbilityActivationContext BuildActivationContext(const FPlayerInputCommand& Command) const;
	void UpdatePendingAbilityContext(const FPlayerInputCommand& Command);
	void HandleMoveIntentOnAuthority();
	void StopPredictedClickMove();
	void SyncActivationContextToServer(const FPendingAbilityActivationContext& Context) const;
	void PumpBufferedAbilityCommand();
};
