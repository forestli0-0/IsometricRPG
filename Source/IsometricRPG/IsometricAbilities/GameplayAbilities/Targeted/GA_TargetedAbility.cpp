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
#include "UObject/ConstructorHelpers.h"
#include "IsometricAbilities/GameplayAbilities/HeroAbilityTargetDataHelper.h"

UGA_TargetedAbility::UGA_TargetedAbility()
{
	RangeToApply = 100.f;
	TargetActorClass = AGATA_CursorTrace::StaticClass();
	InputPolicy.InputMode = EAbilityInputMode::Instant;
	InputPolicy.bUpdateTargetWhileHeld = false;
	InputPolicy.bAllowInputBuffer = true;
	InputPolicy.MaxBufferWindow = 0.25f;

	static ConstructorHelpers::FClassFinder<ANiagaraActor> RangeIndicatorFinder(TEXT("/Game/Blueprints/FX/BP_NA_NiagaraActorBase"));
	if (RangeIndicatorFinder.Succeeded())
	{
		RangeIndicatorNiagaraActorClass = RangeIndicatorFinder.Class;
	}
}

bool UGA_TargetedAbility::OtherCheckBeforeCommit()
{
    AActor* TargetActor = nullptr;
    FVector TargetLocation = FVector::ZeroVector;
    if (!FHeroAbilityTargetDataHelper::TryGetTargetLocation(CurrentTargetDataHandle, TargetLocation))
    {
        UE_LOG(LogTemp, Warning, TEXT("技能提交前检查：无法从TargetData中提取有效的目标。"));
        return false;
    }
    TargetActor = Cast<APawn>(FHeroAbilityTargetDataHelper::GetPrimaryActor(CurrentTargetDataHandle));

    if (!TargetActor)
    {
        CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
        return false;
    }

    //GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Black, FString::Printf(TEXT("目标位置: %s"), *TargetLocation.ToString()));

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
            const AActor* Avatar = GetAvatarActorFromActorInfo();
            const bool bIsServer = (Avatar && Avatar->HasAuthority());
            const bool bIsLocallyControlled = (GetCurrentActorInfo() && GetCurrentActorInfo()->IsLocallyControlled());

            // 服务器：负责真正的到达判断与提交执行
            if (bIsServer)
            {
                auto ThisAbility = const_cast<UGameplayAbility*>(static_cast<const UGameplayAbility*>(this));
                UAbilityTask_WaitMoveToLocation* MoveTask = UAbilityTask_WaitMoveToLocation::WaitMoveToActor(ThisAbility, TargetActor, RangeToApply);
                if (MoveTask)
                {
                    MoveTask->OnMoveFinished.AddDynamic(this, &UGA_TargetedAbility::OnReachedTarget);
                    MoveTask->OnMoveFailed.AddDynamic(this, &UGA_TargetedAbility::OnFailedToTarget);
                    MoveTask->ReadyForActivation();
                    UE_LOG(LogTemp, Verbose, TEXT("%s: Server started WaitMoveToActor (Acceptance=%.1f)."), *GetName(), RangeToApply);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("%s: Failed to create WaitMoveToActor on server."), *GetName());
                }
            }

        // 本地控制的客户端：也创建一个镜像任务用于本地预测和平滑动画；
            if (!bIsServer && bIsLocallyControlled)
            {
                auto ThisAbility = const_cast<UGameplayAbility*>(static_cast<const UGameplayAbility*>(this));
                UAbilityTask_WaitMoveToLocation* ClientMoveTask = UAbilityTask_WaitMoveToLocation::WaitMoveToActor(ThisAbility, TargetActor, RangeToApply);
                if (ClientMoveTask)
                {
                    ClientMoveTask->OnMoveFinished.AddDynamic(this, &UGA_TargetedAbility::OnReachedTarget);
                    ClientMoveTask->OnMoveFailed.AddDynamic(this, &UGA_TargetedAbility::OnFailedToTarget);
                    ClientMoveTask->ReadyForActivation();
                    UE_LOG(LogTemp, Verbose, TEXT("%s: Client mirror WaitMoveToActor started for prediction (Acceptance=%.1f)."), *GetName(), RangeToApply);
                }
        }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("目标超出范围，但没有有效Actor可移动，技能检查失败。"));
        }

        return false;
    }
    else
    {
        return true;
    }
    
}

