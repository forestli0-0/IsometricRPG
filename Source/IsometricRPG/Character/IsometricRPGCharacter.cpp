#include "IsometricRPGCharacter.h"
#include "Character/IsometricRPGCharacterMovementComponent.h"
#include "EnhancedInputComponent.h" 
#include "Components/SceneComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameplayAbilitySpec.h"
#include "IsoPlayerState.h"
#include "AnimationBlueprintLibrary.h"
#include "Components/PrimitiveComponent.h"
#include <Blueprint/AIBlueprintHelperLibrary.h>
#include "GameFramework/PlayerState.h"
#include "IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "IsometricRPGAttributeSetBase.h"
#include <AbilitySystemBlueprintLibrary.h>
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "IsometricAbilities/GameplayAbilities/GA_HeroBaseAbility.h"

namespace
{
FVector ResolveAbilityAimPoint(const FHitResult& HitResult, const AActor* TargetActor)
{
    if (HitResult.bBlockingHit || HitResult.GetActor() != nullptr)
    {
        if (!HitResult.Location.IsNearlyZero())
        {
            return HitResult.Location;
        }

        if (!HitResult.ImpactPoint.IsNearlyZero())
        {
            return HitResult.ImpactPoint;
        }
    }

    return TargetActor ? TargetActor->GetActorLocation() : FVector::ZeroVector;
}

FVector ResolveAbilityAimPointFromTargetData(const FGameplayAbilityTargetDataHandle& TargetData)
{
    const FGameplayAbilityTargetData* Data = TargetData.Get(0);
    if (!Data)
    {
        return FVector::ZeroVector;
    }

    if (Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_SingleTargetHit::StaticStruct()))
    {
        const auto* HitResultData = static_cast<const FGameplayAbilityTargetData_SingleTargetHit*>(Data);
        if (HitResultData && HitResultData->GetHitResult())
        {
            return HitResultData->GetHitResult()->Location;
        }
    }

    if (Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_ActorArray::StaticStruct()))
    {
        const auto* ActorArrayData = static_cast<const FGameplayAbilityTargetData_ActorArray*>(Data);
        if (ActorArrayData && ActorArrayData->TargetActorArray.Num() > 0 && ActorArrayData->TargetActorArray[0].IsValid())
        {
            return ActorArrayData->TargetActorArray[0]->GetActorLocation();
        }
    }

    return FVector::ZeroVector;
}

FVector ResolveAbilityAimDirection(const AActor* SourceActor, const FVector& AimPoint, const AActor* TargetActor)
{
    if (!SourceActor)
    {
        return FVector::ZeroVector;
    }

    FVector TargetLocation = AimPoint;
    if (TargetLocation.IsNearlyZero() && TargetActor)
    {
        TargetLocation = TargetActor->GetActorLocation();
    }

    FVector Direction = TargetLocation - SourceActor->GetActorLocation();
    Direction.Z = 0.0f;
    return Direction.GetSafeNormal();
}

FPendingAbilityActivationContext BuildPendingAbilityActivationContext(
    const AActor* SourceActor,
    EAbilityInputID InputID,
    EInputEventPhase TriggerPhase,
    bool bUseActorTarget,
    bool bIsHeldInput,
    float HeldDuration,
    const FHitResult& HitResult,
    AActor* TargetActor)
{
    FPendingAbilityActivationContext Context;
    Context.InputID = InputID;
    Context.TriggerPhase = TriggerPhase;
    Context.bUseActorTarget = bUseActorTarget && TargetActor != nullptr;
    Context.bIsHeldInput = bIsHeldInput;
    Context.HeldDuration = HeldDuration;
    Context.HitResult = HitResult;
    Context.TargetActor = TargetActor;

    if (Context.HitResult.bBlockingHit || Context.HitResult.GetActor() != nullptr)
    {
        Context.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(Context.HitResult);
    }
    else if (Context.bUseActorTarget && Context.TargetActor)
    {
        Context.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Context.TargetActor.Get());
    }
    else
    {
        Context.TargetData.Clear();
    }

    Context.AimPoint = ResolveAbilityAimPoint(Context.HitResult, Context.TargetActor.Get());
    Context.AimDirection = ResolveAbilityAimDirection(SourceActor, Context.AimPoint, Context.TargetActor.Get());
    return Context;
}

