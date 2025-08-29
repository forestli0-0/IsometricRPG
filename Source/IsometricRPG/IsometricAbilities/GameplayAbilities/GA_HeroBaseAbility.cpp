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

UGA_HeroBaseAbility::UGA_HeroBaseAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
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
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("ActivateAbility: %s"), *GetName()));
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

bool UGA_HeroBaseAbility::OtherCheckBeforeCommit(const FGameplayAbilityTargetDataHandle& Data)
{
    // 基类中的默认实现 - 子类应该重写这个方法
    UE_LOG(LogTemp, Log, TEXT("%s：已调用提交前的最后检查。这应该被子类覆盖."), *GetName());
    if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Failed to commit ability for direct execution."), *GetName());
        CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
        return false;
    }

    // 播放技能动画
    PlayAbilityMontage(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);

    // 执行技能逻辑
    ExecuteSkill(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, CurrentTriggerEventData);
    return true;
}

bool UGA_HeroBaseAbility::OtherCheckBeforeCommit(const FGameplayEventData* TriggerEventData)
{
    // 基类中的默认实现 - 子类应该重写这个方法
    UE_LOG(LogTemp, Log, TEXT("%s：已调用提交前的最后检查。这应该被子类覆盖."), *GetName());
    return true;
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

void UGA_HeroBaseAbility::PlayAbilityMontage(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    UAnimInstance* AnimInstance = CurrentActorInfo->GetAnimInstance();
    if (MontageToPlay && IsValid(AnimInstance))
    {
        // 设置动画蒙太奇的播放速率，对于普攻这类冷却较短的技能，可以根据冷却时间来调整播放速率，防止动画还没播完就结束了
        float PlayRate = CooldownDuration > 0 && MontageToPlay->GetPlayLength() > 0 ?
            MontageToPlay->GetPlayLength() / CooldownDuration : 1.0f;
        PlayRate = FMath::Clamp(PlayRate, 1.f, 5.0f);

        // 播放动画的时候，如果是需要面向目标的技能，可以在这里设置角色的朝向
        TObjectPtr<ACharacter> Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
        // TODO:将Pawn按照目标方向旋转，且旋转时间和动画播放时间保持线性关系
        if (bFaceTarget)
        {
            if (Character != nullptr)
            {
                if (RequiresTargetData())
                {
                    auto TargetDataPtr = CurrentEventData.TargetData.Data[0].ToSharedRef();
                    if (TargetDataPtr.Get().HasHitResult() && TargetDataPtr->GetHitResult())
                    {
                        FVector TargetLocation = TargetDataPtr->GetHitResult()->Location;
                        //DrawDebugSphere(GetWorld(), TargetLocation, 10.f, 12, FColor::Red, true, 5.f); // Increased segments for a smoother sphere

                        if (!TargetLocation.IsZero() && Character) // Added a check for Character validity
                        {
                            FVector CharacterLocation = Character->GetActorLocation();

                            // Calculate direction vector in the XY plane (horizontal)
                            FVector DirectionToTargetHorizontal = TargetLocation - CharacterLocation;
                            DirectionToTargetHorizontal.Z = 0.0f; // Ignore the Z difference

                            // Check if the direction is not zero to avoid issues with Rotation()
                            if (!DirectionToTargetHorizontal.IsNearlyZero())
                            {
                                FRotator LookAtRotation = DirectionToTargetHorizontal.Rotation();
                                //DrawDebugLine(GetWorld(), CharacterLocation, CharacterLocation + DirectionToTargetHorizontal.GetSafeNormal() * 100.f, FColor::Cyan, true, 5.f); // Draw line in the horizontal plane for clarity
                                Character->SetActorRotation(LookAtRotation, ETeleportType::None);
                            }
                        }
                    }
                }
                else
                {
                    if (CurrentEventData.Target && Cast<APawn>(CurrentEventData.Target))
                    {
                        auto TargetLocation = CurrentEventData.Target->GetActorLocation();

                        if (!TargetLocation.IsZero() && Character) // Added a check for Character validity
                        {
                            FVector CharacterLocation = Character->GetActorLocation();

                            // Calculate direction vector in the XY plane (horizontal)
                            FVector DirectionToTargetHorizontal = TargetLocation - CharacterLocation;
                            DirectionToTargetHorizontal.Z = 0.0f; // Ignore the Z difference

                            // Check if the direction is not zero to avoid issues with Rotation()
                            if (!DirectionToTargetHorizontal.IsNearlyZero())
                            {
                                FRotator LookAtRotation = DirectionToTargetHorizontal.Rotation();
                                //DrawDebugLine(GetWorld(), CharacterLocation, CharacterLocation + DirectionToTargetHorizontal.GetSafeNormal() * 100.f, FColor::Cyan, true, 5.f); // Draw line in the horizontal plane for clarity
                                Character->SetActorRotation(LookAtRotation, ETeleportType::None);

                            }
                            else
                            {
                                UE_LOG(LogTemp, Error, TEXT("CurrentEventData.Target is null or invalid."));
                            }
                        }

                    }
                    else
                    {
                        if (CurrentEventData.TargetData.Data.Num() > 0 && CurrentEventData.TargetData.Data[0].IsValid())
                        {
                            auto TargetDataPtr = CurrentEventData.TargetData.Data[0].ToSharedRef();

                            if (TargetDataPtr.Get().HasHitResult() && TargetDataPtr->GetHitResult())
                            {
                                FVector TargetLocation = TargetDataPtr->GetHitResult()->Location;
                                //DrawDebugSphere(GetWorld(), TargetLocation, 10.f, 12, FColor::Red, true, 5.f); // Increased segments for a smoother sphere

                                if (!TargetLocation.IsZero() && Character) // Added a check for Character validity
                                {
                                    FVector CharacterLocation = Character->GetActorLocation();

                                    // Calculate direction vector in the XY plane (horizontal)
                                    FVector DirectionToTargetHorizontal = TargetLocation - CharacterLocation;
                                    DirectionToTargetHorizontal.Z = 0.0f; // Ignore the Z difference

                                    // Check if the direction is not zero to avoid issues with Rotation()
                                    if (!DirectionToTargetHorizontal.IsNearlyZero())
                                    {
                                        FRotator LookAtRotation = DirectionToTargetHorizontal.Rotation();
                                        //DrawDebugLine(GetWorld(), CharacterLocation, CharacterLocation + DirectionToTargetHorizontal.GetSafeNormal() * 100.f, FColor::Cyan, true, 5.f); // Draw line in the horizontal plane for clarity
                                        Character->SetActorRotation(LookAtRotation, ETeleportType::None);
                                    }
                                }
                            }
                        }
                        else
                        {
                            UE_LOG(LogTemp, Error, TEXT("CurrentEventData.TargetData is empty or invalid."));
						}
                    }

                }


            }
            else
            {
                UE_LOG(LogTemp, Display, TEXT("%s: No montage to play or invalid anim instance."), *GetName());
            }
        }

        // 创建并配置蒙太奇任务
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

    if (!OtherCheckBeforeCommit(Data))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 其他检查失败，与消耗和冷却无关。"), *GetName());
        constexpr bool bReplicateEndAbility = true;
        constexpr bool bWasCancelled = true;
    }
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
    UE_LOG(LogTemp, Display, TEXT("%s: Montage completed or blended out. Ending ability."), *GetName());
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_HeroBaseAbility::HandleMontageInterruptedOrCancelled()
{
    UE_LOG(LogTemp, Display, TEXT("%s: Montage interrupted or cancelled. Cancelling ability."), *GetName());
    CancelAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true);
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


