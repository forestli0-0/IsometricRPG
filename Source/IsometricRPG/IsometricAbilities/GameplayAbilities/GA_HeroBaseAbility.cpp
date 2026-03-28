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
#include "Character/IsoPlayerState.h"

namespace
{
FString DescribeTagContainer(const FGameplayTagContainer& Tags)
{
    TArray<FGameplayTag> TagArray;
    Tags.GetGameplayTagArray(TagArray);

    if (TagArray.Num() == 0)
    {
        return TEXT("(None)");
    }

    TArray<FString> TagStrings;
    TagStrings.Reserve(TagArray.Num());
    for (const FGameplayTag& Tag : TagArray)
    {
        TagStrings.Add(Tag.ToString());
    }

    return FString::Join(TagStrings, TEXT(", "));
}

const FGameplayTag& GetCostMagnitudeTag()
{
    static const FGameplayTag CostMagnitudeTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Cost.Magnitude"));
    return CostMagnitudeTag;
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
    // 保存当前技能信息以便子类访问
    CurrentSpecHandle = Handle;
    CurrentActorInfo = ActorInfo;
    CurrentActivationInfo = ActivationInfo;

    AIsometricRPGCharacter* RPGCharacter = Cast<AIsometricRPGCharacter>(GetAvatarActorFromActorInfo());
    if (!RPGCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: AvatarActor is not an AIsometricRPGCharacter. Cannot get target data."), *GetName());
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return;
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

    CurrentTargetDataHandle = RPGCharacter->GetAbilityTargetData();

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
    if (RequiresTargetData())
    {
        const FGameplayAbilityTargetData* Data = CurrentTargetDataHandle.Get(0);
        bool bSuccessfullyFoundTarget = false;
        if (Data && Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_ActorArray::StaticStruct()))
        {
            const auto* ActorArrayData = static_cast<const FGameplayAbilityTargetData_ActorArray*>(Data);
            if (ActorArrayData && ActorArrayData->TargetActorArray.Num() > 0)
            {
                if (ActorArrayData->TargetActorArray[0].IsValid())
                {
                    bSuccessfullyFoundTarget = true;
                }
            }
        }
        else if (Data && Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_SingleTargetHit::StaticStruct()))
        {
            const auto* HitResultData = static_cast<const FGameplayAbilityTargetData_SingleTargetHit*>(Data);
            if (HitResultData && HitResultData->GetHitResult())
            {
                if (Cast<APawn>(HitResultData->GetHitResult()->GetActor()))
                {
                    bSuccessfullyFoundTarget = true;
                }
            }
        }

        if (bSuccessfullyFoundTarget)
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
        DirectExecuteAbility(Handle, ActorInfo, ActivationInfo);
    }
}

bool UGA_HeroBaseAbility::ShouldWaitForReplicatedTargetData(
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo& ActivationInfo) const
{
    if (!RequiresTargetData() || !ActorInfo)
    {
        return false;
    }

    if (!ActorInfo->IsNetAuthority() || ActorInfo->IsLocallyControlled())
    {
        return false;
    }

    if (NetExecutionPolicy != EGameplayAbilityNetExecutionPolicy::LocalPredicted)
    {
        return false;
    }

    return ActivationInfo.GetActivationPredictionKey().IsValidKey();
}

