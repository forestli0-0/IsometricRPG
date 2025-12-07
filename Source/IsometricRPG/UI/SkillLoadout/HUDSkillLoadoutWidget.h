#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IsometricRPG/IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "HUDSkillLoadoutWidget.generated.h"

struct FHUDSkillSlotViewModel;
class UHUDSkillSlotWidget;

/**
 * 技能配置容器控件，负责排布 Q/W/E/R 等技能槽以及辅助动作/物品槽。
 */
UCLASS()
class ISOMETRICRPG_API UHUDSkillLoadoutWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    /** 注册一个槽控件（由 UMG 设计器在运行时绑定具体槽部件）。 */
    UFUNCTION(BlueprintCallable, Category = "SkillLoadout")
    void RegisterSlot(UHUDSkillSlotWidget* InSlot);

    /** 清除所有已注册的槽引用，通常在重新从设计器重建时调用。 */
    UFUNCTION(BlueprintCallable, Category = "SkillLoadout")
    void ResetSlots();

    /** 将给定的视图模型数据分配到指定索引的槽位，用于刷新该槽显示。 */
    void AssignSlot(int32 Index, const FHUDSkillSlotViewModel& ViewModel);

    /** 清除指定索引的槽位显示（恢复为空状态）。 */
    void ClearSlot(int32 Index);

    /** 返回已注册槽位的只读访问引用（用于遍历或查找）。 */
    const TArray<TObjectPtr<UHUDSkillSlotWidget>>& GetSlots() const { return SkillSlots; }

protected:
    void RegisterDesignerSlots();

    /** 被动技能槽（常驻图标，通常显示被动效果或始终可用的技能）。 */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDSkillSlotWidget> Slot_Passive;

    /** 按枚举键绑定的各个技能槽（由 Loadout Blueprint 绑定实际子部件）。 */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDSkillSlotWidget> Slot_Q;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDSkillSlotWidget> Slot_W;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDSkillSlotWidget> Slot_E;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDSkillSlotWidget> Slot_R;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDSkillSlotWidget> Slot_D;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDSkillSlotWidget> Slot_F;

    UPROPERTY(VisibleAnywhere, Category = "SkillLoadout")
    TArray<TObjectPtr<UHUDSkillSlotWidget>> SkillSlots;
};
