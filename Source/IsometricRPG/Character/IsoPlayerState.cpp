// Fill out your copyright notice in the Description page of Project Settings.

#include "IsoPlayerState.h"
#include "AbilitySystemComponent.h"
#include "IsometricRPG/Character/IsometricRPGAttributeSetBase.h"
#include "IsometricAbilities/GameplayAbilities/HeroAbilityLoadoutService.h"
#include "IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpec.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/AssetManager.h"
#include "Engine/Texture2D.h"
#include "IsometricRPGCharacter.h"
#include "GameFramework/PlayerController.h"

AIsoPlayerState::AIsoPlayerState()
{
    // 创建ASC
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

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

    // 仅在本地控制且非专用服务器时绑定输入
    if (AbilitySystemComponent && GetNetMode() != NM_DedicatedServer)
    {
        APlayerController* CachedPlayerController = Cast<APlayerController>(GetOwningController());
        if (CachedPlayerController && CachedPlayerController->IsLocalController())
        {
            UInputComponent* PCInputComponent = CachedPlayerController->InputComponent;
            if (PCInputComponent)
            {
                FTopLevelAssetPath EnumPath = FTopLevelAssetPath(TEXT("/Script/IsometricRPG.EAbilityInputID"));
                FGameplayAbilityInputBinds BindInfo(
                    TEXT(""), // Confirm action name
                    TEXT(""), // Cancel action name
                    EnumPath,
                    (int32)EAbilityInputID::Confirm,
                    (int32)EAbilityInputID::Cancel
                );
                AbilitySystemComponent->BindAbilityActivationToInputComponent(PCInputComponent, BindInfo);
                UE_LOG(LogTemp, Log, TEXT("AIsoPlayerState: GAS Input successfully bound for %s"), *GetName());
            }
        }
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

FEquippedAbilityInfo AIsoPlayerState::GetEquippedAbilityInfo(ESkillSlot Slot) const
{
    const int32 Index = FHeroAbilityLoadoutService::IndexFromSlot(Slot);
    if (EquippedAbilities.IsValidIndex(Index))
    {
        return EquippedAbilities[Index];
    }

    return FEquippedAbilityInfo();
}

void AIsoPlayerState::Server_EquipAbilityToSlot_Implementation(TSubclassOf<UGameplayAbility> NewAbilityClass, ESkillSlot Slot)
{
    if (!AbilitySystemComponent || !NewAbilityClass || Slot == ESkillSlot::Invalid || Slot == ESkillSlot::MAX)
        return;

    if (!FHeroAbilityLoadoutService::EquipAbilityToSlot(*AbilitySystemComponent, this, EquippedAbilities, NewAbilityClass, Slot))
    {
        return;
    }
    
    // 强制触发同步回调以更新 UI（在 Listen Server/Standalone 下）
    if (GetNetMode() != NM_DedicatedServer)
    {
        OnRep_EquippedAbilities();
    }
}

void AIsoPlayerState::Server_UnequipAbilityFromSlot_Implementation(ESkillSlot Slot)
{
    if (!AbilitySystemComponent)
    {
        return;
    }

    if (!FHeroAbilityLoadoutService::UnequipAbilityFromSlot(*AbilitySystemComponent, EquippedAbilities, Slot))
    {
        return;
    }
    
    if (GetNetMode() != NM_DedicatedServer)
    {
        OnRep_EquippedAbilities();
    }
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

void AIsoPlayerState::InitAbilities()
{
    if (!HasAuthority() || !AbilitySystemComponent || DefaultAbilities.Num() == 0 || bAbilitiesInitialized)
        return;

    FHeroAbilityLoadoutService::InitializeDefaultAbilities(*AbilitySystemComponent, this, DefaultAbilities, EquippedAbilities);

    bAbilitiesInitialized = true;
    bPendingPassiveActivation = true;
    
    LogActivatableAbilities();
    TryActivatePassiveAbilities();

    if (GetNetMode() != NM_DedicatedServer)
    {
        OnRep_EquippedAbilities();
    }
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
        UE_LOG(LogTemp, Log, TEXT("[GASLog] Ability: %s, HandleValid: %d, InputID: %d"),
            *GetNameSafe(AbilityCDO),
            Spec.Handle.IsValid(),
            Spec.InputID);
    }
}


void AIsoPlayerState::OnAssetsLoadedForUI()
{
    EnsureAttributeDelegatesBound();
    EnsureGameplayTagDelegatesBound();
    EnsureGameplayEffectDelegatesBound();
    RequestHUDRefresh(EHeroHUDRefreshKind::ActionBar);
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

    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UIsometricRPGAttributeSetBase::GetMaxHealthAttribute())
        .AddUObject(this, &AIsoPlayerState::HandleVitalAttributeValueChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UIsometricRPGAttributeSetBase::GetMaxManaAttribute())
        .AddUObject(this, &AIsoPlayerState::HandleVitalAttributeValueChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UIsometricRPGAttributeSetBase::GetAttackDamageAttribute())
        .AddUObject(this, &AIsoPlayerState::HandleChampionStatAttributeValueChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UIsometricRPGAttributeSetBase::GetAbilityPowerAttribute())
        .AddUObject(this, &AIsoPlayerState::HandleChampionStatAttributeValueChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UIsometricRPGAttributeSetBase::GetPhysicalDefenseAttribute())
        .AddUObject(this, &AIsoPlayerState::HandleChampionStatAttributeValueChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UIsometricRPGAttributeSetBase::GetMagicDefenseAttribute())
        .AddUObject(this, &AIsoPlayerState::HandleChampionStatAttributeValueChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UIsometricRPGAttributeSetBase::GetAttackSpeedAttribute())
        .AddUObject(this, &AIsoPlayerState::HandleChampionStatAttributeValueChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UIsometricRPGAttributeSetBase::GetCriticalChanceAttribute())
        .AddUObject(this, &AIsoPlayerState::HandleChampionStatAttributeValueChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UIsometricRPGAttributeSetBase::GetMoveSpeedAttribute())
        .AddUObject(this, &AIsoPlayerState::HandleChampionStatAttributeValueChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UIsometricRPGAttributeSetBase::GetTotalSkillPointAttribute())
        .AddUObject(this, &AIsoPlayerState::HandleSkillPointAttributeValueChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UIsometricRPGAttributeSetBase::GetUnUsedSkillPointAttribute())
        .AddUObject(this, &AIsoPlayerState::HandleSkillPointAttributeValueChanged);

    bAttributeDelegatesBound = true;
}

void AIsoPlayerState::EnsureGameplayTagDelegatesBound()
{
    if (bGameplayTagDelegatesBound || !AbilitySystemComponent)
    {
        return;
    }

    TSet<FGameplayTag> ObservedTags;
    static const FGameplayTag CombatTag = FGameplayTag::RequestGameplayTag(FName("State.Combat"), false);
    if (CombatTag.IsValid())
    {
        ObservedTags.Add(CombatTag);
    }

    for (const FGameplayTag& ObservedTag : ObservedTags)
    {
        AbilitySystemComponent->RegisterGameplayTagEvent(ObservedTag, EGameplayTagEventType::AnyCountChange)
            .AddUObject(this, &AIsoPlayerState::HandleObservedGameplayTagChanged);
    }

    bGameplayTagDelegatesBound = true;
}

void AIsoPlayerState::EnsureGameplayEffectDelegatesBound()
{
    if (bGameplayEffectDelegatesBound || !AbilitySystemComponent)
    {
        return;
    }

    AbilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &AIsoPlayerState::HandleActiveGameplayEffectAdded);
    AbilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &AIsoPlayerState::HandleActiveGameplayEffectRemoved);

    const TArray<FActiveGameplayEffectHandle> ActiveEffectHandles = AbilitySystemComponent->GetActiveEffects(FGameplayEffectQuery());
    for (const FActiveGameplayEffectHandle& ActiveHandle : ActiveEffectHandles)
    {
        if (const FActiveGameplayEffect* ActiveEffect = AbilitySystemComponent->GetActiveGameplayEffect(ActiveHandle))
        {
            if (ShouldTrackBuffEffect(*ActiveEffect))
            {
                BindBuffEffectDelegates(ActiveHandle);
            }
        }
    }

    bGameplayEffectDelegatesBound = true;
}

