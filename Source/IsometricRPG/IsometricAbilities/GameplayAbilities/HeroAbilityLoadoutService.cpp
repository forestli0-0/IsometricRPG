#include "IsometricAbilities/GameplayAbilities/HeroAbilityLoadoutService.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "IsometricAbilities/Types/HeroAbilityTypes.h"

int32 FHeroAbilityLoadoutService::GetSkillBarSlotCount()
{
    return 7;
}

int32 FHeroAbilityLoadoutService::IndexFromSlot(const ESkillSlot Slot)
{
    switch (Slot)
    {
    case ESkillSlot::Skill_Passive: return 0;
    case ESkillSlot::Skill_Q: return 1;
    case ESkillSlot::Skill_W: return 2;
    case ESkillSlot::Skill_E: return 3;
    case ESkillSlot::Skill_R: return 4;
    case ESkillSlot::Skill_D: return 5;
    case ESkillSlot::Skill_F: return 6;
    default: return INDEX_NONE;
    }
}

bool FHeroAbilityLoadoutService::EquipAbilityToSlot(
    UAbilitySystemComponent& AbilitySystemComponent,
    UObject* SourceObject,
    TArray<FEquippedAbilityInfo>& EquippedAbilities,
    TSubclassOf<UGameplayAbility> NewAbilityClass,
    const ESkillSlot Slot)
{
    if (!NewAbilityClass || Slot == ESkillSlot::Invalid || Slot == ESkillSlot::MAX)
    {
        return false;
    }

    const int32 Index = IndexFromSlot(Slot);
    if (Index == INDEX_NONE)
    {
        return false;
    }

    if (!EquippedAbilities.IsValidIndex(Index))
    {
        EquippedAbilities.SetNum(GetSkillBarSlotCount());
    }

    FEquippedAbilityInfo& EquippedInfo = EquippedAbilities[Index];
    if (IsSameAbilityClass(EquippedInfo, NewAbilityClass) && EquippedInfo.AbilitySpecHandle.IsValid())
    {
        return false;
    }

    EquippedInfo.AbilityClass = NewAbilityClass;
    EquippedInfo.Slot = Slot;
    GrantAbility(AbilitySystemComponent, SourceObject, EquippedInfo, true);
    return true;
}

bool FHeroAbilityLoadoutService::UnequipAbilityFromSlot(
    UAbilitySystemComponent& AbilitySystemComponent,
    TArray<FEquippedAbilityInfo>& EquippedAbilities,
    const ESkillSlot Slot)
{
    const int32 Index = IndexFromSlot(Slot);
    if (!EquippedAbilities.IsValidIndex(Index))
    {
        return false;
    }

    FEquippedAbilityInfo& EquippedInfo = EquippedAbilities[Index];
    if (EquippedInfo.AbilitySpecHandle.IsValid())
    {
        ClearAbility(AbilitySystemComponent, EquippedInfo);
    }

    EquippedInfo = FEquippedAbilityInfo();
    return true;
}

void FHeroAbilityLoadoutService::InitializeDefaultAbilities(
    UAbilitySystemComponent& AbilitySystemComponent,
    UObject* SourceObject,
    const TArray<FEquippedAbilityInfo>& DefaultAbilities,
    TArray<FEquippedAbilityInfo>& EquippedAbilities)
{
    EquippedAbilities.Empty();
    EquippedAbilities.SetNum(GetSkillBarSlotCount());

    for (const FEquippedAbilityInfo& DefaultAbilityInfo : DefaultAbilities)
    {
        if (DefaultAbilityInfo.AbilityClass.ToSoftObjectPath().IsNull())
        {
            continue;
        }

        if (DefaultAbilityInfo.Slot == ESkillSlot::MAX)
        {
            continue;
        }

        UE_LOG(
            LogTemp,
            Log,
            TEXT("[InitAbilities] Granting default ability %s to Slot %d"),
            *DefaultAbilityInfo.AbilityClass.ToString(),
            static_cast<int32>(DefaultAbilityInfo.Slot));

        const int32 Index = IndexFromSlot(DefaultAbilityInfo.Slot);
        if (Index == INDEX_NONE)
        {
            FEquippedAbilityInfo TransientEquippedInfo = DefaultAbilityInfo;
            GrantAbility(AbilitySystemComponent, SourceObject, TransientEquippedInfo, true);
            continue;
        }

        FEquippedAbilityInfo& EquippedInfo = EquippedAbilities[Index];
        EquippedInfo = DefaultAbilityInfo;
        GrantAbility(AbilitySystemComponent, SourceObject, EquippedInfo, true);
    }
}

