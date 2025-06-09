#include "IsometricRPGCharacter.h"
#include "EnhancedInputComponent.h" 
#include "Character/IsometricRPGAttributeSetBase.h"
#include "Components/SceneComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h" // DOREPLIFETIME 所需
#include "GameplayAbilitySpec.h" // FGameplayAbilitySpec 所需

// #include "IsometricAbilities/GameplayAbilities/GA_Death.h" // 示例，如果不使用则移除
// #include "IsometricAbilities/GameplayAbilities/Melee/GA_HeroMeleeAttackAbility.h" // 示例，如果不使用则移除
#include "AnimationBlueprintLibrary.h"
#include <Blueprint/AIBlueprintHelperLibrary.h>

// 设置默认值
AIsometricRPGCharacter::AIsometricRPGCharacter()
{
	// 设置此角色每帧调用 Tick()。如果您不需要它，可以将其关闭以提高性能。
	PrimaryActorTick.bCanEverTick = true;
	// 初始化技能系统组件
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true); // 对多人游戏很重要
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UIsometricRPGAttributeSetBase>(TEXT("AttributeSet"));


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

	EquippedAbilities.SetNum(static_cast<int32>(ESkillSlot::Invalid));
}

void AIsometricRPGCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AIsometricRPGCharacter, EquippedAbilities);
}

// 当游戏开始或生成时调用
void AIsometricRPGCharacter::BeginPlay()
{
	Super::BeginPlay();
	// 服务器应初始化技能。客户端将通过 OnRep_EquippedAbilities 获取它们
	// 或者如果不是玩家控制的，服务器可以在这里初始化。
    if (GetLocalRole() == ROLE_Authority)
    {
	    InitializeAttributes();
        // 启动被动技能，生命和法力回复
        FGameplayTagContainer thisTags;
        thisTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Regen.Basic")));
        GetAbilitySystemComponent()->TryActivateAbilitiesByTag(thisTags);
    }
}

// 每帧调用
void AIsometricRPGCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}



UAbilitySystemComponent* AIsometricRPGCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AIsometricRPGCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }

	// 服务器：初始化属性和技能
    if (GetLocalRole() == ROLE_Authority)
    {
        InitAbilities(); 
    }
    // 客户端：属性通常通过效果复制，技能通过 OnRep_EquippedAbilities 复制
}

void AIsometricRPGCharacter::InitializeAttributes()
{
    if (!AbilitySystemComponent) return;
    if(!AttributeSet) return; // 应在构造函数中创建

    // 授予一个游戏效果，该效果定义属性的默认值
    // 此 GE 应在蓝图中创建。例如，GE_DefaultAttributes
     if (DefaultAttributesEffect)
     {
         FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
         EffectContext.AddSourceObject(this);
         FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributesEffect, 1, EffectContext);
         if (SpecHandle.IsValid())
         {
             AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
         }
     }
}

void AIsometricRPGCharacter::InitAbilities()
{
    if (GetLocalRole() != ROLE_Authority || !AbilitySystemComponent || DefaultAbilities.Num() == 0 || bAbilitiesInitialized)
    {
        return;
    }
    EquippedAbilities.Empty(); // 清除任何现有的（例如，如果未正确清理，则来自上一个 PIE 会话）
    EquippedAbilities.SetNum(static_cast<int32>(ESkillSlot::Invalid)); // 或者如果 ESkillSlot 有一个，则为适当的最大大小

    for (const FEquippedAbilityInfo& AbilityInfo : DefaultAbilities)
    {
        if (AbilityInfo.AbilityClass)
        {
            // 在 EquippedAbilities 中查找或创建此插槽的条目
            // 此逻辑假定每个插槽一个技能。如果一个插槽中可以有多个技能，请进行调整。
            if (AbilityInfo.Slot != ESkillSlot::Invalid && static_cast<uint8>(AbilityInfo.Slot) < EquippedAbilities.Num())
            {
                FEquippedAbilityInfo& NewEquippedInfo = EquippedAbilities[static_cast<uint8>(AbilityInfo.Slot)];
                NewEquippedInfo = AbilityInfo; // 复制类和插槽
                GrantAbilityInternal(NewEquippedInfo, true); // 授予并存储句柄，如果存在则移除旧句柄
            }
            else
            {
                 // 后备或错误：如果在 EquippedAbilities 中插槽无效或超出范围，则在没有特定插槽管理的情况下授予
                 // 如果 UI 依赖于 EquippedAbilities 匹配插槽，则此路径可能有问题。
                 FEquippedAbilityInfo TempInfo = AbilityInfo;
                 GrantAbilityInternal(TempInfo); // 在 EquippedAbilities 数组的特定插槽索引中不存储的情况下授予
                 UE_LOG(LogTemp, Warning, TEXT("InitAbilities: 技能 %s 已授予，但在 EquippedAbilities 数组中没有有效插槽或插槽超出范围。"), *AbilityInfo.AbilityClass->GetName());
            }
        }
    }
    // 如果服务器上的 EquippedAbilities 发生更改，则强制复制
    if (GetNetMode() != NM_Standalone)
    {
        OnRep_EquippedAbilities(); // 手动调用服务器以广播初始状态（如果需要）（尽管复制会处理它）
    }

    bAbilitiesInitialized = true;
}


