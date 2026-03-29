// Fill out your copyright notice in the Description page of Project Settings.

#include "GA_DashStrike.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Character/IsometricRPGCharacter.h"
#include "IsometricComponents/IsometricPathFollowingComponent.h"
#include "Engine/OverlapResult.h"


UGA_DashStrike::UGA_DashStrike()
{
	// 设置技能类型
	AbilityType = EHeroAbilityType::SkillShot;

	// 设置基础属性
	DashDistance = 1000.0f;
	DashSpeed = 1500.0f;
	CollisionRadius = 60.0f;
	SkillShotWidth = CollisionRadius * 2.0f;

	// DashStrike 直接消费当前瞄准方向，不进入交互式选目标流程。
	SetUsesInteractiveTargeting(false);
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InputPolicy.InputMode = EAbilityInputMode::Instant;
	InputPolicy.bUpdateTargetWhileHeld = false;
	InputPolicy.bAllowInputBuffer = true;

	// 设置标签
	FGameplayTagContainer AssetTagContainer = GetAssetTags();
	AssetTagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.DashStrike")));
	SetAssetTags(AssetTagContainer);
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Dashing")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Dead")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Stunned")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Cooldown.Ability.DashStrike")));
}

void UGA_DashStrike::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::ExecuteSkill(Handle, ActorInfo, ActivationInfo);

	if (!ensureAlwaysMsgf(
		HasRequiredExecutionTargetData(CurrentTargetDataHandle),
		TEXT("%s: DashStrike executed without aim context."), *GetName()))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 获取突进方向
	FVector DashDirection = GetSkillShotDirection();
	FVector StartLocation = GetAvatarActorFromActorInfo()->GetActorLocation();
	if (!ensureAlwaysMsgf(!DashDirection.IsNearlyZero(), TEXT("%s: DashStrike received an invalid dash direction."), *GetName()))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("DashStrike: Starting dash in direction: %s"), *DashDirection.ToString());

	// 执行突进
	ExecuteDash(DashDirection, StartLocation);
}

void UGA_DashStrike::ExecuteDash(const FVector& DashDirection, const FVector& StartLocation)
{
	ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!SourceCharacter)
	{
		OnDashComplete();
		return;
	}

	bDashEndRequested = false;
	DashStartLocation = StartLocation;
	CurrentDashDirection = DashDirection.GetSafeNormal2D();
	DashTargetLocation = StartLocation + (DashDirection * DashDistance);
	DashTargetLocation.Z = StartLocation.Z;
	HitActors.Empty();
	StopConflictingMovement(*SourceCharacter);
	LastCollisionSampleLocation = StartLocation;

	float ActualDashDistance = FVector::Dist2D(StartLocation, DashTargetLocation);
	UE_LOG(LogTemp, Log, TEXT("DashStrike: Start=%s, Target=%s, Distance=%f"), 
		*StartLocation.ToString(), *DashTargetLocation.ToString(), ActualDashDistance);

	// 检查目标位置是否可达，避免穿墙
	FVector AdjustedTargetLocation = ValidateDashDestination(StartLocation, DashTargetLocation);
	DashTargetLocation = AdjustedTargetLocation;
	DashTargetLocation.Z = StartLocation.Z;
	ActiveDashDuration = FVector::Dist2D(StartLocation, DashTargetLocation) / FMath::Max(DashSpeed, 1.0f);
	if (ActiveDashDuration <= KINDA_SMALL_NUMBER)
	{
		OnDashComplete();
		return;
	}

	// 如果位置被调整了，记录调整后的距离
	if (!AdjustedTargetLocation.Equals(StartLocation + (DashDirection * DashDistance), 1.0f))
	{
		float AdjustedDistance = FVector::Dist2D(StartLocation, AdjustedTargetLocation);
		UE_LOG(LogTemp, Warning, TEXT("DashStrike: Target adjusted due to wall collision. New distance: %f"), AdjustedDistance);
	}

	// 播放突进声音
	if (DashSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DashSound, StartLocation);
	}

	// 生成突进特效
	if (DashEffectClass)
	{
		ActiveDashEffect = GetWorld()->SpawnActor<ANiagaraActor>(
			DashEffectClass,
			StartLocation,
			CurrentDashDirection.Rotation()
		);
		if (ActiveDashEffect)
		{
			// 附加到角色
			ActiveDashEffect->AttachToActor(SourceCharacter, FAttachmentTransformRules::KeepWorldTransform);

			// 获取 Niagara 组件
			UNiagaraComponent* NiagaraComp = ActiveDashEffect->GetNiagaraComponent();

			if (NiagaraComp)
			{
				// dash 朝向交给 Niagara，用于驱动拖尾或锥形特效。
				NiagaraComp->SetVariableVec3(FName(TEXT("Cone Axis")), CurrentDashDirection * (-10.f));
			}
		}
	}

	// 应用无敌效果（如果有的话）
	ApplyInvulnerabilityEffect();

	// 用 GAS RootMotion task 驱动 dash，底层走 CharacterMovement 的预测/复制链路。
	StartDashMovement();
}

