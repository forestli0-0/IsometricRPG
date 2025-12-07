// Fill out your copyright notice in the Description page of Project Settings.

#include "IsoPlayerState.h"
#include "AbilitySystemComponent.h"
#include "IsometricRPG/Character/IsometricRPGAttributeSetBase.h"
#include "IsometricRPG/Player/IsometricPlayerController.h"
#include "IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpec.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Abilities/GameplayAbility.h"
#include "TimerManager.h"
#include "Engine/AssetManager.h"
#include "IsometricAbilities/GameplayAbilities/GA_HeroBaseAbility.h"
#include "IsometricRPGCharacter.h"
#include "UI/HUD/HUDRootWidget.h"
#include "UI/HUD/HUDViewModelTypes.h"
#include "UI/SkillLoadout/HUDSkillSlotWidget.h"
AIsoPlayerState::AIsoPlayerState()
{
    // 创建ASC
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed); // 推荐使用Mixed模式

    // 创建AttributeSet
    AttributeSet = CreateDefaultSubobject<UIsometricRPGAttributeSetBase>("AttributeSet");

    // 网络更新频率设置，可以根据需要调整
    SetNetUpdateFrequency(100.0f);

}

void AIsoPlayerState::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        InitializeAttributes(); // 先初始化属性
        InitAbilities(); // 再初始化技能
    }
    if (AbilitySystemComponent)
    {
        UE_LOG(LogTemp, Log, TEXT("UIsometricInputComponent: OwnerCharacter and OwnerASC cached in BeginPlay."));
        // 绑定输入
        auto CachedPlayerController = Cast<APlayerController>(GetOwningController());
        if (!CachedPlayerController)
        {
            UE_LOG(LogTemp, Warning, TEXT("UIsometricInputComponent: CachedPlayerController is null in BeginPlay. Input binding might fail or be delayed."));
            return;
        }
        UInputComponent* PCInputComponent = CachedPlayerController->InputComponent;
        if (!PCInputComponent)
        {
            UE_LOG(LogTemp, Warning, TEXT("UIsometricInputComponent: PlayerController's InputComponent is null in BeginPlay. GAS Input Binding cannot occur yet."));
            return;
        }
        // 获取 UEnum*
        UEnum* EnumBinds = StaticEnum<EAbilityInputID>();
        if (!EnumBinds)
        {
            UE_LOG(LogTemp, Error, TEXT("UIsometricInputComponent: StaticEnum<EAbilityInputID>() failed. Ensure EAbilityInputID is correctly defined in a header and UHT has run."));
            return;
        }
        FTopLevelAssetPath EnumPath = FTopLevelAssetPath(TEXT("/Script/IsometricRPG.EAbilityInputID"));
        FString ConfirmCommand = TEXT("");
        FString CancelCommand = TEXT("");
        FGameplayAbilityInputBinds BindInfo(
            ConfirmCommand,      // 项目输入设置中用于“确认”的 Action Mapping 名称
            CancelCommand,       // 项目输入设置中用于“取消”的 Action Mapping 名称
            EnumPath,
            (int32)EAbilityInputID::Confirm, // 对应枚举中的 Confirm
            (int32)EAbilityInputID::Cancel   // 对应枚举中的 Cancel
        );
        AbilitySystemComponent->BindAbilityActivationToInputComponent(CachedPlayerController->InputComponent, BindInfo);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UIsometricInputComponent owned by non-AIsometricRPGCharacter or null owner."));
    }
}

void AIsoPlayerState::InitializeAttributes()
{
    if (!AbilitySystemComponent || !AttributeSet) return;
    if (DefaultAttributesEffect)
    {
        FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
        EffectContext.AddSourceObject(this);
        FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributesEffect, 1, EffectContext);
        if (SpecHandle.IsValid())
        {
            AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
        }
    }
}

UAbilitySystemComponent* AIsoPlayerState::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

int32 AIsoPlayerState::GetSkillBarSlotCount() const
{
    // 显示在技能栏的真实槽位：Passive, Q, W, E, R, D, F -> 7 个
    return 8;
}

