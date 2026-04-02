#include "GA_HeroBaseAbility.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h" 
#include "AbilitySystemComponent.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "GameFramework/Character.h"
#include "Player/IsometricPlayerController.h"
#include "Abilities/GameplayAbilityTargetActor_GroundTrace.h"
#include "Abilities/GameplayAbilityTargetActor_SingleLineTrace.h"
#include "AbilitySystemGlobals.h" 
#include <Character/IsometricRPGCharacter.h>
#include "IsometricAbilities/GameplayAbilities/HeroAbilityCommitHelper.h"
#include "IsometricAbilities/GameplayAbilities/HeroAbilityMontageHelper.h"
#include "IsometricAbilities/GameplayAbilities/HeroAbilityTargetingHelper.h"

namespace
{
template <typename TTask>
void EndAbilityTaskIfActive(TTask*& Task)
{
    if (!Task)
    {
        return;
    }

    if (Task->IsActive())
    {
        Task->EndTask();
    }

    Task = nullptr;
}

}

UGA_HeroBaseAbility::UGA_HeroBaseAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
    TargetActorClass = nullptr;
    // 默认为目标指向型技能
    AbilityType = EHeroAbilityType::Targeted;
}

bool UGA_HeroBaseAbility::ExpectsActorTargetData() const
{
    return AbilityType == EHeroAbilityType::Targeted;
}

bool UGA_HeroBaseAbility::ExpectsLocationTargetData() const
{
    return AbilityType == EHeroAbilityType::SkillShot || AbilityType == EHeroAbilityType::AreaEffect;
}

bool UGA_HeroBaseAbility::ExpectsPreparedTargetData() const
{
    return ExpectsActorTargetData() || ExpectsLocationTargetData();
}

bool UGA_HeroBaseAbility::UsesInteractiveTargeting() const
{
    return bUseInteractiveTargeting;
}

FText UGA_HeroBaseAbility::GetAbilityDisplayNameText() const
{
    if (!AbilityDisplayName.IsEmpty())
    {
        return AbilityDisplayName;
    }

    return GetClass() ? GetClass()->GetDisplayNameText() : FText::FromString(GetName());
}

float UGA_HeroBaseAbility::GetResourceCost() const
{
    return CostMagnitude;
}

FAbilityInputPolicy UGA_HeroBaseAbility::GetAbilityInputPolicy_Implementation() const
{
    return InputPolicy;
}

float UGA_HeroBaseAbility::GetCurrentHeldDuration() const
{
    return CurrentInputContext.HeldDuration;
}

EInputEventPhase UGA_HeroBaseAbility::GetCurrentInputTriggerPhase() const
{
    return CurrentInputContext.TriggerPhase;
}

FVector UGA_HeroBaseAbility::GetCurrentAimPoint() const
{
    return CurrentInputContext.AimPoint;
}

FVector UGA_HeroBaseAbility::GetCurrentAimDirection() const
{
    return CurrentInputContext.AimDirection;
}

void UGA_HeroBaseAbility::SetUsesInteractiveTargeting(const bool bEnabled)
{
    bUseInteractiveTargeting = bEnabled;
}

FHeroAbilityTargetingPolicy UGA_HeroBaseAbility::BuildTargetingPolicy() const
{
    return FHeroAbilityTargetingPolicy{
        AbilityType,
        ExpectsPreparedTargetData(),
        ExpectsLocationTargetData(),
        ExpectsActorTargetData(),
        GetCurrentAimDirection(),
        NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalPredicted
    };
}