void UGA_DashStrike::PerformDashCollisionCheck(const FVector& CurrentLocation, const FVector& PreviousLocation)
{
	// 执行球体扫描检测敌人
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetAvatarActorFromActorInfo());
	QueryParams.bTraceComplex = false;

	// 在突进路径上进行扫描检测
	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		PreviousLocation,
		CurrentLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(CollisionRadius),
		QueryParams
	);

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && !HitActors.Contains(HitActor))
			{
				// 检查是否是敌人
				if (IsEnemyActor(HitActor))
				{
					HitActors.Add(HitActor);
					OnHitEnemy(HitActor, Hit.Location);
				}
			}
		}
	}

	// 额外的重叠检测，确保不会漏掉静态的敌人
	TArray<FOverlapResult> OverlapResults;
	bool bOverlapHit = GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		CurrentLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(CollisionRadius),
		QueryParams
	);

	if (bOverlapHit)
	{
		for (const FOverlapResult& Overlap : OverlapResults)
		{
			AActor* OverlapActor = Overlap.GetActor();
			if (OverlapActor && !HitActors.Contains(OverlapActor))

			{
				if (IsEnemyActor(OverlapActor))
				{
					HitActors.Add(OverlapActor);
					OnHitEnemy(OverlapActor, OverlapActor->GetActorLocation());
				}
			}
		}
	}
}

void UGA_DashStrike::OnHitEnemy(AActor* HitActor, const FVector& HitLocation)
{
	if (!HitActor)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("DashStrike: Hit enemy %s"), *HitActor->GetName());

	// 播放击中音效
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, HitLocation);
	}

	// 生成击中特效
	if (HitEffectClass)
	{
		GetWorld()->SpawnActor<ANiagaraActor>(
			HitEffectClass,
			HitLocation,
			FRotator::ZeroRotator
		);
	}

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const bool bIsAuthority = AvatarActor && AvatarActor->HasAuthority();
	if (!bIsAuthority)
	{
		return;
	}

	// 获取敌人的AbilitySystemComponent
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (!TargetASC)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return;
	}

	// 应用伤害效果
	if (DamageEffect)
	{
		FGameplayEffectContextHandle DamageContext = SourceASC->MakeEffectContext();
		DamageContext.AddSourceObject(this);
		DamageContext.AddHitResult(FHitResult()); // 可以传入实际的HitResult

		FGameplayEffectSpecHandle DamageSpec = SourceASC->MakeOutgoingSpec(
			DamageEffect, GetAbilityLevel(), DamageContext);

		if (DamageSpec.IsValid())
		{
			
			DamageSpec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), -DamageMagnitude);
			// 可以在这里设置伤害值等参数
			SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
		}
	}

	// 应用击退效果
	if (KnockbackEffect)
	{
		FGameplayEffectContextHandle KnockbackContext = SourceASC->MakeEffectContext();
		KnockbackContext.AddSourceObject(this);
		
		// 设置击退方向（从我们的位置指向敌人）
		FVector KnockbackDirection = (HitActor->GetActorLocation() - GetAvatarActorFromActorInfo()->GetActorLocation()).GetSafeNormal();
		
		// 将击退方向信息存储到Context中（需要自定义GameplayEffectContext或使用SetByCaller）
		FGameplayEffectSpecHandle KnockbackSpec = SourceASC->MakeOutgoingSpec(
			KnockbackEffect, GetAbilityLevel(), KnockbackContext);

		if (KnockbackSpec.IsValid())
		{
			// 设置击退参数
			KnockbackSpec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Ability.Knockback.Force")), 800.0f);
			SourceASC->ApplyGameplayEffectSpecToTarget(*KnockbackSpec.Data.Get(), TargetASC);
		}
	}

	// 可选：对敌人施加短暂的眩晕效果
	FGameplayTagContainer StunTags;
	StunTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Debuff.Stunned")));
	
	// 应用0.5秒眩晕
	TargetASC->AddLooseGameplayTags(StunTags);
	
	// 设置计时器移除眩晕
	FTimerHandle StunTimerHandle;
	FTimerDelegate StunDelegate;
	StunDelegate.BindLambda([TargetASC, StunTags]()
	{
		if (IsValid(TargetASC))
		{
			TargetASC->RemoveLooseGameplayTags(StunTags);
		}
	});
	
	GetWorld()->GetTimerManager().SetTimer(StunTimerHandle, StunDelegate, 0.5f, false);
}