FHitResult BuildSyntheticAbilityTargetHitResult(const AActor* SourceActor, AActor* TargetActor)
{
    const FVector SourceLocation = SourceActor ? SourceActor->GetActorLocation() : FVector::ZeroVector;
    const FVector TargetLocation = TargetActor ? TargetActor->GetActorLocation() : FVector::ZeroVector;
    FVector ImpactNormal = SourceLocation - TargetLocation;
    ImpactNormal.Z = 0.0f;

    if (ImpactNormal.IsNearlyZero())
    {
        ImpactNormal = FVector::UpVector;
    }
    else
    {
        ImpactNormal = ImpactNormal.GetSafeNormal();
    }

    UPrimitiveComponent* TargetComponent = TargetActor ? Cast<UPrimitiveComponent>(TargetActor->GetRootComponent()) : nullptr;
    FHitResult HitResult(TargetActor, TargetComponent, TargetLocation, ImpactNormal);
    HitResult.bBlockingHit = true;
    HitResult.TraceStart = SourceLocation;
    HitResult.TraceEnd = TargetLocation;
    HitResult.Location = TargetLocation;
    HitResult.ImpactPoint = TargetLocation;
    HitResult.Distance = FVector::Distance(SourceLocation, TargetLocation);
    return HitResult;
}
}
// 设置默认值
AIsometricRPGCharacter::AIsometricRPGCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UIsometricRPGCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	// 创建输入组件
	IRPGInputComponent = CreateDefaultSubobject<UIsometricInputComponent>(TEXT("IRPGInputComponent"));
	PathFollowingComponent = CreateDefaultSubobject<UIsometricPathFollowingComponent>(TEXT("PathFollowingComponent"));
	// 不要让角色面朝摄像机方向
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = true; // 使角色转向移动方向
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	TeamId = FGenericTeamId(1);
    bReplicates = true;
}
// 实现GetGenericTeamId函数，直接返回我们的TeamId变量
FGenericTeamId AIsometricRPGCharacter::GetGenericTeamId() const
{
    return TeamId;
}

// 实现SetGenericTeamId函数
void AIsometricRPGCharacter::SetGenericTeamId(const FGenericTeamId& InTeamId)
{
    TeamId = InTeamId;
}

// 当游戏开始或生成时调用
void AIsometricRPGCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// 每帧调用
void AIsometricRPGCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

UAbilitySystemComponent* AIsometricRPGCharacter::GetAbilitySystemComponent() const
{
    AIsoPlayerState* PS = Cast<AIsoPlayerState>(GetPlayerState());
    if (PS)
    {
        return PS->GetAbilitySystemComponent();
    }
    return nullptr;
}
 
UIsometricRPGAttributeSetBase* AIsometricRPGCharacter::GetAttributeSet() const
{
    AIsoPlayerState* PS = Cast<AIsoPlayerState>(GetPlayerState());
    if (PS)
    {
        return PS->GetAttributeSet();
    }
    return nullptr;
}

void AIsometricRPGCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    // 为服务器初始化 Ability Actor Info
    InitAbilityActorInfo();
    
    // 只负责表现性逻辑：激活被动技能（如回血蓝）
    if (GetLocalRole() == ROLE_Authority)
    {
        FGameplayTagContainer PassiveTags;
        PassiveTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Regen.Basic")));
        // 这里支持角色激活回血被动
        if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
        {
            ASC->TryActivateAbilitiesByTag(PassiveTags);
        }
    }
}

void AIsometricRPGCharacter::Server_EquipAbilityToSlot_Implementation(TSubclassOf<UGameplayAbility> NewAbilityClass, ESkillSlot Slot)
{
    AIsoPlayerState* PS = Cast<AIsoPlayerState>(GetPlayerState());
    if (PS)
    {
        PS->Server_EquipAbilityToSlot(NewAbilityClass, Slot);
    }
}

void AIsometricRPGCharacter::Server_UnequipAbilityFromSlot_Implementation(ESkillSlot Slot)
{
    AIsoPlayerState* PS = Cast<AIsoPlayerState>(GetPlayerState());
    if (PS)
    {
        PS->Server_UnequipAbilityFromSlot(Slot);
    }
}

