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
#include "IsometricRPGCharacter.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemData.h"
#include "Equipment/EquipmentComponent.h"
#include "SkillTree/SkillTreeComponent.h"
#include "UI/PlayerHUDWidget.h"
#include "IsometricAbilities/Types/HeroAbilityTypes.h"

AIsoPlayerState::AIsoPlayerState()
{
    // 创建ASC
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed); // 推荐使用Mixed模式

    // 创建AttributeSet
    AttributeSet = CreateDefaultSubobject<UIsometricRPGAttributeSetBase>("AttributeSet");

    InventoryComponent = CreateDefaultSubobject<UInventoryComponent>("InventoryComponent");
    EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>("EquipmentComponent");
    SkillTreeComponent = CreateDefaultSubobject<USkillTreeComponent>("SkillTreeComponent");

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
        InitializeInventory();
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
    return 7;
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
        FEquippedAbilityInfo PreviousInfo = Info;
    Info.AbilityClass = NewAbilityClass;
    Info.Slot = Slot;
    GrantAbilityInternal(Info, true);
        if (PreviousInfo.AbilityClass != Info.AbilityClass && !PreviousInfo.AbilityClass.IsNull())
        {
            HandleAbilitySlotUnequipped(PreviousInfo.Slot, PreviousInfo);
        }
        if (Info.Slot != ESkillSlot::Invalid && Info.Slot != ESkillSlot::MAX)
        {
            HandleAbilitySlotEquipped(Info.Slot, Info);
        }
    RefreshInputMappings();
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
        const FEquippedAbilityInfo PreviouslyEquipped = Info;
        ClearAbilityInternal(Info);
        Info = FEquippedAbilityInfo();
    OnRep_EquippedAbilities();
        if (PreviouslyEquipped.Slot != ESkillSlot::Invalid && PreviouslyEquipped.Slot != ESkillSlot::MAX)
        {
            HandleAbilitySlotUnequipped(PreviouslyEquipped.Slot, PreviouslyEquipped);
        }
    RefreshInputMappings();
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
    EquippedAbilities.SetNum(GetSkillBarSlotCount());

    for (const FEquippedAbilityInfo& AbilityInfo : DefaultAbilities)
    {
        if (!AbilityInfo.AbilityClass)
            continue;

        if (AbilityInfo.Slot == ESkillSlot::MAX)
            continue; // 保护：MAX 永远不应被使用

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
                if (NewEquippedInfo.Slot != ESkillSlot::Invalid && NewEquippedInfo.Slot != ESkillSlot::MAX)
                {
                    HandleAbilitySlotEquipped(NewEquippedInfo.Slot, NewEquippedInfo);
                }
        }
    }

    // 在授予技能后，激活被动技能
    FGameplayTagContainer PassiveTags;
    PassiveTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Regen.Basic")));
    AbilitySystemComponent->TryActivateAbilitiesByTag(PassiveTags);
    
    bAbilitiesInitialized = true;
    RefreshInputMappings();
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

                SlotData.InputHint = GetInputHintForSlot(Info.Slot);

                TargetSlot->UpdateSlot(SlotData);
            }
        }
    }

    RefreshInputMappings();
        for (const FEquippedAbilityInfo& Info : EquippedAbilities)
        {
            if (Info.Slot == ESkillSlot::Invalid || Info.Slot == ESkillSlot::MAX)
            {
                continue;
            }

            if (Info.AbilityClass.IsNull())
            {
                HandleAbilitySlotUnequipped(Info.Slot, Info);
            }
            else
            {
                HandleAbilitySlotEquipped(Info.Slot, Info);
            }
        }
}

void AIsoPlayerState::OnUIInitialized()
{
    bUIInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("UI initialized in PlayerState"));

    if (AIsometricPlayerController* PC = Cast<AIsometricPlayerController>(GetPlayerController()))
    {
        if (UPlayerHUDWidget* HUD = Cast<UPlayerHUDWidget>(PC->PlayerHUDInstance))
        {
            HUD->InitializePanels(this);
        }
    }
    
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

    HUD->InitializePanels(this);
    
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

                SlotData.InputHint = GetInputHintForSlot(Info.Slot);

                TargetSlot->UpdateSlot(SlotData);
            }
        }
    }

    RefreshInputMappings();
}

void AIsoPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AIsoPlayerState, EquippedAbilities);
}

void AIsoPlayerState::Server_MoveInventoryItem_Implementation(int32 FromIndex, int32 ToIndex)
{
    if (InventoryComponent)
    {
        InventoryComponent->MoveItem(FromIndex, ToIndex);
    }
}

