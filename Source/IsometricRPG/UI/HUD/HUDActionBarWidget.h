#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IsometricRPG/IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "UI/HUD/HUDViewModelTypes.h"
#include "Materials/MaterialInterface.h"
#include "HUDActionBarWidget.generated.h"

struct FHUDSkillSlotViewModel;
class UHUDSkillLoadoutWidget;
class UHUDSkillSlotWidget;

class UHorizontalBox;
class UProgressBar;
class UTextBlock;
class UOverlay;
class UImage;
/**
 * 底部中央的动作条：包含技能槽、消耗品以及上下文提示（例如物品/召唤提示）。
 */
UCLASS()
class ISOMETRICRPG_API UHUDActionBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 当 WidgetTree 就绪时构建槽位映射表以便快速查找与更新。 */
    virtual void NativeConstruct() override;

    /** 将视图模型数据分配给指定的技能槽（更新图标/冷却/名称等）。 */
    void SetSlot(const FHUDSkillSlotViewModel& ViewModel);

    /** 清空单个技能槽（恢复为空视觉状态）。 */
    void ClearSlot(ESkillSlot Slot);

    /** 清空所有已注册的技能槽（通常用于重置或初始化）。 */
    void ClearAllSlots();

    /** 应用冷却信息到指定槽位（开始或取消冷却显示）。 */
    void ApplyCooldown(ESkillSlot Slot, float Duration, float Remaining);

    /** 设置显示在技能栏下方的生命/法力数值（用于血量与资源条）。 */
    void SetVitals(float InCurrentHealth, float InMaxHealth, float InCurrentMana, float InMaxMana);

    /** 更新显示在生命/法力下方的经验进度条与等级数字。 */
    void SetExperience(int32 InCurrentLevel, float InCurrentXP, float InRequiredXP);

    /** 更新显示在技能上方的简易 Buff 列表（文本标签路径）。 */
    void SetStatusTags(const TArray<FName>& InTags);

    /** 使用策划好的图标条更新 Buff 展示（优先使用图标而非文本标签）。 */
    void SetStatusBuffs(const TArray<FHUDBuffIconViewModel>& InBuffs);

protected:
    UHUDSkillSlotWidget* FindSlot(ESkillSlot Slot) const;
    void RefreshVitalWidgets();
    void RefreshExperienceWidgets();
    void RefreshBuffWidgets();

    /** 渲染技能槽与物品槽的子控件（Loadout 容器）。 */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDSkillLoadoutWidget> SkillLoadout;

    /** 显示在技能槽下方的生命/法力条。 */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> HealthBar;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> ManaBar;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> HealthText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ManaText;

    /** 经验条与其标签，显示在生命/法力条下方。 */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> ExperienceBar;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ExperienceText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> LevelText;

    /** 放置在技能条上方的水平容器，用于显示 Buff 图标或标签。 */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHorizontalBox> BuffStrip;

    /** 可选的冷却遮罩材质（用于为图标叠加径向冷却遮罩）；为空时会在运行时创建并使用默认材质。 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buffs", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UMaterialInterface> BuffCooldownMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buffs", meta = (AllowPrivateAccess = "true"))
    FName BuffCooldownPercentParam = TEXT("Percent");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buffs", meta = (AllowPrivateAccess = "true"))
    FLinearColor BuffCooldownTint = FLinearColor(0.f, 0.f, 0.f, 0.65f);

private:
    TMap<ESkillSlot, TObjectPtr<UHUDSkillSlotWidget>> SlotLookup;
    void RebuildSlotLookup();

    float CurrentHealth = 0.f;
    float MaxHealth = 1.f;
    float CurrentMana = 0.f;
    float MaxMana = 1.f;

    int32 CurrentLevel = 1;
    float CurrentExperience = 0.f;
    float RequiredExperience = 1.f;

    TArray<FName> CachedBuffTags; // 旧的调试路径（遗留用途）
    TArray<FHUDBuffIconViewModel> CachedBuffIcons;

    /** 风格设定：Buff 图标在条上的显示尺寸（XY）。 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style", meta = (AllowPrivateAccess = "true"))
    FVector2D BuffIconSize = FVector2D(20.f, 20.f);
};
