#include "IsometricAbilities/GameplayAbilities/HeroAbilityTargetingHelper.h"

#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "AbilitySystemComponent.h"
#include "Character/IsometricRPGCharacter.h"
#include "IsometricAbilities/GameplayAbilities/GA_HeroBaseAbility.h"

namespace
{
bool HasActorTargetData(const FGameplayAbilityTargetData* Data)
{
    if (!Data)
    {
        return false;
    }

    if (Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_ActorArray::StaticStruct()))
    {
        const auto* ActorArrayData = static_cast<const FGameplayAbilityTargetData_ActorArray*>(Data);
        return ActorArrayData && ActorArrayData->TargetActorArray.Num() > 0 && ActorArrayData->TargetActorArray[0].IsValid();
    }

    return false;
}

bool HasPawnHitTargetData(const FGameplayAbilityTargetData* Data)
{
    if (!Data || !Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_SingleTargetHit::StaticStruct()))
    {
        return false;
    }

    const auto* HitResultData = static_cast<const FGameplayAbilityTargetData_SingleTargetHit*>(Data);
    return HitResultData && HitResultData->GetHitResult() && Cast<APawn>(HitResultData->GetHitResult()->GetActor()) != nullptr;
}

bool HasLocationTargetData(const FGameplayAbilityTargetData* Data)
{
    if (!Data)
    {
        return false;
    }

    if (Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_SingleTargetHit::StaticStruct()))
    {
        const auto* HitResultData = static_cast<const FGameplayAbilityTargetData_SingleTargetHit*>(Data);
        return HitResultData && HitResultData->GetHitResult() != nullptr;
    }

    return HasActorTargetData(Data);
}
}

const TCHAR* FHeroAbilityTargetingHelper::DescribeAbilityType(const EHeroAbilityType AbilityType)
{
    switch (AbilityType)
    {
    case EHeroAbilityType::SelfCast:
        return TEXT("SelfCast");
    case EHeroAbilityType::Targeted:
        return TEXT("Targeted");
    case EHeroAbilityType::SkillShot:
        return TEXT("SkillShot");
    case EHeroAbilityType::AreaEffect:
        return TEXT("AreaEffect");
    case EHeroAbilityType::Passive:
        return TEXT("Passive");
    default:
        return TEXT("Unknown");
    }
}

FString FHeroAbilityTargetingHelper::DescribeTargetDataShape(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
    if (!TargetDataHandle.IsValid(0))
    {
        return TEXT("None");
    }

    const FGameplayAbilityTargetData* Data = TargetDataHandle.Get(0);
    if (!Data)
    {
        return TEXT("NullPayload");
    }

    if (Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_ActorArray::StaticStruct()))
    {
        const auto* ActorArrayData = static_cast<const FGameplayAbilityTargetData_ActorArray*>(Data);
        const int32 NumActors = ActorArrayData ? ActorArrayData->TargetActorArray.Num() : 0;
        const AActor* FirstActor = (ActorArrayData && NumActors > 0) ? ActorArrayData->TargetActorArray[0].Get() : nullptr;
        return FString::Printf(TEXT("ActorArray(Num=%d First=%s)"), NumActors, *GetNameSafe(FirstActor));
    }

    if (Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_SingleTargetHit::StaticStruct()))
    {
        const auto* HitResultData = static_cast<const FGameplayAbilityTargetData_SingleTargetHit*>(Data);
        const FHitResult* HitResult = HitResultData ? HitResultData->GetHitResult() : nullptr;
        return FString::Printf(
            TEXT("SingleHit(Actor=%s Blocking=%s Location=%s)"),
            *GetNameSafe(HitResult ? HitResult->GetActor() : nullptr),
            (HitResult && HitResult->bBlockingHit) ? TEXT("true") : TEXT("false"),
            HitResult ? *HitResult->Location.ToCompactString() : TEXT("None"));
    }

    return Data->GetScriptStruct() ? Data->GetScriptStruct()->GetName() : TEXT("UnknownStruct");
}

