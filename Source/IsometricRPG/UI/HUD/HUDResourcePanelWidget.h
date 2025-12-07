#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModelTypes.h"
#include "HUDResourcePanelWidget.generated.h"

class UHUDInventorySlotWidget;

/**
 * 右下角的资源面板：显示物品轮播（快捷栏）与三个快速功能按钮（例如瞬移/守卫等）。
 */
UCLASS()
class ISOMETRICRPG_API UHUDResourcePanelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 更新右侧的装备/物品槽显示（按索引填充图标/数量）。 */
    void SetItemSlots(const TArray<FHUDItemSlotViewModel>& InSlots);

    /** 更新可配置的快速访问按钮（例如守卫、回城、其它快捷技能/道具）。 */
    void SetUtilityButtons(const TArray<FHUDItemSlotViewModel>& InButtons);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;

private:
    void RefreshItemSlots();
    void RefreshUtilityButtons();
    void RegisterItemSlotWidgets();
    void RegisterUtilityButtonWidgets();

private:
    TArray<FHUDItemSlotViewModel> CachedItemSlots;
    TArray<FHUDItemSlotViewModel> CachedUtilityButtons;

    /** 绑定的子控件集合，用于在 UI 中进行可视化刷新。 */

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> ItemSlot_1;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> ItemSlot_2;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> ItemSlot_3;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> ItemSlot_4;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> ItemSlot_5;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> ItemSlot_6;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> UtilityButton_1;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> UtilityButton_2;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UHUDInventorySlotWidget> UtilityButton_3;

    TArray<TObjectPtr<UHUDInventorySlotWidget>> ItemSlotWidgets;
    TArray<TObjectPtr<UHUDInventorySlotWidget>> UtilityButtonWidgets;
};
