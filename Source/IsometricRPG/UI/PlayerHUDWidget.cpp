// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/PlayerHUDWidget.h"

#include "Character/IsoPlayerState.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "Inventory/InventoryComponent.h"
#include "SkillTree/SkillTreeComponent.h"
#include "UI/CharacterStatsPanelWidget.h"
#include "UI/InventoryPanelWidget.h"
#include "UI/SettingsMenuWidget.h"
#include "UI/SkillTreeWidget.h"

void UPlayerHUDWidget::InitializePanels(AIsoPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return;
	}

	if (InventoryPanel)
	{
		if (UInventoryComponent* Inventory = PlayerState->GetInventoryComponent())
		{
			InventoryPanel->BindToInventory(Inventory);
		}
	}

	if (CharacterStatsPanel)
	{
		if (UIsometricRPGAttributeSetBase* AttributeSet = PlayerState->GetAttributeSet())
		{
			CharacterStatsPanel->InitializeWithAttributeSet(AttributeSet);
		}
	}

	if (SkillTreePanel)
	{
		if (USkillTreeComponent* SkillTree = PlayerState->GetSkillTreeComponent())
		{
			SkillTreePanel->BindSkillTree(SkillTree);
		}
	}

	if (SettingsMenu)
	{
		SettingsMenu->OnSettingsApplied();
	}
}

