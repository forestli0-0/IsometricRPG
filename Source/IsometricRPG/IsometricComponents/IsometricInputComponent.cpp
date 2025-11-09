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
}


void UIsometricInputComponent::HandleLeftClick(const FHitResult& HitResult)
{
 if (!OwnerCharacter) return;
 if (!OwnerASC) // ASC可能尚未复制到客户端，允许照常进行选取逻辑，只是无法发送 Confirm 输入
 {
 OwnerASC = OwnerCharacter->GetAbilitySystemComponent();
 // 不再因为 ASC为空而提前返回，下面的 Confirm 输入仅在 ASC 存在时发送
 }

 //仅当 ASC 存在时才发送确认输入，避免卡死输入链路
 if (OwnerASC)
 {
 SendConfirmTargetInput();
 }

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
 if (!OwnerCharacter) return; //允许在 ASC 未就绪前移动

 // 尝试缓存 ASC，但不要因为拿不到而阻断移动
 if (!OwnerASC)
 {
 OwnerASC = OwnerCharacter->GetAbilitySystemComponent();
 }

 // 总是取消上一次的目标选择状态（仅当 ASC 存在时发送）
 if (OwnerASC)
 {
 SendCancelTargetInput(); 
 }

 AActor* CurrentHitActor = HitResult.GetActor();

 // 优先攻击敌人；若 ASC 尚未就绪，则先向目标位置移动作为降级方案
 if (CurrentHitActor && CurrentHitActor != OwnerCharacter && CurrentHitActor->ActorHasTag(FName("Enemy")))
 {
 //先缓存能力目标数据，ASC 不在也可以缓存
 OwnerCharacter->SetAbilityTargetDataByHit(HitResult);

 if (OwnerASC)
 {
 RequestBasicAttack(CurrentHitActor);
 }
 else
 {
 // ASC 未就绪，先靠近目标
 const FVector FallbackLocation = CurrentHitActor->GetActorLocation();
 RequestMoveToLocation(FallbackLocation);
 }
 }
 else if (HitResult.bBlockingHit)
 {
 // 地面点：始终允许移动
 RequestMoveToLocation(HitResult.ImpactPoint);
 }
}


void UIsometricInputComponent::HandleSkillInput(EAbilityInputID InputID, const FHitResult& TargetData)
{
 if (!OwnerCharacter) return;
 if (!OwnerASC) { OwnerASC = OwnerCharacter->GetAbilitySystemComponent(); }
 if (!OwnerASC) return;

 //先在本地缓存，再RPC同步给服务器，确保服务器拿到同一份目标数据
 OwnerCharacter->SetAbilityTargetDataByHit(TargetData);
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

 //直接通过Spec Handle激活（更稳妥），目标数据已缓存在角色上
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

 // 移动只需角色和控制器即可，ASC 不再是硬依赖
 if (!(OwnerCharacter && OwnerController))
 {
 UE_LOG(LogTemp, Warning, TEXT("RequestMoveToLocation failed: OwnerCharacter (%p) or Controller (%p) is null."), OwnerCharacter, OwnerController);
 return;
 }

 if (!OwnerCharacter->HasAuthority())
 {
 Server_RequestMoveToLocation(TargetLocation);
 }

 //只有在服务器并且 ASC 存在时，才尝试取消攻击能力/停止蒙太奇，避免移动前置不可用
 if (OwnerCharacter->HasAuthority() && CurrentASC)
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
 OwnerCharacter->SetAbilityTargetDataByActor(TargetActor);
 if (OwnerCharacter->GetLocalRole() < ROLE_Authority)
 {
 OwnerCharacter->Server_SetAbilityTargetDataByActor(TargetActor);
 }
 FGameplayTag BasicAttackAbilityTag = FGameplayTag::RequestGameplayTag(FName("Ability.Player.DirBasicAttack"));
 if (BasicAttackAbilityTag.IsValid())
 {
 //直接通过Tag激活技能，这种方式更通用，无需知道具体技能类
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