int32 AIsoPlayerState::IndexFromSlot(ESkillSlot Slot) const
{
    // 将枚举槽位转换为 0 基索引，仅对真实技能栏槽位有效
    switch (Slot)
    {
    case ESkillSlot::Skill_Passive: return 0;
    case ESkillSlot::Skill_Q:       return 1;
    case ESkillSlot::Skill_W:       return 2;
    case ESkillSlot::Skill_E:       return 3;
    case ESkillSlot::Skill_R:       return 4;
    case ESkillSlot::Skill_D:       return 5;
    case ESkillSlot::Skill_F:       return 6;
    default:                        return INDEX_NONE; // Invalid 或 MAX
    }
}

FEquippedAbilityInfo AIsoPlayerState::GetEquippedAbilityInfo(ESkillSlot Slot) const
{
    const int32 Index = IndexFromSlot(Slot);
    if (EquippedAbilities.IsValidIndex(Index))
        return EquippedAbilities[Index];
    return FEquippedAbilityInfo();
}

void AIsoPlayerState::Server_EquipAbilityToSlot_Implementation(TSubclassOf<UGameplayAbility> NewAbilityClass, ESkillSlot Slot)
{
    if (!NewAbilityClass || Slot == ESkillSlot::Invalid || Slot == ESkillSlot::MAX)
        return;
    const int32 Index = IndexFromSlot(Slot);
    if (Index == INDEX_NONE) return;
    if (!EquippedAbilities.IsValidIndex(Index))
        EquippedAbilities.SetNum(GetSkillBarSlotCount());
    FEquippedAbilityInfo& Info = EquippedAbilities[Index];
    Info.AbilityClass = NewAbilityClass;
    Info.Slot = Slot;
    GrantAbilityInternal(Info, true);
    //FTimerHandle InitialDelayHandle;
    //GetWorld()->GetTimerManager().SetTimer(InitialDelayHandle, this, &AIsoPlayerState::OnRep_EquippedAbilities, 3.0f, false);
    //OnRep_EquippedAbilities();
}

void AIsoPlayerState::Server_UnequipAbilityFromSlot_Implementation(ESkillSlot Slot)
{
    const int32 Index = IndexFromSlot(Slot);
    if (!EquippedAbilities.IsValidIndex(Index))
        return;
    FEquippedAbilityInfo& Info = EquippedAbilities[Index];
    ClearAbilityInternal(Info);
    Info = FEquippedAbilityInfo();
    OnRep_EquippedAbilities();
}

void AIsoPlayerState::OnRep_EquippedAbilities()
{
    // 可以在这里广播事件或刷新 UI
    // 授予技能后，更新UI
        // 1. 收集所有需要加载的技能类的路径
    TArray<FSoftObjectPath> PathsToLoad;
    for (const FEquippedAbilityInfo& Info : EquippedAbilities)
    {
        // TSoftClassPtr.ToSoftObjectPath() 获取其资源路径
        if (!Info.AbilityClass.ToSoftObjectPath().IsNull())
        {
            PathsToLoad.Add(Info.AbilityClass.ToSoftObjectPath());
        }
        // 添加图标的路径
        if (!Info.Icon.IsNull())
        {
            PathsToLoad.Add(Info.Icon.ToSoftObjectPath());
        }
    }
    // （可选）可以在这里先将所有UI槽位设置成“加载中”的状态
    //AIsometricPlayerController* PC = Cast<AIsometricPlayerController>(GetPlayerController());
    //if (PC && PC->PlayerHUDInstance && PC->PlayerHUDInstance->SkillBar)
    //{
    //    PC->PlayerHUDInstance->SkillBar->SetSlotsToLoadingState(); // 假设你有这样一个函数
    //}

    if (PathsToLoad.Num() > 0)
    {
        // 2. 请求异步加载这些类资源
        // 使用UAssetManager单例来获取StreamableManager
        UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
            PathsToLoad,
            FStreamableDelegate::CreateUObject(this, &AIsoPlayerState::OnAssetsLoadedForUI) // 绑定一个新的回调函数
        );
    }
    else
    {
        // 如果没有需要加载的技能，直接用空数据更新UI
        OnAssetsLoadedForUI();
    }

}

