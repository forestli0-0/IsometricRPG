// Fill out your copyright notice in the Description page of Project Settings.


#include "IsometricAbilities/GA_HeroBaseAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h" 
#include "AbilitySystemComponent.h"
#include "Character/IsometricRPGAttributeSetBase.h"

UGA_HeroBaseAbility::UGA_HeroBaseAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_HeroBaseAbility::ActivateAbility(
  const FGameplayAbilitySpecHandle Handle,
  const FGameplayAbilityActorInfo* ActorInfo,
  const FGameplayAbilityActivationInfo ActivationInfo,
  const FGameplayEventData* TriggerEventData)
{
  AActor* SelfActor = GetAvatarActorFromActorInfo();
  if (!SelfActor)
  {
      UE_LOG(LogTemp, Error, TEXT("SelfActor is null."));
      return;
  }
// 从 AbilitySystemComponent 获取 AttributeSet
  UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
  if (!ASC)
  {
      CancelAbility(Handle, ActorInfo, ActivationInfo, true);
      UE_LOG(LogTemp, Error, TEXT("AbilitySystemComponent is null in ActivateAbility."));
      return;
  }

  AttributeSet = const_cast<UIsometricRPGAttributeSetBase*>(ASC->GetSet<UIsometricRPGAttributeSetBase>());
  if (!AttributeSet)
  {
      CancelAbility(Handle, ActorInfo, ActivationInfo, true);
      UE_LOG(LogTemp, Error, TEXT("AttributeSet is null in ActivateAbility."));
      return;
  }
  AActor* Target = const_cast<AActor*>(TriggerEventData->Target.Get());

  // 如果是指向型技能，没有目标，那就放弃
  if (SkillType == EHeroSkillType::Targeted)
  {
      if (Cast<APawn>(Target) == nullptr)
      {
          CancelAbility(Handle, ActorInfo, ActivationInfo, true);
          UE_LOG(LogTemp, Error, TEXT("TriggerEventData is null in ActivateAbility."));
          return;
      }
  }


  float Distance = FVector::Dist(Target->GetActorLocation(), SelfActor->GetActorLocation());

  if (Distance > RangeToApply)
  {
      // 构造失败的 gameplay event
      FGameplayEventData FailEventData;
      FailEventData.EventTag = FGameplayTag::RequestGameplayTag(FName("Ability.Failure.OutOfRange"));
      FailEventData.Instigator = SelfActor;
      FailEventData.Target = Target;

      // 通知自身 ASC
      GetAbilitySystemComponentFromActorInfo()->HandleGameplayEvent(FailEventData.EventTag, &FailEventData);

      CancelAbility(Handle, ActorInfo, ActivationInfo, true);
      GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Black, TEXT("距离太远"));
      return;
  }

  if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
  {
      // 如果无法提交技能，则结束技能
      EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
      return;
  }

  if (bFaceTarget && ActorInfo->AvatarActor.IsValid())
  {
      FVector Direction = (Target->GetActorLocation() - ActorInfo->AvatarActor->GetActorLocation()).GetSafeNormal();
      FRotator NewRotation = FRotationMatrix::MakeFromX(Direction).Rotator();
      SelfActor->SetActorRotation(NewRotation);
  }

  UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
  if (MontageToPlay && IsValid(AnimInstance))
  {
      float PlayRate = CooldownDuration > 0 ? MontageToPlay->GetPlayLength() / CooldownDuration : 1.0f;
	  PlayRate = FMath::Clamp(PlayRate, 1.f, 3.0f); // 限制播放速率在合理范围内
      MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
          this, NAME_None, MontageToPlay, PlayRate);

      MontageTask->OnCompleted.AddDynamic(this, &UGA_HeroBaseAbility::K2_EndAbility);
      MontageTask->OnInterrupted.AddDynamic(this, &UGA_HeroBaseAbility::K2_EndAbility);
      MontageTask->OnCancelled.AddDynamic(this, &UGA_HeroBaseAbility::K2_EndAbility);
      MontageTask->ReadyForActivation();
  }

  OnAbilityTriggered();
  EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

void UGA_HeroBaseAbility::OnAbilityTriggered_Implementation()
{
    switch (SkillType)
    {
    case EHeroSkillType::Targeted:
        ExecuteTargeted();
        break;
    case EHeroSkillType::Projectile:
        ExecuteProjectile();
        break;
    case EHeroSkillType::Area:
        ExecuteArea();
        break;
    case EHeroSkillType::SkillShot:
        ExecuteSkillShot();
        break;
    case EHeroSkillType::SelfCast:
        ExecuteSelfCast();
        break;
    }
}

void UGA_HeroBaseAbility::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (MontageTask)
    {
        MontageTask->EndTask();
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, true, bWasCancelled);
}

// 各种技能类型的默认实现（可重写）
void UGA_HeroBaseAbility::ExecuteTargeted() {}
void UGA_HeroBaseAbility::ExecuteProjectile() {}
void UGA_HeroBaseAbility::ExecuteArea() {}
void UGA_HeroBaseAbility::ExecuteSkillShot() {}
void UGA_HeroBaseAbility::ExecuteSelfCast() {}