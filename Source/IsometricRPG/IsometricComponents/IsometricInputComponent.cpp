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
            // 你可能需要等待 InputComponent 被创建。
            // 一种方法是让 PlayerController 在其 InputComponent 创建后调用一个此组件的函数。
            // 或者，将绑定逻辑移到 Character 的 SetupPlayerInputComponent 中。
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

    if (OwnerCharacter->GetLocalRole() < ROLE_Authority)
    {
        Server_HandleSkillInput(InputID, TargetData);
        return;
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

    const UGA_HeroBaseAbility* GrantedCDO = Cast<UGA_HeroBaseAbility>(FoundSpec->Ability);
    if (!GrantedCDO || !GrantedCDO->TriggerTag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Granted ability %s has invalid TriggerTag."), *FoundSpec->Ability->GetClass()->GetName());
        return;
    }

    // 构造事件
    FGameplayEventData EventData;
    EventData.Instigator = OwnerCharacter;
    EventData.Target = TargetData.GetActor();
    EventData.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(TargetData);

    const int32 Count = OwnerASC->HandleGameplayEvent(GrantedCDO->TriggerTag, &EventData);
    UE_LOG(LogTemp, Display, TEXT("HandleGameplayEvent Tag=%s -> %d"),
        *GrantedCDO->TriggerTag.ToString(), Count);
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

    FGameplayTag BasicAttackAbilityTag = FGameplayTag::RequestGameplayTag(FName("Ability.Player.DirBasicAttack")); 
    if (BasicAttackAbilityTag.IsValid())
    {
        if (!OwnerCharacter) return;
        OwnerCharacter->CurrentAbilityTargets.Empty();
        OwnerCharacter->CurrentAbilityTargets.Add(TargetActor);
        // 使用GameplayAbility系统激活技能
        FGameplayEventData EventData;
        EventData.Target = TargetActor;
        EventData.Instigator = OwnerCharacter;
        FGameplayAbilityTargetDataHandle TargetDataHandle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(FHitResult(TargetActor, nullptr, FVector::ZeroVector, FVector::ZeroVector));
        EventData.TargetData = TargetDataHandle;
        auto count = CurrentASC->HandleGameplayEvent(BasicAttackAbilityTag, &EventData);
		UE_LOG(LogTemp, Display, TEXT("RequestBasicAttack: Attempting to activate Basic Attack ability with tag %s. TargetActor: %s, Count: %d"), *BasicAttackAbilityTag.ToString(), *TargetActor->GetName(), count);
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

void UIsometricInputComponent::Server_HandleSkillInput_Implementation(EAbilityInputID InputID, const FHitResult& TargetData)
{
    HandleSkillInput(InputID, TargetData);
}