bool UGA_HeroBaseAbility::InitializeActivationRuntimeState(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    CurrentSpecHandle = Handle;
    CurrentActorInfo = ActorInfo;
    CurrentActivationInfo = ActivationInfo;

    AIsometricRPGCharacter* RPGCharacter = Cast<AIsometricRPGCharacter>(GetAvatarActorFromActorInfo());
    if (!RPGCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: AvatarActor is not an AIsometricRPGCharacter. Cannot get target data."), *GetName());
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return false;
    }

    AActor* SelfActor = GetAvatarActorFromActorInfo();
    if (!SelfActor)
    {
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        UE_LOG(LogTemp, Error, TEXT("%s: SelfActor is null."), *GetName());
        return false;
    }

    UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
    if (!AbilitySystemComponent)
    {
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        UE_LOG(LogTemp, Error, TEXT("%s: AbilitySystemComponent is null in ActivateAbility."), *GetName());
        return false;
    }

    AttributeSet = const_cast<UIsometricRPGAttributeSetBase*>(AbilitySystemComponent->GetSet<UIsometricRPGAttributeSetBase>());
    if (!AttributeSet)
    {
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        UE_LOG(LogTemp, Error, TEXT("%s: AttributeSet is null in ActivateAbility."), *GetName());
        return false;
    }

    CurrentInputContext = RPGCharacter->GetPendingAbilityActivationContext();
    CurrentTargetDataHandle = RPGCharacter->GetAbilityTargetData();
    return true;
}

// =================================================================================================================
// Gampelay Ability Core
// =================================================================================================================

void UGA_HeroBaseAbility::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (ActorInfo && ActorInfo->IsNetAuthority()) { GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("ActivateAbility: %s"), *GetName())); }
    if (!InitializeActivationRuntimeState(Handle, ActorInfo, ActivationInfo))
    {
        return;
    }

    if (ShouldWaitForReplicatedTargetData(ActorInfo, ActivationInfo))
    {
        WaitForReplicatedTargetData(Handle, ActorInfo, ActivationInfo);
        return;
    }

    SendInitialTargetDataToServer(Handle, ActorInfo, ActivationInfo);

    if (ActorInfo->IsNetAuthority() && !ServerValidateTargetData(CurrentTargetDataHandle, ActorInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Server rejected client target data. Cancelling."), *GetName());
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return;
    }

    ContinueAbilityActivation(Handle, ActorInfo, ActivationInfo);
}

void UGA_HeroBaseAbility::ContinueAbilityActivation(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    if (UsesInteractiveTargeting())
    {
        if (HasRequiredExecutionTargetData(CurrentTargetDataHandle))
        {
            OnTargetDataReady(CurrentTargetDataHandle);
        }
        else
        {
            StartTargetSelection(Handle, ActorInfo, ActivationInfo);
        }
    }
    else
    {
        if (!ValidateExecutionTargetData(CurrentTargetDataHandle, TEXT("DirectExecute")))
        {
            CancelAbility(Handle, ActorInfo, ActivationInfo, true);
            return;
        }
        DirectExecuteAbility(Handle, ActorInfo, ActivationInfo);
    }
}

bool UGA_HeroBaseAbility::ShouldWaitForReplicatedTargetData(
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo& ActivationInfo) const
{
    const FHeroAbilityTargetingPolicy Policy = BuildTargetingPolicy();
    return FHeroAbilityTargetingHelper::ShouldWaitForReplicatedTargetData(Policy, ActorInfo, ActivationInfo);
}

void UGA_HeroBaseAbility::SendInitialTargetDataToServer(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo& ActivationInfo)
{
    const FHeroAbilityTargetingPolicy Policy = BuildTargetingPolicy();
    FHeroAbilityTargetingHelper::SendInitialTargetDataToServer(Policy, Handle, ActorInfo, ActivationInfo, CurrentTargetDataHandle);
}

void UGA_HeroBaseAbility::WaitForReplicatedTargetData(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo& ActivationInfo)
{
    UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    if (!ASC || !Handle.IsValid())
    {
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return;
    }

    CleanupReplicatedTargetDataDelegates();

    const FPredictionKey ActivationPredictionKey = ActivationInfo.GetActivationPredictionKey();
    ReplicatedTargetDataDelegateHandle = ASC->AbilityTargetDataSetDelegate(Handle, ActivationPredictionKey)
        .AddUObject(this, &UGA_HeroBaseAbility::HandleReplicatedTargetDataReceived);
    ReplicatedTargetDataCancelledDelegateHandle = ASC->AbilityTargetDataCancelledDelegate(Handle, ActivationPredictionKey)
        .AddUObject(this, &UGA_HeroBaseAbility::HandleReplicatedTargetDataCancelled);

    if (!ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, ActivationPredictionKey))
    {
        UE_LOG(LogTemp, Verbose, TEXT("%s: Waiting for replicated target data."), *GetName());
    }
}

