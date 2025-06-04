#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h" // For FGameplayTag
#include "Engine/HitResult.h"    // For FHitResult

// Forward declarations
class UAbilitySystemComponent;
class AIsometricRPGCharacter;
class APlayerController;
// struct FInputActionValue; // Removed as input actions are now handled by PlayerController

#include "IsometricInputComponent.generated.h" // THIS MUST BE THE LAST INCLUDE

UENUM(BlueprintType)
enum class EAbilityInputID : uint8
{
	None            UMETA(DisplayName = "None"),
	Confirm         UMETA(DisplayName = "Confirm"),
	Cancel          UMETA(DisplayName = "Cancel"),
	Ability1        UMETA(DisplayName = "Ability 1 (Primary)"),
	Ability2        UMETA(DisplayName = "Ability 2 (Secondary)"),
	Jump            UMETA(DisplayName = "Jump"),
	Sprint          UMETA(DisplayName = "Sprint")
	// ... 其他你需要的输入
};

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
	void HandleSkillInput(int32 SkillSlotID, const FHitResult& TargetData);


	// Game action requests, can be called by this component internally or by AI
	void RequestMoveToLocation(const FVector& TargetLocation);
	void RequestBasicAttack(AActor* TargetActor);

public:
	UPROPERTY(EditDefaultsOnly, Category = "Input|Skills")
	TMap<int32, FGameplayTag> SkillSlotToAbilityTagMap;

	UPROPERTY(EditDefaultsOnly, Category = "Input|GAS", meta = (ToolTip = "Input ID for confirming target in GAS. Typically 0."))
	int32 ConfirmInputID = 1; 
	UPROPERTY(EditDefaultsOnly, Category = "Input|GAS", meta = (ToolTip = "Input ID for canceling target in GAS. Typically 1."))
	int32 CancelInputID = 2;


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