void AIsometricRPGCharacter::GrantAbilityInternal(FEquippedAbilityInfo& Info, bool bRemoveExistingFirst /*= false*/)
{
    if (GetLocalRole() != ROLE_Authority || !AbilitySystemComponent || !Info.AbilityClass)
    {
        return;
    }

    if (bRemoveExistingFirst && Info.AbilitySpecHandle.IsValid())
    {
        ClearAbilityInternal(Info); // 清除此插槽中的先前技能
    }

    //FGameplayAbilitySpec AbilitySpec(Info.AbilityClass, 1, static_cast<int32>(Info.Slot), this);
    FGameplayAbilitySpec AbilitySpec(Info.AbilityClass, 1, INDEX_NONE, this);
    Info.AbilitySpecHandle = AbilitySystemComponent->GiveAbility(AbilitySpec);
}

void AIsometricRPGCharacter::ClearAbilityInternal(FEquippedAbilityInfo& Info)
{
    if (GetLocalRole() != ROLE_Authority || !AbilitySystemComponent || !Info.AbilitySpecHandle.IsValid())
    {
        return;
    }
    AbilitySystemComponent->ClearAbility(Info.AbilitySpecHandle);
    Info.AbilitySpecHandle = FGameplayAbilitySpecHandle();  
}

void AIsometricRPGCharacter::OnRep_EquippedAbilities()
{
    if (AbilitySystemComponent && GetLocalRole() == ROLE_SimulatedProxy)
    {

        for (FEquippedAbilityInfo& Info : EquippedAbilities)
        {
            if (Info.AbilityClass && !Info.AbilitySpecHandle.IsValid()) // 如果类存在但未在本地授予
            {
                UE_LOG(LogTemp, Log, TEXT("客户端 %s OnRep_EquippedAbilities：找到插槽 %d 的技能 %s，但没有本地句柄。服务器应授予并复制规范。"),
                    *GetName(),                         // 对应第一个 %s
                    static_cast<int32>(Info.Slot),      // 对应 %d
                    *Info.AbilityClass->GetName()       // 对应第二个 %s
                );
            }
        }
    }
    // 如果 UI 或其他系统需要更新，则广播委托
    // OnEquippedAbilitiesChanged.Broadcast(); 
}

void AIsometricRPGCharacter::Server_EquipAbilityToSlot_Implementation(TSubclassOf<UGameplayAbility> NewAbilityClass, ESkillSlot Slot)
{
    if (GetLocalRole() == ROLE_Authority && AbilitySystemComponent && static_cast<uint8>(Slot) < EquippedAbilities.Num())
    {
        if (NewAbilityClass)
        {
            FEquippedAbilityInfo& SlotInfo = EquippedAbilities[static_cast<uint8>(Slot)];
            ClearAbilityInternal(SlotInfo); // 清除旧技能（如果有）
            
            SlotInfo.AbilityClass = NewAbilityClass;
            SlotInfo.Slot = Slot;
            GrantAbilityInternal(SlotInfo);

            // 如果客户端未更改，则强制 OnRep（例如，如果数组已是此大小）
            if (GetNetMode() != NM_Standalone) { OnRep_EquippedAbilities(); }
        }
        else
        {
            Server_UnequipAbilityFromSlot_Implementation(Slot);
        }
    }
}

void AIsometricRPGCharacter::Server_UnequipAbilityFromSlot_Implementation(ESkillSlot Slot)
{
    if (GetLocalRole() == ROLE_Authority && AbilitySystemComponent && static_cast<uint8>(Slot) < EquippedAbilities.Num())
    {
        FEquippedAbilityInfo& SlotInfo = EquippedAbilities[static_cast<uint8>(Slot)];
        ClearAbilityInternal(SlotInfo);
        SlotInfo.AbilityClass = nullptr;
        // SlotInfo.Slot 保持不变，还是设置为 Invalid？

        if (GetNetMode() != NM_Standalone) { OnRep_EquippedAbilities(); }
    }
}

FEquippedAbilityInfo AIsometricRPGCharacter::GetEquippedAbilityInfo(ESkillSlot Slot) const
{
    if (static_cast<uint8>(Slot) < EquippedAbilities.Num())
    {
        return EquippedAbilities[static_cast<uint8>(Slot)];
    }
    return FEquippedAbilityInfo(); // 返回空/无效信息
}


float AIsometricRPGCharacter::GetCurrentHealth() const
{
    if (AttributeSet)
    {
        return Cast<UIsometricRPGAttributeSetBase>(AttributeSet)->GetHealth();
    }
    return 0.f;
}

float AIsometricRPGCharacter::GetMaxHealth() const
{
    if (AttributeSet)
    {
        return Cast<UIsometricRPGAttributeSetBase>(AttributeSet)->GetMaxHealth();
    }
    return 0.f;
}

void AIsometricRPGCharacter::OnSkillOutOfRange(const FGameplayEventData* EventData)
{
    GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, TEXT("技能因距离失败，移动接近目标，来自角色"));
    if (IsValid(EventData->Target.Get()))
    {
        UAIBlueprintHelperLibrary::SimpleMoveToActor(this->GetController(), EventData->Target.Get());
    }
    else if (EventData->TargetData.Num() > 0 && EventData->TargetData.Data[0].IsValid())
    {
        UAIBlueprintHelperLibrary::SimpleMoveToLocation(this->GetController(), EventData->TargetData.Data[0]->GetHitResult()->Location);
    }
}