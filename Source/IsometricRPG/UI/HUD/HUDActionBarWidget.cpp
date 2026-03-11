#include "UI/HUD/HUDActionBarWidget.h"

#include "UI/SkillLoadout/HUDSkillLoadoutWidget.h"
#include "UI/SkillLoadout/HUDSkillSlotWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	FString SlotToDebugName(ESkillSlot Slot)
	{
		switch (Slot)
		{
		case ESkillSlot::Skill_Q: return TEXT("Skill_Q");
		case ESkillSlot::Skill_W: return TEXT("Skill_W");
		case ESkillSlot::Skill_E: return TEXT("Skill_E");
		case ESkillSlot::Skill_R: return TEXT("Skill_R");
		case ESkillSlot::Skill_D: return TEXT("Skill_D");
		case ESkillSlot::Skill_F: return TEXT("Skill_F");
			default: return TEXT("Invalid");
		}
	}

	bool AreBuffIconArraysEquivalent(const TArray<FHUDBuffIconViewModel>& Left, const TArray<FHUDBuffIconViewModel>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			const FHUDBuffIconViewModel& LeftEntry = Left[Index];
			const FHUDBuffIconViewModel& RightEntry = Right[Index];
			if (LeftEntry.TagName != RightEntry.TagName
				|| LeftEntry.Icon != RightEntry.Icon
				|| LeftEntry.StackCount != RightEntry.StackCount
				|| LeftEntry.bIsDebuff != RightEntry.bIsDebuff
				|| !FMath::IsNearlyEqual(LeftEntry.TimeRemaining, RightEntry.TimeRemaining, 0.05f)
				|| !FMath::IsNearlyEqual(LeftEntry.TotalDuration, RightEntry.TotalDuration, 0.05f))
			{
				return false;
			}
		}

		return true;
	}
}

void UHUDActionBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SlotLookup.Reset();
	BuffIconEntries.Reset();
	BuffTagLabels.Reset();

	if (BuffStrip)
	{
		BuffStrip->ClearChildren();
	}

	RebuildSlotLookup();

	RefreshVitalWidgets();
	RefreshExperienceWidgets();
	RefreshBuffWidgets();
}

