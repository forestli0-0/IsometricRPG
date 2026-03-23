// IsometricInputComponent.cpp
#include "IsometricInputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h" 
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "AbilitySystemComponent.h"
#include "Character/IsometricRPGCharacter.h"
#include "IsometricComponents/IsometricPathFollowingComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
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
	if (!OwnerCharacter)
	{
		return;
	}

	if (!OwnerASC)
	{
		OwnerASC = OwnerCharacter->GetAbilitySystemComponent();
	}

	// 仅当 ASC 存在时才发送确认输入，避免卡死输入链路
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


void UIsometricInputComponent::HandleRightClickTriggered(const FHitResult& HitResult, bool bIsHeldInput)
{
	if (!OwnerCharacter)
	{
		return;
	}

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
		// 先缓存能力目标数据，ASC 不在也可以缓存
		OwnerCharacter->SetAbilityTargetDataByHit(HitResult);

		if (OwnerASC)
		{
			RequestBasicAttack(CurrentHitActor, bIsHeldInput);
		}
		else
		{
			// ASC 未就绪，先靠近目标
			const FVector FallbackLocation = CurrentHitActor->GetActorLocation();
			RequestMoveToLocation(FallbackLocation, bIsHeldInput);
		}
	}
	else if (HitResult.bBlockingHit)
	{
		// 地面点：始终允许移动
		RequestMoveToLocation(HitResult.ImpactPoint, bIsHeldInput);
	}
}


void UIsometricInputComponent::HandleSkillPressed(EAbilityInputID InputID, const FHitResult& TargetData)
{
	if (!OwnerCharacter)
	{
		return;
	}

	if (!OwnerASC)
	{
		OwnerASC = OwnerCharacter->GetAbilitySystemComponent();
	}

	if (!OwnerASC)
	{
		return;
	}

	OwnerCharacter->SetAbilityTargetDataByHit(TargetData);
	if (OwnerCharacter->GetLocalRole() < ROLE_Authority)
	{
		OwnerCharacter->Server_SetAbilityTargetDataByHit(TargetData);
	}

	OwnerASC->AbilityLocalInputPressed((int32)InputID);
}

void UIsometricInputComponent::HandleSkillReleased(EAbilityInputID InputID)
{
	if (!OwnerCharacter)
	{
		return;
	}

	if (!OwnerASC)
	{
		OwnerASC = OwnerCharacter->GetAbilitySystemComponent();
	}

	if (!OwnerASC)
	{
		return;
	}

	OwnerASC->AbilityLocalInputReleased((int32)InputID);
}


void UIsometricInputComponent::RequestMoveToLocation(const FVector& TargetLocation, bool bIsHeldInput)
{
	UIsometricPathFollowingComponent* PathFollower = OwnerCharacter ? OwnerCharacter->PathFollowingComponent : nullptr;

	// owning client 立即启动本地路径跟随，真正的移动仍然交给 CharacterMovement。
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("RequestMoveToLocation failed: OwnerCharacter is null."));
		return;
	}

	if (PathFollower && OwnerCharacter->IsLocallyControlled())
	{
		PathFollower->RequestMoveToLocation(TargetLocation);
	}

	if (!OwnerCharacter->HasAuthority())
	{
		if (!bIsHeldInput)
		{
			Server_RequestMoveToLocation(TargetLocation);
		}
		return;
	}

	HandleMoveIntentOnAuthority();
}


void UIsometricInputComponent::RequestBasicAttack(AActor* TargetActor, bool bUseUnreliableRemoteUpdate)
{
	AController* OwnerController = OwnerCharacter ? OwnerCharacter->GetController() : nullptr;
	UAbilitySystemComponent* CurrentASC = OwnerCharacter ? OwnerCharacter->GetAbilitySystemComponent() : nullptr;

	if (!OwnerCharacter || !CurrentASC || !TargetActor || !OwnerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("RequestBasicAttack failed: OwnerCharacter (%p), Controller (%p), ASC (%p), or TargetActor (%p) is null."), OwnerCharacter, OwnerController, CurrentASC, TargetActor);
		return;
	}

	// 攻击接管移动前，先停掉本地的点击移动预测，避免两套输入同时推角色。
	StopPredictedClickMove();

	// 客户端转到服务器
	if (OwnerCharacter->GetLocalRole() < ROLE_Authority)
	{
		if (bUseUnreliableRemoteUpdate)
		{
			Server_UpdateBasicAttack(TargetActor);
		}
		else
		{
			Server_RequestBasicAttack(TargetActor);
		}
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

void UIsometricInputComponent::HandleMoveIntentOnAuthority()
{
	if (!OwnerCharacter)
	{
		return;
	}

	if (UAbilitySystemComponent* CurrentASC = OwnerCharacter->GetAbilitySystemComponent())
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
	}

	if (OwnerCharacter->GetMesh() && OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		OwnerCharacter->GetMesh()->GetAnimInstance()->Montage_Stop(0.1f);
		UE_LOG(LogTemp, Log, TEXT("Server: Stopped any active montage."));
	}

}

void UIsometricInputComponent::StopPredictedClickMove()
{
	if (!(OwnerCharacter && OwnerCharacter->IsLocallyControlled()))
	{
		return;
	}

	if (OwnerCharacter->PathFollowingComponent)
	{
		OwnerCharacter->PathFollowingComponent->StopMove();
	}
}


// --- Server RPC implementations ---
void UIsometricInputComponent::Server_RequestMoveToLocation_Implementation(const FVector& TargetLocation)
{
	HandleMoveIntentOnAuthority();
}


void UIsometricInputComponent::Server_RequestBasicAttack_Implementation(AActor* TargetActor)
{
	RequestBasicAttack(TargetActor);
}

void UIsometricInputComponent::Server_UpdateBasicAttack_Implementation(AActor* TargetActor)
{
	RequestBasicAttack(TargetActor);
}


