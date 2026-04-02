#include "GA_TargetedAbility.h"
#include "GameFramework/Pawn.h"
#include "Abilities/GameplayAbilityTargetActor_SingleLineTrace.h"
#include "IsometricAbilities/TargetTrace/GATA_CursorTrace.h"
#include "UObject/ConstructorHelpers.h"
#include "IsometricAbilities/GameplayAbilities/Targeted/HeroAbilityApproachHelper.h"
#include "IsometricAbilities/GameplayAbilities/Targeted/HeroTargetedAbilityPresentationHelper.h"

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
    APawn* TargetActor = nullptr;
    FVector TargetLocation = FVector::ZeroVector;
    if (!FHeroAbilityApproachHelper::TryResolveCommitTarget(CurrentTargetDataHandle, TargetActor, TargetLocation))
    {
        UE_LOG(LogTemp, Warning, TEXT("技能提交前检查：无法从TargetData中提取有效的目标。"));
        return false;
    }

    if (!TargetActor)
    {
        CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
        return false;
    }

    float Distance = 0.0f;
    if (!FHeroAbilityApproachHelper::IsTargetWithinRange(GetCurrentActorInfo(), TargetLocation, RangeToApply, Distance))
    {
        UE_LOG(LogTemp, Warning, TEXT("指向性技能 %s 的目标超出施法范围！"), *GetName());
        GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Black, TEXT("目标距离太远----来自技能内部"));
        FHeroAbilityApproachHelper::StartMoveToTarget(*this, *TargetActor, RangeToApply);
        return false;
    }

    return true;
}

bool UGA_TargetedAbility::EnsureCurrentTargetDataAvailable(
    const TWeakObjectPtr<AActor>& CachedTargetActor,
    const FVector& CachedTargetLocation)
{
    if (FHeroAbilityApproachHelper::TryRestoreTargetDataFromCache(
        CurrentTargetDataHandle,
        CachedTargetActor,
        CachedTargetLocation))
    {
        return true;
    }

    OnFailedToTarget();
    return false;
}

void UGA_TargetedAbility::OnReachedTarget()
{
    // 服务器负责提交与最终结算；客户端负责本地表现与阶段进入
    const AActor* Avatar = GetAvatarActorFromActorInfo();
    const bool bIsServer = (Avatar && Avatar->HasAuthority());
     
    if (bIsServer)
    {
        if (!TryCommitAndExecuteAbility(
            CurrentSpecHandle,
            CurrentActorInfo,
            CurrentActivationInfo,
            TEXT("OnReachedTarget"),
            TEXT("到达目标后提交前检查失败。"))
            && IsActive())
        {
            OnFailedToTarget();
        }
        return;
    }

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

    if (!GetAvatarActorFromActorInfo())
    {
        UE_LOG(LogTemp, Error, TEXT("'%s': Avatar actor is null. Cancelling."), *GetName());
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return;
    }

    FHeroTargetedAbilityPresentationHelper::SpawnRangeIndicator(*this, RangeToApply);

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
    FHeroTargetedAbilityPresentationHelper::SyncCurrentAbilityTargets(GetAvatarActorFromActorInfo(), Data);
    FHeroTargetedAbilityPresentationHelper::DestroyRangeIndicator(ActiveRangeIndicatorNiagaraActor);
    Super::OnTargetDataReady(Data);
}
void UGA_TargetedAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    FHeroTargetedAbilityPresentationHelper::ClearCurrentAbilityTargets(GetAvatarActorFromActorInfo());
    FHeroTargetedAbilityPresentationHelper::DestroyRangeIndicator(ActiveRangeIndicatorNiagaraActor);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