FEquippedAbilityInfo AIsometricRPGCharacter::GetEquippedAbilityInfo(ESkillSlot Slot) const
{
    AIsoPlayerState* PS = Cast<AIsoPlayerState>(GetPlayerState());
    if (PS)
    {
        return PS->GetEquippedAbilityInfo(Slot);
    }
    return FEquippedAbilityInfo();
}

float AIsometricRPGCharacter::GetCurrentHealth() const
{
    if (const UIsometricRPGAttributeSetBase* CurrentAttributeSet = GetAttributeSet())
    {
        return CurrentAttributeSet->GetHealth();
    }
    return 0.f;
}

float AIsometricRPGCharacter::GetMaxHealth() const
{
    if (const UIsometricRPGAttributeSetBase* CurrentAttributeSet = GetAttributeSet())
    {
        return CurrentAttributeSet->GetMaxHealth();
    }
    return 0.f;
}
void AIsometricRPGCharacter::SetIsDead(bool NewValue)
{
    bIsDead = NewValue;
}
void AIsometricRPGCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AIsometricRPGCharacter, bIsDead);
}

void AIsometricRPGCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    InitAbilityActorInfo();
}

void AIsometricRPGCharacter::InitAbilityActorInfo()
{
    if (AIsoPlayerState* PS = Cast<AIsoPlayerState>(GetPlayerState()))
    {
        if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
        {
            ASC->InitAbilityActorInfo(PS, this);
            UE_LOG(LogTemp, Log, TEXT("[PassiveDebug][Character] ASC InitAbilityActorInfo on %s (Role=%d Local=%d)"),
                *GetName(), GetRemoteRole(), GetLocalRole());
            PS->NotifyAbilityActorInfoReady();
        }
    }
}

void AIsometricRPGCharacter::SetAbilityTargetDataByHit(const FHitResult& HitResult)
{
    FPendingAbilityActivationContext Context = PendingAbilityActivationContext;
    Context.bUseActorTarget = false;
    Context.TargetActor = nullptr;
    Context.HitResult = HitResult;
    Context.AimPoint = FVector::ZeroVector;
    Context.AimDirection = FVector::ZeroVector;
    Context.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(HitResult);
    SetPendingAbilityActivationContext(Context);
}

void AIsometricRPGCharacter::SetAbilityTargetDataByActor(AActor* TargetActor)
{
    FPendingAbilityActivationContext Context = PendingAbilityActivationContext;
    Context.bUseActorTarget = true;
    Context.TargetActor = TargetActor;
    Context.HitResult = FHitResult();
    Context.AimPoint = FVector::ZeroVector;
    Context.AimDirection = FVector::ZeroVector;
    if (TargetActor)
    {
        Context.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(TargetActor);
    }
    else
    {
        Context.TargetData.Clear();
    }
    SetPendingAbilityActivationContext(Context);
}

bool AIsometricRPGCharacter::PrepareAbilityActivationContextForAbility(TSubclassOf<UGameplayAbility> AbilityClass, AActor* TargetActor)
{
    if (!ensureAlwaysMsgf(AbilityClass != nullptr, TEXT("%s: PrepareAbilityActivationContextForAbility called with a null ability class."), *GetName()))
    {
        return false;
    }

    if (!ensureAlwaysMsgf(TargetActor != nullptr, TEXT("%s: PrepareAbilityActivationContextForAbility called with a null target actor."), *GetName()))
    {
        return false;
    }

    const UGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<UGameplayAbility>();
    if (!ensureAlwaysMsgf(AbilityCDO != nullptr, TEXT("%s: Failed to resolve CDO for ability class %s."), *GetName(), *GetNameSafe(AbilityClass)))
    {
        return false;
    }

    FPendingAbilityActivationContext Context;
    Context.InputID = EAbilityInputID::None;
    Context.TriggerPhase = EInputEventPhase::Pressed;
    Context.bIsHeldInput = false;
    Context.HeldDuration = 0.0f;
    Context.TargetActor = TargetActor;

    if (const UGA_HeroBaseAbility* HeroAbility = Cast<UGA_HeroBaseAbility>(AbilityCDO))
    {
        if (HeroAbility->ExpectsActorTargetData())
        {
            Context.bUseActorTarget = true;
            Context.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(TargetActor);
        }
        else if (HeroAbility->ExpectsLocationTargetData())
        {
            Context.bUseActorTarget = false;
            Context.HitResult = BuildSyntheticAbilityTargetHitResult(this, TargetActor);
            Context.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(Context.HitResult);
        }
        else
        {
            ensureAlwaysMsgf(false, TEXT("%s: AI attack task requested target preparation for unsupported ability type %d on %s."),
                *GetName(),
                static_cast<int32>(HeroAbility->GetHeroAbilityType()),
                *GetNameSafe(AbilityClass));
            return false;
        }
    }
    else
    {
        Context.bUseActorTarget = true;
        Context.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(TargetActor);
    }

    if (!ensureAlwaysMsgf(Context.TargetData.Num() > 0, TEXT("%s: Failed to build activation target data for ability %s against target %s."),
        *GetName(),
        *GetNameSafe(AbilityClass),
        *GetNameSafe(TargetActor)))
    {
        return false;
    }

    SetPendingAbilityActivationContext(Context);
    return true;
}

