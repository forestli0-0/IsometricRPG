// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "IsoPlayerState.generated.h"

class UAbilitySystemComponent;
class UIsometricRPGAttributeSetBase;
class UGameplayAbility;
/**
 * 
 */
UCLASS()
class ISOMETRICRPG_API AIsoPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
    AIsoPlayerState();

    // --- 实现 IAbilitySystemInterface ---
    // 这是让GAS系统能找到ASC的关键函数
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    // --- 公开你的AttributeSet ---
    UIsometricRPGAttributeSetBase* GetAttributeSet() const { return AttributeSet; }

    // 技能相关属性
    UPROPERTY(EditDefaultsOnly, Category = "Abilities")
    TArray<FEquippedAbilityInfo> DefaultAbilities;

    UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_EquippedAbilities, Category = "Abilities")
    TArray<FEquippedAbilityInfo> EquippedAbilities;

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    FEquippedAbilityInfo GetEquippedAbilityInfo(ESkillSlot Slot) const;

    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Abilities")
    void Server_EquipAbilityToSlot(TSubclassOf<UGameplayAbility> NewAbilityClass, ESkillSlot Slot);
    
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Abilities")
    void Server_UnequipAbilityFromSlot(ESkillSlot Slot);
protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;
protected:
    // --- 创建ASC和AS ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UIsometricRPGAttributeSetBase> AttributeSet;

    UFUNCTION()
    void OnRep_EquippedAbilities();

    void GrantAbilityInternal(FEquippedAbilityInfo& Info, bool bRemoveExistingFirst = false);
    void ClearAbilityInternal(FEquippedAbilityInfo& Info);

    bool bAbilitiesInitialized = false;
    void InitAbilities();

public:
    // 属性初始化
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
    TSubclassOf<class UGameplayEffect> DefaultAttributesEffect;
    void InitializeAttributes();
    
    // UI初始化相关
    bool bUIInitialized = false;
    bool bPendingUIUpdate = false;
    void OnUIInitialized();
    void UpdateUIWhenReady();
private:
	int SlotIndex = 0; // 用于跟踪当前技能槽的索引
    void OnAssetsLoadedForUI();

    // --- Slot/Index 映射与工具 ---
    // 有些默认技能（如基础攻击/被动）可能不在技能栏显示，但仍需授予。
    // 约定：
    // - ESkillSlot::Invalid 表示“不在技能栏”，但允许授予（仅不占用栏位）。
    // - ESkillSlot::MAX 仅作枚举边界哨兵，永远不参与逻辑。
    int32 GetSkillBarSlotCount() const;                    // 技能栏槽位数量（不包含 Invalid / MAX）
    int32 IndexFromSlot(ESkillSlot Slot) const;            // 将枚举槽转换为0基索引；非栏位返回 INDEX_NONE
};
