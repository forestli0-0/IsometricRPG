#include "UI/HUD/HUDPresentationBuilder.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/Texture2D.h"
#include "IsometricAbilities/GameplayAbilities/GA_HeroBaseAbility.h"
#include "IsometricRPG/Character/IsometricRPGAttributeSetBase.h"
#include "UI/HUD/HUDRootWidget.h"
#include "UI/HUD/HUDViewModelTypes.h"
#include "UI/SkillLoadout/HUDSkillSlotWidget.h"

namespace
{
constexpr ESkillSlot DisplayableSlots[] = {
    ESkillSlot::Skill_Passive,
    ESkillSlot::Skill_Q,
    ESkillSlot::Skill_W,
    ESkillSlot::Skill_E,
    ESkillSlot::Skill_R,
    ESkillSlot::Skill_D,
    ESkillSlot::Skill_F
};

bool ShouldDisplaySlot(const ESkillSlot Slot)
{
    switch (Slot)
    {
    case ESkillSlot::Skill_Passive:
    case ESkillSlot::Skill_Q:
    case ESkillSlot::Skill_W:
    case ESkillSlot::Skill_E:
    case ESkillSlot::Skill_R:
    case ESkillSlot::Skill_D:
    case ESkillSlot::Skill_F:
        return true;
    default:
        return false;
    }
}

FText BuildHotkeyLabel(const ESkillSlot Slot)
{
    switch (Slot)
    {
    case ESkillSlot::Skill_Passive: return FText::FromString(TEXT("Passive"));
    case ESkillSlot::Skill_Q: return FText::FromString(TEXT("Q"));
    case ESkillSlot::Skill_W: return FText::FromString(TEXT("W"));
    case ESkillSlot::Skill_E: return FText::FromString(TEXT("E"));
    case ESkillSlot::Skill_R: return FText::FromString(TEXT("R"));
    case ESkillSlot::Skill_D: return FText::FromString(TEXT("D"));
    case ESkillSlot::Skill_F: return FText::FromString(TEXT("F"));
    default: return FText::GetEmpty();
    }
}

FHUDSkillSlotViewModel BuildEmptySlotViewModel(const ESkillSlot Slot)
{
    FHUDSkillSlotViewModel ViewModel;
    ViewModel.Slot = Slot;
    ViewModel.bIsUnlocked = ShouldDisplaySlot(Slot);
    ViewModel.bIsEquipped = false;
    ViewModel.DisplayName = FText::GetEmpty();
    ViewModel.HotkeyLabel = BuildHotkeyLabel(Slot);
    ViewModel.Icon = nullptr;
    ViewModel.ResourceCost = 0.f;
    ViewModel.CooldownDuration = 0.f;
    ViewModel.CooldownRemaining = 0.f;
    return ViewModel;
}

FHUDSkillSlotViewModel BuildSlotViewModel(
    const FEquippedAbilityInfo& Info,
    TFunctionRef<bool(const UGameplayAbility*, float&, float&)> QueryCooldownState)
{
    FHUDSkillSlotViewModel ViewModel = BuildEmptySlotViewModel(Info.Slot);
    if (!ShouldDisplaySlot(Info.Slot))
    {
        return ViewModel;
    }

    ViewModel.bIsUnlocked = true;

    const bool bHasAbilitySoftRef = !Info.AbilityClass.ToSoftObjectPath().IsNull();
    UClass* AbilityClass = Info.AbilityClass.Get();
    ViewModel.AbilityClass = AbilityClass;
    ViewModel.bIsEquipped = bHasAbilitySoftRef;
    ViewModel.Icon = Info.Icon.Get();

    if (!AbilityClass)
    {
        return ViewModel;
    }

    const UGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<UGameplayAbility>();
    if (!AbilityCDO)
    {
        return ViewModel;
    }

    if (const UGA_HeroBaseAbility* HeroCDO = Cast<UGA_HeroBaseAbility>(AbilityCDO))
    {
        ViewModel.DisplayName = HeroCDO->GetAbilityDisplayNameText();
        ViewModel.ResourceCost = HeroCDO->GetResourceCost();
        ViewModel.CooldownDuration = HeroCDO->CooldownDuration;
    }
    else
    {
        ViewModel.DisplayName = FText::FromName(AbilityClass->GetFName());
    }

    if (ViewModel.DisplayName.IsEmpty())
    {
        ViewModel.DisplayName = FText::FromName(AbilityClass->GetFName());
    }

    QueryCooldownState(AbilityCDO, ViewModel.CooldownDuration, ViewModel.CooldownRemaining);

    const FGameplayTagContainer AssetTags = AbilityCDO->GetAssetTags();
    if (!AssetTags.IsEmpty())
    {
        TArray<FGameplayTag> LocalTags;
        AssetTags.GetGameplayTagArray(LocalTags);
        if (LocalTags.Num() > 0)
        {
            ViewModel.AbilityPrimaryTag = LocalTags[0];
        }
    }

    return ViewModel;
}

TArray<FHUDItemSlotViewModel> BuildUtilityButtonViewModels()
{
    static const TCHAR* DefaultHotkeys[] = { TEXT("4"), TEXT("5"), TEXT("6") };

    TArray<FHUDItemSlotViewModel> Buttons;
    Buttons.SetNum(UE_ARRAY_COUNT(DefaultHotkeys));
    for (int32 Index = 0; Index < Buttons.Num(); ++Index)
    {
        Buttons[Index].HotkeyLabel = FText::FromString(DefaultHotkeys[Index]);
    }

    return Buttons;
}

TArray<FHUDBuffIconViewModel> BuildBuffViewModels(
    const FGameplayTagContainer& OwnedTags,
    const UAbilitySystemComponent* AbilitySystemComponent,
    const TMap<FGameplayTag, TSoftObjectPtr<UTexture2D>>& BuffIconMap,
    UWorld* World)
{
    TArray<FHUDBuffIconViewModel> Result;
    if (BuffIconMap.Num() == 0)
    {
        return Result;
    }

    const float WorldTime = World ? World->GetTimeSeconds() : 0.f;

    for (const TPair<FGameplayTag, TSoftObjectPtr<UTexture2D>>& Pair : BuffIconMap)
    {
        const FGameplayTag& DisplayTag = Pair.Key;
        if (!DisplayTag.IsValid() || !OwnedTags.HasTag(DisplayTag))
        {
            continue;
        }

        UTexture2D* IconTexture = Pair.Value.Get();
        if (!IconTexture)
        {
            IconTexture = Pair.Value.LoadSynchronous();
            if (!IconTexture)
            {
                UE_LOG(LogTemp, Verbose, TEXT("[BuffIcons] Texture still null for tag %s"), *DisplayTag.ToString());
                continue;
            }
        }

        int32 MaxStackCount = 0;
        float BestTimeRemaining = -1.f;
        float BestTotalDuration = -1.f;

        if (AbilitySystemComponent)
        {
            FGameplayTagContainer QueryTags;
            QueryTags.AddTag(DisplayTag);
            const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(QueryTags);
            const TArray<FActiveGameplayEffectHandle> MatchingEffects = AbilitySystemComponent->GetActiveEffects(Query);

            for (const FActiveGameplayEffectHandle& Handle : MatchingEffects)
            {
                if (const FActiveGameplayEffect* ActiveEffect = AbilitySystemComponent->GetActiveGameplayEffect(Handle))
                {
                    MaxStackCount = FMath::Max(MaxStackCount, ActiveEffect->Spec.GetStackCount());

                    const float Duration = ActiveEffect->GetDuration();
                    const float Remaining = ActiveEffect->GetTimeRemaining(WorldTime);
                    if (Duration > KINDA_SMALL_NUMBER && Remaining >= 0.f && Remaining > BestTimeRemaining)
                    {
                        BestTimeRemaining = Remaining;
                        BestTotalDuration = Duration;
                    }
                }
            }
        }

        FHUDBuffIconViewModel ViewModel;
        ViewModel.TagName = DisplayTag.GetTagName();
        ViewModel.Icon = IconTexture;
        ViewModel.StackCount = MaxStackCount;
        ViewModel.bIsDebuff = false;
        ViewModel.TimeRemaining = BestTimeRemaining;
        ViewModel.TotalDuration = BestTotalDuration;
        Result.Add(ViewModel);
    }

    return Result;
}
}