void AIsoPlayerState::HandleHealthChanged(UIsometricRPGAttributeSetBase* AttributeSetChanged, float NewHealth)
{
    if (!AttributeSet || AttributeSetChanged != AttributeSet)
    {
        return;
    }

    RequestHUDRefresh(EHeroHUDRefreshKind::Vitals);
}

void AIsoPlayerState::HandleManaChanged(UIsometricRPGAttributeSetBase* AttributeSetChanged, float NewMana)
{
    if (!AttributeSet || AttributeSetChanged != AttributeSet)
    {
        return;
    }

    RequestHUDRefresh(EHeroHUDRefreshKind::Vitals);
}

void AIsoPlayerState::HandleExperienceChanged(UIsometricRPGAttributeSetBase* AttributeSetChanged, float NewExperience, float NewMaxExperience)
{
    if (!AttributeSet || AttributeSetChanged != AttributeSet)
    {
        return;
    }

    RequestHUDRefresh(EHeroHUDRefreshKind::Experience);
}

void AIsoPlayerState::HandleLevelChanged(UIsometricRPGAttributeSetBase* AttributeSetChanged, float NewLevel)
{
    if (!AttributeSet || AttributeSetChanged != AttributeSet)
    {
        return;
    }

    RequestHUDRefresh(EHeroHUDRefreshKind::Full);
}

