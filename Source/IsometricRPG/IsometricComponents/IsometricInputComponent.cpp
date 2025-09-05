// IsometricInputComponent.cpp
#include "IsometricInputComponent.h"
#include "EnhancedInputComponent.h" // Should be removed if not used for direct binding here
#include "EnhancedInputSubsystems.h" // Should be removed if not used for direct binding here
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "AbilitySystemComponent.h"
#include "Character/IsometricRPGCharacter.h"
#include "Blueprint/AIBlueprintHelperLibrary.h" 
#include <AbilitySystemBlueprintLibrary.h>
#include "IsometricAbilities/GameplayAbilities/GA_HeroBaseAbility.h"


UIsometricInputComponent::UIsometricInputComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UIsometricInputComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerCharacter = Cast<AIsometricRPGCharacter>(GetOwner());
    if (OwnerCharacter)
    {
        OwnerASC = OwnerCharacter->GetAbilitySystemComponent();

        CachedPlayerController = Cast<APlayerController>(OwnerCharacter->GetController()); 
        if (!CachedPlayerController)
        {
            UE_LOG(LogTemp, Warning, TEXT("UIsometricInputComponent: CachedPlayerController is null in BeginPlay. Input binding might fail or be delayed."));
            return;
        }
        UInputComponent* PCInputComponent = CachedPlayerController->InputComponent;
        if (!PCInputComponent)
        {
            UE_LOG(LogTemp, Warning, TEXT("UIsometricInputComponent: PlayerController's InputComponent is null in BeginPlay. GAS Input Binding cannot occur yet."));
            return;
        }
        // 获取 UEnum*
        UEnum* EnumBinds = StaticEnum<EAbilityInputID>();
        if (!EnumBinds)
        {
            UE_LOG(LogTemp, Error, TEXT("UIsometricInputComponent: StaticEnum<EAbilityInputID>() failed. Ensure EAbilityInputID is correctly defined in a header and UHT has run."));
            return;
        }
        FTopLevelAssetPath EnumPath = FTopLevelAssetPath(TEXT("/Script/IsometricRPG.EAbilityInputID"));
        FString ConfirmCommand = TEXT("");
        FString CancelCommand = TEXT("");
        FGameplayAbilityInputBinds BindInfo(
            ConfirmCommand,      // 项目输入设置中用于“确认”的 Action Mapping 名称
            CancelCommand,       // 项目输入设置中用于“取消”的 Action Mapping 名称
            EnumPath,
            (int32)EAbilityInputID::Confirm, // 对应枚举中的 Confirm
            (int32)EAbilityInputID::Cancel   // 对应枚举中的 Cancel
        );
        OwnerASC->BindAbilityActivationToInputComponent(CachedPlayerController->InputComponent, BindInfo);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UIsometricInputComponent owned by non-AIsometricRPGCharacter or null owner."));
    }

}


void UIsometricInputComponent::HandleLeftClick(const FHitResult& HitResult)
{
    if (!OwnerCharacter || !OwnerASC) return; // CachedPlayerController no longer needed for HitResult

    SendConfirmTargetInput();

    AActor* HitActor = HitResult.GetActor();

    if (HitActor && HitActor != OwnerCharacter)
    {
        CurrentSelectedTargetForUI = HitActor;
        OnTargetSelectedForUI(HitActor); 
    }
    else
    {
        CurrentSelectedTargetForUI = nullptr;
        OnTargetClearedForUI();
    }
}

void UIsometricInputComponent::HandleRightClickTriggered(const FHitResult& HitResult, TWeakObjectPtr<AActor> LastHitActor)
{
    if (!OwnerCharacter || !OwnerASC) return;

    // 总是取消上一次的目标选择状态
    SendCancelTargetInput(); 

    AActor* CurrentHitActor = HitResult.GetActor();

    // 判断当前鼠标下的目标
    if (CurrentHitActor && CurrentHitActor != OwnerCharacter && CurrentHitActor->ActorHasTag(FName("Enemy"))) 
    {
        // 如果当前目标是敌人，则请求攻击
        RequestBasicAttack(CurrentHitActor);
    }
    else if (HitResult.bBlockingHit)    
	{
        // 如果是地面或其他可移动点，则请求移动
        RequestMoveToLocation(HitResult.ImpactPoint);
    }
}


void UIsometricInputComponent::HandleSkillInput(EAbilityInputID InputID, const FHitResult& TargetData)
{
    if (!OwnerCharacter) return;
    if (!OwnerASC) { OwnerASC = OwnerCharacter->GetAbilitySystemComponent(); }
    if (!OwnerASC) return;

    // 先在本地缓存，再RPC同步给服务器，确保服务器拿到同一份目标数据
    OwnerCharacter->SetAbilityTargetData(TargetData);
    if (OwnerCharacter->GetLocalRole() < ROLE_Authority)
    {
        OwnerCharacter->Server_SetAbilityTargetDataByHit(TargetData);
    }
    const TSubclassOf<UGameplayAbility>* AbilityClassPtr = SkillInputMappings.Find(InputID);
    if (!AbilityClassPtr || !(*AbilityClassPtr))
    {
        UE_LOG(LogTemp, Warning, TEXT("InputID %d has no valid mapping."), InputID);
        return;
    }

    const TSubclassOf<UGameplayAbility> MappedClass = *AbilityClassPtr;
    // 在ASC中查找已授予的能力（允许子类）
    FGameplayAbilitySpec* FoundSpec = nullptr;
    for (FGameplayAbilitySpec& Spec : OwnerASC->GetActivatableAbilities())
    {
        if (Spec.Ability && Spec.Ability->GetClass()->IsChildOf(MappedClass))
        {
            FoundSpec = &Spec; break;
        }
    }

    if (!FoundSpec || !FoundSpec->Ability)
    {
        UE_LOG(LogTemp, Warning, TEXT("ASC has not granted %s (or child)."), *MappedClass->GetName());
        return;
    }

    // 直接通过Spec Handle激活（更稳妥），目标数据已缓存在角色上
	bool bSuccessful = OwnerASC->TryActivateAbility(FoundSpec->Handle);
    if (OwnerCharacter->HasAuthority())
    {
        UE_LOG(LogTemp, Display, TEXT("TryActivateAbility Tag=%s -> %d"),
            *FoundSpec->Ability->GetName(), bSuccessful);
    }
}