void UGA_HeroBaseAbility::SendInitialTargetDataToServer(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo& ActivationInfo)
{
    if (!RequiresTargetData() || !ActorInfo || ActorInfo->IsNetAuthority() || !ActorInfo->IsLocallyControlled())
    {
        return;
    }

    if (!Handle.IsValid() || !CurrentTargetDataHandle.IsValid(0))
    {
        return;
    }

    UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    if (!ASC)
    {
        return;
    }

    const FPredictionKey ActivationPredictionKey = ActivationInfo.GetActivationPredictionKey();
    if (!ActivationPredictionKey.IsValidKey())
    {
        return;
    }

    ASC->CallServerSetReplicatedTargetData(
        Handle,
        ActivationPredictionKey,
        CurrentTargetDataHandle,
        FGameplayTag(),
        ASC->ScopedPredictionKey);
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
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (ASC && CurrentSpecHandle.IsValid())
    {
        const FPredictionKey ActivationPredictionKey = CurrentActivationInfo.GetActivationPredictionKey();
        if (ReplicatedTargetDataDelegateHandle.IsValid())
        {
            ASC->AbilityTargetDataSetDelegate(CurrentSpecHandle, ActivationPredictionKey).Remove(ReplicatedTargetDataDelegateHandle);
        }
        if (ReplicatedTargetDataCancelledDelegateHandle.IsValid())
        {
            ASC->AbilityTargetDataCancelledDelegate(CurrentSpecHandle, ActivationPredictionKey).Remove(ReplicatedTargetDataCancelledDelegateHandle);
        }
    }

    ReplicatedTargetDataDelegateHandle.Reset();
    ReplicatedTargetDataCancelledDelegateHandle.Reset();
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

bool UGA_HeroBaseAbility::CheckCost(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC && ActorInfo)
    {
        ASC = ActorInfo->AbilitySystemComponent.Get();
    }

    if (!ASC)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: AbilitySystemComponent is null in CheckCost."), *GetName());
        return false;
    }

    const FGameplayEffectSpecHandle CostSpecHandle = MakeCostGameplayEffectSpecHandle(Handle, ActorInfo, FGameplayAbilityActivationInfo());
    if (!CostSpecHandle.Data.IsValid())
    {
        if (!CostGameplayEffectClass)
        {
            UE_LOG(LogTemp, Error, TEXT("%s: CostGameplayEffectClass is not configured."), *GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("%s: Failed to build cost GE spec from %s."), *GetName(), *GetNameSafe(CostGameplayEffectClass));
        }

        if (OptionalRelevantTags)
        {
            OptionalRelevantTags->AddTag(UAbilitySystemGlobals::Get().ActivateFailCostTag);
        }
        return false;
    }

    const UIsometricRPGAttributeSetBase* LocalAttributeSet = ASC->GetSet<UIsometricRPGAttributeSetBase>();
    if (!LocalAttributeSet)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: AttributeSet is null in CheckCost."), *GetName());
        return false;
    }

    const float ResolvedManaCost = ResolveManaCostFromSpec(*CostSpecHandle.Data.Get());
    if (LocalAttributeSet->GetMana() < ResolvedManaCost)
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
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC && ActorInfo)
    {
        ASC = ActorInfo->AbilitySystemComponent.Get();
    }

    const FGameplayEffectSpecHandle CostSpecHandle = MakeCostGameplayEffectSpecHandle(Handle, ActorInfo, ActivationInfo);
    if (ASC && CostSpecHandle.Data.IsValid())
    {
        ASC->ApplyGameplayEffectSpecToSelf(*CostSpecHandle.Data.Get(), ActivationInfo.GetActivationPredictionKey());
        return;
    }

    if (!CostGameplayEffectClass)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: CostGameplayEffectClass is not configured."), *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to apply cost because cost GE spec could not be built from %s."), *GetName(), *GetNameSafe(CostGameplayEffectClass));
    }
}

