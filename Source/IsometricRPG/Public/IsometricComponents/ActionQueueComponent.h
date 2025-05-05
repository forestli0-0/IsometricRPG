// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "IsometricAbilities/HeroAbilityTypes.h"
#include "ActionQueueComponent.generated.h"

UENUM(BlueprintType)
enum class EQueuedCommandType : uint8
{
	None,
	MoveToLocation,
	AttackTarget,
	UseSkill
};


USTRUCT()
struct FQueuedCommand
{
	GENERATED_BODY()

	EQueuedCommandType Type = EQueuedCommandType::None;
	FVector TargetLocation;
	TWeakObjectPtr<AActor> TargetActor;
	FGameplayTag AbilityEventTag;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ISOMETRICRPG_API UActionQueueComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UActionQueueComponent();


protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	FQueuedCommand CurrentCommand;

	UPROPERTY()
	class ACharacter* OwnerCharacter;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	FGameplayTag AttackEventTag;


public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	FQueuedCommand GetCommand();

	// 命令
	void SetCommand_MoveTo(const FVector& Location);

	// 设置普通攻击命令
	void SetCommand_AttackTarget(const FGameplayTag& AbilityTag, const FVector& TargetLocation, AActor* TargetActor);

	// 设置使用技能命令
	void SetCommand_UseSkill(const FGameplayTag& AbilityTag, const FVector& TargetLocation, AActor* TargetActor = nullptr);

	// 执行技能
	void ExecuteSkill(const FGameplayTag& AbilityTag, const FVector& TargetLocation, AActor* TargetActor = nullptr);

	// 清除命令
	void ClearCommand();

	void OnSkillOutOfRange(const FGameplayEventData* EventData);
public:
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	bool bAttackInProgress = false;
	UPROPERTY(EditDefaultsOnly, Category = "State")
	bool bIsExecuting = false;

public:
	// 技能槽
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitiesSlot")
	TArray<FHeroAbilitySlotData> AbilitySlots;
	// 初始化技能槽
	void InitializeAbilitySlots();

	UPROPERTY()
	UAbilitySystemComponent* ASC;
};