void UGA_DashStrike::OnDashComplete()
{
	if (bDashEndRequested)
	{
		return;
	}

	bDashEndRequested = true;
	UE_LOG(LogTemp, Log, TEXT("DashStrike: Dash completed"));

	// 结束技能
	const FGameplayAbilitySpecHandle Handle = CurrentSpecHandle;
	const FGameplayAbilityActorInfo* ActorInfo = CurrentActorInfo;
	const FGameplayAbilityActivationInfo ActivationInfo = CurrentActivationInfo;

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_DashStrike::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	CleanupDashState();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	bDashEndRequested = false;
}

bool UGA_DashStrike::IsEnemyActor(AActor* Actor) const
{
	// 这里需要根据你的游戏逻辑来判断是否是敌人
	
	// 方法1：通过标签判断
	if (Actor->ActorHasTag(FName("Enemy")))
	{
		return true;
	}

	// 方法2：通过AbilitySystem标签判断
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (TargetASC)
	{
		if (TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Character.Type.Enemy"))))
		{
			return true;
		}
	}

	return false;
}

void UGA_DashStrike::PlayAbilityMontage(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (SourceCharacter == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_DashStrike::PlayAbilityMontage: Owener is not a Character!"));
		return;
	}

	StopConflictingMovement(*SourceCharacter);
	Super::PlayAbilityMontage(Handle, ActorInfo, ActivationInfo);
}

FVector UGA_DashStrike::ValidateDashDestination(const FVector& StartLocation, const FVector& TargetLocation)
{
	// 进行射线检测，确保不会穿墙
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetAvatarActorFromActorInfo());
	QueryParams.bTraceComplex = false;

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic); // 只检测 WorldStatic 对象类型的物体

	bool bHit = GetWorld()->LineTraceSingleByObjectType(
		HitResult,
		StartLocation,
		TargetLocation,
		ObjectQueryParams, // 使用 ObjectQueryParams
		QueryParams
	);

	if (bHit)
	{
		// 如果有阻挡，将目标位置调整到碰撞点前一点
		FVector SafeDistance = CurrentDashDirection * (-60.0f); // 往回退50单位
		return HitResult.Location + SafeDistance;
	}

	return TargetLocation;
}

void UGA_DashStrike::ApplyInvulnerabilityEffect()
{
	if (InvulnerabilityEffect)
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddSourceObject(this);

			FGameplayEffectSpecHandle EffectSpec = ASC->MakeOutgoingSpec(
				InvulnerabilityEffect, 
				GetAbilityLevel(), 
				Context
			);

			if (EffectSpec.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
			}
		}
	}
}