void UGA_TargetedAbility::OnReachedTarget()
{
    // 服务器负责提交与最终结算；客户端负责本地表现与阶段进入
    const AActor* Avatar = GetAvatarActorFromActorInfo();
    const bool bIsServer = (Avatar && Avatar->HasAuthority());
     
    if (bIsServer)
    {
	// 因为已经移动到目标位置，此时主函数已经结束，在这里继续执行后续逻辑
    if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Failed to commit ability after reaching target (server)."), *GetName());
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }
    }

    // 无论服务端还是拥有者客户端，均进入技能阶段：
    // - 服务端：继续推进阶段并进行权威位移/伤害
    // - 客户端：仅播放蒙太奇/特效与本地阶段表现（权威逻辑在服务端）
    PlayAbilityMontage(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
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
    if (!Avatar)
    {
        UE_LOG(LogTemp, Error, TEXT("'%s': Avatar actor is null. Cancelling."), *GetName());
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return;
    }

    auto AvatarCharacter = Cast<ACharacter>(Avatar);
    if (RangeIndicatorNiagaraActorClass && AvatarCharacter && AvatarCharacter->GetMesh())
    {
	    const FVector SpawnLocation = AvatarCharacter->GetMesh()->GetComponentLocation();
        const FRotator SpawnRotation = FRotator::ZeroRotator;

        ANiagaraActor* NiagaraRangeIndicator = GetWorld()->SpawnActor<ANiagaraActor>(
            RangeIndicatorNiagaraActorClass,
            SpawnLocation,
            SpawnRotation
        );

        if (NiagaraRangeIndicator)
        {
            NiagaraRangeIndicator->AttachToComponent(
                AvatarCharacter->GetMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                FName("Root")
            );
            if (NiagaraRangeIndicator->GetNiagaraComponent())
            {
                NiagaraRangeIndicator->GetNiagaraComponent()->SetFloatParameter(FName("user_Radius"), RangeToApply);
            }

            ActiveRangeIndicatorNiagaraActor = NiagaraRangeIndicator;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Failed to spawn NiagaraActor for RangeIndicator. Continuing without indicator."), *GetName());
        }
    }
    else if (RangeIndicatorNiagaraActorClass)
    {
        ensureAlwaysMsgf(
            AvatarCharacter && AvatarCharacter->GetMesh(),
            TEXT("%s: Cannot attach range indicator because avatar %s is not a character with a valid mesh. Continuing without indicator."),
            *GetName(),
            *GetNameSafe(Avatar));
    }
    else
    {
        UE_LOG(LogTemp, Verbose, TEXT("%s: RangeIndicatorNiagaraActorClass is not configured. Continuing without indicator."), *GetName());
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
        TargetDataTask->BeginSpawningActor(this, TargetActorClass, TempSpawnedTargetActorRaw);
        AGATA_CursorTrace* SpawnedTargetActor = Cast<AGATA_CursorTrace>(TempSpawnedTargetActorRaw);
        if (SpawnedTargetActor)
        {
            const ECollisionChannel TraceChannel =
                (AbilityType == EHeroAbilityType::SkillShot || AbilityType == EHeroAbilityType::AreaEffect)
                ? ECC_Visibility
                : ECC_Pawn;
            SpawnedTargetActor->TraceChannel = UEngineTypes::ConvertToTraceType(TraceChannel);
            SpawnedTargetActor->bTraceComplex = false; // 根据需要设置
        }
        TargetDataTask->FinishSpawningActor(this, TempSpawnedTargetActorRaw);
        TargetDataTask->ReadyForActivation();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("'%s': FAILED to create TargetDataTask!"), *GetName());
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
    }
}
void UGA_TargetedAbility::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
    auto MyCharacter = Cast<AIsometricRPGCharacter>(GetAvatarActorFromActorInfo());
    if (MyCharacter)
    {
        MyCharacter->CurrentAbilityTargets.Empty(); // Clear previous
        for (int32 TargetDataIndex = 0; TargetDataIndex < Data.Num(); ++TargetDataIndex)
        {
            const FGameplayAbilityTargetData* TargetData = Data.Get(TargetDataIndex);
            if (TargetData)
            {
                if (AActor* Actor = FHeroAbilityTargetDataHelper::GetPrimaryActor(*TargetData))
                {
                    MyCharacter->CurrentAbilityTargets.AddUnique(Actor);
                }

                for (const TWeakObjectPtr<AActor>& WeakActor : TargetData->GetActors())
                {
                    if (AActor* Actor = WeakActor.Get())
                    {
                        MyCharacter->CurrentAbilityTargets.AddUnique(Actor);
                    }
                }
            }
        }
    }
    // 选中目标后立即移除范围指示圈，避免后续阶段（起跳/移动）仍附着在角色身上
    if (ActiveRangeIndicatorNiagaraActor && IsValid(ActiveRangeIndicatorNiagaraActor))
    {
        ActiveRangeIndicatorNiagaraActor->Destroy();
        ActiveRangeIndicatorNiagaraActor = nullptr;
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
