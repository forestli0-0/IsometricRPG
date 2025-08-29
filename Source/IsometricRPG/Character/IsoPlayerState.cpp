// Fill out your copyright notice in the Description page of Project Settings.

#include "IsoPlayerState.h"
#include "AbilitySystemComponent.h"
#include "IsometricRPG/Character/IsometricRPGAttributeSetBase.h"
#include "IsometricRPG/Player/IsometricPlayerController.h"
#include "IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "GameplayAbilitySpec.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "TimerManager.h"
#include "Engine/AssetManager.h"
#include "IsometricAbilities/GameplayAbilities/GA_HeroBaseAbility.h"

AIsoPlayerState::AIsoPlayerState()
{
    // 创建ASC
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed); // 推荐使用Mixed模式

    // 创建AttributeSet
    AttributeSet = CreateDefaultSubobject<UIsometricRPGAttributeSetBase>("AttributeSet");

    // 网络更新频率设置，可以根据需要调整
    NetUpdateFrequency = 100.0f;
}

void AIsoPlayerState::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        InitializeAttributes(); // 先初始化属性
        InitAbilities(); // 再初始化技能
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
    if (EquippedAbilities.IsValidIndex(static_cast<int32>(Slot)))
    {
        return EquippedAbilities[static_cast<int32>(Slot)];
    }
    return FEquippedAbilityInfo();
}

void AIsoPlayerState::Server_EquipAbilityToSlot_Implementation(TSubclassOf<UGameplayAbility> NewAbilityClass, ESkillSlot Slot)
{
    if (!NewAbilityClass || Slot == ESkillSlot::Invalid)
        return;
    if (!EquippedAbilities.IsValidIndex(static_cast<int32>(Slot)))
        EquippedAbilities.SetNum(static_cast<int32>(ESkillSlot::Invalid));
    FEquippedAbilityInfo& Info = EquippedAbilities[static_cast<int32>(Slot)];
    Info.AbilityClass = NewAbilityClass;
    Info.Slot = Slot;
    GrantAbilityInternal(Info, true);
    //FTimerHandle InitialDelayHandle;
    //GetWorld()->GetTimerManager().SetTimer(InitialDelayHandle, this, &AIsoPlayerState::OnRep_EquippedAbilities, 3.0f, false);
    //OnRep_EquippedAbilities();
}

