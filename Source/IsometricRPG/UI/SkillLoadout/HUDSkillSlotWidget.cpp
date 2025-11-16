#include "UI/SkillLoadout/HUDSkillSlotWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UMG.h"

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
        if (SlotData.DisplayName.IsEmpty())
        {
            NameText->SetVisibility(ESlateVisibility::Collapsed);
        }
        else
        {
            NameText->SetText(SlotData.DisplayName);
            NameText->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
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
            HotkeyText->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }
}

void UHUDSkillSlotWidget::ApplyCooldownToWidgets(float RemainingSeconds)
{
    if (!CooldownProgress)
    {
        return;
    }

    if (!bCooldownActive)
    {
        CooldownProgress->SetVisibility(ESlateVisibility::Collapsed);
        CooldownProgress->SetPercent(0.f);
        return;
    }

    CooldownProgress->SetVisibility(ESlateVisibility::Visible);

    if (CooldownDuration > 0.f)
    {
        const float Percent = FMath::Clamp(RemainingSeconds / CooldownDuration, 0.f, 1.f);
        CooldownProgress->SetPercent(Percent);
    }
    else
    {
        CooldownProgress->SetPercent(0.f);
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