void UGA_HeroBaseAbility::CleanupReplicatedTargetDataDelegates()
{
    UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
    FHeroAbilityTargetingHelper::CleanupReplicatedTargetDataDelegates(
        AbilitySystemComponent,
        CurrentSpecHandle,
        CurrentActivationInfo,
        ReplicatedTargetDataDelegateHandle,
        ReplicatedTargetDataCancelledDelegateHandle);
}

void UGA_HeroBaseAbility::HandleReplicatedTargetDataReceived(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ActivationTag)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (ASC && CurrentSpecHandle.IsValid())
    {
        ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
    }

    CleanupReplicatedTargetDataDelegates();
    CurrentTargetDataHandle = Data;

    if (AIsometricRPGCharacter* RPGCharacter = Cast<AIsometricRPGCharacter>(GetAvatarActorFromActorInfo()))
    {
        FPendingAbilityActivationContext Context = RPGCharacter->GetPendingAbilityActivationContext();
        Context.TargetData = Data;
        RPGCharacter->SetPendingAbilityActivationContext(Context);
        CurrentInputContext = RPGCharacter->GetPendingAbilityActivationContext();
    }

    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
    if (!ActorInfo)
    {
        CancelAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true);
        return;
    }

    if (!ServerValidateTargetData(CurrentTargetDataHandle, ActorInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Server rejected replicated target data. Cancelling."), *GetName());
        CancelAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true);
        return;
    }

    ContinueAbilityActivation(CurrentSpecHandle, ActorInfo, CurrentActivationInfo);
}

void UGA_HeroBaseAbility::HandleReplicatedTargetDataCancelled()
{
    CleanupReplicatedTargetDataDelegates();
    UE_LOG(LogTemp, Warning, TEXT("%s: Replicated target data cancelled."), *GetName());
    CancelAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true);
}

bool UGA_HeroBaseAbility::HasRequiredExecutionTargetData(const FGameplayAbilityTargetDataHandle& TargetData) const
{
    const FHeroAbilityTargetingPolicy Policy = BuildTargetingPolicy();
    return FHeroAbilityTargetingHelper::HasRequiredExecutionTargetData(Policy, TargetData);
}

bool UGA_HeroBaseAbility::ValidateAbilityConfiguration() const
{
    if (GetClass()->HasAnyClassFlags(CLASS_Abstract))
    {
        return true;
    }

    const bool bInteractiveTargetingEnabled = UsesInteractiveTargeting();

    switch (AbilityType)
    {
    case EHeroAbilityType::Targeted:
        return ensureAlwaysMsgf(
            bInteractiveTargetingEnabled,
            TEXT("%s: Targeted abilities must keep interactive targeting enabled so they can reuse an existing target or enter the confirmation flow."),
            *GetName())
            && ensureAlwaysMsgf(
                TargetActorClass != nullptr,
                TEXT("%s: Targeted abilities must configure TargetActorClass for the interactive targeting flow."),
                *GetName());

    case EHeroAbilityType::SelfCast:
    case EHeroAbilityType::Passive:
        return ensureAlwaysMsgf(
            !bInteractiveTargetingEnabled,
            TEXT("%s: %s abilities must keep interactive targeting disabled."),
            *GetName(),
            FHeroAbilityTargetingHelper::DescribeAbilityType(AbilityType));

    default:
        return true;
    }
}

bool UGA_HeroBaseAbility::ValidateExecutionTargetData(const FGameplayAbilityTargetDataHandle& TargetData, const TCHAR* Phase) const
{
    const FHeroAbilityTargetingPolicy Policy = BuildTargetingPolicy();
    return FHeroAbilityTargetingHelper::ValidateExecutionTargetData(*this, Policy, TargetData, Phase);
}