void AIsoPlayerState::HandleObservedGameplayTagChanged(const FGameplayTag ChangedTag, int32 NewCount)
{
    RequestHUDRefresh(EHeroHUDRefreshKind::GameplayPresentation);
}

void AIsoPlayerState::HandleVitalAttributeValueChanged(const FOnAttributeChangeData& ChangeData)
{
    if (!AttributeSet)
    {
        return;
    }

    RequestHUDRefresh(EHeroHUDRefreshKind::Vitals);
}

FHUDPresentationContext AIsoPlayerState::MakeHUDPresentationContext() const
{
    FHUDPresentationContext Context;
    Context.AttributeSet = GetAttributeSet();
    Context.AbilitySystemComponent = GetAbilitySystemComponent();
    Context.EquippedAbilities = &EquippedAbilities;
    Context.BuffIconMap = &BuffIconMap;
    Context.World = GetWorld();
    Context.bShowOwnedTagsOnHUD = bShowOwnedTagsOnHUD;
    return Context;
}

void AIsoPlayerState::HandleChampionStatAttributeValueChanged(const FOnAttributeChangeData& ChangeData)
{
    if (!AttributeSet)
    {
        return;
    }

    RequestHUDRefresh(EHeroHUDRefreshKind::ChampionStats);
}

void AIsoPlayerState::HandleSkillPointAttributeValueChanged(const FOnAttributeChangeData& ChangeData)
{
    if (!AttributeSet)
    {
        return;
    }

    RequestHUDRefresh(EHeroHUDRefreshKind::GameplayPresentation);
}

void AIsoPlayerState::HandleActiveGameplayEffectAdded(UAbilitySystemComponent* TargetASC, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle)
{
    if (TargetASC != AbilitySystemComponent || !ShouldTrackBuffEffect(SpecApplied))
    {
        return;
    }

    BindBuffEffectDelegates(ActiveHandle);
    RefreshBuffPresentationIfReady();
}

void AIsoPlayerState::HandleActiveGameplayEffectRemoved(const FActiveGameplayEffect& ActiveEffect)
{
    const bool bWasTracked = TrackedBuffEffectHandles.Remove(ActiveEffect.Handle) > 0;
    if (!bWasTracked && !ShouldTrackBuffEffect(ActiveEffect))
    {
        return;
    }

    RefreshBuffPresentationIfReady();
}

void AIsoPlayerState::HandleTrackedGameplayEffectStackChanged(FActiveGameplayEffectHandle ActiveHandle, int32 NewStackCount, int32 PreviousStackCount)
{
    if (!TrackedBuffEffectHandles.Contains(ActiveHandle))
    {
        return;
    }

    RefreshBuffPresentationIfReady();
}

void AIsoPlayerState::HandleTrackedGameplayEffectTimeChanged(FActiveGameplayEffectHandle ActiveHandle, float NewStartTime, float NewDuration)
{
    if (!TrackedBuffEffectHandles.Contains(ActiveHandle))
    {
        return;
    }

    RefreshBuffPresentationIfReady();
}

void AIsoPlayerState::RefreshBuffPresentationIfReady()
{
    RequestHUDRefresh(EHeroHUDRefreshKind::GameplayPresentation);
}

