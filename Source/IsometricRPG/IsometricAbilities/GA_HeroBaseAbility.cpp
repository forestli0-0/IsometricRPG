// 在项目设置的描述页面填写您的版权声明。

#include "GA_HeroBaseAbility.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h" 
#include "AbilitySystemComponent.h"
#include "Character/IsometricRPGAttributeSetBase.h"

UGA_HeroBaseAbility::UGA_HeroBaseAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_HeroBaseAbility::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    AActor* SelfActor = GetAvatarActorFromActorInfo();
    if (!SelfActor)
    {
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        UE_LOG(LogTemp, Error, TEXT("%s: SelfActor is null."), *GetName());
        return;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC)
    {
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        UE_LOG(LogTemp, Error, TEXT("%s: AbilitySystemComponent is null in ActivateAbility."), *GetName());
        return;
    }

    AttributeSet = const_cast<UIsometricRPGAttributeSetBase*>(ASC->GetSet<UIsometricRPGAttributeSetBase>());
    if (!AttributeSet)
    {
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        UE_LOG(LogTemp, Error, TEXT("%s: AttributeSet is null in ActivateAbility."), *GetName());
        return;
    }

    FGameplayTag FailureTag;
    if (!CanActivateSkill(Handle, ActorInfo, ActivationInfo, TriggerEventData, FailureTag))
    {
        if (FailureTag.IsValid())
        {
            FGameplayEventData FailEventData;
            FailEventData.EventTag = FailureTag;
            FailEventData.Instigator = SelfActor;
            if (TriggerEventData)
            {
                FailEventData.Target = TriggerEventData->Target.Get();
                FailEventData.TargetData = TriggerEventData->TargetData;
            }
            ASC->HandleGameplayEvent(FailEventData.EventTag, &FailEventData);
            UE_LOG(LogTemp, Warning, TEXT("%s: Failed CanActivateSkill check with tag %s."), *GetName(), *FailureTag.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Failed CanActivateSkill check with no specific failure tag."), *GetName());
        }
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return;
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Failed to commit ability."), *GetName());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // 提交失败，取消技能
        return;
    }    // 面向目标的逻辑 - 如有需要，可在派生类中重写或扩展
    if (bFaceTarget && ActorInfo->AvatarActor.IsValid())
    {
        // 优先使用Target数据，因为它包含实际的Actor引用
        if (TriggerEventData && IsValid(TriggerEventData->Target))
        {
            AActor* TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
            if (TargetActor)
            {
                // 使用目标Actor的实时位置
                FVector TargetLocation = TargetActor->GetActorLocation();
                FVector SelfLocation = SelfActor->GetActorLocation();
                
                if (TargetLocation != FVector::ZeroVector) // 确保目标位置有效
                {
                    // 计算朝向目标的方向
                    FVector Direction = (TargetLocation - SelfLocation).GetSafeNormal();
                    if (!Direction.IsZero())
                    {
                        // 创建朝向目标的旋转
                        FRotator NewRotation = FRotationMatrix::MakeFromX(Direction).Rotator();
                        // 保持水平旋转，不改变俯仰和侧倾
                        NewRotation.Pitch = 0.0f;
                        NewRotation.Roll = 0.0f;
                        // 设置角色旋转
                        SelfActor->SetActorRotation(NewRotation);
                        
                        UE_LOG(LogTemp, Verbose, TEXT("%s: 面向目标位置: %s"), 
                            *GetName(), *TargetLocation.ToString());
                    }
                }
            }
        }
        // 如果没有Target数据，尝试使用TargetData
        else if (TriggerEventData && TriggerEventData->TargetData.Num() > 0)
        {
            const FGameplayAbilityTargetData* TargetData = TriggerEventData->TargetData.Data[0].Get();
            if (TargetData)
            {
                FVector TargetLocation = TargetData->GetHitResult() ? FVector(TargetData->GetHitResult()->Location) : FVector(TargetData->GetEndPoint());
                
                if (TargetLocation != FVector::ZeroVector) // 确保目标位置有效
                {
                    DrawDebugSphere(GetWorld(), TargetLocation, 20.f, 12, FColor::Blue, false, 5.f);
                    FVector Direction = (TargetLocation - ActorInfo->AvatarActor->GetActorLocation()).GetSafeNormal();
                    FRotator NewRotation = FRotationMatrix::MakeFromX(Direction).Rotator();
                    // 保持水平旋转
                    NewRotation.Pitch = 0.0f;
                    NewRotation.Roll = 0.0f;
                    SelfActor->SetActorRotation(NewRotation);
                    
                    UE_LOG(LogTemp, Verbose, TEXT("%s: 使用TargetData位置面向: %s"), 
                        *GetName(), *TargetLocation.ToString());
                }
            }
        }
    }

    UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
    if (MontageToPlay && IsValid(AnimInstance))
    {
        float PlayRate = CooldownDuration > 0 && MontageToPlay->GetPlayLength() > 0 ? MontageToPlay->GetPlayLength() / CooldownDuration : 1.0f;
        PlayRate = FMath::Clamp(PlayRate, 0.1f, 5.0f); // 调整Clamp以获得更大灵活性
        MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, MontageToPlay, PlayRate);

        MontageTask->OnCompleted.AddDynamic(this, &UGA_HeroBaseAbility::K2_EndAbility);
        MontageTask->OnInterrupted.AddDynamic(this, &UGA_HeroBaseAbility::K2_EndAbility);
        MontageTask->OnCancelled.AddDynamic(this, &UGA_HeroBaseAbility::K2_EndAbility);
        MontageTask->ReadyForActivation();
        // 不要在这里调用EndAbility；让Montage的回调来处理。
    }

    ExecuteSkill(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // 如果MontageToPlay为null且ExecuteSkill被设计为不调用EndAbility的瞬发技能，
    // 那么此处可能需要调用EndAbility。但推荐的做法是让ExecuteSkill自行管理非Montage技能的结束。
    // 目前假设ExecuteSkill或Montage会处理技能的结束。
    // 如果没有Montage且ExecuteSkill没有结束技能，请考虑是否需要调用K2_EndAbility。
}

// CanActivateSkill的默认实现。派生类应提供具体检查。
bool UGA_HeroBaseAbility::CanActivateSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData, FGameplayTag& OutFailureTag)
{
    // 基类实现允许激活。派生类重写以进行具体检查。
    // 例如: if (!Target) { OutFailureTag = FGameplayTag::RequestGameplayTag(FName("Ability.Failure.NoTarget")); return false; }
    return true;
}

// ExecuteSkill的默认实现。派生类必须重写此方法。
void UGA_HeroBaseAbility::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    UE_LOG(LogTemp, Warning, TEXT("UGA_HeroBaseAbility::ExecuteSkill called for %s but not implemented in a derived class. If this is an instant ability without a montage, it should call EndAbility()."), *GetName());
    // 如果是应立即结束且没有Montage的技能：
    // EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
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
    MontageTask = nullptr; // 清除引用

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
