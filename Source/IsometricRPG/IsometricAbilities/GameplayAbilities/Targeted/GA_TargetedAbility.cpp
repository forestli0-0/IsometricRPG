#include "GA_TargetedAbility.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTargetActor_SingleLineTrace.h"
#include "Components/DecalComponent.h" // Add this include to resolve the incomplete type error for UDecalComponent
#include <Kismet/GameplayStatics.h>
#include "IsometricAbilities/TargetTrace/GATA_CursorTrace.h"
#include "IsometricAbilities/AbilityTasks/AbilityTask_WaitMoveToLocation.h"
#include "Character/IsometricRPGCharacter.h"
#include "Components/CapsuleComponent.h" // 添加此行以解决不完整类型错误
#include "NiagaraComponent.h" // Add this include to resolve the incomplete type error for UNiagaraComponent

UGA_TargetedAbility::UGA_TargetedAbility()
{
	RangeToApply = 100.f;
}

bool UGA_TargetedAbility::OtherCheckBeforeCommit()
{
    // 1. 检查目标数据有效性，使用卫语句提前退出
    if (!CurrentTargetDataHandle.IsValid(0))
    {
        UE_LOG(LogTemp, Warning, TEXT("技能提交前检查：目标数据无效或为空。"));
        return false;
    }

    // 2. 声明变量并解析目标数据
    // 修正了原始代码中变量被重复声明和覆盖的问题
    AActor* TargetActor = nullptr;
    FVector TargetLocation = FVector::ZeroVector;
    bool bSuccessfullyFoundTarget = false;

    const FGameplayAbilityTargetData* Data = CurrentTargetDataHandle.Get(0);
    // 情况一：目标是 Actor
    if (Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_ActorArray::StaticStruct()))
    {
        const auto* ActorArrayData = static_cast<const FGameplayAbilityTargetData_ActorArray*>(Data);
        if (ActorArrayData && ActorArrayData->TargetActorArray.Num() > 0)
        {
            TargetActor = ActorArrayData->TargetActorArray[0].Get();
            if (TargetActor)
            {
                TargetLocation = TargetActor->GetActorLocation();
                bSuccessfullyFoundTarget = true;
                UE_LOG(LogTemp, Log, TEXT("TargetData is an Actor. Location: %s"), *TargetLocation.ToString());
            }
        }
    }
    // 情况二：目标是射线检测点
    else if (Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_SingleTargetHit::StaticStruct()))
    {
        const auto* HitResultData = static_cast<const FGameplayAbilityTargetData_SingleTargetHit*>(Data);
        if (HitResultData && HitResultData->GetHitResult())
        {
            TargetLocation = HitResultData->GetHitResult()->Location;
            TargetActor = HitResultData->GetHitResult()->GetActor(); // 尝试获取被击中的Actor
            bSuccessfullyFoundTarget = true;
            UE_LOG(LogTemp, Log, TEXT("TargetData is a HitResult. Location: %s"), *TargetLocation.ToString());
        }
    }

    // 如果未能从任何已知类型中解析出目标，则检查失败
    if (!bSuccessfullyFoundTarget)
    {
        UE_LOG(LogTemp, Warning, TEXT("技能提交前检查：无法从TargetData中提取有效的目标。"));
        return false;
    }

    GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Black, FString::Printf(TEXT("目标位置: %s"), *TargetLocation.ToString()));

    // 3. 保留原始的距离检查和后续逻辑
    const FVector SelfActorLocation = GetCurrentActorInfo()->AvatarActor->GetActorLocation();
    const float Distance = FVector::Distance(TargetLocation, SelfActorLocation);

    if (Distance > RangeToApply)
    {
        // 距离过远，创建移动任务并返回 false
        UE_LOG(LogTemp, Warning, TEXT("指向性技能 %s 的目标超出施法范围！"), *GetName());
        GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Black, TEXT("目标距离太远----来自技能内部"));

        // [安全性检查] 确保有有效的目标Actor才能创建WaitMoveToActor任务
        if (TargetActor)
        {
            auto ThisAbility = const_cast<UGameplayAbility*>(static_cast<const UGameplayAbility*>(this));
            UAbilityTask_WaitMoveToLocation* MoveTask = UAbilityTask_WaitMoveToLocation::WaitMoveToActor(ThisAbility, TargetActor);
            MoveTask->OnMoveFinished.AddDynamic(this, &UGA_TargetedAbility::OnReachedTarget);
            MoveTask->OnMoveFailed.AddDynamic(this, &UGA_TargetedAbility::OnFailedToTarget);
            MoveTask->ReadyForActivation();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("目标超出范围，但没有有效Actor可移动，技能检查失败。"));
        }

        return false;
    }
    else
    {
        // 距离足够，调用 OnReachedTarget 并返回 true
        OnReachedTarget();
        return true;
    }
}

