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
#include <Blueprint/AIBlueprintHelperLibrary.h>
#include "GameFramework/PlayerState.h"
#include "IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "IsometricRPGAttributeSetBase.h"
#include <AbilitySystemBlueprintLibrary.h>
#include "Abilities/GameplayAbilityTargetTypes.h"
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
    Context.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(HitResult);
    SetPendingAbilityActivationContext(Context);
}

void AIsometricRPGCharacter::SetAbilityTargetDataByActor(AActor* TargetActor)
{
    FPendingAbilityActivationContext Context = PendingAbilityActivationContext;
    Context.bUseActorTarget = true;
    Context.TargetActor = TargetActor;
    Context.HitResult = FHitResult();
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

FGameplayAbilityTargetDataHandle AIsometricRPGCharacter::GetAbilityTargetData() const
{
    return StoredTargetData;
}

void AIsometricRPGCharacter::SetPendingAbilityActivationContext(const FPendingAbilityActivationContext& InContext)
{
    PendingAbilityActivationContext = InContext;

    if (InContext.TargetData.Num() > 0)
    {
        StoredTargetData = InContext.TargetData;
    }
    else if (InContext.bUseActorTarget && InContext.TargetActor.IsValid())
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
