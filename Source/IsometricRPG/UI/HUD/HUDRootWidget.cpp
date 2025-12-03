#include "UI/HUD/HUDRootWidget.h"

#include "UI/HUD/HUDActionBarWidget.h"
#include "UI/HUD/HUDResourcePanelWidget.h"
#include "UI/HUD/HUDStatusPanelWidget.h"
#include "UI/HUD/HUDInventorySlotWidget.h"
#include "UI/SkillLoadout/HUDSkillSlotWidget.h"
#include "UObject/NameTypes.h"

void UHUDRootWidget::SetAbilitySlot(const FHUDSkillSlotViewModel& ViewModel)
{
	if (ActionBar)
	{
		ActionBar->SetSlot(ViewModel);
	}
}

void UHUDRootWidget::ClearAbilitySlot(ESkillSlot InSlot)
{
	if (ActionBar)
	{
		ActionBar->ClearSlot(InSlot);
	}
}

void UHUDRootWidget::ClearAllAbilitySlots()
{
	if (ActionBar)
	{
		ActionBar->ClearAllSlots();
	}
}

void UHUDRootWidget::UpdateAbilityCooldown(ESkillSlot InSlot, float Duration, float Remaining)
{
	if (ActionBar)
	{
		ActionBar->ApplyCooldown(InSlot, Duration, Remaining);
	}
}

void UHUDRootWidget::UpdateHealth(float CurrentHealth, float MaxHealth, float /*ShieldValue*/)
{
	CachedCurrentHealth = CurrentHealth;
	CachedMaxHealth = FMath::Max(MaxHealth, KINDA_SMALL_NUMBER);
	PushVitalStateToActionBar();
}

void UHUDRootWidget::UpdateChampionStats(const FHUDChampionStatsViewModel& Stats)
{
	if (StatusPanel)
	{
		StatusPanel->SetChampionStats(Stats);
	}
}

void UHUDRootWidget::UpdateStatusEffects(const TArray<FName>& TagNames)
{
	if (ActionBar)
	{
		ActionBar->SetStatusTags(TagNames);
	}
}

void UHUDRootWidget::UpdateStatusBuffs(const TArray<FHUDBuffIconViewModel>& Buffs)
{
	if (ActionBar)
	{
		ActionBar->SetStatusBuffs(Buffs);
	}
}

void UHUDRootWidget::UpdatePortrait(UTexture2D* PortraitTexture, bool bIsInCombat, bool bHasLevelUp)
{
	if (StatusPanel)
	{
		StatusPanel->SetPortraitData(PortraitTexture, bIsInCombat, bHasLevelUp);
	}
}

void UHUDRootWidget::UpdateResources(float CurrentPrimary, float MaxPrimary, float CurrentSecondary, float MaxSecondary)
{
	CachedCurrentMana = CurrentPrimary;
	CachedMaxMana = FMath::Max(MaxPrimary, KINDA_SMALL_NUMBER);
	PushVitalStateToActionBar();
}

void UHUDRootWidget::UpdateExperience(int32 CurrentLevel, float CurrentExperience, float RequiredExperience)
{
	if (ActionBar)
	{
		ActionBar->SetExperience(CurrentLevel, CurrentExperience, RequiredExperience);
	}
}

void UHUDRootWidget::UpdateItemSlots(const TArray<FHUDItemSlotViewModel>& Slots)
{
	if (ResourcePanel)
	{
		ResourcePanel->SetItemSlots(Slots);
	}
}

void UHUDRootWidget::UpdateUtilityButtons(const TArray<FHUDItemSlotViewModel>& Buttons)
{
	if (ResourcePanel)
	{
		ResourcePanel->SetUtilityButtons(Buttons);
	}
}

void UHUDRootWidget::PushVitalStateToActionBar()
{
	if (ActionBar)
	{
		ActionBar->SetVitals(CachedCurrentHealth, CachedMaxHealth, CachedCurrentMana, CachedMaxMana);
	}
}