void UHUDActionBarWidget::SetSlot(const FHUDSkillSlotViewModel& ViewModel)
{
	if (ViewModel.Slot == ESkillSlot::Invalid || ViewModel.Slot == ESkillSlot::MAX || !ViewModel.bIsEquipped)
	{
		return;
	}

	if (UHUDSkillSlotWidget* FoundSlot = FindSlot(ViewModel.Slot))
	{
		FoundSlot->SetSlotData(ViewModel);

		if (SkillLoadout)
		{
			const TArray<TObjectPtr<UHUDSkillSlotWidget>>& Slots = SkillLoadout->GetSlots();
			const int32 Index = Slots.IndexOfByKey(FoundSlot);
			if (Index != INDEX_NONE)
			{
				SkillLoadout->AssignSlot(Index, ViewModel);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[HUDActionBar] No slot widget bound for %s"), *SlotToDebugName(ViewModel.Slot));
	}
}

void UHUDActionBarWidget::ClearSlot(ESkillSlot InSlot)
{
	if (UHUDSkillSlotWidget* SlotWidget = FindSlot(InSlot))
	{
		SlotWidget->ClearSlot();

		if (SkillLoadout)
		{
			const TArray<TObjectPtr<UHUDSkillSlotWidget>>& Slots = SkillLoadout->GetSlots();
			const int32 Index = Slots.IndexOfByKey(SlotWidget);
			if (Index != INDEX_NONE)
			{
				SkillLoadout->ClearSlot(Index);
			}
		}
	}
}

void UHUDActionBarWidget::ClearAllSlots()
{
	for (TPair<ESkillSlot, TObjectPtr<UHUDSkillSlotWidget>>& Pair : SlotLookup)
	{
		if (Pair.Value)
		{
			Pair.Value->ClearSlot();
		}
	}
}

void UHUDActionBarWidget::ApplyCooldown(ESkillSlot InSlot, float Duration, float Remaining)
{
	if (UHUDSkillSlotWidget* SlotWidget = FindSlot(InSlot))
	{
		if (Duration <= 0.f || Remaining <= 0.f)
		{
			SlotWidget->CancelCooldown();
		}
		else
		{
			SlotWidget->BeginCooldown(Duration, Remaining);
		}
	}
}

void UHUDActionBarWidget::SetVitals(float InCurrentHealth, float InMaxHealth, float InCurrentMana, float InMaxMana)
{
	const float NewCurrentHealth = FMath::Max(0.f, InCurrentHealth);
	const float NewMaxHealth = FMath::Max(KINDA_SMALL_NUMBER, InMaxHealth);
	const float NewCurrentMana = FMath::Max(0.f, InCurrentMana);
	const float NewMaxMana = FMath::Max(KINDA_SMALL_NUMBER, InMaxMana);

	if (FMath::IsNearlyEqual(CurrentHealth, NewCurrentHealth, 0.01f)
		&& FMath::IsNearlyEqual(MaxHealth, NewMaxHealth, 0.01f)
		&& FMath::IsNearlyEqual(CurrentMana, NewCurrentMana, 0.01f)
		&& FMath::IsNearlyEqual(MaxMana, NewMaxMana, 0.01f))
	{
		return;
	}

	CurrentHealth = NewCurrentHealth;
	MaxHealth = NewMaxHealth;
	CurrentMana = NewCurrentMana;
	MaxMana = NewMaxMana;

	RefreshVitalWidgets();
}

void UHUDActionBarWidget::SetExperience(int32 InCurrentLevel, float InCurrentXP, float InRequiredXP)
{
	const int32 NewCurrentLevel = FMath::Max(1, InCurrentLevel);
	const float NewCurrentExperience = FMath::Max(0.f, InCurrentXP);
	const float NewRequiredExperience = FMath::Max(KINDA_SMALL_NUMBER, InRequiredXP);

	if (CurrentLevel == NewCurrentLevel
		&& FMath::IsNearlyEqual(CurrentExperience, NewCurrentExperience, 0.01f)
		&& FMath::IsNearlyEqual(RequiredExperience, NewRequiredExperience, 0.01f))
	{
		return;
	}

	CurrentLevel = NewCurrentLevel;
	CurrentExperience = NewCurrentExperience;
	RequiredExperience = NewRequiredExperience;

	RefreshExperienceWidgets();
}

void UHUDActionBarWidget::SetStatusTags(const TArray<FName>& InTags)
{
	if (CachedBuffTags == InTags)
	{
		return;
	}

	CachedBuffTags = InTags;
	RefreshBuffWidgets();
}

void UHUDActionBarWidget::SetStatusBuffs(const TArray<FHUDBuffIconViewModel>& InBuffs)
{
	if (AreBuffIconArraysEquivalent(CachedBuffIcons, InBuffs))
	{
		return;
	}

	CachedBuffIcons = InBuffs;
	RefreshBuffWidgets();
}

UHUDSkillSlotWidget* UHUDActionBarWidget::FindSlot(ESkillSlot InSlot) const
{
	if (const TObjectPtr<UHUDSkillSlotWidget>* Found = SlotLookup.Find(InSlot))
	{
		return Found->Get();
	}
	return nullptr;
}

void UHUDActionBarWidget::RebuildSlotLookup()
{
	if (!SkillLoadout)
	{
		return;
	}

	const TArray<TObjectPtr<UHUDSkillSlotWidget>>& Slots = SkillLoadout->GetSlots();
	for (UHUDSkillSlotWidget* SlotWidget : Slots)
	{
		if (!SlotWidget)
		{
			continue;
		}

		const ESkillSlot SlotType = SlotWidget->GetConfiguredSlot();
		if (SlotType == ESkillSlot::Invalid || SlotType == ESkillSlot::MAX)
		{
			continue;
		}

		SlotLookup.Add(SlotType, SlotWidget);
	}
}

UHUDActionBarWidget::FBuffIconEntryWidgets& UHUDActionBarWidget::GetOrCreateBuffIconEntry(int32 Index)
{
	while (BuffIconEntries.Num() <= Index)
	{
		FBuffIconEntryWidgets NewEntry;
		if (WidgetTree && BuffStrip)
		{
			NewEntry.Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			if (NewEntry.Root)
			{
				NewEntry.Root->SetVisibility(ESlateVisibility::Collapsed);

				NewEntry.IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
				if (NewEntry.IconImage)
				{
					if (UOverlaySlot* IconSlot = NewEntry.Root->AddChildToOverlay(NewEntry.IconImage))
					{
						IconSlot->SetHorizontalAlignment(HAlign_Fill);
						IconSlot->SetVerticalAlignment(VAlign_Fill);
					}
				}

				NewEntry.CooldownMask = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
				if (NewEntry.CooldownMask)
				{
					NewEntry.CooldownMask->SetVisibility(ESlateVisibility::Collapsed);
					NewEntry.CooldownMask->SetDesiredSizeOverride(BuffIconSize);

					if (UOverlaySlot* CooldownSlot = NewEntry.Root->AddChildToOverlay(NewEntry.CooldownMask))
					{
						CooldownSlot->SetHorizontalAlignment(HAlign_Fill);
						CooldownSlot->SetVerticalAlignment(VAlign_Fill);
					}
				}

				NewEntry.StackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
				if (NewEntry.StackText)
				{
					NewEntry.StackText->SetVisibility(ESlateVisibility::Collapsed);
					NewEntry.StackText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
					NewEntry.StackText->SetShadowOffset(FVector2D(1.f, 1.f));
					NewEntry.StackText->SetJustification(ETextJustify::Right);
					if (UOverlaySlot* StackSlot = NewEntry.Root->AddChildToOverlay(NewEntry.StackText))
					{
						StackSlot->SetHorizontalAlignment(HAlign_Right);
						StackSlot->SetVerticalAlignment(VAlign_Bottom);
						StackSlot->SetPadding(FMargin(0.f, 0.f, 1.f, 1.f));
					}
				}

				BuffStrip->AddChildToHorizontalBox(NewEntry.Root);
			}
		}

		BuffIconEntries.Add(MoveTemp(NewEntry));
	}

	return BuffIconEntries[Index];
}

UTextBlock* UHUDActionBarWidget::GetOrCreateBuffTagLabel(int32 Index)
{
	while (BuffTagLabels.Num() <= Index)
	{
		TObjectPtr<UTextBlock> NewLabel = nullptr;
		if (WidgetTree && BuffStrip)
		{
			NewLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			if (NewLabel)
			{
				NewLabel->SetVisibility(ESlateVisibility::Collapsed);
				NewLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
				NewLabel->SetShadowOffset(FVector2D(1.f, 1.f));
				BuffStrip->AddChildToHorizontalBox(NewLabel);
			}
		}

		BuffTagLabels.Add(NewLabel);
	}

	return BuffTagLabels[Index];
}

void UHUDActionBarWidget::HideAllBuffWidgets()
{
	for (FBuffIconEntryWidgets& Entry : BuffIconEntries)
	{
		if (Entry.Root)
		{
			Entry.Root->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	for (TObjectPtr<UTextBlock>& TagLabel : BuffTagLabels)
	{
		if (TagLabel)
		{
			TagLabel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UHUDActionBarWidget::RefreshVitalWidgets()
{
	const float HealthPercent = FMath::Clamp(CurrentHealth / MaxHealth, 0.f, 1.f);
	if (HealthBar)
	{
		HealthBar->SetPercent(HealthPercent);
	}

	if (HealthText)
	{
		HealthText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(CurrentHealth), FMath::RoundToInt(MaxHealth))));
	}

	const float ManaPercent = FMath::Clamp(CurrentMana / MaxMana, 0.f, 1.f);
	if (ManaBar)
	{
		ManaBar->SetPercent(ManaPercent);
	}

	if (ManaText)
	{
		ManaText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(CurrentMana), FMath::RoundToInt(MaxMana))));
	}
}

void UHUDActionBarWidget::RefreshExperienceWidgets()
{
	const float SafeRequiredXP = FMath::Max(RequiredExperience, KINDA_SMALL_NUMBER);
	const float XPPercent = FMath::Clamp(CurrentExperience / SafeRequiredXP, 0.f, 1.f);

	if (ExperienceBar)
	{
		ExperienceBar->SetPercent(XPPercent);
	}

	if (ExperienceText)
	{
		ExperienceText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), CurrentExperience, SafeRequiredXP)));
	}

	if (LevelText)
	{
		LevelText->SetText(FText::AsNumber(CurrentLevel));
	}
}

