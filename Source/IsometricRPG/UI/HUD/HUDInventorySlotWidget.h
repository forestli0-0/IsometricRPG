#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModelTypes.h"
#include "Styling/SlateBrush.h"
#include "HUDInventorySlotWidget.generated.h"

class UImage;
class UTextBlock;

/** 显示单个物品/装备格位的控件（负责图标、数量与快捷键显示）。 */
UCLASS()
class ISOMETRICRPG_API UHUDInventorySlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetSlotData(const FHUDItemSlotViewModel& InData);
    void ClearSlot();

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;

private:
    void RefreshSlot();

private:
    FHUDItemSlotViewModel CachedData;
    bool bHasData = false;

    /** 保存设计器中设置的初始 Brush，以便在无数据时恢复占位样式。 */
    UPROPERTY(Transient)
    FSlateBrush OriginalBrush;

    UPROPERTY(Transient)
    bool bOriginalBrushCaptured = false;

    /** 占位时应用的颜色遮罩（当格位为空时使用）。 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
    FLinearColor EmptySlotTint = FLinearColor(0.1f, 0.1f, 0.1f, 0.9f);

    /** 有实际物品图标时应用的颜色遮罩（用于恢复高亮/正常显示）。 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
    FLinearColor FilledSlotTint = FLinearColor::White;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> IconImage;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> StackCountText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> HotkeyText;
};
