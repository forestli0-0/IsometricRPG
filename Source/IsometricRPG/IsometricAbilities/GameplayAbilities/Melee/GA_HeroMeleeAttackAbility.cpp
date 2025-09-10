// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\GA_HeroMeleeAttackAbility.cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_HeroMeleeAttackAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h" 
#include "Character/IsometricRPGAttributeSetBase.h"
// Targeting helpers
#include "IsometricAbilities/TargetTrace/GATA_CursorTrace.h"
#include "FX/NA_NiagaraActorBase.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "IsometricAbilities/AbilityTasks/AbilityTask_WaitMoveToLocation.h"
#include "Components/CapsuleComponent.h"
UGA_HeroMeleeAttackAbility::UGA_HeroMeleeAttackAbility()
{
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    // 近战 A 键：希望进入目标选择流程（显示范围圈与选取目标）
    bRequiresTargetData = true;
    // 默认目标选择器（若蓝图未配置时的兜底），使用鼠标光标追踪
    TargetActorClass = AGATA_CursorTrace::StaticClass();
    // 初始化攻击蒙太奇路径
    static ConstructorHelpers::FObjectFinder<UAnimMontage> AttackMontageObj(TEXT("/Game/Characters/Animations/AM_Attack_Melee"));
    if (AttackMontageObj.Succeeded())
    {
        MontageToPlay = AttackMontageObj.Object;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load AttackMontage! Check path validity."));
        // 可考虑添加调试断言
        checkNoEntry();
    }
    // 设置触发条件
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Player.BasicAttack"));

    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Player.BasicAttack"));

    // 设置触发事件
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag("Ability.Player.BasicAttack");
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
    // 设定冷却阻断标签
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Cooldown.Ability.MeleeAttack"));

    // 范围圈 Niagara 指示器（可在蓝图覆盖），提供一个合理默认
    RangeIndicatorNiagaraActorClass = ANA_NiagaraActorBase::StaticClass();

}

void UGA_HeroMeleeAttackAbility::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
    // 打印调试信息
    UE_LOG(LogTemp, Warning, TEXT("使用A键普攻"));

    AActor* OwnerActor = GetAvatarActorFromActorInfo();

    if (!OwnerActor) return;

    // 通过TriggerEventData获取目标，再次确认朝向正确
    if (CurrentTargetDataHandle.Num() > 0)
    {
        const FGameplayAbilityTargetDataHandle& TargetDataHandle = CurrentTargetDataHandle;
        const FGameplayAbilityTargetData* TargetData = TargetDataHandle.Get(0); // 获取第一个目标数据

        if (TargetData)
        {

            const FHitResult* HitResult = TargetData->GetHitResult();
            if (HitResult)
            {
                const TWeakObjectPtr<UObject> TargetObject = HitResult->GetActor();
                if (TargetObject.IsValid())
                {
                    AActor* TargetActor = Cast<AActor>(TargetObject.Get());
                    if (TargetActor)
                    {
                        // 打印目标和自身位置，用于调试
                        UE_LOG(LogTemp, Verbose, TEXT("攻击目标位置: %s, 自身位置: %s"),
                            *TargetActor->GetActorLocation().ToString(),
                            *OwnerActor->GetActorLocation().ToString());
                    }
                }
            }
        }
    }

    // 注意：此处不需要自己结束技能，因为我们使用了蒙太奇，让蒙太奇完成时自动结束技能
    // 如果这是一个即时技能(没有蒙太奇)，则需要手动结束
    if (!MontageToPlay)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}

bool UGA_HeroMeleeAttackAbility::CheckCost(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}

void UGA_HeroMeleeAttackAbility::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
    // 缓存一次目标数据（Actor 或 位置）
    CachedTargetActor = nullptr;
    CachedTargetLocation = FVector::ZeroVector;

    if (Data.Num() > 0 && Data.Data[0].IsValid())
    {
        const TSharedPtr<FGameplayAbilityTargetData> D = Data.Data[0];
        if (D->HasHitResult() && D->GetHitResult())
        {
            CachedTargetLocation = D->GetHitResult()->Location;
            if (AActor* HRActor = D->GetHitResult()->GetActor())
            {
                CachedTargetActor = HRActor;
            }
        }
        else if (D->GetActors().Num() > 0 && D->GetActors()[0].IsValid())
        {
            CachedTargetActor = D->GetActors()[0].Get();
            if (CachedTargetActor.IsValid())
            {
                CachedTargetLocation = CachedTargetActor->GetActorLocation();
            }
        }
    }

    Super::OnTargetDataReady(Data);
}

