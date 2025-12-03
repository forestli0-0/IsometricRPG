#include "UI/HUD/HUDInventorySlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void UHUDInventorySlotWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    RefreshSlot();
}

void UHUDInventorySlotWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshSlot();
}

void UHUDInventorySlotWidget::SetSlotData(const FHUDItemSlotViewModel& InData)
{
    CachedData = InData;
    bHasData = true;
    RefreshSlot();
}

void UHUDInventorySlotWidget::ClearSlot()
{
    CachedData = FHUDItemSlotViewModel();
    bHasData = false;
    RefreshSlot();
}

void UHUDInventorySlotWidget::RefreshSlot()
{
    if (IconImage)
    {
        if (!bOriginalBrushCaptured)
        {
            OriginalBrush = IconImage->GetBrush();
            bOriginalBrushCaptured = true;
        }

        if (bHasData && CachedData.Icon)
        {
            IconImage->SetBrushFromTexture(CachedData.Icon);
            IconImage->SetColorAndOpacity(FilledSlotTint);
        }
        else
        {
            IconImage->SetBrush(OriginalBrush);
            IconImage->SetColorAndOpacity(EmptySlotTint);
        }

        IconImage->SetVisibility(ESlateVisibility::Visible);
    }

    if (StackCountText)
    {
        if (bHasData && CachedData.StackCount > 1)
        {
            StackCountText->SetText(FText::AsNumber(CachedData.StackCount));
            StackCountText->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            StackCountText->SetText(FText::GetEmpty());
            StackCountText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (HotkeyText)
    {
        if (bHasData && !CachedData.HotkeyLabel.IsEmpty())
        {
            HotkeyText->SetText(CachedData.HotkeyLabel);
            HotkeyText->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            HotkeyText->SetText(FText::GetEmpty());
            HotkeyText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}
