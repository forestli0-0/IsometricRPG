#include "GA_HeroBaseAbility.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h" 
#include "AbilitySystemComponent.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "GameFramework/Character.h"
#include "Player/IsometricPlayerController.h"
#include "Abilities/GameplayAbilityTargetActor_GroundTrace.h"
#include "Abilities/GameplayAbilityTargetActor_SingleLineTrace.h"
#include "AbilitySystemGlobals.h" 

UGA_HeroBaseAbility::UGA_HeroBaseAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    TargetActorClass = nullptr;
    // 默认为目标指向型技能
    AbilityType = EHeroAbilityType::Targeted;
}



bool UGA_HeroBaseAbility::RequiresTargetData_Implementation() const
{
    return bRequiresTargetData;
}

void UGA_HeroBaseAbility::ExecuteSkill(
    const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, 
    const FGameplayEventData* TriggerEventData)
{
    // 基类中的默认实现 - 子类应该重写这个方法
    UE_LOG(LogTemp, Log, TEXT("%s：已调用基本ExecuteSkill。这应该被子类覆盖."), *GetName());
    
    // 如果没有动画蒙太奇，则直接结束技能
    if (!MontageToPlay)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}

void UGA_HeroBaseAbility::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    // 保存当前技能信息以便子类访问
    CurrentSpecHandle = Handle;
    CurrentActorInfo = ActorInfo;
    CurrentActivationInfo = ActivationInfo;

    if (TriggerEventData)
    {
        CurrentTriggerEventData = TriggerEventData;
    }

    // 获取Avatar Actor
    AActor* SelfActor = GetAvatarActorFromActorInfo();
    if (!SelfActor)
    {
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        UE_LOG(LogTemp, Error, TEXT("%s: SelfActor is null."), *GetName());
        return;
    }

    // 获取AbilitySystemComponent
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC)
    {
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        UE_LOG(LogTemp, Error, TEXT("%s: AbilitySystemComponent is null in ActivateAbility."), *GetName());
        return;
    }

    // 获取属性集
    AttributeSet = const_cast<UIsometricRPGAttributeSetBase*>(ASC->GetSet<UIsometricRPGAttributeSetBase>());
    if (!AttributeSet)
    {
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        UE_LOG(LogTemp, Error, TEXT("%s: AttributeSet is null in ActivateAbility."), *GetName());
        return;
    }

    // 根据技能是否需要目标选择来决定执行流程
    if (RequiresTargetData())
    {
        // 如果需要目标选择但没有设置TargetActorClass
        if (!TargetActorClass)
        {
            UE_LOG(LogTemp, Error, TEXT("%s: TargetActorClass is not set but skill requires target data!"), *GetName());
            CancelAbility(Handle, ActorInfo, ActivationInfo, true);
            return;
        }
        
        // 开始目标选择
        StartTargetSelection(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    }
    else
    {
        // 不需要目标选择，直接执行技能
        DirectExecuteAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    }
}

void UGA_HeroBaseAbility::StartTargetSelection(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{

    if (!TargetActorClass)
    {
        UE_LOG(LogTemp, Error, TEXT("'%s': TargetActorClass is NOT SET! Cancelling."), *GetName()); // <-- 添加
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return;
    }


    TargetDataTask = UAbilityTask_WaitTargetData::WaitTargetData(
        this,
        FName("TargetData"),
        EGameplayTargetingConfirmation::UserConfirmed,
        TargetActorClass
    );

    if (TargetDataTask)
    {

        TargetDataTask->ValidData.AddDynamic(this, &UGA_HeroBaseAbility::OnTargetDataReady);
        TargetDataTask->Cancelled.AddDynamic(this, &UGA_HeroBaseAbility::OnTargetingCancelled);

        AGameplayAbilityTargetActor* SpawnedActor = nullptr;

        TargetDataTask->BeginSpawningActor(this, TargetActorClass, SpawnedActor);
        TargetDataTask->FinishSpawningActor(this, SpawnedActor);

        TargetDataTask->ReadyForActivation();

    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("'%s': FAILED to create TargetDataTask!"), *GetName()); // <-- 添加
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
    }
}

void UGA_HeroBaseAbility::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
    // 保存目标数据
    CurrentTargetDataHandle = Data;
    
    if (TargetDataTask)
    {
        TargetDataTask->EndTask();
    }
    TargetDataTask = nullptr;

    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
    FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
    FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();

    OtherCheckBeforeCommit(Data);
    UE_LOG(LogTemp, Warning, TEXT("%s: 其他检查失败，与消耗和冷却无关。"), *GetName());
	constexpr bool bReplicateEndAbility = true;
    constexpr bool bWasCancelled = true;

    return;
}
bool UGA_HeroBaseAbility::OtherCheckBeforeCommit(const FGameplayAbilityTargetDataHandle& Data) const
{
    // 这里可以添加其他检查逻辑，例如检查目标数据的有效性等
    return true;
}

bool UGA_HeroBaseAbility::OtherCheckBeforeCommit(const FGameplayEventData* TriggerEventData) const
{
    return true;
}