void AIsoPlayerState::Server_RemoveInventoryItem_Implementation(int32 InSlotIndex, int32 Quantity)
{
    if (InventoryComponent)
    {
        InventoryComponent->RemoveItemAtIndex(InSlotIndex, Quantity);
    }
}

void AIsoPlayerState::Server_UseInventoryItem_Implementation(int32 InSlotIndex)
{
    if (InventoryComponent)
    {
        InventoryComponent->UseItemAtIndex(InSlotIndex);
    }
}

void AIsoPlayerState::Server_EquipInventoryItem_Implementation(int32 InInventorySlotIndex, EEquipmentSlot InEquipmentSlot)
{
    if (EquipmentComponent)
    {
        EquipmentComponent->EquipItemFromInventory(InInventorySlotIndex, InEquipmentSlot);
    }
}

void AIsoPlayerState::Server_UnequipItem_Implementation(EEquipmentSlot InEquipmentSlot, bool bReturnToInventory)
{
    if (EquipmentComponent)
    {
        EquipmentComponent->UnequipSlot(InEquipmentSlot, bReturnToInventory);
    }
}

void AIsoPlayerState::InitializeInventory()
{
    if (!InventoryComponent)
    {
        return;
    }

    for (const FInventoryItemStack& Stack : DefaultInventoryItems)
    {
        if (Stack.Quantity <= 0 || Stack.ItemData.IsNull())
        {
            continue;
        }

        UInventoryItemData* ItemData = Stack.ItemData.IsValid()
            ? Stack.ItemData.Get()
            : Stack.ItemData.LoadSynchronous();

        if (!ItemData)
        {
            UE_LOG(LogTemp, Warning, TEXT("[IsoPlayerState] Failed to load default inventory item for %s"), *GetName());
            continue;
        }

        const int32 Remaining = InventoryComponent->AddItem(ItemData, Stack.Quantity);
        if (Remaining > 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("[IsoPlayerState] Not enough space to add default item %s (Remaining=%d)"), *ItemData->GetName(), Remaining);
        }
    }
}

void AIsoPlayerState::RefreshInputMappings()
{
    if (!AbilitySystemComponent)
    {
        return;
    }

    for (const FEquippedAbilityInfo& Info : EquippedAbilities)
    {
        if (!Info.AbilitySpecHandle.IsValid())
        {
            continue;
        }

        const EAbilityInputID InputID = TryMapSlotToInputID(Info.Slot);
        if (InputID == EAbilityInputID::None)
        {
            continue;
        }

        FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromHandle(Info.AbilitySpecHandle);
        if (!Spec)
        {
            continue;
        }

        const int32 DesiredInputID = static_cast<int32>(InputID);
        if (Spec->InputID != DesiredInputID)
        {
            Spec->InputID = DesiredInputID;
            AbilitySystemComponent->MarkAbilitySpecDirty(*Spec);
        }
    }
}

EAbilityInputID AIsoPlayerState::TryMapSlotToInputID(ESkillSlot Slot)
{
    switch (Slot)
    {
    case ESkillSlot::Skill_Q: return EAbilityInputID::Ability_Q;
    case ESkillSlot::Skill_W: return EAbilityInputID::Ability_W;
    case ESkillSlot::Skill_E: return EAbilityInputID::Ability_E;
    case ESkillSlot::Skill_R: return EAbilityInputID::Ability_R;
    case ESkillSlot::Skill_D: return EAbilityInputID::Ability_Summoner1;
    case ESkillSlot::Skill_F: return EAbilityInputID::Ability_Summoner2;
    default:
        return EAbilityInputID::None;
    }
}

FText AIsoPlayerState::GetInputHintForSlot(ESkillSlot Slot)
{
    switch (Slot)
    {
    case ESkillSlot::Skill_Q: return NSLOCTEXT("IsometricRPG", "SkillInput_Q", "Q");
    case ESkillSlot::Skill_W: return NSLOCTEXT("IsometricRPG", "SkillInput_W", "W");
    case ESkillSlot::Skill_E: return NSLOCTEXT("IsometricRPG", "SkillInput_E", "E");
    case ESkillSlot::Skill_R: return NSLOCTEXT("IsometricRPG", "SkillInput_R", "R");
    case ESkillSlot::Skill_D: return NSLOCTEXT("IsometricRPG", "SkillInput_D", "D");
    case ESkillSlot::Skill_F: return NSLOCTEXT("IsometricRPG", "SkillInput_F", "F");
    case ESkillSlot::Skill_Passive: return NSLOCTEXT("IsometricRPG", "SkillInput_Passive", "Passive");
    default:
        return FText::GetEmpty();
    }
}