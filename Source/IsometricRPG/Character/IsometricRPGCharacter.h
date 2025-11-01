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
	UFUNCTION(BlueprintCallable, Category = "Abilities")
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category="Attributes")
    virtual UIsometricRPGAttributeSetBase* GetAttributeSet() const;
	
    TArray<TWeakObjectPtr<AActor>> CurrentAbilityTargets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    FGenericTeamId TeamId;

    virtual FGenericTeamId GetGenericTeamId() const override;
    virtual void SetGenericTeamId(const FGenericTeamId& InTeamId);

    // 初始化技能系统
    virtual void PossessedBy(AController* NewController) override;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsDead, Category = "Character State")
    bool bIsDead = false;

    UFUNCTION(BlueprintCallable, Category = "Character State")
    void SetIsDead(bool NewValue);

    UFUNCTION()
    void OnRep_IsDead();

    UFUNCTION(BlueprintImplementableEvent, Category = "Character State")
    void OnDeathStateChanged(bool bNewValue);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
    // 当PlayerState在客户端上被复制时调用
    virtual void OnRep_PlayerState() override;
    // 初始化GAS组件的辅助函数
    void InitAbilityActorInfo();
    void HandleDeathStateChanged();
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

public:
    /** 由InputComponent在激活技能前调用，用于暂存目标数据 */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "SetAbilityTargetDataByHit"))
    void SetAbilityTargetDataByHit(const FHitResult& HitResult);
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "SetAbilityTargetDataByActor"))
    void SetAbilityTargetDataByActor(AActor* TargetActor);

    /** 同步到服务器：在客户端激活前调用，确保服务器也拿到相同的目标数据 */
    UFUNCTION(Server, Reliable)
    void Server_SetAbilityTargetDataByHit(const FHitResult& HitResult);
    UFUNCTION(Server, Reliable)
    void Server_SetAbilityTargetDataByActor(AActor* TargetActor);

    /** 供GameplayAbility在激活时调用，用于获取目标数据 */
    FGameplayAbilityTargetDataHandle GetAbilityTargetData() const;

    // 客户端本地表现：用于能力阶段时同步动画与停止行走
    UFUNCTION(Client, Reliable)
    void Client_PlayMontage(class UAnimMontage* Montage, float PlayRate = 1.f);
    UFUNCTION(Client, Reliable)
    void Client_SetMovementLocked(bool bLocked);

protected:
    /** 技能的目标数据暂存区 */
    UPROPERTY()
    FGameplayAbilityTargetDataHandle StoredTargetData;
};