static float CalcEffectiveAcceptanceRadius_Melee(ACharacter* SelfChar, AActor* TargetActor, float SkillRange)
{
    float SelfCapsuleR = 0.f;
    if (SelfChar && SelfChar->GetCapsuleComponent())
    {
        SelfCapsuleR = SelfChar->GetCapsuleComponent()->GetScaledCapsuleRadius();
    }
    float TargetCapsuleR = 0.f;
    if (ACharacter* TargetChar = Cast<ACharacter>(TargetActor))
    {
        if (TargetChar->GetCapsuleComponent())
        {
            TargetCapsuleR = TargetChar->GetCapsuleComponent()->GetScaledCapsuleRadius();
        }
    }
    const float Buffer = 10.f;
    return SkillRange + SelfCapsuleR + TargetCapsuleR + Buffer;
}

bool UGA_HeroMeleeAttackAbility::OtherCheckBeforeCommit()
{
    // 解析当前数据（若缺失则尝试用缓存推断）
    AActor* TargetActor = nullptr;
    FVector TargetLocation = FVector::ZeroVector;

    const FGameplayAbilityTargetData* Data = CurrentTargetDataHandle.Get(0);
    if (Data)
    {
        if (const FGameplayAbilityTargetData_SingleTargetHit* HR = static_cast<const FGameplayAbilityTargetData_SingleTargetHit*>(
                Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_SingleTargetHit::StaticStruct()) ? Data : nullptr))
        {
            if (HR->GetHitResult())
            {
                TargetLocation = HR->GetHitResult()->Location;
                TargetActor = HR->GetHitResult()->GetActor();
            }
        }
        else if (const FGameplayAbilityTargetData_ActorArray* AA = static_cast<const FGameplayAbilityTargetData_ActorArray*>(
                     Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_ActorArray::StaticStruct()) ? Data : nullptr))
        {
            if (AA->TargetActorArray.Num() > 0)
            {
                TargetActor = AA->TargetActorArray[0].Get();
                if (TargetActor)
                {
                    TargetLocation = TargetActor->GetActorLocation();
                }
            }
        }
    }

    if (!TargetActor && CachedTargetActor.IsValid())
    {
        TargetActor = CachedTargetActor.Get();
    }
    if (TargetLocation.IsNearlyZero() && !CachedTargetLocation.IsNearlyZero())
    {
        TargetLocation = CachedTargetLocation;
    }

    if (!TargetActor && TargetLocation.IsNearlyZero())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 没有有效目标可用于检查。"), *GetName());
        return false;
    }

    ACharacter* SelfChar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!SelfChar)
    {
        return false;
    }

    const float Distance = FVector::Distance(SelfChar->GetActorLocation(), TargetLocation);
    const float EffectiveAcceptance = CalcEffectiveAcceptanceRadius_Melee(SelfChar, TargetActor, RangeToApply);
    if (Distance > EffectiveAcceptance)
    {
        const bool bIsServer = SelfChar->HasAuthority();
        const bool bIsLocallyControlled = GetCurrentActorInfo() && GetCurrentActorInfo()->IsLocallyControlled();

        auto ThisAbility = const_cast<UGameplayAbility*>(static_cast<const UGameplayAbility*>(this));
        if (TargetActor)
        {
            if (bIsServer)
            {
                if (auto* MoveTask = UAbilityTask_WaitMoveToLocation::WaitMoveToActor(ThisAbility, TargetActor, EffectiveAcceptance))
                {
                    MoveTask->OnMoveFinished.AddDynamic(this, &UGA_HeroMeleeAttackAbility::OnReachedTarget);
                    MoveTask->OnMoveFailed.AddDynamic(this, &UGA_HeroMeleeAttackAbility::OnFailedToTarget);
                    MoveTask->ReadyForActivation();
                }
            }
            if (!bIsServer && bIsLocallyControlled)
            {
                if (auto* ClientMoveTask = UAbilityTask_WaitMoveToLocation::WaitMoveToActor(ThisAbility, TargetActor, EffectiveAcceptance))
                {
                    ClientMoveTask->OnMoveFinished.AddDynamic(this, &UGA_HeroMeleeAttackAbility::OnReachedTarget);
                    ClientMoveTask->OnMoveFailed.AddDynamic(this, &UGA_HeroMeleeAttackAbility::OnFailedToTarget);
                    ClientMoveTask->ReadyForActivation();
                }
            }
        }
        else
        {
            if (bIsServer)
            {
                if (auto* MoveTask = UAbilityTask_WaitMoveToLocation::WaitMoveToLocation(ThisAbility, TargetLocation, EffectiveAcceptance))
                {
                    MoveTask->OnMoveFinished.AddDynamic(this, &UGA_HeroMeleeAttackAbility::OnReachedTarget);
                    MoveTask->OnMoveFailed.AddDynamic(this, &UGA_HeroMeleeAttackAbility::OnFailedToTarget);
                    MoveTask->ReadyForActivation();
                }
            }
            if (!bIsServer && bIsLocallyControlled)
            {
                if (auto* ClientMoveTask = UAbilityTask_WaitMoveToLocation::WaitMoveToLocation(ThisAbility, TargetLocation, EffectiveAcceptance))
                {
                    ClientMoveTask->OnMoveFinished.AddDynamic(this, &UGA_HeroMeleeAttackAbility::OnReachedTarget);
                    ClientMoveTask->OnMoveFailed.AddDynamic(this, &UGA_HeroMeleeAttackAbility::OnFailedToTarget);
                    ClientMoveTask->ReadyForActivation();
                }
            }
        }

        return false;
    }

    return true;
}