bool FHeroAbilityTargetingHelper::HasRequiredExecutionTargetData(
    const FHeroAbilityTargetingPolicy& Policy,
    const FGameplayAbilityTargetDataHandle& TargetData)
{
    if (!Policy.bExpectsPreparedTargetData)
    {
        return true;
    }

    const FGameplayAbilityTargetData* Data = TargetData.Get(0);
    if (!Data)
    {
        return false;
    }

    if (Policy.bExpectsLocationTargetData)
    {
        return HasLocationTargetData(Data) || !Policy.CurrentAimDirection.IsNearlyZero();
    }

    if (Policy.bExpectsActorTargetData)
    {
        return HasActorTargetData(Data) || HasPawnHitTargetData(Data);
    }

    return true;
}

bool FHeroAbilityTargetingHelper::ValidateExecutionTargetData(
    const UObject& AbilityObject,
    const FHeroAbilityTargetingPolicy& Policy,
    const FGameplayAbilityTargetDataHandle& TargetData,
    const TCHAR* Phase)
{
    if (!Policy.bExpectsPreparedTargetData)
    {
        return true;
    }

    if (HasRequiredExecutionTargetData(Policy, TargetData))
    {
        return true;
    }

    const TCHAR* ExpectedTargetShape = Policy.bExpectsLocationTargetData
        ? TEXT("AimDirection-or-hit-result")
        : TEXT("actor-or-pawn-hit");
    ensureAlwaysMsgf(
        false,
        TEXT("%s: %s attempted to execute a %s ability without the required %s target data. Received=%s"),
        *AbilityObject.GetName(),
        Phase,
        DescribeAbilityType(Policy.AbilityType),
        ExpectedTargetShape,
        *DescribeTargetDataShape(TargetData));
    return false;
}

bool FHeroAbilityTargetingHelper::ShouldWaitForReplicatedTargetData(
    const FHeroAbilityTargetingPolicy& Policy,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo& ActivationInfo)
{
    if (!Policy.bExpectsPreparedTargetData || !ActorInfo)
    {
        return false;
    }

    if (!ActorInfo->IsNetAuthority() || ActorInfo->IsLocallyControlled())
    {
        return false;
    }

    if (Policy.bExpectsLocationTargetData && !Policy.CurrentAimDirection.IsNearlyZero())
    {
        return false;
    }

    if (!Policy.bIsLocalPredicted)
    {
        return false;
    }

    return ActivationInfo.GetActivationPredictionKey().IsValidKey();
}

void FHeroAbilityTargetingHelper::SendInitialTargetDataToServer(
    const FHeroAbilityTargetingPolicy& Policy,
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo& ActivationInfo,
    const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
    if (!Policy.bExpectsPreparedTargetData || !ActorInfo || ActorInfo->IsNetAuthority() || !ActorInfo->IsLocallyControlled())
    {
        return;
    }

    if (!Handle.IsValid() || !TargetDataHandle.IsValid(0))
    {
        return;
    }

    UAbilitySystemComponent* AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get();
    if (!AbilitySystemComponent)
    {
        return;
    }

    const FPredictionKey ActivationPredictionKey = ActivationInfo.GetActivationPredictionKey();
    if (!ActivationPredictionKey.IsValidKey())
    {
        return;
    }

    AbilitySystemComponent->CallServerSetReplicatedTargetData(
        Handle,
        ActivationPredictionKey,
        TargetDataHandle,
        FGameplayTag(),
        AbilitySystemComponent->ScopedPredictionKey);
}

void FHeroAbilityTargetingHelper::CleanupReplicatedTargetDataDelegates(
    UAbilitySystemComponent* AbilitySystemComponent,
    const FGameplayAbilitySpecHandle& SpecHandle,
    const FGameplayAbilityActivationInfo& ActivationInfo,
    FDelegateHandle& TargetDataDelegateHandle,
    FDelegateHandle& CancelledDelegateHandle)
{
    if (AbilitySystemComponent && SpecHandle.IsValid())
    {
        const FPredictionKey ActivationPredictionKey = ActivationInfo.GetActivationPredictionKey();
        if (TargetDataDelegateHandle.IsValid())
        {
            AbilitySystemComponent->AbilityTargetDataSetDelegate(SpecHandle, ActivationPredictionKey).Remove(TargetDataDelegateHandle);
        }
        if (CancelledDelegateHandle.IsValid())
        {
            AbilitySystemComponent->AbilityTargetDataCancelledDelegate(SpecHandle, ActivationPredictionKey).Remove(CancelledDelegateHandle);
        }
    }

    TargetDataDelegateHandle.Reset();
    CancelledDelegateHandle.Reset();
}