int32 FHeroAbilityLoadoutService::ResolveInputID(const ESkillSlot Slot)
{
    switch (Slot)
    {
    case ESkillSlot::Skill_Q: return static_cast<int32>(EAbilityInputID::Ability_Q);
    case ESkillSlot::Skill_W: return static_cast<int32>(EAbilityInputID::Ability_W);
    case ESkillSlot::Skill_E: return static_cast<int32>(EAbilityInputID::Ability_E);
    case ESkillSlot::Skill_R: return static_cast<int32>(EAbilityInputID::Ability_R);
    case ESkillSlot::Skill_D: return static_cast<int32>(EAbilityInputID::Ability_Summoner1);
    case ESkillSlot::Skill_F: return static_cast<int32>(EAbilityInputID::Ability_Summoner2);
    default: return -1;
    }
}

bool FHeroAbilityLoadoutService::IsSameAbilityClass(
    const FEquippedAbilityInfo& EquippedInfo,
    TSubclassOf<UGameplayAbility> AbilityClass)
{
    return TSoftClassPtr<UGameplayAbility>(EquippedInfo.AbilityClass).ToSoftObjectPath()
        == TSoftClassPtr<UGameplayAbility>(AbilityClass).ToSoftObjectPath();
}

void FHeroAbilityLoadoutService::GrantAbility(
    UAbilitySystemComponent& AbilitySystemComponent,
    UObject* SourceObject,
    FEquippedAbilityInfo& EquippedInfo,
    const bool bRemoveExistingFirst)
{
    if (bRemoveExistingFirst && EquippedInfo.AbilitySpecHandle.IsValid())
    {
        ClearAbility(AbilitySystemComponent, EquippedInfo);
    }

    if (EquippedInfo.AbilityClass.ToSoftObjectPath().IsNull())
    {
        return;
    }

    TSubclassOf<UGameplayAbility> LoadedAbilityClass = EquippedInfo.AbilityClass.LoadSynchronous();
    if (!LoadedAbilityClass || EquippedInfo.AbilitySpecHandle.IsValid())
    {
        return;
    }

    const int32 InputID = ResolveInputID(EquippedInfo.Slot);
    FGameplayAbilitySpec Spec(LoadedAbilityClass, 1, InputID, SourceObject);
    EquippedInfo.AbilitySpecHandle = AbilitySystemComponent.GiveAbility(Spec);

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[AbilityGrant] Granted %s -> HandleValid=%d Slot=%d InputID=%d"),
        *GetNameSafe(LoadedAbilityClass),
        EquippedInfo.AbilitySpecHandle.IsValid(),
        static_cast<int32>(EquippedInfo.Slot),
        InputID);
}

void FHeroAbilityLoadoutService::ClearAbility(
    UAbilitySystemComponent& AbilitySystemComponent,
    FEquippedAbilityInfo& EquippedInfo)
{
    if (!EquippedInfo.AbilitySpecHandle.IsValid())
    {
        return;
    }

    AbilitySystemComponent.ClearAbility(EquippedInfo.AbilitySpecHandle);
    EquippedInfo.AbilitySpecHandle = FGameplayAbilitySpecHandle();
}