void AIsoPlayerState::GrantAbilityInternal(FEquippedAbilityInfo& Info, bool bRemoveExistingFirst)
{
    if (GetLocalRole() != ROLE_Authority || !AbilitySystemComponent)
        return;
    if (bRemoveExistingFirst && Info.AbilitySpecHandle.IsValid())
        ClearAbilityInternal(Info);

    // 关键：判断是否存在资源路径，而不是是否已加载对象
    if (Info.AbilityClass.ToSoftObjectPath().IsNull())
        return;

    // 从软引用加载类。因为这是在服务器上授权技能，通常可以接受同步加载。
    TSubclassOf<UGameplayAbility> LoadedAbilityClass = Info.AbilityClass.LoadSynchronous();
    if (!LoadedAbilityClass) return;

    if (!Info.AbilitySpecHandle.IsValid())
    {
        // 创建技能规格(Spec)时，将 InputID 硬编码为 -1。
        // -1 是一个特殊值，代表“没有绑定到任何直接输入”。
        FGameplayAbilitySpec Spec(LoadedAbilityClass, 1, -1, this);
        Info.AbilitySpecHandle = AbilitySystemComponent->GiveAbility(Spec);
        UE_LOG(LogTemp, Log, TEXT("[PassiveDebug][Grant] Granted %s -> HandleValid=%d Slot=%d"),
            *GetNameSafe(LoadedAbilityClass), Info.AbilitySpecHandle.IsValid(), static_cast<int32>(Info.Slot));
    }
}

void AIsoPlayerState::ClearAbilityInternal(FEquippedAbilityInfo& Info)
{
    if (AbilitySystemComponent && Info.AbilitySpecHandle.IsValid())
    {
        AbilitySystemComponent->ClearAbility(Info.AbilitySpecHandle);
        Info.AbilitySpecHandle = FGameplayAbilitySpecHandle();
    }
}

void AIsoPlayerState::InitAbilities()
{
    if (GetLocalRole() != ROLE_Authority || !AbilitySystemComponent || DefaultAbilities.Num() == 0 || bAbilitiesInitialized)
        return;
    EquippedAbilities.Empty();
    EquippedAbilities.SetNum(GetSkillBarSlotCount());

    for (const FEquippedAbilityInfo& AbilityInfo : DefaultAbilities)
    {
        auto a = AbilityInfo;
        // 使用软路径判空，避免未加载时被跳过
        if (AbilityInfo.AbilityClass.ToSoftObjectPath().IsNull())
            continue;
        if (AbilityInfo.Slot == ESkillSlot::MAX)
            continue; // 保护：MAX 永远不应被使用

        UE_LOG(LogTemp, Log, TEXT("[PassiveDebug][Init] 授予默认技能 %s 给 槽 %d (Invalid=%d)"),
            *GetNameSafe(AbilityInfo.AbilityClass.Get()), static_cast<int32>(AbilityInfo.Slot), AbilityInfo.Slot == ESkillSlot::Invalid);

        const int32 Index = IndexFromSlot(AbilityInfo.Slot);
        if (Index == INDEX_NONE)
        {
            // Slot=Invalid：授予但不占用技能栏（如基础攻击/某些被动）
            FEquippedAbilityInfo TempInfo = AbilityInfo;
            GrantAbilityInternal(TempInfo, true);
        }
        else
        {
            FEquippedAbilityInfo& NewEquippedInfo = EquippedAbilities[Index];
            NewEquippedInfo = AbilityInfo;
            GrantAbilityInternal(NewEquippedInfo, true);
        }
    }

    bAbilitiesInitialized = true;
    bPendingPassiveActivation = true;
    UE_LOG(LogTemp, Log, TEXT("[PassiveDebug][Init] 已授予默认技能组, 等待激活被动技能"));
    LogActivatableAbilities();
    TryActivatePassiveAbilities();
}


void AIsoPlayerState::NotifyAbilityActorInfoReady()
{
    bAbilityActorInfoInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("[PassiveDebug][ActorInfo] ASC actor info ready for %s"), *GetName());

    if (!HasAuthority() && AbilitySystemComponent && AbilitySystemComponent->GetOwnerRole() == ROLE_AutonomousProxy)
    {
        bPendingPassiveActivation = true;
    }

    TryActivatePassiveAbilities();
}