void UIsometricInputComponent::RequestMoveToLocation(const FVector& TargetLocation)
{
    AController* OwnerController = OwnerCharacter ? OwnerCharacter->GetController() : nullptr;
    UAbilitySystemComponent* CurrentASC = OwnerCharacter ? OwnerCharacter->GetAbilitySystemComponent() : nullptr;
    if (!(OwnerCharacter && OwnerController && CurrentASC))
    {
        UE_LOG(LogTemp, Warning, TEXT("RequestMoveToLocation failed: OwnerCharacter (%p), Controller (%p), or ASC (%p) is null."), OwnerCharacter, OwnerController, CurrentASC);
        return;
    }

    if (!OwnerCharacter->HasAuthority())
    {
        Server_RequestMoveToLocation(TargetLocation);
    }

    if (OwnerCharacter->HasAuthority())
    {
        FGameplayTag BasicAttackAbilityTag = FGameplayTag::RequestGameplayTag(FName("Ability.Player.BasicAttack"));
        FGameplayTag DirBasicAttackAbilityTag = FGameplayTag::RequestGameplayTag(FName("Ability.Player.DirBasicAttack"));
        if (BasicAttackAbilityTag.IsValid())
        {
            FGameplayTagContainer TagContainer(BasicAttackAbilityTag);
            TagContainer.AddTag(DirBasicAttackAbilityTag);
            CurrentASC->CancelAbilities(&TagContainer);
            UE_LOG(LogTemp, Log, TEXT("Server: Canceled attack abilities."));
        }

        if (OwnerCharacter->GetMesh() && OwnerCharacter->GetMesh()->GetAnimInstance())
        {
            OwnerCharacter->GetMesh()->GetAnimInstance()->Montage_Stop(0.1f); // Stop any montage
            UE_LOG(LogTemp, Log, TEXT("Server: Stopped any active montage."));
        }
    }
    UAIBlueprintHelperLibrary::SimpleMoveToLocation(OwnerController, TargetLocation);
}
void UIsometricInputComponent::RequestBasicAttack(AActor* TargetActor)
{
    AController* OwnerController = OwnerCharacter ? OwnerCharacter->GetController() : nullptr;
    UAbilitySystemComponent* CurrentASC = OwnerCharacter ? OwnerCharacter->GetAbilitySystemComponent() : nullptr;

    if (!OwnerCharacter || !CurrentASC || !TargetActor || !OwnerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("RequestBasicAttack failed: OwnerCharacter (%p), Controller (%p), ASC (%p), or TargetActor (%p) is null."), OwnerCharacter, OwnerController, CurrentASC, TargetActor);
        return;
    }

    // 客户端转到服务器
    if (OwnerCharacter->GetLocalRole() < ROLE_Authority)
    {
        Server_RequestBasicAttack(TargetActor);
        return;
    }
    OwnerCharacter->SetAbilityTargetData(TargetActor);
    if (OwnerCharacter->GetLocalRole() < ROLE_Authority)
    {
        OwnerCharacter->Server_SetAbilityTargetDataByActor(TargetActor);
    }
    FGameplayTag BasicAttackAbilityTag = FGameplayTag::RequestGameplayTag(FName("Ability.Player.DirBasicAttack"));
    if (BasicAttackAbilityTag.IsValid())
    {
        // 直接通过Tag激活技能，这种方式更通用，无需知道具体技能类
        if (CurrentASC->TryActivateAbilitiesByTag(FGameplayTagContainer(BasicAttackAbilityTag), true))
        {
            UE_LOG(LogTemp, Display, TEXT("RequestBasicAttack: Successfully activated Basic Attack ability with tag %s."), *BasicAttackAbilityTag.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("RequestBasicAttack: Failed to activate Basic Attack ability with tag %s."), *BasicAttackAbilityTag.ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Basic Attack ability tag is invalid for RequestBasicAttack."));
    }
}


void UIsometricInputComponent::SendConfirmTargetInput()
{
    if (OwnerASC)
    {
        OwnerASC->AbilityLocalInputPressed((int32)EAbilityInputID::Confirm); 
    }
}

void UIsometricInputComponent::SendCancelTargetInput()
{
    if (OwnerASC)
    {
        OwnerASC->AbilityLocalInputPressed((int32)EAbilityInputID::Cancel); 
    }
}

// --- Server RPC implementations ---
void UIsometricInputComponent::Server_RequestMoveToLocation_Implementation(const FVector& TargetLocation)
{
    RequestMoveToLocation(TargetLocation);
}

void UIsometricInputComponent::Server_RequestBasicAttack_Implementation(AActor* TargetActor)
{
    RequestBasicAttack(TargetActor);
}


