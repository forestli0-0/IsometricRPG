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

    void HandleRightClickTriggered(const FHitResult& HitResult, TWeakObjectPtr<AActor> LastHitActor);

	void HandleSkillInput(EAbilityInputID InputID, const FHitResult& TargetData);


	// 游戏行为请求，可由该组件内部或 AI 调用
	void RequestMoveToLocation(const FVector& TargetLocation);
	void RequestBasicAttack(AActor* TargetActor);

public:
	/**
	 * 【新的技能映射】
	 * 在蓝图中直接为每个输入动作（键）分配一个技能类。
	 * 这比使用 GameplayTag 更加直观和类型安全。
	 * 例如：Key = EAbilityInputID::Skill1, Value = GA_Fireball::StaticClass()
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Input|Skills", meta = (DisplayName = "Skill Input Mappings"))
	TMap<EAbilityInputID, TSubclassOf<class UGameplayAbility>> SkillInputMappings;

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


};
