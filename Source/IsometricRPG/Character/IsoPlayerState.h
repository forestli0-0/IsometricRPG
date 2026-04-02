// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "GameplayTagContainer.h"
#include "UI/HUD/HUDPresentationBuilder.h"
#include "UI/HUD/HUDRefreshTypes.h"
#include "IsoPlayerState.generated.h"

class UAbilitySystemComponent;
class UIsometricRPGAttributeSetBase;
class UGameplayAbility;
struct FOnAttributeChangeData;
struct FGameplayEffectSpec;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHeroHUDRefreshRequested, const FHeroHUDRefreshRequest&);
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
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

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
    void TryActivatePassiveAbilities();
    bool bAbilityActorInfoInitialized = false;
    bool bPendingPassiveActivation = false;
    void LogActivatableAbilities() const;

public:
    // 属性初始化
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
    TSubclassOf<class UGameplayEffect> DefaultAttributesEffect;
    void InitializeAttributes();
    void NotifyAbilityActorInfoReady();

    // UI Debug: whether to show all owned gameplay tags as text badges on HUD
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Debug")
    bool bShowOwnedTagsOnHUD = false;

    // Whitelist of gameplay tags to surface on the HUD buff strip, mapping to icon assets
    UPROPERTY(EditDefaultsOnly, Category = "UI|Buffs")
    TMap<FGameplayTag, TSoftObjectPtr<class UTexture2D>> BuffIconMap;

    FHUDPresentationContext MakeHUDPresentationContext() const;
    bool QueryCooldownState(const UGameplayAbility* AbilityCDO, float& OutDuration, float& OutRemaining) const;
    bool TryGetEquippedAbilityInfoByHandle(const FGameplayAbilitySpecHandle& Handle, FEquippedAbilityInfo& OutInfo) const;
    FOnHeroHUDRefreshRequested& OnHUDRefreshRequested() { return HUDRefreshRequested; }
private:
	int SlotIndex = 0; // 用于跟踪当前技能槽的索引
    void OnAssetsLoadedForUI();
    void EnsureAttributeDelegatesBound();
    void EnsureGameplayTagDelegatesBound();
    void EnsureGameplayEffectDelegatesBound();
    void RequestHUDRefresh(EHeroHUDRefreshKind Kind, const FGameplayAbilitySpecHandle& SpecHandle = FGameplayAbilitySpecHandle(), float DurationSeconds = 0.f);
    void HandleObservedGameplayTagChanged(const FGameplayTag ChangedTag, int32 NewCount);
    void HandleVitalAttributeValueChanged(const FOnAttributeChangeData& ChangeData);
    void HandleChampionStatAttributeValueChanged(const FOnAttributeChangeData& ChangeData);
    void HandleSkillPointAttributeValueChanged(const FOnAttributeChangeData& ChangeData);
    void HandleActiveGameplayEffectAdded(UAbilitySystemComponent* TargetASC, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle);
    void HandleActiveGameplayEffectRemoved(const FActiveGameplayEffect& ActiveEffect);
    void HandleTrackedGameplayEffectStackChanged(FActiveGameplayEffectHandle ActiveHandle, int32 NewStackCount, int32 PreviousStackCount);
    void HandleTrackedGameplayEffectTimeChanged(FActiveGameplayEffectHandle ActiveHandle, float NewStartTime, float NewDuration);
    void RefreshBuffPresentationIfReady();
    bool ShouldTrackBuffEffect(const FGameplayEffectSpec& EffectSpec) const;
    bool ShouldTrackBuffEffect(const FActiveGameplayEffect& ActiveEffect) const;
    void BindBuffEffectDelegates(FActiveGameplayEffectHandle ActiveHandle);

    UFUNCTION()
    void HandleHealthChanged(UIsometricRPGAttributeSetBase* AttributeSetChanged, float NewHealth);

    UFUNCTION()
    void HandleManaChanged(UIsometricRPGAttributeSetBase* AttributeSetChanged, float NewMana);

    UFUNCTION()
    void HandleExperienceChanged(UIsometricRPGAttributeSetBase* AttributeSetChanged, float NewExperience, float NewMaxExperience);

    UFUNCTION()
    void HandleLevelChanged(UIsometricRPGAttributeSetBase* AttributeSetChanged, float NewLevel);

    bool bAttributeDelegatesBound = false;
    bool bGameplayTagDelegatesBound = false;
    bool bGameplayEffectDelegatesBound = false;
    TSet<FActiveGameplayEffectHandle> TrackedBuffEffectHandles;

    // --- Slot/Index 映射与工具 ---
    // 有些默认技能（如基础攻击/被动）可能不在技能栏显示，但仍需授予。
    // 约定：
    // - ESkillSlot::Invalid 表示“不在技能栏”，但允许授予（仅不占用栏位）。
    // - ESkillSlot::MAX 仅作枚举边界哨兵，永远不参与逻辑。
    int32 GetSkillBarSlotCount() const;                    // 技能栏槽位数量（不包含 Invalid / MAX）
    int32 IndexFromSlot(ESkillSlot Slot) const;            // 将枚举槽转换为0基索引；非栏位返回 INDEX_NONE

    const FEquippedAbilityInfo* FindEquippedInfoByHandle(const FGameplayAbilitySpecHandle& Handle) const;
    FOnHeroHUDRefreshRequested HUDRefreshRequested;
};