void UGA_TargetedAbility::OnReachedTarget()
{
	// 因为已经移动到目标位置，此时主函数已经结束，在这里继续执行后续逻辑
    if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Failed to commit ability after target data ready."), *GetName());
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    // 播放技能动画 (如果有)
    PlayAbilityMontage(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);

    // 执行技能逻辑
    ExecuteSkill(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
}
void UGA_TargetedAbility::OnFailedToTarget()
{
    constexpr bool bReplicateEndAbility = true;
    constexpr bool bWasCancelled = true;
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCancelled);
}
void UGA_TargetedAbility::StartTargetSelection(
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

    AActor* Avatar = GetAvatarActorFromActorInfo();
    if (!Avatar || !RangeIndicatorMaterial)
    {
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return;
    }

    if (!RangeIndicatorNiagaraActorClass) // 检查新的 NiagaraActor 类
    {
        UE_LOG(LogTemp, Error, TEXT("'%s': RangeIndicatorNiagaraActorClass is NOT SET! Cancelling."), *GetName());
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return;
    }
    //FVector SpawnLocation = Avatar->GetActorLocation() - Avatar->GetComponentByClass<UCapsuleComponent>()->GetScaledCapsuleHalfHeight(); // 初始位置与Avatar相同
    auto AvatarCharacter = Cast<ACharacter>(Avatar);
	FVector SpawnLocation = AvatarCharacter->GetMesh()->GetComponentLocation();
    FRotator SpawnRotation = FRotator::ZeroRotator;


    ANiagaraActor* NiagaraRangeIndicator = GetWorld()->SpawnActor<ANiagaraActor>(
        RangeIndicatorNiagaraActorClass,
        SpawnLocation,
        SpawnRotation
    );

    if (NiagaraRangeIndicator)
    {
        //NiagaraRangeIndicator->AttachToActor(Avatar, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
            // 附着到角色的Skeletal Mesh Component上的指定Socket
        NiagaraRangeIndicator->AttachToComponent(
            AvatarCharacter->GetMesh(), // 父组件是Skeletal Mesh Component
            FAttachmentTransformRules::SnapToTargetNotIncludingScale, // 依然使用SnapToTarget
            FName("Root") // 您在编辑器中创建的Socket名称
        );
        if (NiagaraRangeIndicator->GetNiagaraComponent())
        {
            NiagaraRangeIndicator->GetNiagaraComponent()->SetFloatParameter(FName("user_Radius"), RangeToApply);

            // 如果你的Niagara System用Vector来控制大小，例如 (RangeToApply, RangeToApply, 1.0f)
            // NiagaraRangeIndicator->GetNiagaraComponent()->SetVectorParameter(FName("Scale"), FVector(RangeToApply, RangeToApply, 1.0f));
        }

        ActiveRangeIndicatorNiagaraActor = NiagaraRangeIndicator;

        // NiagaraRangeIndicator->SetLifeSpan(3.0f); // 如果需要自动销毁
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to spawn NiagaraActor for RangeIndicator"));
    }

    TargetDataTask = UAbilityTask_WaitTargetData::WaitTargetData(
        this,
        FName("TargetData"),
        EGameplayTargetingConfirmation::UserConfirmed,
        TargetActorClass
    );

    if (TargetDataTask)
    {
        TargetDataTask->ValidData.AddDynamic(this, &UGA_TargetedAbility::OnTargetDataReady);
        TargetDataTask->Cancelled.AddDynamic(this, &UGA_HeroBaseAbility::OnTargetingCancelled);

        // 如果需要在 GATA 实例上设置属性 (比如 TraceChannel)
        AGameplayAbilityTargetActor* TempSpawnedTargetActorRaw = nullptr;
        TargetDataTask->BeginSpawningActor(this, AGATA_CursorTrace::StaticClass(), TempSpawnedTargetActorRaw);
        AGATA_CursorTrace* SpawnedTargetActor = Cast<AGATA_CursorTrace>(TempSpawnedTargetActorRaw);
        if (SpawnedTargetActor)
        {
            // 例如，如果你想追踪 Pawn 而不是 Visibility
            SpawnedTargetActor->TraceChannel = UEngineTypes::ConvertToTraceType(ECC_Pawn);
            SpawnedTargetActor->bTraceComplex = false; // 根据需要设置
            // 你甚至可以在这里传递过滤参数等
        }
        TargetDataTask->FinishSpawningActor(this, SpawnedTargetActor);
        TargetDataTask->ReadyForActivation();
    }
}
void UGA_TargetedAbility::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
    auto MyCharacter = Cast<AIsometricRPGCharacter>(GetAvatarActorFromActorInfo());
    if (MyCharacter)
    {
        MyCharacter->CurrentAbilityTargets.Empty(); // Clear previous
        for (const TSharedPtr<FGameplayAbilityTargetData>& TargetData : Data.Data)
        {
            if (TargetData.IsValid())
            {
                    for (TWeakObjectPtr<AActor> WeakActor : TargetData->GetActors())  
                    {  
                        if (AActor* Actor = WeakActor.Get())  
                        {  
                            MyCharacter->CurrentAbilityTargets.Add(Actor);  
                        }  
                    }
            }
        }
    }
    // Call Super if it does important things you want to keep
    Super::OnTargetDataReady(Data);
}
void UGA_TargetedAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    auto MyCharacter = Cast<AIsometricRPGCharacter>(GetAvatarActorFromActorInfo());
    if (MyCharacter)
    {
        MyCharacter->CurrentAbilityTargets.Empty(); // Clear previous
    }
    if (ActiveRangeIndicatorNiagaraActor && IsValid(ActiveRangeIndicatorNiagaraActor))
    {
        ActiveRangeIndicatorNiagaraActor->Destroy();
        ActiveRangeIndicatorNiagaraActor = nullptr; // 清空引用
    }
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}