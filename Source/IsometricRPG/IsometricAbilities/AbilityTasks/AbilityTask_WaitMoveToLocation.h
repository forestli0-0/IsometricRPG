// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "AbilityTask_WaitMoveToLocation.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWaitMoveToLocationDelegate);

UCLASS()
class ISOMETRICRPG_API UAbilityTask_WaitMoveToLocation : public UAbilityTask
{
	GENERATED_BODY()
	
	

public:
    UPROPERTY(BlueprintAssignable)
    FWaitMoveToLocationDelegate OnMoveFinished;

    UPROPERTY(BlueprintAssignable)
    FWaitMoveToLocationDelegate OnMoveFailed;

    ACharacter* SourceCharacter;
    /** 移动到固定位置 */
    UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (DisplayName = "WaitMoveToLocation", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility"))
    static UAbilityTask_WaitMoveToLocation* WaitMoveToLocation(
        UGameplayAbility* OwningAbility,
        FVector InTargetLocation,
        float InAcceptanceRadius = 38.0f);

    /** 移动到一个动态 Actor */
    UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (DisplayName = "WaitMoveToActor", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility"))
    static UAbilityTask_WaitMoveToLocation* WaitMoveToActor(
        UGameplayAbility* OwningAbility,
        AActor* InTargetActor,
        float InAcceptanceRadius = 70.0f);

protected:
    FVector TargetLocation;
    AActor* TargetActor;
    float AcceptanceRadius;
    class AAIController* AIController;

    FTimerHandle RepathTimerHandle;

    virtual void Activate() override;
    virtual void OnDestroy(bool bInOwnerFinished) override;


    /** 动态目标持续更新位置 */
    void UpdateMoveToActor();
    void UpdateMoveToLocation();
};