bool UGA_HeroBaseAbility::CheckCost(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    return FHeroAbilityCommitHelper::CheckCost(
        *this,
        Handle,
        ActorInfo,
        CostGameplayEffectClass,
        CostMagnitude,
        OptionalRelevantTags);
}


void UGA_HeroBaseAbility::ApplyCost(
    const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo) const
{
    FHeroAbilityCommitHelper::ApplyCost(
        *this,
        Handle,
        ActorInfo,
        ActivationInfo,
        CostGameplayEffectClass,
        CostMagnitude,
        GetAbilityLevel(Handle, ActorInfo));
}

void UGA_HeroBaseAbility::ApplyCooldown(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo) const
{
    if (!FHeroAbilityCommitHelper::ApplyCooldown(
        *this,
        ActorInfo,
        CooldownGameplayEffectClass,
        CooldownDuration,
        GetAbilityLevel(Handle, ActorInfo)))
    {
        Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
    }

    NotifyCooldownTriggered(Handle, ActorInfo);
}

void UGA_HeroBaseAbility::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    UE_LOG(LogTemp, Display, TEXT("%s: EndAbility called. bWasCancelled=%s"), *GetName(), bWasCancelled ? TEXT("true") : TEXT("false"));
    ResetRuntimeAbilityState();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// =================================================================================================================
// Ability Execution
// =================================================================================================================

void UGA_HeroBaseAbility::StartTargetSelection(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo)
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

void UGA_HeroBaseAbility::DirectExecuteAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    TryCommitAndExecuteAbility(
        Handle,
        ActorInfo,
        ActivationInfo,
        TEXT("DirectExecute"),
        TEXT("提交前的检查失败，与消耗和冷却无关。"));
}

bool UGA_HeroBaseAbility::OtherCheckBeforeCommit()
{
    // 基类中的默认实现 - 子类应该重写这个方法
    UE_LOG(LogTemp, Log, TEXT("%s：检查除Cost和Cooldown之外的条件。这应该被子类覆盖."), *GetName());
    return true;
}


void UGA_HeroBaseAbility::ExecuteSkill(
    const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    // 基类中的默认实现 - 子类应该重写这个方法
    UE_LOG(LogTemp, Log, TEXT("%s：已调用基本ExecuteSkill。这应该被子类覆盖."), *GetName());
    
    // 如果没有动画蒙太奇，则直接结束技能
    if (!MontageToPlay)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}

void UGA_HeroBaseAbility::PlayAbilityMontage(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    // 如果关闭了基类蒙太奇流程，则不做任何事（由子类自主管理动画与收尾）
    if (!bUseBaseMontageFlow)
    {
        UE_LOG(LogTemp, Verbose, TEXT("%s: Base montage flow disabled, skipping PlayAbilityMontage."), *GetName());
        return;
    }
    UAnimInstance* AnimInstance = CurrentActorInfo->GetAnimInstance();
    if (MontageToPlay && IsValid(AnimInstance))
    {
        const float PlayRate = FHeroAbilityMontageHelper::ResolvePlayRate(MontageToPlay, CooldownDuration);

        if (bFaceTarget)
        {
            if (ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
            {
                if (!FHeroAbilityMontageHelper::TryFaceCharacterToCurrentTarget(*Character, GetCurrentAimDirection(), CurrentTargetDataHandle))
                {
                    UE_LOG(LogTemp, Verbose, TEXT("%s: No target data to face when playing montage."), *GetName());
                }
            }
        }

        // 创建并配置蒙太奇任务
        MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, MontageToPlay, PlayRate);
        if (MontageTask)
        {
            UE_LOG(LogTemp, Verbose, TEXT("%s: PlayMontage %s at rate %.2f"), *GetName(), *MontageToPlay->GetName(), PlayRate);
            BindMontageTaskCallbacks(*MontageTask);
            MontageTask->ReadyForActivation();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Failed to create MontageTask."), *GetName());
        }
    }
    else
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("No montage to play or invalid anim instance in %s."), *GetName()));
    }
}

