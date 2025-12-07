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
}

void UHUDActionBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SlotLookup.Reset();

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
	CurrentHealth = FMath::Max(0.f, InCurrentHealth);
	MaxHealth = FMath::Max(KINDA_SMALL_NUMBER, InMaxHealth);
	CurrentMana = FMath::Max(0.f, InCurrentMana);
	MaxMana = FMath::Max(KINDA_SMALL_NUMBER, InMaxMana);

	RefreshVitalWidgets();
}

void UHUDActionBarWidget::SetExperience(int32 InCurrentLevel, float InCurrentXP, float InRequiredXP)
{
	CurrentLevel = FMath::Max(1, InCurrentLevel);
	CurrentExperience = FMath::Max(0.f, InCurrentXP);
	RequiredExperience = FMath::Max(KINDA_SMALL_NUMBER, InRequiredXP);

	RefreshExperienceWidgets();
}

void UHUDActionBarWidget::SetStatusTags(const TArray<FName>& InTags)
{
	CachedBuffTags = InTags;
	RefreshBuffWidgets();
}

void UHUDActionBarWidget::SetStatusBuffs(const TArray<FHUDBuffIconViewModel>& InBuffs)
{
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

	BuffStrip->ClearChildren();

	// 优先使用预先策划并在 PlayerState 中指定的图标（更美观且可控）；
	// 当没有可用图标时退回到显示调试用的标签名（避免出现白色占位方块）
	if (CachedBuffIcons.Num() > 0)
	{
		BuffStrip->SetVisibility(ESlateVisibility::HitTestInvisible);

		for (const FHUDBuffIconViewModel& Buff : CachedBuffIcons)
		{
			if (!WidgetTree)
			{
				break;
			}

			if (!Buff.Icon)
			{
				continue; // 跳过未解析的图标（避免出现白色占位方块）
			}

			UOverlay* IconOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			if (!IconOverlay)
			{
				continue;
			}

			// 基础图标：从纹理构造 Image 并设置尺寸/颜色
			UImage* IconWidget = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			if (IconWidget)
			{
				IconWidget->SetBrushFromTexture(Buff.Icon);
				FSlateBrush Brush = IconWidget->GetBrush();
				Brush.ImageSize = BuffIconSize;
				IconWidget->SetBrush(Brush);
				IconWidget->SetDesiredSizeOverride(BuffIconSize);
				IconWidget->SetColorAndOpacity(Buff.bIsDebuff ? FLinearColor(1.f, 0.3f, 0.3f, 1.f) : FLinearColor::White);
				if (UOverlaySlot* IconSlot = IconOverlay->AddChildToOverlay(IconWidget))
				{
					IconSlot->SetHorizontalAlignment(HAlign_Fill);
					IconSlot->SetVerticalAlignment(VAlign_Fill);
				}
			}

			// 圆形冷却遮罩：当存在时间信息时，创建基于材质的遮罩并按照剩余时间百分比设置参数
			if (Buff.TimeRemaining >= 0.f && Buff.TotalDuration > KINDA_SMALL_NUMBER)
			{
				UImage* CooldownMask = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
				if (CooldownMask && BuffCooldownMaterial)
				{
					UMaterialInstanceDynamic* CooldownMID = UMaterialInstanceDynamic::Create(BuffCooldownMaterial, this);
					if (CooldownMID)
					{
						const float Percent = FMath::Clamp(Buff.TimeRemaining / Buff.TotalDuration, 0.f, 1.f);
						CooldownMID->SetScalarParameterValue(BuffCooldownPercentParam, Percent);
						CooldownMask->SetBrushFromMaterial(CooldownMID);
						CooldownMask->SetColorAndOpacity(BuffCooldownTint);
						CooldownMask->SetDesiredSizeOverride(BuffIconSize);
						if (UOverlaySlot* CooldownSlot = IconOverlay->AddChildToOverlay(CooldownMask))
						{
							CooldownSlot->SetHorizontalAlignment(HAlign_Fill);
							CooldownSlot->SetVerticalAlignment(VAlign_Fill);
						}
					}
				}
			}

			// 堆叠数徽章：当该 Buff 有多层叠加（StackCount>1）时，右下角显示数量
			if (Buff.StackCount > 1)
			{
				UTextBlock* StackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
				if (StackText)
				{
					StackText->SetText(FText::AsNumber(Buff.StackCount));
					StackText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
					StackText->SetShadowOffset(FVector2D(1.f, 1.f));
					StackText->SetJustification(ETextJustify::Right);
					if (UOverlaySlot* StackSlot = IconOverlay->AddChildToOverlay(StackText))
					{
						StackSlot->SetHorizontalAlignment(HAlign_Right);
						StackSlot->SetVerticalAlignment(VAlign_Bottom);
						StackSlot->SetPadding(FMargin(0.f, 0.f, 1.f, 1.f));
					}
				}
			}

			BuffStrip->AddChildToHorizontalBox(IconOverlay);
		}
		return;
	}

	if (CachedBuffTags.Num() == 0)
	{
		BuffStrip->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	BuffStrip->SetVisibility(ESlateVisibility::HitTestInvisible);

	for (const FName& TagName : CachedBuffTags)
	{
		if (!WidgetTree)
		{
			break;
		}

		UTextBlock* TagLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		TagLabel->SetText(FText::FromName(TagName));
		TagLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		TagLabel->SetShadowOffset(FVector2D(1.f, 1.f));
		BuffStrip->AddChildToHorizontalBox(TagLabel);
	}
}