void UHUDActionBarWidget::RefreshBuffWidgets()
{
	if (!BuffStrip)
	{
		return;
	}

	HideAllBuffWidgets();

	// 优先使用预先策划并在 PlayerState 中指定的图标（更美观且可控）；
	// 当没有可用图标时退回到显示调试用的标签名（避免出现白色占位方块）
	if (CachedBuffIcons.Num() > 0)
	{
		int32 VisibleIconCount = 0;

		for (const FHUDBuffIconViewModel& Buff : CachedBuffIcons)
		{
			if (!Buff.Icon)
			{
				continue; // 跳过未解析的图标（避免出现白色占位方块）
			}

			FBuffIconEntryWidgets& Entry = GetOrCreateBuffIconEntry(VisibleIconCount++);
			if (!Entry.Root || !Entry.IconImage)
			{
				continue;
			}

			Entry.Root->SetVisibility(ESlateVisibility::HitTestInvisible);
			Entry.IconImage->SetBrushFromTexture(Buff.Icon);
			FSlateBrush Brush = Entry.IconImage->GetBrush();
			Brush.ImageSize = BuffIconSize;
			Entry.IconImage->SetBrush(Brush);
			Entry.IconImage->SetDesiredSizeOverride(BuffIconSize);
			Entry.IconImage->SetColorAndOpacity(Buff.bIsDebuff ? FLinearColor(1.f, 0.3f, 0.3f, 1.f) : FLinearColor::White);

			const bool bShowCooldown = Buff.TimeRemaining >= 0.f && Buff.TotalDuration > KINDA_SMALL_NUMBER && Entry.CooldownMask;
			if (bShowCooldown)
			{
				if (!Entry.CooldownMID && BuffCooldownMaterial)
				{
					Entry.CooldownMID = UMaterialInstanceDynamic::Create(BuffCooldownMaterial, this);
					if (Entry.CooldownMID)
					{
						Entry.CooldownMask->SetBrushFromMaterial(Entry.CooldownMID);
					}
				}
				if (Entry.CooldownMID)
				{
					const float Percent = FMath::Clamp(Buff.TimeRemaining / Buff.TotalDuration, 0.f, 1.f);
					Entry.CooldownMID->SetScalarParameterValue(BuffCooldownPercentParam, Percent);
				}
				Entry.CooldownMask->SetColorAndOpacity(BuffCooldownTint);
				Entry.CooldownMask->SetDesiredSizeOverride(BuffIconSize);
				Entry.CooldownMask->SetVisibility(Entry.CooldownMID ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			}
			else if (Entry.CooldownMask)
			{
				Entry.CooldownMask->SetVisibility(ESlateVisibility::Collapsed);
			}

			if (Entry.StackText)
			{
				if (Buff.StackCount > 1)
				{
					Entry.StackText->SetText(FText::AsNumber(Buff.StackCount));
					Entry.StackText->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
				else
				{
					Entry.StackText->SetText(FText::GetEmpty());
					Entry.StackText->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}

		if (VisibleIconCount > 0)
		{
			BuffStrip->SetVisibility(ESlateVisibility::HitTestInvisible);
			return;
		}
	}

	if (CachedBuffTags.Num() == 0)
	{
		BuffStrip->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	BuffStrip->SetVisibility(ESlateVisibility::HitTestInvisible);

	int32 VisibleTagCount = 0;
	for (const FName& TagName : CachedBuffTags)
	{
		if (UTextBlock* TagLabel = GetOrCreateBuffTagLabel(VisibleTagCount++))
		{
			TagLabel->SetText(FText::FromName(TagName));
			TagLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}
