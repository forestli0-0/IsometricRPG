#include "UI/SkillLoadout/HUDSkillLoadoutWidget.h"

#include "UI/SkillLoadout/HUDSkillSlotWidget.h"

void UHUDSkillLoadoutWidget::NativeConstruct()
{
    Super::NativeConstruct();

    ResetSlots();
    RegisterDesignerSlots();
}

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

void UHUDSkillLoadoutWidget::RegisterDesignerSlots()
{
    const struct
    {
        TObjectPtr<UHUDSkillSlotWidget> SlotWidget;
        ESkillSlot SlotType;
    } SlotMap[] = {
        { Slot_Passive, ESkillSlot::Skill_Passive },
        { Slot_Q, ESkillSlot::Skill_Q },
        { Slot_W, ESkillSlot::Skill_W },
        { Slot_E, ESkillSlot::Skill_E },
        { Slot_R, ESkillSlot::Skill_R },
        { Slot_D, ESkillSlot::Skill_D },
        { Slot_F, ESkillSlot::Skill_F }
    };

    for (const auto& Entry : SlotMap)
    {
        if (Entry.SlotWidget)
        {
            Entry.SlotWidget->SetConfiguredSlot(Entry.SlotType);
            RegisterSlot(Entry.SlotWidget);
        }
    }
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
