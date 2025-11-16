#include "UI/HUD/HUDActionBarWidget.h"

#include "UI/SkillLoadout/HUDSkillLoadoutWidget.h"
#include "UI/SkillLoadout/HUDSkillSlotWidget.h"

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

	if (SkillLoadout)
	{
		SkillLoadout->ResetSlots();
	}

	RegisterSlot(ESkillSlot::Skill_Q, Slot_Q);
	RegisterSlot(ESkillSlot::Skill_W, Slot_W);
	RegisterSlot(ESkillSlot::Skill_E, Slot_E);
	RegisterSlot(ESkillSlot::Skill_R, Slot_R);
	RegisterSlot(ESkillSlot::Skill_D, Slot_D);
	RegisterSlot(ESkillSlot::Skill_F, Slot_F);
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

UHUDSkillSlotWidget* UHUDActionBarWidget::FindSlot(ESkillSlot InSlot) const
{
	if (const TObjectPtr<UHUDSkillSlotWidget>* Found = SlotLookup.Find(InSlot))
	{
		return Found->Get();
	}
	return nullptr;
}

void UHUDActionBarWidget::RegisterSlot(ESkillSlot InSlot, UHUDSkillSlotWidget* SlotWidget)
{
	if (!SlotWidget)
	{
		return;
	}

	SlotLookup.Add(InSlot, SlotWidget);

	if (SkillLoadout)
	{
		SkillLoadout->RegisterSlot(SlotWidget);
	}
}