void AIsoPlayerState::Server_UnequipAbilityFromSlot_Implementation(ESkillSlot Slot)
{
    if (!EquippedAbilities.IsValidIndex(static_cast<int32>(Slot)))
        return;
    FEquippedAbilityInfo& Info = EquippedAbilities[static_cast<int32>(Slot)];
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
    if (GetLocalRole() != ROLE_Authority || !AbilitySystemComponent || !Info.AbilityClass)
        return;
    if (bRemoveExistingFirst && Info.AbilitySpecHandle.IsValid())
        ClearAbilityInternal(Info);

    // 从软引用加载类。因为这是在服务器上授权技能，通常可以接受同步加载。
    TSubclassOf<UGameplayAbility> LoadedAbilityClass = Info.AbilityClass.LoadSynchronous();
    if (!LoadedAbilityClass) return;

    if (!Info.AbilitySpecHandle.IsValid())
    {
        // 创建技能规格(Spec)时，将 InputID 硬编码为 -1。
        // -1 是一个特殊值，代表“没有绑定到任何直接输入”。
        FGameplayAbilitySpec Spec(LoadedAbilityClass, 1, -1, this);
        Info.AbilitySpecHandle = AbilitySystemComponent->GiveAbility(Spec);
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
	EquippedAbilities.SetNum(static_cast<int32>(ESkillSlot::Skill_F)); // 这里技能槽数量暂时是F键，也就是7个技能槽
    for (const FEquippedAbilityInfo& AbilityInfo : DefaultAbilities)
    {
        if (AbilityInfo.AbilityClass && AbilityInfo.Slot != ESkillSlot::MAX)
        {
            FEquippedAbilityInfo& NewEquippedInfo = EquippedAbilities[static_cast<int32>(AbilityInfo.Slot) - 1];
            NewEquippedInfo = AbilityInfo;
            GrantAbilityInternal(NewEquippedInfo, true);
        }
    }

    // 在授予技能后，激活被动技能
    FGameplayTagContainer PassiveTags;
    PassiveTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Regen.Basic")));
    AbilitySystemComponent->TryActivateAbilitiesByTag(PassiveTags);
    
    bAbilitiesInitialized = true;
}


void AIsoPlayerState::OnAssetsLoadedForUI()
{
    // 安全区域：现在所有在PathsToLoad里的资源都已加载完毕
    AIsometricPlayerController* PC = Cast<AIsometricPlayerController>(GetPlayerController());
    if (!PC) return;
	UE_LOG(LogTemp, Log, TEXT("OnAssetsLoadedForUI called in PlayerState"));
    UPlayerHUDWidget* HUD = Cast<UPlayerHUDWidget>(PC->PlayerHUDInstance);
    if (!HUD || !HUD->SkillBar) 
    {
        // UI还未初始化，标记需要更新
        bPendingUIUpdate = true;
        UE_LOG(LogTemp, Warning, TEXT("UI not initialized yet, marking for pending update"));
        return;
    }

    // 现在可以安全地遍历并更新UI了
    for (int32 i = 0; i < EquippedAbilities.Num(); ++i)
    {
        if (HUD->SkillBar->SkillSlots.IsValidIndex(i))
        {
            USkillSlotWidget* TargetSlot = HUD->SkillBar->SkillSlots[i];
            if (TargetSlot)
            {
                const FEquippedAbilityInfo& Info = EquippedAbilities[i];
                FSkillSlotData SlotData;

                // 使用 .Get() 从软引用获取已加载的类，现在它不会是nullptr
                UClass* LoadedAbilityClass = Info.AbilityClass.Get();
                SlotData.AbilityClass = LoadedAbilityClass;
                SlotData.Icon = Info.Icon.Get();
                if (LoadedAbilityClass)
                {
                    const UGA_HeroBaseAbility* AbilityCDO = Cast<UGA_HeroBaseAbility>(LoadedAbilityClass->GetDefaultObject());
                    if (AbilityCDO)
                    {
                        SlotData.CooldownDuration = AbilityCDO->CooldownDuration;
                        SlotData.CooldownRemaining = 0.f;
                    }
                }
                else
                {
                    SlotData.CooldownDuration = 0.f;
                    SlotData.CooldownRemaining = 0.f;
                }

                TargetSlot->UpdateSlot(SlotData);
            }
        }
    }
}

void AIsoPlayerState::OnUIInitialized()
{
    bUIInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("UI initialized in PlayerState"));
    
    // 如果有待处理的UI更新，现在执行
    if (bPendingUIUpdate)
    {
        UE_LOG(LogTemp, Log, TEXT("Processing pending UI update"));
        UpdateUIWhenReady();
        bPendingUIUpdate = false;
    }
}

void AIsoPlayerState::UpdateUIWhenReady()
{
    AIsometricPlayerController* PC = Cast<AIsometricPlayerController>(GetPlayerController());
    if (!PC) return;
    
    UPlayerHUDWidget* HUD = Cast<UPlayerHUDWidget>(PC->PlayerHUDInstance);
    if (!HUD || !HUD->SkillBar) 
    {
        UE_LOG(LogTemp, Warning, TEXT("UI still not ready in UpdateUIWhenReady"));
        return;
    }
    
    // 现在可以安全地遍历并更新UI了
    for (int32 i = 0; i < EquippedAbilities.Num(); ++i)
    {
        if (HUD->SkillBar->SkillSlots.IsValidIndex(i))
        {
            USkillSlotWidget* TargetSlot = HUD->SkillBar->SkillSlots[i];
            if (TargetSlot)
            {
                const FEquippedAbilityInfo& Info = EquippedAbilities[i];
                FSkillSlotData SlotData;

                // 使用 .Get() 从软引用获取已加载的类，现在它不会是nullptr
                UClass* LoadedAbilityClass = Info.AbilityClass.Get();
                SlotData.AbilityClass = LoadedAbilityClass;
                SlotData.Icon = Info.Icon.Get();
                if (LoadedAbilityClass)
                {
                    const UGA_HeroBaseAbility* AbilityCDO = Cast<UGA_HeroBaseAbility>(LoadedAbilityClass->GetDefaultObject());
                    if (AbilityCDO)
                    {
                        SlotData.CooldownDuration = AbilityCDO->CooldownDuration;
                        SlotData.CooldownRemaining = 0.f;
                    }
                }
                else
                {
                    SlotData.CooldownDuration = 0.f;
                    SlotData.CooldownRemaining = 0.f;
                }

                TargetSlot->UpdateSlot(SlotData);
            }
        }
    }
}

void AIsoPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AIsoPlayerState, EquippedAbilities);
}