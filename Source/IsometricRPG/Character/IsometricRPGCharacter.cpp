#include "IsometricRPGCharacter.h"
#include "EnhancedInputComponent.h" 
#include "Components/SceneComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h" // DOREPLIFETIME 所需
#include "GameplayAbilitySpec.h" // FGameplayAbilitySpec 所需
#include "IsoPlayerState.h"
#include "AnimationBlueprintLibrary.h"
#include <Blueprint/AIBlueprintHelperLibrary.h>
#include "GameFramework/PlayerState.h"
#include "IsometricAbilities/Types/EquippedAbilityInfo.h"
#include "IsometricRPGAttributeSetBase.h"
// 设置默认值
AIsometricRPGCharacter::AIsometricRPGCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	// 不再创建 AbilitySystemComponent 和 AttributeSet

	// 创建输入组件
	IRPGInputComponent = CreateDefaultSubobject<UIsometricInputComponent>(TEXT("IRPGInputComponent"));
	// 不要让角色面朝摄像机方向
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = true; // 使角色转向移动方向
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	TeamId = FGenericTeamId(1);
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
  //      AIsoPlayerState* PS = Cast<AIsoPlayerState>(GetPlayerState());
		//if (!PS) return;
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
    AIsoPlayerState* PS = Cast<AIsoPlayerState>(GetPlayerState());
    if (PS && PS->GetAttributeSet())
    {
        return PS->GetAttributeSet()->GetHealth();

    }
    return 0.f;
}

float AIsometricRPGCharacter::GetMaxHealth() const
{
    AIsoPlayerState* PS = Cast<AIsoPlayerState>(GetPlayerState());
    if (PS && PS->GetAttributeSet())
    {
        return PS->GetAttributeSet()->GetMaxHealth();
    }
    return 0.f;
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
        }
    }
}