bool AIsoPlayerState::ShouldTrackBuffEffect(const FGameplayEffectSpec& EffectSpec) const
{
    if (BuffIconMap.Num() == 0)
    {
        return false;
    }

    FGameplayTagContainer GrantedTags;
    EffectSpec.GetAllGrantedTags(GrantedTags);
    if (GrantedTags.IsEmpty())
    {
        return false;
    }

    for (const TPair<FGameplayTag, TSoftObjectPtr<UTexture2D>>& Pair : BuffIconMap)
    {
        if (Pair.Key.IsValid() && GrantedTags.HasTag(Pair.Key))
        {
            return true;
        }
    }

    return false;
}

bool AIsoPlayerState::ShouldTrackBuffEffect(const FActiveGameplayEffect& ActiveEffect) const
{
    return ShouldTrackBuffEffect(ActiveEffect.Spec);
}

void AIsoPlayerState::BindBuffEffectDelegates(FActiveGameplayEffectHandle ActiveHandle)
{
    if (!AbilitySystemComponent || !ActiveHandle.IsValid() || TrackedBuffEffectHandles.Contains(ActiveHandle))
    {
        return;
    }

    if (FOnActiveGameplayEffectStackChange* StackChangedDelegate = AbilitySystemComponent->OnGameplayEffectStackChangeDelegate(ActiveHandle))
    {
        StackChangedDelegate->AddUObject(this, &AIsoPlayerState::HandleTrackedGameplayEffectStackChanged);
    }

    if (FOnActiveGameplayEffectTimeChange* TimeChangedDelegate = AbilitySystemComponent->OnGameplayEffectTimeChangeDelegate(ActiveHandle))
    {
        TimeChangedDelegate->AddUObject(this, &AIsoPlayerState::HandleTrackedGameplayEffectTimeChanged);
    }

    TrackedBuffEffectHandles.Add(ActiveHandle);
}

void AIsoPlayerState::RequestHUDRefresh(
    EHeroHUDRefreshKind Kind,
    const FGameplayAbilitySpecHandle& SpecHandle,
    float DurationSeconds)
{
    FHeroHUDRefreshRequest Request;
    Request.Kind = Kind;
    Request.SpecHandle = SpecHandle;
    Request.DurationSeconds = DurationSeconds;
    HUDRefreshRequested.Broadcast(Request);
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

bool AIsoPlayerState::TryGetEquippedAbilityInfoByHandle(const FGameplayAbilitySpecHandle& Handle, FEquippedAbilityInfo& OutInfo) const
{
    if (const FEquippedAbilityInfo* FoundInfo = FindEquippedInfoByHandle(Handle))
    {
        OutInfo = *FoundInfo;
        return true;
    }

    return false;
}

bool AIsoPlayerState::QueryCooldownState(const UGameplayAbility* AbilityCDO, float& OutDuration, float& OutRemaining) const
{
    if (!AbilitySystemComponent || !AbilityCDO)
    {
        return false;
    }

    const FGameplayTagContainer* CooldownTags = AbilityCDO->GetCooldownTags();
    if (!(CooldownTags && !CooldownTags->IsEmpty()))
    {
        return false;
    }

    const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(*CooldownTags);
    const TArray<FActiveGameplayEffectHandle> ActiveCooldownEffects = AbilitySystemComponent->GetActiveEffects(Query);
    if (ActiveCooldownEffects.Num() == 0)
    {
        return false;
    }

    const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    float BestRemaining = -1.f;
    float BestDuration = 0.f;

    for (const FActiveGameplayEffectHandle& Handle : ActiveCooldownEffects)
    {
        const FActiveGameplayEffect* ActiveEffect = AbilitySystemComponent->GetActiveGameplayEffect(Handle);
        if (!ActiveEffect)
        {
            continue;
        }

        const float Duration = ActiveEffect->GetDuration();
        const float Remaining = ActiveEffect->GetTimeRemaining(WorldTime);
        if (Remaining > BestRemaining)
        {
            BestRemaining = Remaining;
            BestDuration = Duration;
        }
    }

    if (BestRemaining <= 0.f)
    {
        return false;
    }

    OutDuration = BestDuration > 0.f ? BestDuration : BestRemaining;
    OutRemaining = BestRemaining;
    return true;
}

void AIsoPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AIsoPlayerState, EquippedAbilities);
}
