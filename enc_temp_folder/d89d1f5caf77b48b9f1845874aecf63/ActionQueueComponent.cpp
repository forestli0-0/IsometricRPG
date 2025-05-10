#include "IsometricComponents/ActionQueueComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Engine/Engine.h"
#include "AbilitySystemComponent.h"
#include "Character/IsometricRPGAttributeSetBase.h"
UActionQueueComponent::UActionQueueComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	AttackEventTag = FGameplayTag::RequestGameplayTag("Ability.MeleeAttack");
}

void UActionQueueComponent::BeginPlay()
{
	Super::BeginPlay();
	// 在技能系统上绑定一个回调，当技能超出距离时，给角色挂一个提示
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	ASC = OwnerCharacter->FindComponentByClass<UAbilitySystemComponent>();
	ASC->GenericGameplayEventCallbacks.FindOrAdd(
		FGameplayTag::RequestGameplayTag(FName("Ability.Failure.OutOfRange"))
	).AddUObject(this, &UActionQueueComponent::OnSkillOutOfRange);
}

void UActionQueueComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerCharacter || CurrentCommand.Type == EQueuedCommandType::None) return;
    GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Magenta, FString::Printf(TEXT("当前命令：%s"), *UEnum::GetValueAsString(CurrentCommand.Type)));
	switch (CurrentCommand.Type)
	{
	case EQueuedCommandType::MoveToLocation:
	{
		const float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), CurrentCommand.TargetLocation);
		if (Distance < 50.f)
		{
			ClearCommand();
		}
		else
		{
			// 计算目标方向
			FVector DirectionToTarget = CurrentCommand.TargetLocation - OwnerCharacter->GetActorLocation();
			DirectionToTarget.Z = 0.0f;
			// 计算目标旋转
			FRotator TargetRotation = FRotationMatrix::MakeFromX(DirectionToTarget).Rotator();
			// 使用插值平滑旋转
			FRotator NewRotation = FMath::RInterpTo(
				OwnerCharacter->GetActorRotation(),
				TargetRotation,
				DeltaTime,  // 使用 DeltaTime 实现平滑过渡
				10.0f      // 旋转速度系数
			);
			OwnerCharacter->SetActorRotation(NewRotation);
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(OwnerCharacter->GetController(), CurrentCommand.TargetLocation);
			ClearCommand();
		}
		break;
	}
	case EQueuedCommandType::AttackTarget:
	{
		// 检查目标Health属性
		auto TargetCharacter = Cast<ACharacter>(CurrentCommand.TargetActor);
		if (!TargetCharacter) return;
		auto TargetASC = TargetCharacter->FindComponentByClass<UAbilitySystemComponent>();
		auto TargetHealth = TargetASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetHealthAttribute());

		if (TargetHealth <= 0)
		{
			// 目标死亡，清除命令
			ClearCommand();
			break;
		}

		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("当前命令：普通攻击"));
		ASC = OwnerCharacter->FindComponentByClass<UAbilitySystemComponent>();
		// 使用GameplayAbility系统激活技能
		FGameplayEventData EventData;
		EventData.Target = CurrentCommand.TargetActor.Get();
		EventData.Instigator = OwnerCharacter;
		FGameplayAbilityTargetDataHandle TargetDataHandle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(FHitResult(CurrentCommand.TargetActor.Get(), nullptr, CurrentCommand.TargetLocation, FVector::ZeroVector));
		EventData.TargetData = TargetDataHandle;
		ASC->HandleGameplayEvent(CurrentCommand.AbilityEventTag, &EventData);
		break;
	}
	case EQueuedCommandType::UseSkill:
	{
		if (CurrentCommand.TargetActor->IsActorBeingDestroyed())
		{
			// 目标死亡，清除命令
			ClearCommand();
			break;
		}
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("当前命令：使用技能"));
		ExecuteSkill(CurrentCommand.AbilityEventTag, CurrentCommand.TargetLocation, CurrentCommand.TargetActor.Get());

		break;
	}
	}
}

FQueuedCommand UActionQueueComponent::GetCommand()
{
	return CurrentCommand;
}

void UActionQueueComponent::SetCommand_MoveTo(const FVector& Location)  
{  
   if (!OwnerCharacter) return;  
   if (bAttackInProgress)  
   {  
       return; // 如果正在攻击，直接返回  
   }  
   // 停止正在播放的蒙太奇  
   if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())  
   {  
       if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())  
       {  
           AnimInstance->Montage_Stop(0.1f); // 0.2秒的混合时间，可以根据需要调整  
       }  
   }  
   ClearCommand();  

   CurrentCommand.Type = EQueuedCommandType::MoveToLocation;  
   CurrentCommand.TargetLocation = Location;  

}

void UActionQueueComponent::SetCommand_AttackTarget(const FGameplayTag& AbilityTag, const FVector& TargetLocation, AActor* TargetActor)
{
	if (!OwnerCharacter || !TargetActor) return;

	ClearCommand();

	CurrentCommand.Type = EQueuedCommandType::AttackTarget;
	CurrentCommand.TargetActor = TargetActor;
	CurrentCommand.TargetLocation = TargetLocation;
	CurrentCommand.TargetActor = TargetActor;
	CurrentCommand.AbilityEventTag = AbilityTag;
	bIsExecuting = true;
}

void UActionQueueComponent::ClearCommand()
{
	CurrentCommand = FQueuedCommand(); // Reset to default
	bIsExecuting = false;
}

void UActionQueueComponent::OnSkillOutOfRange(const FGameplayEventData* EventData)
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, TEXT("技能因距离失败，移动接近目标"));
	DrawDebugSphere(GetWorld(), EventData->Target.Get()->GetActorLocation(), 50.f, 12, FColor::Red, false, 19.0f, 0, 1.0f); // 可选：调试可视化);
	UAIBlueprintHelperLibrary::SimpleMoveToActor(OwnerCharacter->GetController(), EventData->Target.Get());
}

void UActionQueueComponent::InitializeAbilitySlots()
{
	if (!OwnerCharacter) return;
	for (const FHeroAbilitySlotData& SlotData : AbilitySlots)
	{
		if (*SlotData.AbilityClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("初始化技能槽中, 技能：%s, 槽位：%d"), *SlotData.AbilityClass->GetName(), static_cast<uint8>(SlotData.Slot));

			ASC->GiveAbility(FGameplayAbilitySpec(SlotData.AbilityClass, 1, static_cast<int32>(SlotData.Slot)));
		}
	}
}

void UActionQueueComponent::SetCommand_UseSkill(const FGameplayTag& AbilityTag, const FVector& TargetLocation, AActor* TargetActor)
{
	CurrentCommand.Type = EQueuedCommandType::UseSkill;
	CurrentCommand.TargetLocation = TargetLocation;
	CurrentCommand.TargetActor = TargetActor;
	CurrentCommand.AbilityEventTag = AbilityTag;
	bIsExecuting = true;
}
void UActionQueueComponent::ExecuteSkill(const FGameplayTag& AbilityTag, const FVector& TargetLocation, AActor* TargetActor)
{
	if (!OwnerCharacter) return;

	// 使用GameplayAbility系统激活技能
	FGameplayEventData EventData;
	EventData.Target = TargetActor;
	EventData.Instigator = OwnerCharacter;
    FGameplayAbilityTargetDataHandle TargetDataHandle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(FHitResult(TargetActor, nullptr, TargetLocation, FVector::ZeroVector));  
    EventData.TargetData = TargetDataHandle;
	ASC->HandleGameplayEvent(CurrentCommand.AbilityEventTag, &EventData);
}