// =================================================================================================================
// Callbacks
// =================================================================================================================

void UGA_HeroBaseAbility::ResetRuntimeAbilityState()
{
    CleanupReplicatedTargetDataDelegates();
    EndMontageTask();
    EndTargetDataTask();
    CurrentInputContext = FPendingAbilityActivationContext();
}

void UGA_HeroBaseAbility::EndTargetDataTask()
{
    EndAbilityTaskIfActive(TargetDataTask);
}

void UGA_HeroBaseAbility::EndMontageTask()
{
    EndAbilityTaskIfActive(MontageTask);
}

void UGA_HeroBaseAbility::BindMontageTaskCallbacks(UAbilityTask_PlayMontageAndWait& InMontageTask)
{
    if (bEndAbilityWhenBaseMontageEnds)
    {
        InMontageTask.OnCompleted.AddDynamic(this, &UGA_HeroBaseAbility::HandleMontageCompleted);
        InMontageTask.OnBlendOut.AddDynamic(this, &UGA_HeroBaseAbility::HandleMontageCompleted);
    }

    if (bCancelAbilityWhenBaseMontageInterrupted)
    {
        InMontageTask.OnInterrupted.AddDynamic(this, &UGA_HeroBaseAbility::HandleMontageInterruptedOrCancelled);
        InMontageTask.OnCancelled.AddDynamic(this, &UGA_HeroBaseAbility::HandleMontageInterruptedOrCancelled);
    }
}

bool UGA_HeroBaseAbility::TryCommitAndExecuteAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const TCHAR* Phase,
    const TCHAR* OtherCheckFailureReason)
{
    if (!OtherCheckBeforeCommit())
    {
        if (OtherCheckFailureReason)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: %s"), *GetName(), OtherCheckFailureReason);
        }
        return false;
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        LogCommitFailureDiagnostics(Handle, ActorInfo, ActivationInfo, Phase);
        UE_LOG(LogTemp, Warning, TEXT("%s: 技能提交失败，可能是由于消耗或冷却问题。"), *GetName());
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return false;
    }

    UE_LOG(LogTemp, Verbose, TEXT("%s: CommitAbility OK (%s). Montage=%s"), *GetName(), Phase, MontageToPlay ? *MontageToPlay->GetName() : TEXT("None"));
    PlayAbilityMontage(Handle, ActorInfo, ActivationInfo);
    ExecuteSkill(Handle, ActorInfo, ActivationInfo);
    return true;
}

void UGA_HeroBaseAbility::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
    // 保存目标数据
    CurrentTargetDataHandle = Data;

    EndTargetDataTask();

    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
    FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
    FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();

    TryCommitAndExecuteAbility(
        Handle,
        ActorInfo,
        ActivationInfo,
        TEXT("OnTargetDataReady"),
        TEXT("超出范围，向目标移动······"));
}

void UGA_HeroBaseAbility::OnTargetingCancelled(const FGameplayAbilityTargetDataHandle& Data)
{
    EndTargetDataTask();

    UE_LOG(LogTemp, Display, TEXT("%s: Targeting cancelled."), *GetName());
    CancelAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true);
}

void UGA_HeroBaseAbility::HandleMontageCompleted()
{
    if (!bEndAbilityWhenBaseMontageEnds)
    {
        UE_LOG(LogTemp, Verbose, TEXT("%s: Montage completed, but auto-end disabled."), *GetName());
        return;
    }
    UE_LOG(LogTemp, Display, TEXT("%s: Montage completed or blended out. Ending ability (base flow)."), *GetName());
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_HeroBaseAbility::HandleMontageInterruptedOrCancelled()
{
    if (!bCancelAbilityWhenBaseMontageInterrupted)
    {
        UE_LOG(LogTemp, Verbose, TEXT("%s: Montage interrupted/cancelled, but auto-cancel disabled."), *GetName());
        return;
    }
    UE_LOG(LogTemp, Display, TEXT("%s: Montage interrupted or cancelled. Cancelling ability (base flow)."), *GetName());
    CancelAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true);
}

