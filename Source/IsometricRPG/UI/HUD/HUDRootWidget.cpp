#include "UI/HUD/HUDRootWidget.h"

#include "UI/HUD/HUDActionBarWidget.h"
#include "UI/HUD/HUDResourcePanelWidget.h"
#include "UI/HUD/HUDStatusPanelWidget.h"
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

void UHUDRootWidget::UpdateHealth(float CurrentHealth, float MaxHealth, float ShieldValue)
{
	if (StatusPanel)
	{
		StatusPanel->SetHealthValues(CurrentHealth, MaxHealth, ShieldValue);
	}
}

void UHUDRootWidget::UpdateStatusEffects(const TArray<FName>& TagNames)
{
	if (StatusPanel)
	{
		StatusPanel->SetStatusTags(TagNames);
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
	if (ResourcePanel)
	{
		ResourcePanel->SetResourceValues(CurrentPrimary, MaxPrimary, CurrentSecondary, MaxSecondary);
	}
}

void UHUDRootWidget::UpdateExperience(int32 CurrentLevel, float CurrentExperience, float RequiredExperience)
{
	if (ResourcePanel)
	{
		ResourcePanel->SetExperienceValues(CurrentLevel, CurrentExperience, RequiredExperience);
	}
}

void UHUDRootWidget::UpdateCurrencies(const TMap<FName, int32>& CurrencyValues)
{
	if (ResourcePanel)
	{
		ResourcePanel->SetCurrencies(CurrencyValues);
	}
}
