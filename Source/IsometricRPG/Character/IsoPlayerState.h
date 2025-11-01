// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "IsometricAbilities/Types/HeroAbilityTypes.h"
#include "Inventory/InventoryTypes.h"
#include "Equipment/EquipmentTypes.h"
#include "IsoPlayerState.generated.h"

class UAbilitySystemComponent;
class UIsometricRPGAttributeSetBase;
class UGameplayAbility;
class UInventoryComponent;
class UEquipmentComponent;
class USkillTreeComponent;
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

    UFUNCTION(BlueprintImplementableEvent, Category = "Abilities")
    void HandleAbilitySlotEquipped(ESkillSlot Slot, const FEquippedAbilityInfo& Info);

    UFUNCTION(BlueprintImplementableEvent, Category = "Abilities")
    void HandleAbilitySlotUnequipped(ESkillSlot Slot, const FEquippedAbilityInfo& Info);

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

    UFUNCTION(BlueprintPure, Category = "Inventory")
    UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

    UFUNCTION(BlueprintPure, Category = "Equipment")
    UEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

    UFUNCTION(BlueprintPure, Category = "Skill Tree")
    USkillTreeComponent* GetSkillTreeComponent() const { return SkillTreeComponent; }

    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Inventory")
    void Server_MoveInventoryItem(int32 FromIndex, int32 ToIndex);

    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Inventory")
    void Server_RemoveInventoryItem(int32 InSlotIndex, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Inventory")
    void Server_UseInventoryItem(int32 InSlotIndex);

    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Equipment")
    void Server_EquipInventoryItem(int32 InventorySlotIndex, EEquipmentSlot EquipmentSlot);

    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Equipment")
    void Server_UnequipItem(EEquipmentSlot EquipmentSlot, bool bReturnToInventory = true);
    
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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UInventoryComponent> InventoryComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UEquipmentComponent> EquipmentComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill Tree", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USkillTreeComponent> SkillTreeComponent;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
    TArray<FInventoryItemStack> DefaultInventoryItems;

    void InitializeInventory();

    void RefreshInputMappings();
    static EAbilityInputID TryMapSlotToInputID(ESkillSlot Slot);
    static FText GetInputHintForSlot(ESkillSlot Slot);
};