void UGA_HeroBaseAbility::CancelAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateCancelAbility)
{
    UE_LOG(LogTemp, Warning, TEXT("%s: CancelAbility invoked. Replicate=%s. MontageTaskActive=%s"),
        *GetName(), bReplicateCancelAbility ? TEXT("true") : TEXT("false"),
        (MontageTask && MontageTask->IsActive()) ? TEXT("true") : TEXT("false"));

    ResetRuntimeAbilityState();

    Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void UGA_HeroBaseAbility::NotifyCooldownTriggered(const FGameplayAbilitySpecHandle& Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
    FHeroAbilityCommitHelper::NotifyCooldownTriggered(Handle, ActorInfo, CooldownDuration);
}

FGameplayEffectSpecHandle UGA_HeroBaseAbility::MakeCostGameplayEffectSpecHandle(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo) const
{
    return FHeroAbilityCommitHelper::MakeCostGameplayEffectSpecHandle(
        *this,
        Handle,
        ActorInfo,
        ActivationInfo,
        CostGameplayEffectClass,
        CostMagnitude,
        GetAbilityLevel(Handle, ActorInfo));
}

float UGA_HeroBaseAbility::ResolveManaCostFromSpec(const FGameplayEffectSpec& CostSpec) const
{
    return FHeroAbilityCommitHelper::ResolveManaCostFromSpec(*this, CostSpec);
}

bool UGA_HeroBaseAbility::HasMeaningfulCost() const
{
    return FHeroAbilityCommitHelper::HasMeaningfulCost(CostMagnitude);
}

void UGA_HeroBaseAbility::LogCommitFailureDiagnostics(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const TCHAR* Phase) const
{
    if (!ActorInfo)
    {
        return;
    }

    UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    if (!ASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: CommitAbility failed at %s, ASC is null."), *GetName(), Phase);
        return;
    }

    const UIsometricRPGAttributeSetBase* LocalAttributeSet = ASC->GetSet<UIsometricRPGAttributeSetBase>();
    const float CurrentMana = LocalAttributeSet ? LocalAttributeSet->GetMana() : -1.f;
    const float MaxMana = LocalAttributeSet ? LocalAttributeSet->GetMaxMana() : -1.f;

    FGameplayTagContainer CostFailureTags;
    const bool bCostOk = CheckCost(Handle, ActorInfo, &CostFailureTags);

    FGameplayTagContainer CooldownFailureTags;
    const bool bCooldownOk = CheckCooldown(Handle, ActorInfo, &CooldownFailureTags);

    FGameplayTagContainer OwnedTags;
    ASC->GetOwnedGameplayTags(OwnedTags);

    FString CooldownTagsText = TEXT("(None)");
    FString MatchedOwnedCooldownTagsText = TEXT("(None)");
    if (const FGameplayTagContainer* CooldownTags = GetCooldownTags())
    {
        CooldownTagsText = FHeroAbilityCommitHelper::DescribeTagContainer(*CooldownTags);
        MatchedOwnedCooldownTagsText = FHeroAbilityCommitHelper::DescribeMatchedOwnedCooldownTags(CooldownTags, OwnedTags);
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s: CommitAbility failed at %s. CostOk=%s CooldownOk=%s Mana=%.2f/%.2f CostMagnitude=%.2f CooldownDuration=%.2f CooldownTags=[%s] MatchedOwnedCooldownTags=[%s] CostFailureTags=[%s] CooldownFailureTags=[%s] OwnedTags=[%s]"),
        *GetName(),
        Phase,
        bCostOk ? TEXT("true") : TEXT("false"),
        bCooldownOk ? TEXT("true") : TEXT("false"),
        CurrentMana,
        MaxMana,
        GetResourceCost(),
        CooldownDuration,
        *CooldownTagsText,
        *MatchedOwnedCooldownTagsText,
        *FHeroAbilityCommitHelper::DescribeTagContainer(CostFailureTags),
        *FHeroAbilityCommitHelper::DescribeTagContainer(CooldownFailureTags),
        *FHeroAbilityCommitHelper::DescribeTagContainer(OwnedTags));
}

bool UGA_HeroBaseAbility::ServerValidateTargetData(
    const FGameplayAbilityTargetDataHandle& TargetData,
    const FGameplayAbilityActorInfo* ActorInfo) const
{
    // 客户端不做校验，保持预测流畅
    if (!ActorInfo || !ActorInfo->IsNetAuthority())
    {
        return true;
    }

    // 不依赖预制目标数据的技能（SelfCast / Passive）直接通过
    if (!ExpectsPreparedTargetData())
    {
        return true;
    }

    if (bForcePredictionFailureForTests
        && NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalPredicted)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: ServerValidate forced a prediction failure for testing."), *GetName());
        return false;
    }

    const AActor* AvatarActor = ActorInfo->AvatarActor.Get();
    if (!AvatarActor)
    {
        return false;
    }

    // 从 TargetData 中提取目标 Actor
    AActor* TargetActor = nullptr;
    FVector TargetLocation = FVector::ZeroVector;

    const FGameplayAbilityTargetData* Data = TargetData.Get(0);
    if (!Data)
    {
        // 没有目标数据——后续 StartTargetSelection 会处理
        return true;
    }

    if (Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_ActorArray::StaticStruct()))
    {
        const auto* ActorArrayData = static_cast<const FGameplayAbilityTargetData_ActorArray*>(Data);
        if (ActorArrayData && ActorArrayData->TargetActorArray.Num() > 0)
        {
            TargetActor = ActorArrayData->TargetActorArray[0].Get();
        }
    }
    else if (Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_SingleTargetHit::StaticStruct()))
    {
        const auto* HitResultData = static_cast<const FGameplayAbilityTargetData_SingleTargetHit*>(Data);
        if (HitResultData && HitResultData->GetHitResult())
        {
            TargetActor = HitResultData->GetHitResult()->GetActor();
            TargetLocation = HitResultData->GetHitResult()->Location;
        }
    }

    if (!TargetActor)
    {
        // 只有位置数据（如 AreaEffect），基类不做额外限制
        return true;
    }

    // 检查目标 Actor 是否仍然存活
    if (TargetActor->IsPendingKillPending())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: ServerValidate FAILED - target %s is pending kill."),
            *GetName(), *TargetActor->GetName());
        return false;
    }

    // 距离校验（仅当 ServerMaxTargetDistance > 0 时启用）
    if (ServerMaxTargetDistance > 0.f)
    {
        const float Distance = FVector::Dist(AvatarActor->GetActorLocation(), TargetActor->GetActorLocation());
        if (Distance > ServerMaxTargetDistance)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: ServerValidate FAILED - target %s at distance %.1f exceeds max %.1f."),
                *GetName(), *TargetActor->GetName(), Distance, ServerMaxTargetDistance);
            return false;
        }
    }

    return true;
}

void UGA_HeroBaseAbility::PostInitProperties()
{
    Super::PostInitProperties();

    // 仅在CDO上做一次触发器注入，避免实例期重复添加
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        ValidateAbilityConfiguration();

        if (TriggerTag.IsValid())
        {
            bool bAlreadyAdded = false;
            for (const FAbilityTriggerData& T : AbilityTriggers)
            {
                if (T.TriggerSource == EGameplayAbilityTriggerSource::GameplayEvent &&
                    T.TriggerTag.MatchesTagExact(TriggerTag))
                {
                    bAlreadyAdded = true;
                    break;
                }
            }
            if (!bAlreadyAdded)
            {
                FAbilityTriggerData Data;
                Data.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
                Data.TriggerTag = TriggerTag;
                AbilityTriggers.Add(Data);
            }
        }
    }
}
