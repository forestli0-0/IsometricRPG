#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IsometricRPG/IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "UI/HUD/HUDViewModelTypes.h"
#include "HUDRootWidget.generated.h"

class UHUDActionBarWidget;
class UHUDStatusPanelWidget;
class UHUDResourcePanelWidget;
class UTexture2D;
struct FHUDSkillSlotViewModel;

/**
 * 根 HUD 界面控件，负责组织底部战斗相关的 HUD（技能栏、状态栏、资源）及辅助覆盖层。
 */
UCLASS()
class ISOMETRICRPG_API UHUDRootWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 返回显示在底部中央的动作条控件（用于呈现技能、物品与召唤槽）。 */
    UFUNCTION(BlueprintPure, Category = "HUD")
    UHUDActionBarWidget* GetActionBar() const { return ActionBar; }

    /** 返回显示在左下角的状态面板（头像、生命、增益/减益等）。 */
    UFUNCTION(BlueprintPure, Category = "HUD")
    UHUDStatusPanelWidget* GetStatusPanel() const { return StatusPanel; }

    /** 返回显示在右下角的资源面板（物品栏与快捷按钮）。 */
    UFUNCTION(BlueprintPure, Category = "HUD")
    UHUDResourcePanelWidget* GetResourcePanel() const { return ResourcePanel; }

    /** 将技能槽的视图模型数据路由到动作条，用于更新图标、名称与冷却显示。 */
    void SetAbilitySlot(const FHUDSkillSlotViewModel& ViewModel);

    /** 清空指定的技能槽（将槽位恢复为未装备状态）。 */
    void ClearAbilitySlot(ESkillSlot Slot);

    /** 清空所有技能槽（用于重置或初始化 UI）。 */
    void ClearAllAbilitySlots();

    /** 更新指定技能槽的冷却时长与剩余时间，用于进度显示。 */
    void UpdateAbilityCooldown(ESkillSlot Slot, float Duration, float Remaining);

    /** 更新缓存的生命值并将其推送到血量/资源显示条。 */
    void UpdateHealth(float CurrentHealth, float MaxHealth, float ShieldValue);

    /** 更新英雄属性概览（如攻击力、法强、护甲等）的显示内容。 */
    void UpdateChampionStats(const FHUDChampionStatsViewModel& Stats);

    /** 将当前生效的状态标签（Gameplay Tags）转发到增益/减益图标条以便显示文本备选。 */
    void UpdateStatusEffects(const TArray<FName>& TagNames);

    /** 将经过策划/筛选的增益图标视图模型转发到动作条的图标条，优先显示图形化图标。 */
    void UpdateStatusBuffs(const TArray<FHUDBuffIconViewModel>& Buffs);

    /** 更新头像贴图及相关提示（是否处于战斗、是否有升级未分配等）。 */
    void UpdatePortrait(UTexture2D* PortraitTexture, bool bIsInCombat, bool bHasLevelUp);

    /** 更新显示在动作条下方的法力/能量（主/次资源）数值。 */
    void UpdateResources(float CurrentPrimary, float MaxPrimary, float CurrentSecondary, float MaxSecondary);

    /** 更新动作条上的经验进度条与等级数字显示。 */
    void UpdateExperience(int32 CurrentLevel, float CurrentExperience, float RequiredExperience);

    /** 更新右侧的装备/物品槽显示（物品图标与可用性）。 */
    void UpdateItemSlots(const TArray<FHUDItemSlotViewModel>& Slots);

    /** 更新右侧的三个快捷功能按钮的显示（例如快捷使用或召唤按钮）。 */
    void UpdateUtilityButtons(const TArray<FHUDItemSlotViewModel>& Buttons);

protected:
    void PushVitalStateToActionBar();

    /** 底部中央的动作条（用于呈现技能、物品与召唤槽）。 */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDActionBarWidget> ActionBar;

    /** 左下角的状态面板（头像、生命/法力、Buff 列表）。 */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDStatusPanelWidget> StatusPanel;

    /** 右下角的资源面板（物品格、快捷功能按钮等）。 */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDResourcePanelWidget> ResourcePanel;

private:
    float CachedCurrentHealth = 0.f;
    float CachedMaxHealth = 1.f;
    float CachedCurrentMana = 0.f;
    float CachedMaxMana = 1.f;
};