void AIsoPlayerState::TryActivatePassiveAbilities()
{
    if (!AbilitySystemComponent)
    {
        return;
    }

    const bool bIsAuthority = HasAuthority();
    const ENetRole OwnerRole = AbilitySystemComponent->GetOwnerRole();
    const bool bIsAutonomousProxy = OwnerRole == ROLE_AutonomousProxy;

    UE_LOG(LogTemp, Verbose, TEXT("[PassiveDebug][TryActivate] Authority=%d AutoProxy=%d OwnerRole=%d AbilitiesInit=%d ActorInfoInit=%d Pending=%d"),
        bIsAuthority, bIsAutonomousProxy, OwnerRole, bAbilitiesInitialized, bAbilityActorInfoInitialized, bPendingPassiveActivation);

    if (!bIsAuthority && !bIsAutonomousProxy)
    {
        return;
    }

    if (bIsAuthority && !bAbilitiesInitialized)
    {
        bPendingPassiveActivation = true;
        return;
    }

    if (!bAbilityActorInfoInitialized)
    {
        bPendingPassiveActivation = true;
        return;
    }

    if (!bPendingPassiveActivation)
    {
        return;
    }

    const FGameplayTag PassiveTag = FGameplayTag::RequestGameplayTag(FName("Ability.Regen.Basic"), false);
    if (!PassiveTag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[PassiveDebug][TryActivate] 被动标签 Ability.Regen.Basic 未找到"));
        bPendingPassiveActivation = false;
        return;
    }

    FGameplayTagContainer PassiveTags;
    PassiveTags.AddTag(PassiveTag);
    const bool bAllowRemoteActivation = !bIsAuthority;
    const bool bActivated = AbilitySystemComponent->TryActivateAbilitiesByTag(PassiveTags, bAllowRemoteActivation);
    UE_LOG(LogTemp, Log, TEXT("[PassiveDebug][TryActivate] 尝试激活 -> %d (AllowRemote=%d)"), bActivated, bAllowRemoteActivation);
    bPendingPassiveActivation = false;
}

void AIsoPlayerState::LogActivatableAbilities() const
{
    if (!AbilitySystemComponent)
    {
        return;
    }

    const TArray<FGameplayAbilitySpec>& Specs = AbilitySystemComponent->GetActivatableAbilities();
    for (const FGameplayAbilitySpec& Spec : Specs)
    {
        const UGameplayAbility* AbilityCDO = Spec.Ability;
        const FString AbilityName = GetNameSafe(AbilityCDO);
        const FString TagString = AbilityCDO ? AbilityCDO->AbilityTags.ToStringSimple() : TEXT("<NoTags>");
        UE_LOG(LogTemp, Log, TEXT("[PassiveDebug][Spec] %s Tags=%s HandleValid=%d"),
            *AbilityName,
            *TagString,
            Spec.Handle.IsValid());
    }
}


void AIsoPlayerState::OnAssetsLoadedForUI()
{
    AIsometricPlayerController* PC = Cast<AIsometricPlayerController>(GetPlayerController());
    if (!PC)
    {
        return;
    }

    EnsureAttributeDelegatesBound();

    if (!bUIInitialized)
    {
        bPendingUIUpdate = true;
        return;
    }

    if (UHUDRootWidget* HUD = PC->PlayerHUDInstance)
    {
        RefreshEntireHUD(*HUD);
        bPendingUIUpdate = false;
    }
    else
    {
        bPendingUIUpdate = true;
    }
}

void AIsoPlayerState::OnUIInitialized()
{
    bUIInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("UI initialized in PlayerState"));
    EnsureAttributeDelegatesBound();
    UpdateUIWhenReady();
}

