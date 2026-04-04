#include "GA_TargetedAbility.h"
#include "GameFramework/Character.h"
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
    const TWeakObjectPtr<AActor>& CachedTargetActorOverride,
    const FVector& CachedTargetLocationOverride)
{
    if (FHeroAbilityApproachHelper::TryRestoreTargetDataFromCache(
        CurrentTargetDataHandle,
        CachedTargetActorOverride,
        CachedTargetLocationOverride))
    {
        return true;
    }

    OnFailedToTarget();
    return false;
}

void UGA_TargetedAbility::CacheCurrentTargetDataForApproach(const FGameplayAbilityTargetDataHandle& Data)
{
    FHeroAbilityApproachHelper::CachePrimaryTargetData(Data, CachedTargetActor, CachedTargetLocation);
}

bool UGA_TargetedAbility::CheckCachedTargetInRangeOrStartApproach(const float AcceptanceRadius)
{
    AActor* TargetActor = nullptr;
    FVector TargetLocation = FVector::ZeroVector;
    if (!FHeroAbilityApproachHelper::TryResolveTargetOrCached(
        CurrentTargetDataHandle,
        CachedTargetActor,
        CachedTargetLocation,
        TargetActor,
        TargetLocation))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 没有有效目标可用于检查。"), *GetName());
        return false;
    }

    ACharacter* SelfCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!SelfCharacter)
    {
        return false;
    }

    const float Distance = FVector::Distance(SelfCharacter->GetActorLocation(), TargetLocation);
    if (Distance > AcceptanceRadius)
    {
        FHeroAbilityApproachHelper::StartMoveToActorOrLocation(*this, TargetActor, TargetLocation, AcceptanceRadius);
        return false;
    }

    return true;
}

void UGA_TargetedAbility::OnReachedTarget()
{
    // 服务器负责提交与最终结算；客户端负责本地表现与阶段进入
    const AActor* Avatar = GetAvatarActorFromActorInfo();
    const bool bIsServer = (Avatar && Avatar->HasAuthority());
     
    if (bIsServer)
    {
        if (!EnsureCurrentTargetDataAvailable(CachedTargetActor, CachedTargetLocation))
        {
            return;
        }

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

void UGA_TargetedAbility::ConfigureTargetSelectionActor(AGameplayAbilityTargetActor& TargetActor)
{
    AGATA_CursorTrace* CursorTraceTargetActor = Cast<AGATA_CursorTrace>(&TargetActor);
    if (!CursorTraceTargetActor)
    {
        return;
    }

    const ECollisionChannel TraceChannel =
        (AbilityType == EHeroAbilityType::SkillShot || AbilityType == EHeroAbilityType::AreaEffect)
        ? ECC_Visibility
        : ECC_Pawn;
    CursorTraceTargetActor->TraceChannel = UEngineTypes::ConvertToTraceType(TraceChannel);
    CursorTraceTargetActor->bTraceComplex = false;
}

void UGA_TargetedAbility::BeginTargetSelectionPresentation()
{
    FHeroTargetedAbilityPresentationHelper::SpawnRangeIndicator(*this, RangeToApply);
}

void UGA_TargetedAbility::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
    CacheCurrentTargetDataForApproach(Data);
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
