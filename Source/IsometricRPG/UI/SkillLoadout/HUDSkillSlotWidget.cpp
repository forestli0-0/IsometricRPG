#include "UI/SkillLoadout/HUDSkillSlotWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UMG.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
    constexpr float EmptyIconOpacity = 0.2f;
    constexpr float FilledIconOpacity = 1.0f;
}

void UHUDSkillSlotWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    ApplySlotDataToWidgets();
    UpdateEmptyState();
    ApplyCooldownToWidgets(bCooldownActive ? CooldownEndTime - GetWorldTime() : 0.f);
}

void UHUDSkillSlotWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 设置圆形冷却材质实例：如果为遮罩分配了材质，则使用或从 Image 的 Brush 中读取材质并创建动态实例
    if (CooldownMaskImage)
    {
        UMaterialInterface* SourceMat = CooldownRadialMaterial;
        if (!SourceMat)
        {
            if (UObject* Res = CooldownMaskImage->GetBrush().GetResourceObject())
            {
                SourceMat = Cast<UMaterialInterface>(Res);
            }
        }

        if (SourceMat)
        {
            CooldownMID = UMaterialInstanceDynamic::Create(SourceMat, this);
            if (CooldownMID)
            {
                // 将动态材质应用到遮罩图像，并初始化颜色/可见性
                CooldownMaskImage->SetBrushFromMaterial(CooldownMID);
                CooldownMaskImage->SetColorAndOpacity(CooldownMaskTint);
                CooldownMaskImage->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }

    UpdateEmptyState();
}

void UHUDSkillSlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 若未处于冷却状态则跳过
    if (!bCooldownActive)
    {
        return;
    }

    const float Remaining = CooldownEndTime - GetWorldTime();
    // 剩余时间为 0 或以下则终止冷却并重置显示
    if (Remaining <= 0.f)
    {
        CancelCooldown();
        return;
    }

    // 更新冷却遮罩与文本
    ApplyCooldownToWidgets(Remaining);
}

void UHUDSkillSlotWidget::SetSlotData(const FHUDSkillSlotViewModel& InData)
{
    SlotData = InData;
    bHasData = true;

    ApplySlotDataToWidgets();
    UpdateEmptyState();

    if (SlotData.CooldownDuration > 0.f && SlotData.CooldownRemaining > 0.f)
    {
        BeginCooldown(SlotData.CooldownDuration, SlotData.CooldownRemaining);
    }
    else
    {
        CancelCooldown();
    }
}

void UHUDSkillSlotWidget::ClearSlot()
{
    SlotData = FHUDSkillSlotViewModel{};
    bHasData = false;

    ApplySlotDataToWidgets();
    UpdateEmptyState();
    CancelCooldown();
}

void UHUDSkillSlotWidget::BeginCooldown(float DurationSeconds, float InitialRemainingSeconds)
{
    CooldownDuration = FMath::Max(DurationSeconds, 0.f);
    if (CooldownDuration <= KINDA_SMALL_NUMBER)
    {
        CancelCooldown();
        return;
    }

    float Remaining = InitialRemainingSeconds;
    if (Remaining <= 0.f || Remaining > CooldownDuration)
    {
        Remaining = CooldownDuration;
    }

    CooldownEndTime = GetWorldTime() + Remaining;
    bCooldownActive = true;

    ApplyCooldownToWidgets(Remaining);
}

void UHUDSkillSlotWidget::CancelCooldown()
{
    bCooldownActive = false;
    CooldownDuration = 0.f;
    CooldownEndTime = 0.f;

    ApplyCooldownToWidgets(0.f);
}

void UHUDSkillSlotWidget::ApplySlotDataToWidgets()
{
    if (IconImage)
    {
        IconImage->SetBrushFromTexture(SlotData.Icon);
        IconImage->SetOpacity((bHasData && SlotData.Icon) ? FilledIconOpacity : EmptyIconOpacity);
    }

    if (NameText)
    {
        NameText->SetText(SlotData.DisplayName);
        NameText->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (HotkeyText)
    {
        if (SlotData.HotkeyLabel.IsEmpty())
        {
            HotkeyText->SetVisibility(ESlateVisibility::Collapsed);
        }
        else
        {
            HotkeyText->SetText(SlotData.HotkeyLabel);
            HotkeyText->SetColorAndOpacity(HotkeyTextColor);
            HotkeyText->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }
}

void UHUDSkillSlotWidget::ApplyCooldownToWidgets(float RemainingSeconds)
{
    // 隐藏旧的线性冷却条（保留兼容性），当前优先使用圆形遮罩与中心倒计时
    if (CooldownProgress)
    {
        CooldownProgress->SetVisibility(ESlateVisibility::Collapsed);
        CooldownProgress->SetPercent(0.f);
    }

    const bool bShow = bCooldownActive && CooldownDuration > 0.f;

    // 根据是否处于冷却显示或隐藏遮罩与文本
    if (CooldownMaskImage)
    {
        CooldownMaskImage->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    if (CooldownText)
    {
        CooldownText->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }

    if (!bShow)
    {
        // 若不展示则将材质参数与文本重置
        if (CooldownMID)
        {
            CooldownMID->SetScalarParameterValue(CooldownPercentParamName, 0.f);
        }
        if (CooldownText)
        {
            CooldownText->SetText(FText::GetEmpty());
        }
        return;
    }

    // 计算冷却百分比并把值写入动态材质参数以驱动遮罩表现
    const float Percent = FMath::Clamp(RemainingSeconds / CooldownDuration, 0.f, 1.f);
    if (CooldownMID)
    {
        CooldownMID->SetScalarParameterValue(CooldownPercentParamName, Percent);
    }

    // 中央倒计时文本以整秒显示剩余时间
    if (CooldownText)
    {
        const int32 SecondsLeft = FMath::CeilToInt(RemainingSeconds);
        CooldownText->SetText(FText::AsNumber(SecondsLeft));
    }
}

void UHUDSkillSlotWidget::UpdateEmptyState()
{
    const bool bShowEmpty = !bHasData || !SlotData.bIsEquipped || !SlotData.bIsUnlocked;

    if (EmptyStateOverlay)
    {
        EmptyStateOverlay->SetVisibility(bShowEmpty ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (IconImage && (!SlotData.Icon || bShowEmpty))
    {
        IconImage->SetOpacity(EmptyIconOpacity);
    }
}

float UHUDSkillSlotWidget::GetWorldTime() const
{
    if (const UWorld* World = GetWorld())
    {
        return World->GetTimeSeconds();
    }

    return 0.f;
}
