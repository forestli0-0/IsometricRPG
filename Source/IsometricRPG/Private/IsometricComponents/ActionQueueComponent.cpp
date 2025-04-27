#include "IsometricComponents/ActionQueueComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Engine/Engine.h"

UActionQueueComponent::UActionQueueComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	AttackEventTag = FGameplayTag::RequestGameplayTag("Ability.MeleeAttack");
}

void UActionQueueComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
}

void UActionQueueComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerCharacter || CurrentCommand.Type == EQueuedCommandType::None) return;

	switch (CurrentCommand.Type)
	{
	case EQueuedCommandType::MoveToLocation:
	{
		const float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), CurrentCommand.TargetLocation);
		if (Distance < 50.f)
		{
			ClearCommand();
		}
		break;
	}
	case EQueuedCommandType::AttackTarget:
	{
		if (!CurrentCommand.TargetActor.IsValid()) {
			ClearCommand();
			break;
		}

		const float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), CurrentCommand.TargetActor->GetActorLocation());
		if (Distance <= AttackRange)
		{
			if (AAIController* AICon = Cast<AAIController>(OwnerCharacter->GetController()))
			{
				AICon->StopMovement();
			}
			// ✅ 攻击冷却判定
			double Now = FPlatformTime::Seconds();
			if (Now - LastAttackTime >= AttackInterval) {
				LastAttackTime = Now;
				// 攻击一次
				FGameplayEventData EventData;
				EventData.Target = CurrentCommand.TargetActor.Get();
				EventData.Instigator = OwnerCharacter;
				EventData.EventTag = AttackEventTag;
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerCharacter, AttackEventTag, EventData);
				// 检查目标是否死亡
				if (CurrentCommand.TargetActor->IsActorBeingDestroyed())
				{
					// 目标死亡，清除命令
					ClearCommand();
				}
			}

		}
		break;
	}
	}
}

void UActionQueueComponent::SetCommand_MoveTo(const FVector& Location)
{
	if (!OwnerCharacter) return;

	// 停止正在播放的蒙太奇
	if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
	{
		if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.2f); // 0.2秒的混合时间，可以根据需要调整
		}
	}

	ClearCommand();

	CurrentCommand.Type = EQueuedCommandType::MoveToLocation;
	CurrentCommand.TargetLocation = Location;

	UAIBlueprintHelperLibrary::SimpleMoveToLocation(OwnerCharacter->GetController(), Location);
}

void UActionQueueComponent::SetCommand_AttackTarget(AActor* Target)
{
	if (!OwnerCharacter || !Target) return;

	ClearCommand();

	CurrentCommand.Type = EQueuedCommandType::AttackTarget;
	CurrentCommand.TargetActor = Target;

	UAIBlueprintHelperLibrary::SimpleMoveToActor(OwnerCharacter->GetController(), Target);
}

void UActionQueueComponent::ClearCommand()
{
	CurrentCommand = FQueuedCommand(); // Reset to default
	bIsExecuting = false;
	//LastAttackTime = -100.f;
}