void UGA_HeroBaseAbility::ApplyCooldown(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo) const
{
    bool bAppliedCustomCooldown = false;

    if (CooldownGameplayEffectClass)
    {
        if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
        {
            FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownGameplayEffectClass, GetAbilityLevel());
            if (SpecHandle.Data.IsValid())
            {
                FGameplayEffectSpec& GESpec = *SpecHandle.Data.Get();

                // 寻找冷却时间标签
                const FGameplayTag CooldownDurationTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Cooldown.Duration"));
                if (CooldownDurationTag.IsValid())
                {
                    // 设置冷却时间
                    GESpec.SetSetByCallerMagnitude(CooldownDurationTag, CooldownDuration);
                }

                ASC->ApplyGameplayEffectSpecToSelf(GESpec);
                bAppliedCustomCooldown = true;
            }
        }
    }

    if (!bAppliedCustomCooldown)
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
    CleanupReplicatedTargetDataDelegates();
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
    if (!OtherCheckBeforeCommit())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 提交前的检查失败，与消耗和冷却无关。"), *GetName());
        constexpr bool bReplicateEndAbility = true;
        return;
    }
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        LogCommitFailureDiagnostics(Handle, ActorInfo, ActivationInfo, TEXT("DirectExecute"));
        UE_LOG(LogTemp, Warning, TEXT("%s: 技能提交失败，可能是由于消耗或冷却问题。"), *GetName());
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return;
    }
    UE_LOG(LogTemp, Verbose, TEXT("%s: CommitAbility OK (DirectExecute). Montage=%s"), *GetName(), MontageToPlay ? *MontageToPlay->GetName() : TEXT("None"));
    
    // 播放技能动画
    PlayAbilityMontage(Handle, ActorInfo, ActivationInfo);
    
    // 执行技能逻辑
    ExecuteSkill(Handle, ActorInfo, ActivationInfo);
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
        // 设置动画蒙太奇的播放速率：避免过度加速导致动画“抽动”
        float PlayRate = 1.0f;
        if (CooldownDuration > 0.f && MontageToPlay->GetPlayLength() > 0.f)
        {
            const float DesiredRate = MontageToPlay->GetPlayLength() / CooldownDuration;
            PlayRate = FMath::Clamp(DesiredRate, 1.f, 3.f);
        }
        // 播放动画的时候，如果是需要面向目标的技能，可以在这里设置角色的朝向
        TObjectPtr<ACharacter> Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
        // TODO:将Pawn按照目标方向旋转，且旋转时间和动画播放时间保持线性关系
        if (bFaceTarget && Character)
        {
            // 改为从 CurrentTargetDataHandle 获取目标信息（该数据来源于角色在按键时暂存的 TargetData）
            FVector TargetLocation = FVector::ZeroVector;
            bool bHasTarget = false;

            if (CurrentTargetDataHandle.Num() > 0 && CurrentTargetDataHandle.Data[0].IsValid())
                        {
                const TSharedPtr<FGameplayAbilityTargetData> DataPtr = CurrentTargetDataHandle.Data[0];
                if (DataPtr->HasHitResult() && DataPtr->GetHitResult())
                            {
                    TargetLocation = DataPtr->GetHitResult()->Location;
                    bHasTarget = true;
                            }
                else if (DataPtr->GetActors().Num() > 0 && DataPtr->GetActors()[0].IsValid())
                {
                    TargetLocation = DataPtr->GetActors()[0]->GetActorLocation();
                    bHasTarget = true;
                            }
                        }

            if (bHasTarget)
                    {
                const FVector CharacterLocation = Character->GetActorLocation();
                                    FVector DirectionToTargetHorizontal = TargetLocation - CharacterLocation;
                DirectionToTargetHorizontal.Z = 0.0f; // Ignore vertical
                                    if (!DirectionToTargetHorizontal.IsNearlyZero())
                                    {
                    const FRotator LookAtRotation = DirectionToTargetHorizontal.Rotation();
                                        Character->SetActorRotation(LookAtRotation, ETeleportType::None);
                                    }
                                }
                        else
                        {
                UE_LOG(LogTemp, Verbose, TEXT("%s: No target data to face when playing montage."), *GetName());
            }
        }

        // 创建并配置蒙太奇任务
        MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, MontageToPlay, PlayRate);
        if (MontageTask)
        {
            UE_LOG(LogTemp, Verbose, TEXT("%s: PlayMontage %s at rate %.2f"), *GetName(), *MontageToPlay->GetName(), PlayRate);
            if (bEndAbilityWhenBaseMontageEnds)
            {
            MontageTask->OnCompleted.AddDynamic(this, &UGA_HeroBaseAbility::HandleMontageCompleted);
            MontageTask->OnBlendOut.AddDynamic(this, &UGA_HeroBaseAbility::HandleMontageCompleted);
            }
            if (bCancelAbilityWhenBaseMontageInterrupted)
            {
            MontageTask->OnInterrupted.AddDynamic(this, &UGA_HeroBaseAbility::HandleMontageInterruptedOrCancelled);
            MontageTask->OnCancelled.AddDynamic(this, &UGA_HeroBaseAbility::HandleMontageInterruptedOrCancelled);
            }
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

    if (!OtherCheckBeforeCommit())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 超出范围，向目标移动······"), *GetName());
		return;
    }
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        LogCommitFailureDiagnostics(Handle, ActorInfo, ActivationInfo, TEXT("OnTargetDataReady"));
        UE_LOG(LogTemp, Warning, TEXT("%s: 技能提交失败，可能是由于消耗或冷却问题。"), *GetName());
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return;
	}

    UE_LOG(LogTemp, Verbose, TEXT("%s: CommitAbility OK (OnTargetDataReady). Montage=%s"), *GetName(), MontageToPlay ? *MontageToPlay->GetName() : TEXT("None"));
	// 播放技能动画
	PlayAbilityMontage(Handle, ActorInfo, ActivationInfo);
	// 执行技能逻辑
	ExecuteSkill(Handle, ActorInfo, ActivationInfo);
    return;
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

    CleanupReplicatedTargetDataDelegates();

    if (MontageTask && MontageTask->IsActive())
    {
        MontageTask->EndTask();
    }
    MontageTask = nullptr;

    if (TargetDataTask && TargetDataTask->IsActive())
    {
        TargetDataTask->EndTask();
    }
    TargetDataTask = nullptr;

    Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void UGA_HeroBaseAbility::NotifyCooldownTriggered(const FGameplayAbilitySpecHandle& Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
    if (!ActorInfo || CooldownDuration <= 0.f)
    {
        return;
    }

    AIsoPlayerState* IsoPlayerState = nullptr;

    if (const APawn* Pawn = Cast<APawn>(ActorInfo->AvatarActor.Get()))
    {
        IsoPlayerState = Pawn->GetPlayerState<AIsoPlayerState>();
    }

    if (!IsoPlayerState)
    {
        if (const AController* Controller = Cast<AController>(ActorInfo->PlayerController.Get()))
        {
            IsoPlayerState = Controller->GetPlayerState<AIsoPlayerState>();
        }
    }

    if (IsoPlayerState)
    {
        IsoPlayerState->HandleAbilityCooldownTriggered(Handle, CooldownDuration);
    }
}

