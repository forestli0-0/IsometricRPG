#include "UI/SkillLoadout/HUDSkillLoadoutWidget.h"

#include "UI/SkillLoadout/HUDSkillSlotWidget.h"

void UHUDSkillLoadoutWidget::RegisterSlot(UHUDSkillSlotWidget* InSlot)
{
    if (!InSlot)
    {
        return;
    }

    SkillSlots.AddUnique(InSlot);
}

void UHUDSkillLoadoutWidget::ResetSlots()
{
    SkillSlots.Reset();
}

void UHUDSkillLoadoutWidget::AssignSlot(int32 Index, const FHUDSkillSlotViewModel& ViewModel)
{
    if (!SkillSlots.IsValidIndex(Index) || !SkillSlots[Index])
    {
        return;
    }

    SkillSlots[Index]->SetSlotData(ViewModel);
}

void UHUDSkillLoadoutWidget::ClearSlot(int32 Index)
{
    if (!SkillSlots.IsValidIndex(Index) || !SkillSlots[Index])
    {
        return;
    }

    SkillSlots[Index]->ClearSlot();
}