bool FHeroAbilityTargetingHelper::RegisterReplicatedTargetDataDelegates(
    UGA_HeroBaseAbility& Ability,
    UAbilitySystemComponent& AbilitySystemComponent,
    const FGameplayAbilitySpecHandle& SpecHandle,
    const FGameplayAbilityActivationInfo& ActivationInfo,
    FDelegateHandle& TargetDataDelegateHandle,
    FDelegateHandle& CancelledDelegateHandle)
{
    if (!SpecHandle.IsValid())
    {
        return false;
    }

    CleanupReplicatedTargetDataDelegates(
        &AbilitySystemComponent,
        SpecHandle,
        ActivationInfo,
        TargetDataDelegateHandle,
        CancelledDelegateHandle);

    const FPredictionKey ActivationPredictionKey = ActivationInfo.GetActivationPredictionKey();
    TargetDataDelegateHandle = AbilitySystemComponent.AbilityTargetDataSetDelegate(SpecHandle, ActivationPredictionKey)
        .AddUObject(&Ability, &UGA_HeroBaseAbility::HandleReplicatedTargetDataReceived);
    CancelledDelegateHandle = AbilitySystemComponent.AbilityTargetDataCancelledDelegate(SpecHandle, ActivationPredictionKey)
        .AddUObject(&Ability, &UGA_HeroBaseAbility::HandleReplicatedTargetDataCancelled);
    return AbilitySystemComponent.CallReplicatedTargetDataDelegatesIfSet(SpecHandle, ActivationPredictionKey);
}

bool FHeroAbilityTargetingHelper::TryResolveActivationTargetingContext(
    const AActor* AvatarActor,
    FPendingAbilityActivationContext& OutInputContext,
    FGameplayAbilityTargetDataHandle& OutTargetDataHandle)
{
    const AIsometricRPGCharacter* Character = Cast<AIsometricRPGCharacter>(AvatarActor);
    if (!Character)
    {
        return false;
    }

    OutInputContext = Character->GetPendingAbilityActivationContext();
    OutTargetDataHandle = Character->GetAbilityTargetData();
    return true;
}

void FHeroAbilityTargetingHelper::ApplyReplicatedTargetDataToActivationContext(
    AIsometricRPGCharacter& Character,
    const FGameplayAbilityTargetDataHandle& Data,
    FPendingAbilityActivationContext& OutInputContext)
{
    FPendingAbilityActivationContext Context = Character.GetPendingAbilityActivationContext();
    Context.TargetData = Data;
    Character.SetPendingAbilityActivationContext(Context);
    OutInputContext = Character.GetPendingAbilityActivationContext();
}

UAbilityTask_WaitTargetData* FHeroAbilityTargetingHelper::CreateTargetSelectionTask(
    UGA_HeroBaseAbility& Ability,
    TSubclassOf<AGameplayAbilityTargetActor> TargetActorClass)
{
    if (!TargetActorClass)
    {
        return nullptr;
    }

    UAbilityTask_WaitTargetData* TargetDataTask = UAbilityTask_WaitTargetData::WaitTargetData(
        &Ability,
        FName("TargetData"),
        EGameplayTargetingConfirmation::UserConfirmed,
        TargetActorClass);
    if (!TargetDataTask)
    {
        return nullptr;
    }

    TargetDataTask->ValidData.AddDynamic(&Ability, &UGA_HeroBaseAbility::OnTargetDataReady);
    TargetDataTask->Cancelled.AddDynamic(&Ability, &UGA_HeroBaseAbility::OnTargetingCancelled);

    AGameplayAbilityTargetActor* SpawnedActor = nullptr;
    TargetDataTask->BeginSpawningActor(&Ability, TargetActorClass, SpawnedActor);
    if (SpawnedActor)
    {
        Ability.ConfigureTargetSelectionActor(*SpawnedActor);
    }
    TargetDataTask->FinishSpawningActor(&Ability, SpawnedActor);
    Ability.BeginTargetSelectionPresentation();
    TargetDataTask->ReadyForActivation();
    return TargetDataTask;
}