FGameplayAbilityTargetDataHandle AIsometricRPGCharacter::GetAbilityTargetData() const
{
    return StoredTargetData;
}

void AIsometricRPGCharacter::SetPendingAbilityActivationContext(const FPendingAbilityActivationContext& InContext)
{
    PendingAbilityActivationContext = InContext;

    if (PendingAbilityActivationContext.AimPoint.IsNearlyZero())
    {
        PendingAbilityActivationContext.AimPoint = ResolveAbilityAimPoint(
            PendingAbilityActivationContext.HitResult,
            PendingAbilityActivationContext.TargetActor.Get());
    }

    if (PendingAbilityActivationContext.AimPoint.IsNearlyZero())
    {
        PendingAbilityActivationContext.AimPoint = ResolveAbilityAimPointFromTargetData(PendingAbilityActivationContext.TargetData);
    }

    if (PendingAbilityActivationContext.AimDirection.IsNearlyZero())
    {
        PendingAbilityActivationContext.AimDirection = ResolveAbilityAimDirection(
            this,
            PendingAbilityActivationContext.AimPoint,
            PendingAbilityActivationContext.TargetActor.Get());
    }
    else
    {
        PendingAbilityActivationContext.AimDirection.Z = 0.0f;
        PendingAbilityActivationContext.AimDirection = PendingAbilityActivationContext.AimDirection.GetSafeNormal();
    }

    if (InContext.TargetData.Num() > 0)
    {
        StoredTargetData = InContext.TargetData;
    }
    else if (InContext.bUseActorTarget && InContext.TargetActor)
    {
        StoredTargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(InContext.TargetActor.Get());
    }
    else if (InContext.HitResult.bBlockingHit || InContext.HitResult.GetActor() != nullptr)
    {
        StoredTargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(InContext.HitResult);
    }
    else
    {
        StoredTargetData.Clear();
    }

    PendingAbilityActivationContext.TargetData = StoredTargetData;
}

const FPendingAbilityActivationContext& AIsometricRPGCharacter::GetPendingAbilityActivationContext() const
{
    return PendingAbilityActivationContext;
}

void AIsometricRPGCharacter::ClearPendingAbilityActivationContext()
{
    PendingAbilityActivationContext = FPendingAbilityActivationContext();
    StoredTargetData.Clear();
}

bool AIsometricRPGCharacter::HasStoredTargetActor(const AActor* InTargetActor) const
{
    if (!InTargetActor || !StoredTargetData.IsValid(0))
    {
        return false;
    }

    if (const FGameplayAbilityTargetData* Data = StoredTargetData.Get(0))
    {
        if (Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_ActorArray::StaticStruct()))
        {
            const auto* ActorArrayData = static_cast<const FGameplayAbilityTargetData_ActorArray*>(Data);
            if (ActorArrayData && ActorArrayData->TargetActorArray.Num() > 0)
            {
                return ActorArrayData->TargetActorArray[0].Get() == InTargetActor;
            }
        }
        else if (Data->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_SingleTargetHit::StaticStruct()))
        {
            const auto* HitData = static_cast<const FGameplayAbilityTargetData_SingleTargetHit*>(Data);
            if (HitData && HitData->GetHitResult())
            {
                return HitData->GetHitResult()->GetActor() == InTargetActor;
            }
        }
    }

    return false;
}