TArray<FHUDBuffIconViewModel> AIsoPlayerState::BuildBuffViewModels(const FGameplayTagContainer& OwnedTags) const
{
    TArray<FHUDBuffIconViewModel> Result;
    if (BuffIconMap.Num() == 0)
    {
        return Result;
    }

    const UAbilitySystemComponent* ASC = AbilitySystemComponent;
    const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

    for (const TPair<FGameplayTag, TSoftObjectPtr<UTexture2D>>& Pair : BuffIconMap)
    {
        const FGameplayTag& DisplayTag = Pair.Key;
        if (!DisplayTag.IsValid())
        {
            continue;
        }
        // Show if any owned tag matches or is child of the display tag
        if (!OwnedTags.HasTag(DisplayTag))
        {
            continue;
        }

        // Ensure texture loaded (soft reference may not yet be resolved client-side)
        UTexture2D* IconTex = Pair.Value.Get();
        if (!IconTex)
        {
            IconTex = Pair.Value.LoadSynchronous();
            if (!IconTex)
            {
                UE_LOG(LogTemp, Verbose, TEXT("[BuffIcons] Texture still null for tag %s"), *DisplayTag.ToString());
                continue; // Skip null to avoid white box
            }
        }

        int32 MaxStackCount = 0;
        float BestTimeRemaining = -1.f;
        float BestTotalDuration = -1.f;

        if (ASC)
        {
            FGameplayTagContainer QueryTags;
            QueryTags.AddTag(DisplayTag);
            const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(QueryTags);

            TArray<FActiveGameplayEffectHandle> MatchingEffects;
            ASC->GetActiveEffects(Query);

            for (const FActiveGameplayEffectHandle& Handle : MatchingEffects)
            {
                if (const FActiveGameplayEffect* ActiveEffect = ASC->GetActiveGameplayEffect(Handle))
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

        FHUDBuffIconViewModel VM;
        VM.TagName = DisplayTag.GetTagName();
        VM.Icon = IconTex;
        VM.StackCount = MaxStackCount;
        VM.bIsDebuff = false;
        VM.TimeRemaining = BestTimeRemaining;
        VM.TotalDuration = BestTotalDuration;
        Result.Add(VM);
    }

    return Result;
}

void AIsoPlayerState::UpdateUIWhenReady()
{
    EnsureAttributeDelegatesBound();

    AIsometricPlayerController* PC = Cast<AIsometricPlayerController>(GetPlayerController());
    if (!PC) return;

    if (UHUDRootWidget* HUD = PC->PlayerHUDInstance)
    {
        RefreshEntireHUD(*HUD);
        bPendingUIUpdate = false;
    }
    else
    {
        UE_LOG(LogTemp, Verbose, TEXT("UI still not ready in UpdateUIWhenReady"));
        bPendingUIUpdate = true;
    }
}

void AIsoPlayerState::RefreshActionBar(UHUDRootWidget& HUD)
{
    static const ESkillSlot DisplayableSlots[] = {
        ESkillSlot::Skill_Passive,
        ESkillSlot::Skill_Q,
        ESkillSlot::Skill_W,
        ESkillSlot::Skill_E,
        ESkillSlot::Skill_R,
        ESkillSlot::Skill_D,
        ESkillSlot::Skill_F
    };

    HUD.ClearAllAbilitySlots();

    for (ESkillSlot Slot : DisplayableSlots)
    {
        const FEquippedAbilityInfo* FoundInfo = nullptr;
        for (const FEquippedAbilityInfo& Info : EquippedAbilities)
        {
            if (Info.Slot == Slot)
            {
                FoundInfo = &Info;
                break;
            }
        }

        const FHUDSkillSlotViewModel ViewModel = FoundInfo ? BuildSlotViewModel(*FoundInfo)
                                                           : BuildEmptySlotViewModel(Slot);
        HUD.SetAbilitySlot(ViewModel);
    }
}

void AIsoPlayerState::RefreshEntireHUD(UHUDRootWidget& HUD)
{
    RefreshActionBar(HUD);
    PushHUDSnapshot(HUD);
}

void AIsoPlayerState::PushHUDSnapshot(UHUDRootWidget& HUD)
{
    if (!AttributeSet)
    {
        return;
    }

    const float CurrentHealth = AttributeSet->GetHealth();
    const float MaxHealth = AttributeSet->GetMaxHealth();
    const float ShieldValue = 0.f;

    HUD.UpdateHealth(CurrentHealth, MaxHealth, ShieldValue);

    // --- Champion stats summary for status panel ---
    FHUDChampionStatsViewModel ChampionStats;
    ChampionStats.AttackDamage = AttributeSet->GetAttackDamage();
    ChampionStats.AbilityPower = AttributeSet->GetAbilityPower();
    ChampionStats.Armor = AttributeSet->GetPhysicalDefense();
    ChampionStats.MagicResist = AttributeSet->GetMagicDefense();
    ChampionStats.PhysicalResist = AttributeSet->GetPhysicalResistance();
    ChampionStats.FireResist = AttributeSet->GetFireResistance();
    ChampionStats.IceResist = AttributeSet->GetIceResistance();
    ChampionStats.LightningResist = AttributeSet->GetLightningResistance();
    ChampionStats.ArmorPenetration = AttributeSet->GetArmorPenetration();
    ChampionStats.MagicPenetration = AttributeSet->GetMagicPenetration();
    ChampionStats.ElementalPenetration = AttributeSet->GetElementalPenetration();
    ChampionStats.AttackSpeed = AttributeSet->GetAttackSpeed();
    ChampionStats.CritChance = AttributeSet->GetCriticalChance();
    ChampionStats.MoveSpeed = AttributeSet->GetMoveSpeed();
    HUD.UpdateChampionStats(ChampionStats);

    // -- Owned Gameplay Tags to HUD (debug/optional) --
    FGameplayTagContainer OwnedTags;
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->GetOwnedGameplayTags(OwnedTags);
    }

    bool bIsInCombat = false;
    if (AbilitySystemComponent)
    {
        static const FGameplayTag CombatTag = FGameplayTag::RequestGameplayTag(FName("State.Combat"), false);
        if (CombatTag.IsValid())
        {
            bIsInCombat = OwnedTags.HasTagExact(CombatTag);
        }
    }

    // Curated buff icons (preferred)
    HUD.UpdateStatusBuffs(BuildBuffViewModels(OwnedTags));

    // Optional debug text badges
    if (bShowOwnedTagsOnHUD)
    {
        TArray<FGameplayTag> OwnedTagArray;
        OwnedTags.GetGameplayTagArray(OwnedTagArray);
        TArray<FName> TagNameArray;
        TagNameArray.Reserve(OwnedTagArray.Num());
        for (const FGameplayTag& Tag : OwnedTagArray)
        {
            TagNameArray.Add(Tag.GetTagName());
        }
        HUD.UpdateStatusEffects(TagNameArray);
    }

    const bool bHasPendingLevelUp = AttributeSet->GetUnUsedSkillPoint() > 0.f;
    HUD.UpdatePortrait(nullptr, bIsInCombat, bHasPendingLevelUp);

    const float CurrentMana = AttributeSet->GetMana();
    const float MaxMana = AttributeSet->GetMaxMana();
    HUD.UpdateResources(CurrentMana, MaxMana, 0.f, 0.f);

    const int32 CurrentLevel = FMath::FloorToInt(AttributeSet->GetLevel());
    const float CurrentXP = AttributeSet->GetExperience();
    const float RequiredXP = AttributeSet->GetExperienceToNextLevel();
    HUD.UpdateExperience(CurrentLevel, CurrentXP, RequiredXP);

    HUD.UpdateUtilityButtons(BuildUtilityButtonViewModels());
}

void AIsoPlayerState::EnsureAttributeDelegatesBound()
{
    if (bAttributeDelegatesBound || !AttributeSet)
    {
        return;
    }

    AttributeSet->OnHealthChanged.AddDynamic(this, &AIsoPlayerState::HandleHealthChanged);
    AttributeSet->OnManaChanged.AddDynamic(this, &AIsoPlayerState::HandleManaChanged);
    AttributeSet->OnExperienceChanged.AddDynamic(this, &AIsoPlayerState::HandleExperienceChanged);
    AttributeSet->OnLevelChanged.AddDynamic(this, &AIsoPlayerState::HandleLevelChanged);

    bAttributeDelegatesBound = true;
}

UHUDRootWidget* AIsoPlayerState::ResolveHUDWidget() const
{
    if (AIsometricPlayerController* PC = Cast<AIsometricPlayerController>(GetPlayerController()))
    {
        if (!PC->IsLocalController())
        {
            return nullptr;
        }

        return PC->PlayerHUDInstance;
    }

    return nullptr;
}

void AIsoPlayerState::HandleHealthChanged(UIsometricRPGAttributeSetBase* AttributeSetChanged, float NewHealth)
{
    if (!AttributeSet || AttributeSetChanged != AttributeSet)
    {
        return;
    }

    if (UHUDRootWidget* HUD = ResolveHUDWidget())
    {
        PushHUDSnapshot(*HUD);
    }
    else
    {
        bPendingUIUpdate = true;
    }
}

void AIsoPlayerState::HandleManaChanged(UIsometricRPGAttributeSetBase* AttributeSetChanged, float NewMana)
{
    if (!AttributeSet || AttributeSetChanged != AttributeSet)
    {
        return;
    }

    if (UHUDRootWidget* HUD = ResolveHUDWidget())
    {
        PushHUDSnapshot(*HUD);
    }
    else
    {
        bPendingUIUpdate = true;
    }
}

void AIsoPlayerState::HandleExperienceChanged(UIsometricRPGAttributeSetBase* AttributeSetChanged, float NewExperience, float NewMaxExperience)
{
    if (!AttributeSet || AttributeSetChanged != AttributeSet)
    {
        return;
    }

    if (UHUDRootWidget* HUD = ResolveHUDWidget())
    {
        PushHUDSnapshot(*HUD);
    }
    else
    {
        bPendingUIUpdate = true;
    }
}

void AIsoPlayerState::HandleLevelChanged(UIsometricRPGAttributeSetBase* AttributeSetChanged, float NewLevel)
{
    if (!AttributeSet || AttributeSetChanged != AttributeSet)
    {
        return;
    }

    if (UHUDRootWidget* HUD = ResolveHUDWidget())
    {
        PushHUDSnapshot(*HUD);
    }
    else
    {
        bPendingUIUpdate = true;
    }
}

void AIsoPlayerState::HandleAbilityCooldownTriggered(const FGameplayAbilitySpecHandle& SpecHandle, float DurationSeconds)
{
    if (!SpecHandle.IsValid() || DurationSeconds <= 0.f)
    {
        return;
    }

    const FEquippedAbilityInfo* FoundInfo = FindEquippedInfoByHandle(SpecHandle);
    if (!FoundInfo || FoundInfo->Slot == ESkillSlot::Invalid || FoundInfo->Slot == ESkillSlot::MAX)
    {
        return;
    }

    if (UHUDRootWidget* HUD = ResolveHUDWidget())
    {
        HUD->UpdateAbilityCooldown(FoundInfo->Slot, DurationSeconds, DurationSeconds);
    }
}

const FEquippedAbilityInfo* AIsoPlayerState::FindEquippedInfoByHandle(const FGameplayAbilitySpecHandle& SpecHandle) const
{
    for (const FEquippedAbilityInfo& Info : EquippedAbilities)
    {
        if (Info.AbilitySpecHandle.IsValid() && Info.AbilitySpecHandle == SpecHandle)
        {
            return &Info;
        }
    }

    return nullptr;
}

FHUDSkillSlotViewModel AIsoPlayerState::BuildSlotViewModel(const FEquippedAbilityInfo& Info) const
{
    FHUDSkillSlotViewModel ViewModel = BuildEmptySlotViewModel(Info.Slot);

    if (!ShouldDisplaySlot(Info.Slot))
    {
        return ViewModel;
    }

    ViewModel.bIsUnlocked = true;

    // Treat slot as 'equipped' if the soft class pointer has a valid path (even if not yet loaded).
    const bool bHasAbilitySoftRef = !Info.AbilityClass.ToSoftObjectPath().IsNull();
    UClass* AbilityClass = Info.AbilityClass.Get();
    ViewModel.AbilityClass = AbilityClass;
    ViewModel.bIsEquipped = bHasAbilitySoftRef; // allow UI to show placeholder before class loads

    // Icon: attempt to resolve; if not yet loaded, keep nullptr (will refresh after async load)
    ViewModel.Icon = Info.Icon.Get();

    if (AbilityClass)
    {
        if (const UGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<UGameplayAbility>())
        {
            if (const UGA_HeroBaseAbility* HeroCDO = Cast<UGA_HeroBaseAbility>(AbilityCDO))
            {
                ViewModel.DisplayName = HeroCDO->GetAbilityDisplayNameText();
                ViewModel.ResourceCost = HeroCDO->GetResourceCost();
                ViewModel.CooldownDuration = HeroCDO->CooldownDuration;
                ViewModel.CooldownRemaining = 0.f; // TODO: query ASC for active cooldowns
            }
            else
            {
                ViewModel.DisplayName = FText::FromName(AbilityClass->GetFName());
            }

            if (ViewModel.DisplayName.IsEmpty())
            {
                ViewModel.DisplayName = FText::FromName(AbilityClass->GetFName());
            }

            FGameplayTagContainer AssetTags;
            AssetTags = AbilityCDO->GetAssetTags();
            if (!AssetTags.IsEmpty())
            {
                TArray<FGameplayTag> LocalTags;
                AssetTags.GetGameplayTagArray(LocalTags);
                if (LocalTags.Num() > 0)
                {
                    ViewModel.AbilityPrimaryTag = LocalTags[0];
                }
            }
        }
    }

    return ViewModel;
}

FHUDSkillSlotViewModel AIsoPlayerState::BuildEmptySlotViewModel(ESkillSlot Slot) const
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

bool AIsoPlayerState::ShouldDisplaySlot(ESkillSlot Slot) const
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

FText AIsoPlayerState::BuildHotkeyLabel(ESkillSlot Slot) const
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

TArray<FHUDItemSlotViewModel> AIsoPlayerState::BuildUtilityButtonViewModels() const
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

void AIsoPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AIsoPlayerState, EquippedAbilities);
}