void UGA_HeroBaseAbility::PlayAbilityMontage(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
    if (MontageToPlay && IsValid(AnimInstance))
    {
        float PlayRate = CooldownDuration > 0 && MontageToPlay->GetPlayLength() > 0 ? 
            MontageToPlay->GetPlayLength() / CooldownDuration : 1.0f;
        PlayRate = FMath::Clamp(PlayRate, 1.f, 5.0f);

        MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, MontageToPlay, PlayRate);

        if (MontageTask)
        {
            MontageTask->OnCompleted.AddDynamic(this, &UGA_HeroBaseAbility::HandleMontageCompleted);
            MontageTask->OnBlendOut.AddDynamic(this, &UGA_HeroBaseAbility::HandleMontageCompleted);
            MontageTask->OnInterrupted.AddDynamic(this, &UGA_HeroBaseAbility::HandleMontageInterruptedOrCancelled);
            MontageTask->OnCancelled.AddDynamic(this, &UGA_HeroBaseAbility::HandleMontageInterruptedOrCancelled);
            MontageTask->ReadyForActivation();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Failed to create MontageTask."), *GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("%s: No montage to play or invalid anim instance."), *GetName());
    }
}

void UGA_HeroBaseAbility::DirectExecuteAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!OtherCheckBeforeCommit(TriggerEventData))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 其他检查失败，与消耗和冷却无关。"), *GetName());
        constexpr bool bReplicateEndAbility = true;
        return;
    }
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Failed to commit ability for direct execution."), *GetName());
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return;
    }
    
    // 播放技能动画
    PlayAbilityMontage(Handle, ActorInfo, ActivationInfo);
    
    // 执行技能逻辑
    ExecuteSkill(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_HeroBaseAbility::OnTargetingCancelled(const FGameplayAbilityTargetDataHandle& Data)
{
    if (TargetDataTask)
    {
        TargetDataTask->EndTask();
    }
    TargetDataTask = nullptr;

    UE_LOG(LogTemp, Display, TEXT("%s: Targeting cancelled."), *GetName());
    CancelAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true);
}

void UGA_HeroBaseAbility::HandleMontageCompleted()
{
    UE_LOG(LogTemp, Display, TEXT("%s: Montage completed or blended out. Ending ability."), *GetName());
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_HeroBaseAbility::HandleMontageInterruptedOrCancelled()
{
    UE_LOG(LogTemp, Display, TEXT("%s: Montage interrupted or cancelled. Cancelling ability."), *GetName());
    CancelAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true);
}

void UGA_HeroBaseAbility::ApplyCooldown(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo) const
{
    // 使用自定义冷却效果类或退回到基类实现
    if (CooldownGameplayEffectClass)
    {
        UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
        if (ASC)
        {
            FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownGameplayEffectClass, GetAbilityLevel());
            if (SpecHandle.Data.IsValid())
            {
                FGameplayEffectSpec& GESpec = *SpecHandle.Data.Get();
                
                // 寻找冷却时间标签
                FGameplayTag CooldownDurationTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Cooldown.Duration"));
                if (CooldownDurationTag.IsValid())
                {
                    // 设置冷却时间
                    GESpec.SetSetByCallerMagnitude(CooldownDurationTag, CooldownDuration);
                }
                
                ASC->ApplyGameplayEffectSpecToSelf(GESpec);
                return;
            }
        }
    }
    
    // 退回到基类实现
    Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
}

bool UGA_HeroBaseAbility::CheckCost(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    // 获取AbilitySystemComponent
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: AbilitySystemComponent is null in ActivateAbility."), *GetName());
        return false;
    }

    // 获取属性集
    auto AttriSet = const_cast<UIsometricRPGAttributeSetBase*>(ASC->GetSet<UIsometricRPGAttributeSetBase>());
    if (!AttriSet)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: AttributeSet is null in ActivateAbility."), *GetName());
        return false;
    }
    // 获取当前资源值（比如 Mana）
    float CurrentMana = AttriSet->GetMana(); // 你需要有这个 Getter

    // 假设我们从 Ability 属性里设定 CostMagnitude
    if (CurrentMana < CostMagnitude)
    {
        if (OptionalRelevantTags)
        {
            OptionalRelevantTags->AddTag(UAbilitySystemGlobals::Get().ActivateFailCostTag);
        }
        return false;
    }

    return true;
}


void UGA_HeroBaseAbility::ApplyCost(
    const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo) const
{
    // 使用自定义消耗效果类或退回到基类实现
    if (CostGameplayEffectClass)
    {
        UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
        if (ASC)
        {
            FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CostGameplayEffectClass, GetAbilityLevel());
            if (SpecHandle.Data.IsValid())
            {
                FGameplayEffectSpec& GESpec = *SpecHandle.Data.Get();
                
                // 寻找消耗标签
                FGameplayTag CostTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Cost.Magnitude"));
                if (CostTag.IsValid())
                {
                    // 设置消耗值
                    GESpec.SetSetByCallerMagnitude(CostTag, CostMagnitude);
                }
                
                // 应用消耗效果
                ASC->ApplyGameplayEffectSpecToSelf(GESpec);
                return;
			}
            if (SpecHandle.Data.IsValid())
            {
                ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
                return;
            }
        }
    }
    else
    {
        // 退回到基类实现
        Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
    }
}

void UGA_HeroBaseAbility::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (MontageTask)
    {
        if (MontageTask->IsActive())
        {
            MontageTask->EndTask();
        }
    }
    MontageTask = nullptr;

    if (TargetDataTask)
    {
        if (TargetDataTask->IsActive())
        {
            TargetDataTask->EndTask();
        }
    }
    TargetDataTask = nullptr;

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