void AIsometricRPGCharacter::Server_SetAbilityTargetDataByHit_Implementation(const FHitResult& HitResult)
{
    SetAbilityTargetDataByHit(HitResult);
}

void AIsometricRPGCharacter::Server_SetAbilityTargetDataByActor_Implementation(AActor* TargetActor)
{
    SetAbilityTargetDataByActor(TargetActor);
}

void AIsometricRPGCharacter::Server_SetPendingAbilityActivationContext_Implementation(
    EAbilityInputID InputID,
    EInputEventPhase TriggerPhase,
    bool bUseActorTarget,
    bool bIsHeldInput,
    float HeldDuration,
    const FHitResult& HitResult,
    AActor* TargetActor)
{
    SetPendingAbilityActivationContext(BuildPendingAbilityActivationContext(
        this,
        InputID,
        TriggerPhase,
        bUseActorTarget,
        bIsHeldInput,
        HeldDuration,
        HitResult,
        TargetActor));
}

void AIsometricRPGCharacter::Server_UpdatePendingAbilityActivationContext_Implementation(
    EAbilityInputID InputID,
    EInputEventPhase TriggerPhase,
    bool bUseActorTarget,
    bool bIsHeldInput,
    float HeldDuration,
    const FHitResult& HitResult,
    AActor* TargetActor)
{
    SetPendingAbilityActivationContext(BuildPendingAbilityActivationContext(
        this,
        InputID,
        TriggerPhase,
        bUseActorTarget,
        bIsHeldInput,
        HeldDuration,
        HitResult,
        TargetActor));
}

void AIsometricRPGCharacter::Client_PlayMontage_Implementation(UAnimMontage* Montage, float PlayRate)
{
    if (Montage && GetMesh())
    {
        if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
        {
            AnimInst->Montage_Play(Montage, PlayRate);
        }
    }
}

void AIsometricRPGCharacter::Client_SetMovementLocked_Implementation(bool bLocked)
{
    if (UCharacterMovementComponent* CMC = GetCharacterMovement())
    {
        if (bLocked)
        {
            CMC->DisableMovement();
        }
        else
        {
            if (CMC->IsMovingOnGround())
            {
                CMC->SetMovementMode(MOVE_Walking);
            }
            else
            {
                CMC->SetMovementMode(MOVE_Falling);
            }
        }
    }
}

float AIsometricRPGCharacter::GetAttackRange() const
{
    if (const UIsometricRPGAttributeSetBase* CurrentAttributeSet = GetAttributeSet())
    {
        return CurrentAttributeSet->GetAttackRange();
    }
    return 0.0f;
}

void AIsometricRPGCharacter::RecordRecentBasicAttackTarget(AActor* Target, float WorldTime)
{
    RecentBasicAttackTarget = Target;
    RecentBasicAttackTimestamp = Target ? WorldTime : -FLT_MAX;
}

void AIsometricRPGCharacter::RecordRecentCharmTarget(AActor* Target, float ExpireTime)
{
    RecentCharmTarget = Target;
    RecentCharmExpireTime = Target ? ExpireTime : -FLT_MAX;
}

AActor* AIsometricRPGCharacter::GetRecentBasicAttackTarget(float MaxAgeSeconds) const
{
    if (!RecentBasicAttackTarget.IsValid() || !GetWorld())
    {
        return nullptr;
    }

    const float WorldTime = GetWorld()->GetTimeSeconds();
    if ((WorldTime - RecentBasicAttackTimestamp) > MaxAgeSeconds)
    {
        return nullptr;
    }

    return RecentBasicAttackTarget.Get();
}

AActor* AIsometricRPGCharacter::GetRecentCharmTarget() const
{
    if (!RecentCharmTarget.IsValid() || !GetWorld())
    {
        return nullptr;
    }

    if (GetWorld()->GetTimeSeconds() > RecentCharmExpireTime)
    {
        return nullptr;
    }

    return RecentCharmTarget.Get();
}
