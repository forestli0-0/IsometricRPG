#include "UI/CharacterStatsPanelWidget.h"

#include "Character/IsometricRPGAttributeSetBase.h"

void UCharacterStatsPanelWidget::InitializeWithAttributeSet(UIsometricRPGAttributeSetBase* InAttributeSet)
{
    if (BoundAttributeSet == InAttributeSet)
    {
        return;
    }

    ClearBindings();
    BoundAttributeSet = InAttributeSet;

    if (BoundAttributeSet)
    {
        BindDelegates();
        RebuildStats();
        RefreshStatList(CachedStats);
    }
}

void UCharacterStatsPanelWidget::ClearBindings()
{
    if (BoundAttributeSet)
    {
        BoundAttributeSet->OnHealthChanged.RemoveAll(this);
        BoundAttributeSet->OnManaChanged.RemoveAll(this);
        BoundAttributeSet->OnExperienceChanged.RemoveAll(this);
        BoundAttributeSet->OnLevelChanged.RemoveAll(this);
        BoundAttributeSet->OnUnUsedSkillPointChanged.RemoveAll(this);
    }

    BoundAttributeSet = nullptr;
    CachedStats.Reset();
    StatIndexLookup.Reset();
}

void UCharacterStatsPanelWidget::NativeDestruct()
{
    ClearBindings();
    Super::NativeDestruct();
}

void UCharacterStatsPanelWidget::RebuildStats()
{
    if (!BoundAttributeSet)
    {
        return;
    }

    CachedStats.Reset();
    StatIndexLookup.Reset();

    auto AddStat = [&](FName Name, float Current, float Max)
    {
        const int32 Index = CachedStats.Add({Name, Current, Max});
        StatIndexLookup.Add(Name, Index);
    };

    AddStat(TEXT("Health"), BoundAttributeSet->GetHealth(), BoundAttributeSet->GetMaxHealth());
    AddStat(TEXT("Mana"), BoundAttributeSet->GetMana(), BoundAttributeSet->GetMaxMana());
    AddStat(TEXT("AttackDamage"), BoundAttributeSet->GetAttackDamage(), BoundAttributeSet->GetAttackDamage());
    AddStat(TEXT("PhysicalDefense"), BoundAttributeSet->GetPhysicalDefense(), BoundAttributeSet->GetPhysicalDefense());
    AddStat(TEXT("MagicDefense"), BoundAttributeSet->GetMagicDefense(), BoundAttributeSet->GetMagicDefense());
    AddStat(TEXT("MoveSpeed"), BoundAttributeSet->GetMoveSpeed(), BoundAttributeSet->GetMoveSpeed());
    AddStat(TEXT("Experience"), BoundAttributeSet->GetExperience(), BoundAttributeSet->GetExperienceToNextLevel());
    AddStat(TEXT("Level"), BoundAttributeSet->GetLevel(), BoundAttributeSet->GetLevel());
    AddStat(TEXT("SkillPoints"), BoundAttributeSet->GetUnUsedSkillPoint(), BoundAttributeSet->GetTotalSkillPoint());
}

void UCharacterStatsPanelWidget::BindDelegates()
{
    if (!BoundAttributeSet)
    {
        return;
    }

    BoundAttributeSet->OnHealthChanged.AddDynamic(this, &UCharacterStatsPanelWidget::HandleHealthChanged);
    BoundAttributeSet->OnManaChanged.AddDynamic(this, &UCharacterStatsPanelWidget::HandleManaChanged);
    BoundAttributeSet->OnExperienceChanged.AddDynamic(this, &UCharacterStatsPanelWidget::HandleExperienceChanged);
    BoundAttributeSet->OnLevelChanged.AddDynamic(this, &UCharacterStatsPanelWidget::HandleLevelChanged);
    BoundAttributeSet->OnUnUsedSkillPointChanged.AddDynamic(this, &UCharacterStatsPanelWidget::HandleSkillPointChanged);
}

void UCharacterStatsPanelWidget::UpdateStatValue(FName StatName, float CurrentValue, float MaxValue, bool bBroadcast)
{
    int32* FoundIndex = StatIndexLookup.Find(StatName);
    if (!FoundIndex)
    {
        const int32 NewIndex = CachedStats.Add({StatName, CurrentValue, MaxValue});
        StatIndexLookup.Add(StatName, NewIndex);
        if (bBroadcast)
        {
            OnStatChanged(StatName, CachedStats[NewIndex]);
        }
        return;
    }

    const int32 Index = *FoundIndex;
    if (!CachedStats.IsValidIndex(Index))
    {
        return;
    }

    FCharacterStatView& Stat = CachedStats[Index];
    Stat.StatName = StatName;
    Stat.CurrentValue = CurrentValue;
    Stat.MaxValue = MaxValue;

    if (bBroadcast)
    {
        OnStatChanged(StatName, Stat);
    }
}

void UCharacterStatsPanelWidget::HandleHealthChanged(UIsometricRPGAttributeSetBase* AttributeSet, float NewValue)
{
    UpdateStatValue(TEXT("Health"), NewValue, AttributeSet->GetMaxHealth());
}

void UCharacterStatsPanelWidget::HandleManaChanged(UIsometricRPGAttributeSetBase* AttributeSet, float NewValue)
{
    UpdateStatValue(TEXT("Mana"), NewValue, AttributeSet->GetMaxMana());
}

void UCharacterStatsPanelWidget::HandleExperienceChanged(UIsometricRPGAttributeSetBase* AttributeSet, float NewValue, float NewMaxValue)
{
    UpdateStatValue(TEXT("Experience"), NewValue, NewMaxValue);
}

void UCharacterStatsPanelWidget::HandleLevelChanged(UIsometricRPGAttributeSetBase* AttributeSet, float NewLevel)
{
    UpdateStatValue(TEXT("Level"), NewLevel, NewLevel);
}

void UCharacterStatsPanelWidget::HandleSkillPointChanged(UIsometricRPGAttributeSetBase* AttributeSet, float NewValue)
{
    UpdateStatValue(TEXT("SkillPoints"), NewValue, AttributeSet->GetTotalSkillPoint());
}
