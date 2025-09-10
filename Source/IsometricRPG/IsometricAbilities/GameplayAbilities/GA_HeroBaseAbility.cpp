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

UGA_HeroBaseAbility::UGA_HeroBaseAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
    TargetActorClass = nullptr;
    // 默认为目标指向型技能
    AbilityType = EHeroAbilityType::Targeted;
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
    // 主动从角色身上拉取暂存的目标数据
    CurrentTargetDataHandle = RPGCharacter->GetAbilityTargetData();
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
        const FGameplayAbilityTargetData* Data = CurrentTargetDataHandle.Get(0);
        bool bSuccessfullyFoundTarget = false;
        // 情况一：目标是 Actor
        if (Data && Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_ActorArray::StaticStruct()))
        {
            const auto* ActorArrayData = static_cast<const FGameplayAbilityTargetData_ActorArray*>(Data);
            if (ActorArrayData && ActorArrayData->TargetActorArray.Num() > 0)
            {
                auto TargetActor = ActorArrayData->TargetActorArray[0].Get();
                if (TargetActor)
                {
                    bSuccessfullyFoundTarget = true;
                }
            }
        }
        // 情况二：目标是射线检测点
        else if (Data && Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_SingleTargetHit::StaticStruct()))
        {
            const auto* HitResultData = static_cast<const FGameplayAbilityTargetData_SingleTargetHit*>(Data);
            if (HitResultData && HitResultData->GetHitResult())
            {
                auto TargetActor = Cast<APawn>(HitResultData->GetHitResult()->GetActor()); // 尝试获取被击中的Actor
                if (TargetActor)
                {
                    bSuccessfullyFoundTarget = true;
                }
            }
        }
		// 如果已经有目标数据，直接执行技能(比如按下技能时，鼠标已经指向了一个目标)
        if (bSuccessfullyFoundTarget)
        {
            OnTargetDataReady(CurrentTargetDataHandle);
        }
        else
        {
            // 如果需要目标但数据为空（例如直接按Q但没点目标），则开始等待目标数据
            StartTargetSelection(Handle, ActorInfo, ActivationInfo);
        }
    }
    else
    {
        // 不需要目标选择，直接执行技能
        DirectExecuteAbility(Handle, ActorInfo, ActivationInfo);
    }
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
    float CurrentMana = AttriSet->GetMana();

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
                    GESpec.SetSetByCallerMagnitude(CostTag, -CostMagnitude);
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

void UGA_HeroBaseAbility::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    UE_LOG(LogTemp, Display, TEXT("%s: EndAbility called. bWasCancelled=%s"), *GetName(), bWasCancelled ? TEXT("true") : TEXT("false"));
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

// =================================================================================================================
// Misc
// =================================================_================================================================

bool UGA_HeroBaseAbility::RequiresTargetData_Implementation() const
{
    return bRequiresTargetData;
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