void UGA_DashStrike::RemoveInvulnerabilityEffect()
{
	if (InvulnerabilityEffect)
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			// 移除无敌效果 - 这里需要根据你的GE设计来调整
			FGameplayTagContainer TagsToRemove;
			TagsToRemove.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Invulnerable")));
			ASC->RemoveActiveEffectsWithTags(TagsToRemove);
		}
	}
}

void UGA_DashStrike::StartDashMovement()
{
	ActiveDashMoveTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this,
		FName(TEXT("DashStrikeMove")),
		DashTargetLocation,
		ActiveDashDuration,
		true,
		MOVE_Flying,
		true,
		nullptr,
		ERootMotionFinishVelocityMode::SetVelocity,
		FVector::ZeroVector,
		0.0f);

	if (!ActiveDashMoveTask)
	{
		OnDashComplete();
		return;
	}

	ActiveDashMoveTask->OnTimedOut.AddDynamic(this, &UGA_DashStrike::HandleDashMovementFinished);
	ActiveDashMoveTask->OnTimedOutAndDestinationReached.AddDynamic(this, &UGA_DashStrike::HandleDashMovementFinished);
	ActiveDashMoveTask->ReadyForActivation();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DashCollisionTimerHandle,
			this,
			&UGA_DashStrike::UpdateDashCollisionSweep,
			CollisionCheckInterval,
			true);
	}
}

void UGA_DashStrike::UpdateDashCollisionSweep()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		OnDashComplete();
		return;
	}

	const FVector CurrentLocation = AvatarActor->GetActorLocation();
	PerformDashCollisionCheck(CurrentLocation, LastCollisionSampleLocation);
	LastCollisionSampleLocation = CurrentLocation;
}

void UGA_DashStrike::HandleDashMovementFinished()
{
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		const FVector CurrentLocation = AvatarActor->GetActorLocation();
		PerformDashCollisionCheck(CurrentLocation, LastCollisionSampleLocation);
		LastCollisionSampleLocation = CurrentLocation;
	}

	OnDashComplete();
}

void UGA_DashStrike::StopConflictingMovement(ACharacter& SourceCharacter) const
{
	if (AIsometricRPGCharacter* RPGCharacter = Cast<AIsometricRPGCharacter>(&SourceCharacter))
	{
		if (RPGCharacter->PathFollowingComponent)
		{
			RPGCharacter->PathFollowingComponent->StopMove();
		}
	}

	if (UCharacterMovementComponent* MovementComponent = SourceCharacter.GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	if (AController* Controller = SourceCharacter.GetController())
	{
		Controller->StopMovement();
	}
}

void UGA_DashStrike::CleanupDashState()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashCollisionTimerHandle);
	}

	RemoveInvulnerabilityEffect();

	if (ActiveDashEffect && IsValid(ActiveDashEffect))
	{
		ActiveDashEffect->Destroy();
		ActiveDashEffect = nullptr;
	}

	ActiveDashMoveTask = nullptr;
	ActiveDashDuration = 0.0f;
	LastCollisionSampleLocation = FVector::ZeroVector;
	HitActors.Empty();
}

FVector UGA_DashStrike::GetSkillShotDirection() const
{
	const FVector AimDirection = GetCurrentAimDirection();
	if (!ensureAlwaysMsgf(!AimDirection.IsNearlyZero(), TEXT("%s: DashStrike is missing AimDirection at execution time."), *GetName()))
	{
		return FVector::ZeroVector;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!ensureAlwaysMsgf(AvatarActor != nullptr, TEXT("%s: DashStrike avatar actor is null during direction calculation."), *GetName()))
	{
		return FVector::ZeroVector;
	}

	const FVector StartLocation = AvatarActor->GetActorLocation();
	const FVector TargetLocation = GetCurrentAimPoint();

	const float ClickDistance = FVector::Dist(StartLocation, TargetLocation);
	UE_LOG(LogTemp, Log, TEXT("DashStrike: Click distance: %f, Will dash: %f"), ClickDistance, DashDistance);

	return AimDirection;
}