void FHUDPresentationBuilder::RefreshActionBar(
    UHUDRootWidget& HUD,
    const TArray<FEquippedAbilityInfo>& EquippedAbilities,
    TFunctionRef<bool(const UGameplayAbility*, float&, float&)> QueryCooldownState)
{
    HUD.ClearAllAbilitySlots();

    TMap<ESkillSlot, const FEquippedAbilityInfo*> SlotMap;
    for (const FEquippedAbilityInfo& Info : EquippedAbilities)
    {
        SlotMap.Add(Info.Slot, &Info);
    }

    for (const ESkillSlot Slot : DisplayableSlots)
    {
        const FEquippedAbilityInfo* const* FoundInfo = SlotMap.Find(Slot);
        const FHUDSkillSlotViewModel ViewModel = FoundInfo
            ? BuildSlotViewModel(**FoundInfo, QueryCooldownState)
            : BuildEmptySlotViewModel(Slot);
        HUD.SetAbilitySlot(ViewModel);
    }
}

void FHUDPresentationBuilder::RefreshVitals(UHUDRootWidget& HUD, const UIsometricRPGAttributeSetBase* AttributeSet)
{
    if (!AttributeSet)
    {
        return;
    }

    HUD.UpdateHealth(AttributeSet->GetHealth(), AttributeSet->GetMaxHealth(), 0.f);
    HUD.UpdateResources(AttributeSet->GetMana(), AttributeSet->GetMaxMana(), 0.f, 0.f);
}

