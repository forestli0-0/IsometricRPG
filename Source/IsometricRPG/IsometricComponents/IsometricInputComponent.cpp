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


UIsometricInputComponent::UIsometricInputComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // 通常输入组件不需要Tick
}

void UIsometricInputComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerCharacter = Cast<AIsometricRPGCharacter>(GetOwner());
    if (OwnerCharacter)
    {
        OwnerASC = OwnerCharacter->GetAbilitySystemComponent();
        // CachedPlayerController should be set by the PlayerController itself when it initializes this component,
        // or this component should get it when needed, if it's always owned by a pawn controlled by a PlayerController.
        // For now, we assume PlayerController will pass HitResult, so direct caching might not be strictly needed for cursor traces.
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

// SetupPlayerInputComponent is removed as PlayerController now handles input bindings.
// void UIsometricInputComponent::SetupPlayerInputComponent(UEnhancedInputComponent* PlayerInputComponent, APlayerController* InPlayerController)
// {
//    ...
// }

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

void UIsometricInputComponent::HandleRightClick(const FHitResult& HitResult)
{
    if (!OwnerCharacter || !OwnerASC) return; // CachedPlayerController no longer needed for HitResult

    SendCancelTargetInput(); 

    AActor* HitActor = HitResult.GetActor();

    if (HitActor && HitActor != OwnerCharacter && HitActor->ActorHasTag(FName("Enemy"))) 
    {
        RequestBasicAttack(HitActor);
    }
    else if (HitResult.bBlockingHit)    
	{
        RequestMoveToLocation(HitResult.ImpactPoint);
    }
}

void UIsometricInputComponent::HandleSkillInput(int32 SkillSlotID, const FHitResult& TargetData)
{
    if (!OwnerCharacter || !OwnerASC) return;

    // Here, TargetData (FHitResult) is available if the skill needs it.
    // The GameplayAbility itself will decide if and how to use this TargetData.
    // For example, a targeted skill might use TargetData.GetActor() or TargetData.ImpactPoint.
    // A self-cast skill might ignore it.

    if (const FGameplayTag* AbilityTag = SkillSlotToAbilityTagMap.Find(SkillSlotID))
    {
        FGameplayAbilitySpec* Spec = OwnerASC->FindAbilitySpecFromInputID(SkillSlotID); //This is an example, actual activation might differ
        // Or, more commonly, the ability is activated by tag, and it internally handles targeting.
        
        FGameplayTagContainer AbilityTags;
        AbilityTags.AddTag(*AbilityTag);

        // We might need to pass target data to the ability. 
        // One way is to use a GameplayEvent with the target data payload, which the ability listens for.
        // Or, if the ability uses WaitTargetData, SendConfirmTargetInput (called from HandleLeftClick) would provide it.
        // For simplicity, let's assume abilities activated by tag handle their own targeting or use a target already set.
        // If a skill *always* uses the cursor location/target from the moment it's pressed, 
        // then the ability's ActivateAbility could use TargetData.
        // This part highly depends on your GAS setup and ability design.

        OwnerASC->TryActivateAbilitiesByTag(AbilityTags); 
        UE_LOG(LogTemp, Display, TEXT("Attempting to activate ability with tag %s for slot %d. TargetData available."), *AbilityTag->ToString(), SkillSlotID);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillSlotID: %d has no mapped AbilityTag."), SkillSlotID);
    }
}

void UIsometricInputComponent::HandleBasicAttackInput(const FHitResult& HitResult)
{
    if (!OwnerCharacter || !OwnerASC) return;

    // Similar to HandleSkillInput, the HitResult is available if the basic attack ability needs it.
    // For an "A-Key" style attack, the typical flow is: press A (activates ability, waits for target), then left-click target.
    // So, this function might just activate the ability, and HandleLeftClick would confirm the target.

    FGameplayTag BasicAttackAbilityTag = FGameplayTag::RequestGameplayTag(FName("Ability.Player.BasicAttack")); 
    if (BasicAttackAbilityTag.IsValid())
    {
        FGameplayTagContainer AbilityTags;
        AbilityTags.AddTag(BasicAttackAbilityTag);
        OwnerASC->TryActivateAbilitiesByTag(AbilityTags);
        UE_LOG(LogTemp, Display, TEXT("Basic Attack ability (A-Key) activated, HitResult was available if needed by the ability."));
        // If the basic attack should immediately target what's under the cursor from the A-press:
        // AActor* Target = HitResult.GetActor();
        // if (Target) { RequestBasicAttack(Target); } 
        // However, this might conflict with a WaitTargetData flow.
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Basic Attack ability tag ('Ability.Player.BasicAttack') is invalid."));
    }
}

// ... rest of the file (RequestMoveToLocation, RequestBasicAttack, SendConfirmTargetInput, SendCancelTargetInput) remains largely the same
// but ensure they don't rely on FInputActionValue or direct input bindings.

void UIsometricInputComponent::RequestMoveToLocation(const FVector& TargetLocation)
{
    if (OwnerCharacter && OwnerCharacter->GetController() && OwnerASC) 
    {
        FGameplayTag BasicAttackAbilityTag = FGameplayTag::RequestGameplayTag(FName("Ability.Player.BasicAttack"));
        if (BasicAttackAbilityTag.IsValid())
        {
            FGameplayTagContainer TagContainer(BasicAttackAbilityTag);  
            OwnerASC->CancelAbilities(&TagContainer);
        }

        if (OwnerCharacter->GetMesh() && OwnerCharacter->GetMesh()->GetAnimInstance())
        {
            OwnerCharacter->GetMesh()->GetAnimInstance()->Montage_Stop(0.1f); // Stop any montage
        }

        UAIBlueprintHelperLibrary::SimpleMoveToLocation(OwnerCharacter->GetController(), TargetLocation);
    }
}

void UIsometricInputComponent::RequestBasicAttack(AActor* TargetActor)
{
    if (!OwnerCharacter || !OwnerASC || !TargetActor) return;

    FGameplayTag BasicAttackAbilityTag = FGameplayTag::RequestGameplayTag(FName("Ability.Player.BasicAttack.Directly")); 
    if (BasicAttackAbilityTag.IsValid())
    {
        // Store target for the ability to pick up, or pass via event/payload
        // This is a common pattern: set a target on ASC or Character, then activate.
        if (!OwnerCharacter) return;
        OwnerCharacter->CurrentAbilityTargets.Empty();
        OwnerCharacter->CurrentAbilityTargets.Add(TargetActor);
        // 使用GameplayAbility系统激活技能
        FGameplayEventData EventData;
        EventData.Target = TargetActor;
        EventData.Instigator = OwnerCharacter;
        FGameplayAbilityTargetDataHandle TargetDataHandle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(FHitResult(TargetActor, nullptr, FVector::ZeroVector, FVector::ZeroVector));
        EventData.TargetData = TargetDataHandle;
        OwnerASC->HandleGameplayEvent(BasicAttackAbilityTag, &EventData);


        //bool bActivated = OwnerASC->TryActivateAbilitiesByTag(AbilityTags);
        //if (bActivated)
        //{
        //    // If the ability is a WaitTargetData style, this confirm might be redundant or handled by left click.
        //    // If it's an instant attack on the provided target, this is fine.
        //    // SendConfirmTargetInput(); // Potentially, if the ability expects it immediately after activation with a target.
        //    UE_LOG(LogTemp, Display, TEXT("Requested Basic Attack on %s. Ability activated: %d"), *TargetActor->GetName(), bActivated);
        //}
        //else
        //{
        //    UE_LOG(LogTemp, Warning, TEXT("Failed to activate Basic Attack on %s."), *TargetActor->GetName());
        //}
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Basic Attack ability tag is invalid for RequestBasicAttack."));
    }
}


// GetTargetUnderCursor and GetLocationUnderCursor are removed as PlayerController handles this.

void UIsometricInputComponent::SendConfirmTargetInput()
{
    if (OwnerASC)
    {
        OwnerASC->AbilityLocalInputPressed(ConfirmInputID); 
    }
}

void UIsometricInputComponent::SendCancelTargetInput()
{
    if (OwnerASC)
    {
        OwnerASC->AbilityLocalInputPressed(CancelInputID); 
    }
}