FGameplayEffectSpecHandle UGA_HeroBaseAbility::MakeCostGameplayEffectSpecHandle(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo) const
{
    if (!CostGameplayEffectClass)
    {
        return FGameplayEffectSpecHandle();
    }

    FGameplayEffectSpecHandle CostSpecHandle =
        MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CostGameplayEffectClass, GetAbilityLevel(Handle, ActorInfo));
    if (!CostSpecHandle.Data.IsValid())
    {
        return FGameplayEffectSpecHandle();
    }

    FGameplayEffectSpec& CostSpec = *CostSpecHandle.Data.Get();
    CostSpec.SetSetByCallerMagnitude(GetCostMagnitudeTag(), -CostMagnitude);
    CostSpec.CalculateModifierMagnitudes();
    return CostSpecHandle;
}

float UGA_HeroBaseAbility::ResolveManaCostFromSpec(const FGameplayEffectSpec& CostSpec) const
{
    const FGameplayAttribute ManaAttribute = UIsometricRPGAttributeSetBase::GetManaAttribute();
    float ResolvedManaCost = 0.f;

    const UGameplayEffect* CostDefinition = CostSpec.Def;
    if (!CostDefinition)
    {
        return ResolvedManaCost;
    }

    const TArray<FGameplayModifierInfo>& Modifiers = CostDefinition->Modifiers;
    for (int32 ModifierIndex = 0; ModifierIndex < Modifiers.Num(); ++ModifierIndex)
    {
        const FGameplayModifierInfo& Modifier = Modifiers[ModifierIndex];
        if (Modifier.Attribute != ManaAttribute)
        {
            continue;
        }

        if (Modifier.ModifierOp != EGameplayModOp::Additive)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Cost GE %s uses a non-additive mana modifier. Current resolver ignores this path."), *GetName(), *GetNameSafe(CostDefinition));
            continue;
        }

        const float ModifierMagnitude = CostSpec.GetModifierMagnitude(ModifierIndex, true);
        if (ModifierMagnitude < 0.f)
        {
            ResolvedManaCost += -ModifierMagnitude;
        }
    }

    return ResolvedManaCost;
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
        CooldownTagsText = DescribeTagContainer(*CooldownTags);

        TArray<FGameplayTag> CooldownTagArray;
        CooldownTags->GetGameplayTagArray(CooldownTagArray);

        TArray<FGameplayTag> OwnedTagArray;
        OwnedTags.GetGameplayTagArray(OwnedTagArray);

        TArray<FString> MatchedOwnedTagStrings;
        for (const FGameplayTag& OwnedTag : OwnedTagArray)
        {
            for (const FGameplayTag& CooldownTag : CooldownTagArray)
            {
                if (OwnedTag.MatchesTag(CooldownTag))
                {
                    MatchedOwnedTagStrings.AddUnique(OwnedTag.ToString());
                    break;
                }
            }
        }

        if (MatchedOwnedTagStrings.Num() > 0)
        {
            MatchedOwnedCooldownTagsText = FString::Join(MatchedOwnedTagStrings, TEXT(", "));
        }
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
        *DescribeTagContainer(CostFailureTags),
        *DescribeTagContainer(CooldownFailureTags),
        *DescribeTagContainer(OwnedTags));
}

// =================================================================================================================
// Misc
// =================================================_================================================================

bool UGA_HeroBaseAbility::RequiresTargetData_Implementation() const
{
    return bRequiresTargetData;
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

    // 不需要目标数据的技能（SelfCast / SkillShot 等）直接通过
    if (!RequiresTargetData())
    {
        return true;
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


