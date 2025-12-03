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

    // Setup radial cooldown material instance if a material is assigned
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

    if (!bCooldownActive)
    {
        return;
    }

    const float Remaining = CooldownEndTime - GetWorldTime();
    if (Remaining <= 0.f)
    {
        CancelCooldown();
        return;
    }

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
    // Always hide legacy linear bar if present
    if (CooldownProgress)
    {
        CooldownProgress->SetVisibility(ESlateVisibility::Collapsed);
        CooldownProgress->SetPercent(0.f);
    }

    const bool bShow = bCooldownActive && CooldownDuration > 0.f;

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

    const float Percent = FMath::Clamp(RemainingSeconds / CooldownDuration, 0.f, 1.f);
    if (CooldownMID)
    {
        CooldownMID->SetScalarParameterValue(CooldownPercentParamName, Percent);
    }

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
