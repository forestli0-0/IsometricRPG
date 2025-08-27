// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UObject/ObjectMacros.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "IsometricComponents/IsometricInputComponent.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "IsoPlayerState.h"
#include "IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "IsometricRPGCharacter.generated.h"

class UIsometricRPGAttributeSetBase;
class UGameplayAbility;
class UGameplayEffect;

UCLASS()
class ISOMETRICRPG_API AIsometricRPGCharacter : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
    GENERATED_BODY()
public:
    // Sets default values for this character's properties
    AIsometricRPGCharacter();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:    
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // IAbilitySystemInterface implementation
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category="Attributes")
    virtual UIsometricRPGAttributeSetBase* GetAttributeSet() const;
	
    TArray<TWeakObjectPtr<AActor>> CurrentAbilityTargets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    FGenericTeamId TeamId;

    // 4. 声明接口需要我们实现的函数
    virtual FGenericTeamId GetGenericTeamId() const override;
    virtual void SetGenericTeamId(const FGenericTeamId& InTeamId);

    // 初始化技能系统
    virtual void PossessedBy(AController* NewController) override;


protected:
    // 当PlayerState在客户端上被复制时调用
    virtual void OnRep_PlayerState() override;
    // 初始化GAS组件的辅助函数
    void InitAbilityActorInfo();
public:
    // 输入组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
    UIsometricInputComponent* IRPGInputComponent;


    UFUNCTION(Server, Reliable)
    void Server_EquipAbilityToSlot(TSubclassOf<UGameplayAbility> NewAbilityClass, ESkillSlot Slot);
    UFUNCTION(Server, Reliable)
    void Server_UnequipAbilityFromSlot(ESkillSlot Slot);
    FEquippedAbilityInfo GetEquippedAbilityInfo(ESkillSlot Slot) const;
    // Current Health (方便蓝图访问)
    UFUNCTION(BlueprintPure, Category="Attributes")
    float GetCurrentHealth() const;
    UFUNCTION(BlueprintPure, Category="Attributes")
    float GetMaxHealth() const;

};
