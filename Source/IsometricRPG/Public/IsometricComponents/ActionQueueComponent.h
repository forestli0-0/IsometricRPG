// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ActionQueueComponent.generated.h"

UENUM(BlueprintType)
enum class EQueuedCommandType : uint8
{
	None,
	MoveToLocation,
	AttackTarget
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
	float AttackRange = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	FGameplayTag AttackEventTag;

	bool bIsExecuting = false;
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 命令
	void SetCommand_MoveTo(const FVector& Location);
	void SetCommand_AttackTarget(AActor* Target);
	void ClearCommand();
	// 攻击冷却
	float AttackInterval = 1.0f; // 每次攻击间隔（秒）

	double LastAttackTime = -100.0; // 上一次攻击的时间
		
};
