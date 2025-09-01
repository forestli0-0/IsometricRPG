#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Engine/HitResult.h"
#include "IsometricRPG/IsometricAbilities/Types/HeroAbilityTypes.h"
// Forward declarations
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
	// These methods are called by AIsometricPlayerController (or an AI Controller)
	void HandleLeftClick(const FHitResult& HitResult);

    void HandleRightClickTriggered(const FHitResult& HitResult, TWeakObjectPtr<AActor> LastHitActor);

	void HandleSkillInput(EAbilityInputID InputID, const FHitResult& TargetData);


	// Game action requests, can be called by this component internally or by AI
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
	// Helper methods to send GAS input confirmations/cancellations
	void SendConfirmTargetInput();
	void SendCancelTargetInput();

	// --- Server RPCs for networked actions ---
public:
	UFUNCTION(Server, Reliable)
	void Server_RequestMoveToLocation(const FVector& TargetLocation);

	UFUNCTION(Server, Reliable)
	void Server_RequestBasicAttack(AActor* TargetActor);


};
