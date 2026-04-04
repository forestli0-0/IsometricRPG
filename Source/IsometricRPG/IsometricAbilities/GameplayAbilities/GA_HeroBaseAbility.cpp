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
#include "IsometricAbilities/GameplayAbilities/HeroAbilityFlowHelper.h"
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

void UGA_HeroBaseAbility::AddAssetTagIfMissing(const FGameplayTag& Tag)
{
    if (!Tag.IsValid())
    {
        return;
    }

    FGameplayTagContainer AssetTagContainer = GetAssetTags();
    if (!AssetTagContainer.HasTagExact(Tag))
    {
        AssetTagContainer.AddTag(Tag);
        SetAssetTags(AssetTagContainer);
    }
}

void UGA_HeroBaseAbility::AddOwnedTagIfMissing(const FGameplayTag& Tag)
{
    if (Tag.IsValid() && !ActivationOwnedTags.HasTagExact(Tag))
    {
        ActivationOwnedTags.AddTag(Tag);
    }
}

void UGA_HeroBaseAbility::AddBlockedTagIfMissing(const FGameplayTag& Tag)
{
    if (Tag.IsValid() && !ActivationBlockedTags.HasTagExact(Tag))
    {
        ActivationBlockedTags.AddTag(Tag);
    }
}

void UGA_HeroBaseAbility::SetGameplayEventTriggerTag(const FGameplayTag& Tag)
{
    if (Tag.IsValid())
    {
        TriggerTag = Tag;
    }
}

void UGA_HeroBaseAbility::ConfigureAbilityIdentityTag(
    const FGameplayTag& AbilityTag,
    const bool bAddOwnedTag,
    const bool bUseAsGameplayEventTrigger)
{
    AddAssetTagIfMissing(AbilityTag);

    if (bAddOwnedTag)
    {
        AddOwnedTagIfMissing(AbilityTag);
    }

    if (bUseAsGameplayEventTrigger)
    {
        SetGameplayEventTriggerTag(AbilityTag);
    }
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

    if (!FHeroAbilityTargetingHelper::TryResolveActivationTargetingContext(SelfActor, CurrentInputContext, CurrentTargetDataHandle))
    {
        UE_LOG(LogTemp, Error, TEXT("%s: AvatarActor is not an AIsometricRPGCharacter. Cannot get target data."), *GetName());
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return false;
    }

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

    if (!FHeroAbilityTargetingHelper::RegisterReplicatedTargetDataDelegates(
        *this,
        *ASC,
        Handle,
        ActivationInfo,
        ReplicatedTargetDataDelegateHandle,
        ReplicatedTargetDataCancelledDelegateHandle))
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
        FHeroAbilityTargetingHelper::ApplyReplicatedTargetDataToActivationContext(*RPGCharacter, Data, CurrentInputContext);
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
        UE_LOG(LogTemp, Error, TEXT("'%s': TargetActorClass is NOT SET! Cancelling."), *GetName());
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return;
    }

    if (!GetAvatarActorFromActorInfo())
    {
        UE_LOG(LogTemp, Error, TEXT("'%s': Avatar actor is null. Cancelling."), *GetName());
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return;
    }

    TargetDataTask = FHeroAbilityTargetingHelper::CreateTargetSelectionTask(*this, TargetActorClass);
    if (!TargetDataTask)
    {
        UE_LOG(LogTemp, Error, TEXT("'%s': FAILED to create TargetDataTask!"), *GetName());
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
    }
}

void UGA_HeroBaseAbility::ConfigureTargetSelectionActor(AGameplayAbilityTargetActor& TargetActor)
{
}

void UGA_HeroBaseAbility::BeginTargetSelectionPresentation()
{
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
    if (!bUseBaseMontageFlow)
    {
        UE_LOG(LogTemp, Verbose, TEXT("%s: Base montage flow disabled, skipping PlayAbilityMontage."), *GetName());
        return;
    }

    MontageTask = FHeroAbilityMontageHelper::PlayAbilityMontage(
        *this,
        ActorInfo,
        CurrentTargetDataHandle,
        GetCurrentAimDirection(),
        MontageToPlay,
        CooldownDuration,
        bFaceTarget);
    if (!MontageTask)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("No montage to play or invalid anim instance in %s."), *GetName()));
        return;
    }

    BindMontageTaskCallbacks(*MontageTask);
    MontageTask->ReadyForActivation();
}

// =================================================================================================================
// Callbacks
// =================================================================================================================

void UGA_HeroBaseAbility::ResetRuntimeAbilityState()
{
    FHeroAbilityFlowHelper::ResetRuntimeAbilityState(*this);
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
    FHeroAbilityMontageHelper::BindBaseMontageTaskCallbacks(
        *this,
        InMontageTask,
        bEndAbilityWhenBaseMontageEnds,
        bCancelAbilityWhenBaseMontageInterrupted);
}

bool UGA_HeroBaseAbility::TryCommitAndExecuteAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const TCHAR* Phase,
    const TCHAR* OtherCheckFailureReason)
{
    return FHeroAbilityFlowHelper::TryCommitAndExecuteAbility(
        *this,
        Handle,
        ActorInfo,
        ActivationInfo,
        Phase,
        OtherCheckFailureReason);
}

void UGA_HeroBaseAbility::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
    FHeroAbilityFlowHelper::HandleTargetDataReady(*this, Data);
}

void UGA_HeroBaseAbility::OnTargetingCancelled(const FGameplayAbilityTargetDataHandle& Data)
{
    FHeroAbilityFlowHelper::HandleTargetingCancelled(*this);
}

void UGA_HeroBaseAbility::HandleMontageCompleted()
{
    FHeroAbilityFlowHelper::HandleMontageCompleted(*this);
}

void UGA_HeroBaseAbility::HandleMontageInterruptedOrCancelled()
{
    FHeroAbilityFlowHelper::HandleMontageInterruptedOrCancelled(*this);
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
