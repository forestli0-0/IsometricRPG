// Fill out your copyright notice in the Description page of Project Settings.

#include "IsoPlayerState.h"
#include "AbilitySystemComponent.h"
#include "IsometricRPG/Character/IsometricRPGAttributeSetBase.h" // 引入你的AS头文件
#include "IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "GameplayAbilitySpec.h"
#include "Net/UnrealNetwork.h"

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
    InitializeAttributes(); // 先初始化属性
    InitAbilities(); // 再初始化技能
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
    OnRep_EquippedAbilities();
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
}

void AIsoPlayerState::GrantAbilityInternal(FEquippedAbilityInfo& Info, bool bRemoveExistingFirst)
{
    if (GetLocalRole() != ROLE_Authority || !AbilitySystemComponent || !Info.AbilityClass)
        return;
    if (bRemoveExistingFirst && Info.AbilitySpecHandle.IsValid())
        ClearAbilityInternal(Info);
    if (!Info.AbilitySpecHandle.IsValid())
    {
        // 【核心修改】
        // 创建技能规格(Spec)时，将 InputID 硬编码为 -1。
        // -1 是一个特殊值，代表“没有绑定到任何直接输入”。
        // 这将技能的“逻辑槽位(Slot)”与“物理输入ID(InputID)”彻底解耦。
        // 技能的激活将完全由GameplayEvent驱动，而不是错误的InputID匹配。
        FGameplayAbilitySpec Spec(Info.AbilityClass, 1, -1, this);
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
    EquippedAbilities.SetNum(static_cast<int32>(ESkillSlot::MAX));
    for (const FEquippedAbilityInfo& AbilityInfo : DefaultAbilities)
    {
        if (AbilityInfo.AbilityClass && AbilityInfo.Slot != ESkillSlot::MAX)
        {
            FEquippedAbilityInfo& NewEquippedInfo = EquippedAbilities[static_cast<int32>(AbilityInfo.Slot)];
            NewEquippedInfo = AbilityInfo;
            GrantAbilityInternal(NewEquippedInfo, true);
        }
    }
    bAbilitiesInitialized = true;
    OnRep_EquippedAbilities();
}

void AIsoPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AIsoPlayerState, EquippedAbilities);
}