void FHUDPresentationBuilder::RefreshChampionStats(UHUDRootWidget& HUD, const UIsometricRPGAttributeSetBase* AttributeSet)
{
    if (!AttributeSet)
    {
        return;
    }

    FHUDChampionStatsViewModel ChampionStats;
    ChampionStats.AttackDamage = AttributeSet->GetAttackDamage();
    ChampionStats.AbilityPower = AttributeSet->GetAbilityPower();
    ChampionStats.Armor = AttributeSet->GetPhysicalDefense();
    ChampionStats.MagicResist = AttributeSet->GetMagicDefense();
    ChampionStats.AttackSpeed = AttributeSet->GetAttackSpeed();
    ChampionStats.CritChance = AttributeSet->GetCriticalChance();
    ChampionStats.MoveSpeed = AttributeSet->GetMoveSpeed();
    HUD.UpdateChampionStats(ChampionStats);
}

void FHUDPresentationBuilder::RefreshGameplayTagPresentation(UHUDRootWidget& HUD, const FHUDPresentationContext& Context)
{
    FGameplayTagContainer OwnedTags;
    if (Context.AbilitySystemComponent)
    {
        Context.AbilitySystemComponent->GetOwnedGameplayTags(OwnedTags);
    }

    static const TMap<FGameplayTag, TSoftObjectPtr<UTexture2D>> EmptyBuffIconMap;
    const TMap<FGameplayTag, TSoftObjectPtr<UTexture2D>>& BuffIconMap = Context.BuffIconMap
        ? *Context.BuffIconMap
        : EmptyBuffIconMap;
    HUD.UpdateStatusBuffs(BuildBuffViewModels(OwnedTags, Context.AbilitySystemComponent, BuffIconMap, Context.World));

    TArray<FName> TagNameArray;
    if (Context.bShowOwnedTagsOnHUD)
    {
        TArray<FGameplayTag> OwnedTagArray;
        OwnedTags.GetGameplayTagArray(OwnedTagArray);
        TagNameArray.Reserve(OwnedTagArray.Num());
        for (const FGameplayTag& Tag : OwnedTagArray)
        {
            TagNameArray.Add(Tag.GetTagName());
        }
    }
    HUD.UpdateStatusEffects(TagNameArray);

    bool bIsInCombat = false;
    if (Context.AbilitySystemComponent)
    {
        static const FGameplayTag CombatTag = FGameplayTag::RequestGameplayTag(FName("State.Combat"), false);
        if (CombatTag.IsValid())
        {
            bIsInCombat = OwnedTags.HasTagExact(CombatTag);
        }
    }

    const bool bHasPendingLevelUp = Context.AttributeSet && Context.AttributeSet->GetUnUsedSkillPoint() > 0.f;
    HUD.UpdatePortrait(nullptr, bIsInCombat, bHasPendingLevelUp);
}

void FHUDPresentationBuilder::RefreshExperience(UHUDRootWidget& HUD, const UIsometricRPGAttributeSetBase* AttributeSet)
{
    if (!AttributeSet)
    {
        return;
    }

    HUD.UpdateExperience(
        FMath::FloorToInt(AttributeSet->GetLevel()),
        AttributeSet->GetExperience(),
        AttributeSet->GetExperienceToNextLevel());
}

void FHUDPresentationBuilder::RefreshUtilityButtons(UHUDRootWidget& HUD)
{
    HUD.UpdateUtilityButtons(BuildUtilityButtonViewModels());
}

void FHUDPresentationBuilder::RefreshEntireHUD(
    UHUDRootWidget& HUD,
    const FHUDPresentationContext& Context,
    TFunctionRef<bool(const UGameplayAbility*, float&, float&)> QueryCooldownState)
{
    static const TArray<FEquippedAbilityInfo> EmptyEquippedAbilities;
    const TArray<FEquippedAbilityInfo>& EquippedAbilities = Context.EquippedAbilities
        ? *Context.EquippedAbilities
        : EmptyEquippedAbilities;

    RefreshActionBar(HUD, EquippedAbilities, QueryCooldownState);
    RefreshVitals(HUD, Context.AttributeSet);
    RefreshChampionStats(HUD, Context.AttributeSet);
    RefreshGameplayTagPresentation(HUD, Context);
    RefreshExperience(HUD, Context.AttributeSet);
    RefreshUtilityButtons(HUD);
}