void UGA_HeroMeleeAttackAbility::OnReachedTarget()
{
    if (!GetAvatarActorFromActorInfo() || !GetAvatarActorFromActorInfo()->HasAuthority()) return;
    // 如果当前 TargetData 在移动过程中丢失，这里用缓存重建
    if (!CurrentTargetDataHandle.IsValid(0))
    {
        FGameplayAbilityTargetDataHandle Rebuilt;
        if (CachedTargetActor.IsValid())
        {
            FGameplayAbilityTargetData_ActorArray* Arr = new FGameplayAbilityTargetData_ActorArray();
            Arr->TargetActorArray.Add(CachedTargetActor);
            Rebuilt.Add(Arr);
        }
        else if (!CachedTargetLocation.IsNearlyZero())
        {
            FHitResult HR;
            HR.Location = CachedTargetLocation;
            FGameplayAbilityTargetData_SingleTargetHit* HitData = new FGameplayAbilityTargetData_SingleTargetHit(HR);
            Rebuilt.Add(HitData);
        }
        if (Rebuilt.Num() > 0)
        {
            CurrentTargetDataHandle = Rebuilt;
        }
    }

    if (!CurrentTargetDataHandle.IsValid(0))
    {
        UGA_TargetedAbility::OnFailedToTarget();
        return;
    }
    Super::OnReachedTarget();
}

void UGA_HeroMeleeAttackAbility::OnFailedToTarget()
{
    UGA_TargetedAbility::OnFailedToTarget();
}

void UGA_HeroMeleeAttackAbility::ApplyCooldown(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo) const
{
    // 应用冷却效果
    if (CooldownGameplayEffectClass)
    {
        FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownGameplayEffectClass, GetAbilityLevel());
        if (SpecHandle.Data.IsValid())
        {
            FGameplayEffectSpec& GESpec = *SpecHandle.Data.Get();

            // 获取你在 GE 中配置的 Data Tag
            FGameplayTag CooldownDurationTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Cooldown.Duration")); // !!! 确保这里的 Tag 字符串与 GE 中配置的一致 !!!
            auto AttackSpeed = AttributeSet->GetAttackSpeed();
            if (AttackSpeed <= 0.0f)
            {
                AttackSpeed = 1.0f; // 防止除以零
            }
            float ThisCooldownDuration = 1.0 / AttackSpeed; // 使用局部变量代替类成员变量
            // 设置 Set by Caller 的值
            GESpec.SetSetByCallerMagnitude(CooldownDurationTag, ThisCooldownDuration);

            UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
            ASC->ApplyGameplayEffectSpecToSelf(GESpec);
        }
    }
}
