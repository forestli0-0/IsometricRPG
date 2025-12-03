#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModelTypes.h"
#include "Styling/SlateBrush.h"
#include "HUDInventorySlotWidget.generated.h"

class UImage;
class UTextBlock;

/** Widget responsible for showing one inventory/equipment slot. */
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

    /** Captured designer brush so we can restore it when no data is present. */
    UPROPERTY(Transient)
    FSlateBrush OriginalBrush;

    UPROPERTY(Transient)
    bool bOriginalBrushCaptured = false;

    /** Tint applied while showing the placeholder. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
    FLinearColor EmptySlotTint = FLinearColor(0.1f, 0.1f, 0.1f, 0.9f);

    /** Tint applied when an actual item icon is present. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
    FLinearColor FilledSlotTint = FLinearColor::White;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> IconImage;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> StackCountText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> HotkeyText;
};
