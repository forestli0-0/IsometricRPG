#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h" // For FGameplayTag
#include "Engine/HitResult.h"    // For FHitResult
#include "IsometricRPG/IsometricAbilities/Types/HeroAbilityTypes.h"
// Forward declarations
class UAbilitySystemComponent;
class AIsometricRPGCharacter;
class APlayerController;
// struct FInputActionValue; // Removed as input actions are now handled by PlayerController

#include "IsometricInputComponent.generated.h" // THIS MUST BE THE LAST INCLUDE



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
	void HandleRightClick(const FHitResult& HitResult);
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

	/*
	 * 【旧的映射 - 已废弃】
	 * UPROPERTY(EditDefaultsOnly, Category = "Input|Skills")
	 * TMap<int32, FGameplayTag> SkillSlotToAbilityTagMap;
	 */

private:
	UPROPERTY()
	AIsometricRPGCharacter* OwnerCharacter;

	UPROPERTY()
	APlayerController* CachedPlayerController; // Retained for convenience, e.g. if component needs to get cursor info directly for some reason, though PC should pass it.

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
};
