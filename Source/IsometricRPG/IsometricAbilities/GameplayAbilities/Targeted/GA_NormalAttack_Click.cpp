// Fill out your copyright notice in the Description page of Project Settings.


//#include "IsometricAbilities/GameplayAbilities/Targeted/GA_NormalAttack_Click.h"
#include "GA_NormalAttack_Click.h"
#include "IsometricAbilities/TargetTrace/GATA_CursorTrace.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "IsometricAbilities/AbilityTasks/AbilityTask_WaitMoveToLocation.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

UGA_NormalAttack_Click::UGA_NormalAttack_Click()
{
    // 按标准 Targeted 流程处理：需要目标数据（若已有Actor则直接用，否则进入目标选择）
    bRequiresTargetData = true;
    TargetActorClass = AGATA_CursorTrace::StaticClass();
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
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
    // 设置标签/触发
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Player.DirBasicAttack"));

    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Player.DirBasicAttack"));

    // 设置触发事件
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag("Ability.Player.DirBasicAttack");
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
    // 设定冷却阻断标签
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Cooldown.Ability.MeleeAttack"));

}
void UGA_NormalAttack_Click::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	// 普攻的命中绑定在动画里，用动画通知触发
    // 打印调试信息
    UE_LOG(LogTemp, Warning, TEXT("使用右键普攻"));

    AActor* OwnerActor = GetAvatarActorFromActorInfo();

    if (!OwnerActor) return;

    // 使用缓存的目标数据进行调试打印，避免重复解析
    if (CachedTargetActor.IsValid())
    {
        UE_LOG(LogTemp, Verbose, TEXT("攻击目标位置: %s, 自身位置: %s"),
            *CachedTargetActor->GetActorLocation().ToString(),
            *OwnerActor->GetActorLocation().ToString());
    }
    else if (!CachedTargetLocation.IsNearlyZero())
    {
        UE_LOG(LogTemp, Verbose, TEXT("攻击目标位置: %s, 自身位置: %s"),
            *CachedTargetLocation.ToString(),
            *OwnerActor->GetActorLocation().ToString());
    }

    if (!MontageToPlay)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}
void UGA_NormalAttack_Click::ApplyCooldown(
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
    // 确保通知系统冷却已触发（UI刷新等依赖此通知）
    NotifyCooldownTriggered(Handle, ActorInfo);
}

void UGA_NormalAttack_Click::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
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

    // 交回基类，继续原有流程（会调用 OtherCheckBeforeCommit → Commit/移动任务）
    Super::OnTargetDataReady(Data);
}

static float CalcEffectiveAcceptanceRadius(ACharacter* SelfChar, AActor* TargetActor, float SkillRange)
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

bool UGA_NormalAttack_Click::OtherCheckBeforeCommit()
{
    // 直接使用 OnTargetDataReady 中解析并缓存的数据，避免重复解析 CurrentTargetDataHandle
    AActor* TargetActor = CachedTargetActor.Get();
    FVector TargetLocation = CachedTargetLocation;

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
    const float EffectiveAcceptance = CalcEffectiveAcceptanceRadius(SelfChar, TargetActor, RangeToApply);
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
                    MoveTask->OnMoveFinished.AddDynamic(this, &UGA_NormalAttack_Click::OnReachedTarget);
                    MoveTask->OnMoveFailed.AddDynamic(this, &UGA_NormalAttack_Click::OnFailedToTarget);
                    MoveTask->ReadyForActivation();
                }
            }
            if (!bIsServer && bIsLocallyControlled)
            {
                if (auto* ClientMoveTask = UAbilityTask_WaitMoveToLocation::WaitMoveToActor(ThisAbility, TargetActor, EffectiveAcceptance))
                {
                    ClientMoveTask->OnMoveFinished.AddDynamic(this, &UGA_NormalAttack_Click::OnReachedTarget);
                    ClientMoveTask->OnMoveFailed.AddDynamic(this, &UGA_NormalAttack_Click::OnFailedToTarget);
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
                    MoveTask->OnMoveFinished.AddDynamic(this, &UGA_NormalAttack_Click::OnReachedTarget);
                    MoveTask->OnMoveFailed.AddDynamic(this, &UGA_NormalAttack_Click::OnFailedToTarget);
                    MoveTask->ReadyForActivation();
                }
            }
            if (!bIsServer && bIsLocallyControlled)
            {
                if (auto* ClientMoveTask = UAbilityTask_WaitMoveToLocation::WaitMoveToLocation(ThisAbility, TargetLocation, EffectiveAcceptance))
                {
                    ClientMoveTask->OnMoveFinished.AddDynamic(this, &UGA_NormalAttack_Click::OnReachedTarget);
                    ClientMoveTask->OnMoveFailed.AddDynamic(this, &UGA_NormalAttack_Click::OnFailedToTarget);
                    ClientMoveTask->ReadyForActivation();
                }
            }
        }

        return false;
    }

    return true;
}

void UGA_NormalAttack_Click::OnReachedTarget()
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

    // 若仍无有效目标数据，则按失败处理
    if (!CurrentTargetDataHandle.IsValid(0))
    {
        UGA_TargetedAbility::OnFailedToTarget();
        return;
    }
    // 继续基类：提交与执行
    Super::OnReachedTarget();
}

void UGA_NormalAttack_Click::OnFailedToTarget()
{
    // 失败沿用父类的取消逻辑
    UGA_TargetedAbility::OnFailedToTarget();
}