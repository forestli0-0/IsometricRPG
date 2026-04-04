#include "IsometricAbilities/GameplayAbilities/HeroAbilityFlowHelper.h"

#include "IsometricAbilities/GameplayAbilities/GA_HeroBaseAbility.h"

void FHeroAbilityFlowHelper::ResetRuntimeAbilityState(UGA_HeroBaseAbility& Ability)
{
    Ability.CleanupReplicatedTargetDataDelegates();
    Ability.EndMontageTask();
    Ability.EndTargetDataTask();
    Ability.CurrentInputContext = FPendingAbilityActivationContext();
}

bool FHeroAbilityFlowHelper::TryCommitAndExecuteAbility(
    UGA_HeroBaseAbility& Ability,
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const TCHAR* Phase,
    const TCHAR* OtherCheckFailureReason)
{
    if (!Ability.OtherCheckBeforeCommit())
    {
        if (OtherCheckFailureReason)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: %s"), *Ability.GetName(), OtherCheckFailureReason);
        }
        return false;
    }

    if (!Ability.CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        Ability.LogCommitFailureDiagnostics(Handle, ActorInfo, ActivationInfo, Phase);
        UE_LOG(LogTemp, Warning, TEXT("%s: 技能提交失败，可能是由于消耗或冷却问题。"), *Ability.GetName());
        Ability.CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return false;
    }

    UE_LOG(
        LogTemp,
        Verbose,
        TEXT("%s: CommitAbility OK (%s). Montage=%s"),
        *Ability.GetName(),
        Phase,
        Ability.MontageToPlay ? *Ability.MontageToPlay->GetName() : TEXT("None"));
    Ability.PlayAbilityMontage(Handle, ActorInfo, ActivationInfo);
    Ability.ExecuteSkill(Handle, ActorInfo, ActivationInfo);
    return true;
}

void FHeroAbilityFlowHelper::HandleTargetDataReady(
    UGA_HeroBaseAbility& Ability,
    const FGameplayAbilityTargetDataHandle& Data)
{
    Ability.CurrentTargetDataHandle = Data;
    Ability.EndTargetDataTask();

    const FGameplayAbilityActorInfo* ActorInfo = Ability.GetCurrentActorInfo();
    const FGameplayAbilitySpecHandle Handle = Ability.GetCurrentAbilitySpecHandle();
    const FGameplayAbilityActivationInfo ActivationInfo = Ability.GetCurrentActivationInfo();

    TryCommitAndExecuteAbility(
        Ability,
        Handle,
        ActorInfo,
        ActivationInfo,
        TEXT("OnTargetDataReady"),
        TEXT("超出范围，向目标移动······"));
}

void FHeroAbilityFlowHelper::HandleTargetingCancelled(UGA_HeroBaseAbility& Ability)
{
    Ability.EndTargetDataTask();
    UE_LOG(LogTemp, Display, TEXT("%s: Targeting cancelled."), *Ability.GetName());
    Ability.CancelAbility(
        Ability.GetCurrentAbilitySpecHandle(),
        Ability.GetCurrentActorInfo(),
        Ability.GetCurrentActivationInfo(),
        true);
}

void FHeroAbilityFlowHelper::HandleMontageCompleted(UGA_HeroBaseAbility& Ability)
{
    if (!Ability.bEndAbilityWhenBaseMontageEnds)
    {
        UE_LOG(LogTemp, Verbose, TEXT("%s: Montage completed, but auto-end disabled."), *Ability.GetName());
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("%s: Montage completed or blended out. Ending ability (base flow)."), *Ability.GetName());
    Ability.EndAbility(
        Ability.GetCurrentAbilitySpecHandle(),
        Ability.GetCurrentActorInfo(),
        Ability.GetCurrentActivationInfo(),
        true,
        false);
}

void FHeroAbilityFlowHelper::HandleMontageInterruptedOrCancelled(UGA_HeroBaseAbility& Ability)
{
    if (!Ability.bCancelAbilityWhenBaseMontageInterrupted)
    {
        UE_LOG(LogTemp, Verbose, TEXT("%s: Montage interrupted/cancelled, but auto-cancel disabled."), *Ability.GetName());
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("%s: Montage interrupted or cancelled. Cancelling ability (base flow)."), *Ability.GetName());
    Ability.CancelAbility(
        Ability.GetCurrentAbilitySpecHandle(),
        Ability.GetCurrentActorInfo(),
        Ability.GetCurrentActivationInfo(),
        true);
}
