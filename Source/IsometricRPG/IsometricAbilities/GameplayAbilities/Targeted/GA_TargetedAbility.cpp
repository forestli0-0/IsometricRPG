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

UGA_TargetedAbility::UGA_TargetedAbility()
{
	RangeToApply = 100.f;
}
bool UGA_TargetedAbility::OtherCheckBeforeCommit(const FGameplayAbilityTargetDataHandle& Data) const
{
	// 对于指向性技能，检查目标是否有效
	if (Data.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("技能提交前检查：目标为空"));
		return false;
	}
	// 检查是否在施法范围内
	auto TargetActorLocation = Data.Get(0)->GetHitResult()->GetActor()->GetActorLocation();
	auto SelfActorLocation = GetCurrentActorInfo()->AvatarActor->GetActorLocation();
	auto Distance = FVector::Distance(TargetActorLocation, SelfActorLocation);
	if (Distance > RangeToApply)
	{
		UE_LOG(LogTemp, Warning, TEXT("指向性技能 %s 的目标超出施法范围！"), *GetName());
        // 构造失败的 gameplay event
        FGameplayEventData FailEventData;
        FailEventData.EventTag = FGameplayTag::RequestGameplayTag(FName("Ability.Failure.OutOfRange"));

        AActor* SelfActor = GetAvatarActorFromActorInfo();  
        FailEventData.Instigator = SelfActor;

        AActor* Target = Data.Get(0)->GetHitResult()->GetActor();
        FailEventData.Target = Target;

        //// 通知自身 ASC
        //GetAbilitySystemComponentFromActorInfo()->HandleGameplayEvent(FailEventData.EventTag, &FailEventData);
        auto ThisAbility = const_cast<UGameplayAbility*>(static_cast<const UGameplayAbility*>(this));
        UAbilityTask_WaitMoveToLocation* MoveTask = UAbilityTask_WaitMoveToLocation::WaitMoveToActor(ThisAbility,Target);
        // Replace the line causing the error with the following code
        MoveTask->OnMoveFinished.AddDynamic(this, &UGA_TargetedAbility::OnReachedTarget);
        MoveTask->OnMoveFailed.AddDynamic(this, &UGA_TargetedAbility::OnFailedToTarget);
		MoveTask->ReadyForActivation();
        GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Black, TEXT("目标距离太远----来自技能内部"));
		return false;
	}
	return true;
}
bool UGA_TargetedAbility::OtherCheckBeforeCommit(const FGameplayEventData* TriggerEventData) const
{
    // 对于指向性技能，检查目标是否有效
    if (!TriggerEventData)
    {
        UE_LOG(LogTemp, Warning, TEXT("技能提交前检查：目标为空"));
        return false;
    }
    // 检查是否在施法范围内
    auto TargetActor = TriggerEventData->Target;
    auto TargetActorLocation = TargetActor->GetActorLocation();
    auto SelfActorLocation = GetCurrentActorInfo()->AvatarActor->GetActorLocation();
    auto Distance = FVector::Distance(TargetActorLocation, SelfActorLocation);
    if (Distance > RangeToApply)
    {
        UE_LOG(LogTemp, Warning, TEXT("指向性技能 %s 的目标超出施法范围！"), *GetName());
        //// 通知自身 ASC
        //GetAbilitySystemComponentFromActorInfo()->HandleGameplayEvent(FailEventData.EventTag, &FailEventData);
        auto ThisAbility = const_cast<UGameplayAbility*>(static_cast<const UGameplayAbility*>(this));
        UAbilityTask_WaitMoveToLocation* MoveTask = UAbilityTask_WaitMoveToLocation::WaitMoveToActor(ThisAbility, const_cast<AActor*>(TargetActor.Get()));
        // Replace the line causing the error with the following code
        MoveTask->OnMoveFinished.AddDynamic(this, &UGA_TargetedAbility::OnReachedTarget);
        MoveTask->OnMoveFailed.AddDynamic(this, &UGA_TargetedAbility::OnFailedToTarget);
        MoveTask->ReadyForActivation();
        GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Black, TEXT("目标距离太远----来自技能内部"));
        return false;
    }
    return true;
}

void UGA_TargetedAbility::OnReachedTarget()
{
    if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Failed to commit ability after target data ready."), *GetName());
        constexpr bool bReplicateEndAbility = true;
        constexpr bool bWasCancelled = true;
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCancelled);
        return;
    }

    // 播放技能动画 (如果有)
    PlayAbilityMontage(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);

    // 创建 GameplayEventData 并填充目标信息
    FGameplayEventData EventData;
    if (CurrentTargetDataHandle.Num() > 0 && CurrentTargetDataHandle.Get(0) != nullptr)
    {
        EventData.TargetData = CurrentTargetDataHandle;
    }

    // 执行技能逻辑
    ExecuteSkill(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, &EventData);
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
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
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

    // 创建 Decal
    FVector DecalLocation = Avatar->GetActorLocation() - FVector(0, 0, 88);
    FRotator DecalRotation = FRotator(0, 0, 1.0f); // 指向地面
    FVector DecalSize = FVector(RangeToApply, RangeToApply, RangeToApply); // Decal 的大小
    // Create a Decal Actor instead of directly assigning the UDecalComponent
    ADecalActor* DecalActor = GetWorld()->SpawnActor<ADecalActor>(DecalLocation, DecalRotation);
    if (DecalActor)
    {
        DecalActor->SetDecalMaterial(RangeIndicatorMaterial);
        DecalActor->GetDecal()->DecalSize = DecalSize;
        // No other changes are needed in the existing code
        DecalActor->SetLifeSpan(3.0f); // Set the lifespan of the decal actor

        DecalActor->AttachToActor(Avatar, FAttachmentTransformRules::KeepWorldTransform);
        DecalActor->SetActorRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

        RangeIndicatorDecal = DecalActor; // Assign the DecalActor to RangeIndicatorDecal
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to spawn DecalActor for RangeIndicatorDecal"));
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
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}