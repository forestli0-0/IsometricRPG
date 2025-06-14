// Fill out your copyright notice in the Description page of Project Settings.

#include "GA_DashStrike.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
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

	// 需要目标数据（方向选择）
	bRequiresTargetData = true;

	// 设置标签
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.DashStrike")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Dashing")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Dead")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Stunned")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Cooldown.Ability.DashStrike")));
}

void UGA_DashStrike::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ExecuteSkill(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!TriggerEventData || !ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 获取突进方向
	FVector DashDirection = GetSkillShotDirection(TriggerEventData);
	FVector StartLocation = ActorInfo->AvatarActor->GetActorLocation();

	UE_LOG(LogTemp, Log, TEXT("DashStrike: Starting dash in direction: %s"), *DashDirection.ToString());

	// 执行突进
	ExecuteDash(DashDirection, StartLocation);
}

void UGA_DashStrike::ExecuteDash(const FVector& DashDirection, const FVector& StartLocation)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		OnDashComplete();
		return;
	}
	// 初始化突进数据
	DashStartLocation = StartLocation;
	CurrentDashDirection = DashDirection;
	DashTargetLocation = StartLocation + (DashDirection * DashDistance);
	HitActors.Empty();

	// 记录调试信息
	float ActualDashDistance = FVector::Dist(StartLocation, DashTargetLocation);
	UE_LOG(LogTemp, Log, TEXT("DashStrike: Start=%s, Target=%s, Distance=%f"), 
		*StartLocation.ToString(), *DashTargetLocation.ToString(), ActualDashDistance);

	// 检查目标位置是否可达，避免穿墙
	FVector AdjustedTargetLocation = ValidateDashDestination(StartLocation, DashTargetLocation);
	DashTargetLocation = AdjustedTargetLocation;

	// 如果位置被调整了，记录调整后的距离
	if (!AdjustedTargetLocation.Equals(StartLocation + (DashDirection * DashDistance), 1.0f))
	{
		float AdjustedDistance = FVector::Dist(StartLocation, AdjustedTargetLocation);
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
			DashDirection.Rotation()
		);
		if (ActiveDashEffect)
		{
			// 附加到角色
			ActiveDashEffect->AttachToActor(AvatarActor, FAttachmentTransformRules::KeepWorldTransform);

			// 获取 Niagara 组件
			UNiagaraComponent* NiagaraComp = ActiveDashEffect->GetNiagaraComponent();

			if (NiagaraComp)
			{
				// 设置 Beam End 参数
				// "Beam End" 是你在Niagara系统中定义的参数名称
				// 假设它是一个FVector类型的用户参数
				FVector BeamEndLocation = DashStartLocation + CurrentDashDirection * (-10.f) ; // 你要设置的值

				NiagaraComp->SetNiagaraVariableVec3(TEXT("Cone Axis"), CurrentDashDirection * (-10.f));


				// 如果 Beam End 下的 X, Y, Z 是独立的浮点参数，你可以这样设置：
				// NiagaraComp->SetNiagaraVariableFloat(TEXT("Beam End.X"), 200.0f);
				// NiagaraComp->SetNiagaraVariableFloat(TEXT("Beam End.Y"), 0.0f);
				// NiagaraComp->SetNiagaraVariableFloat(TEXT("Beam End.Z"), 0.0f);
				// 注意：具体是 Vec3 还是独立的 Float 取决于你在 Niagara 系统中如何定义这个用户参数。
				// 鉴于图片显示为 X, Y, Z 嵌套在 Beam End 下，很可能 Beam End 是一个 FVector 类型。
			}
		}
	}

	// 应用无敌效果（如果有的话）
	ApplyInvulnerabilityEffect();

	// 执行渐进式突进（更平滑的移动）
	StartProgressiveDash();
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
		return;

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

	// 获取敌人的AbilitySystemComponent
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (!TargetASC)
		return;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
		return;

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
	// 清理所有计时器
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DashTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(ProgressiveDashTimerHandle);
	}

	// 移除无敌效果
	RemoveInvulnerabilityEffect();

	// 清理特效
	if (ActiveDashEffect && IsValid(ActiveDashEffect))
	{
		ActiveDashEffect->Destroy();
		ActiveDashEffect = nullptr;
	}

	// 重置突进数据
	DashProgress = 0.0f;
	HitActors.Empty();

	UE_LOG(LogTemp, Log, TEXT("DashStrike: Dash completed"));

	// 结束技能
	const FGameplayAbilitySpecHandle Handle = CurrentSpecHandle;
	const FGameplayAbilityActorInfo* ActorInfo = CurrentActorInfo;
	const FGameplayAbilityActivationInfo ActivationInfo = CurrentActivationInfo;

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
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
	// 停止移动，否则在发射后会出现很怪的逻辑
	auto Owener = GetAvatarActorFromActorInfo();
	auto SourCharacter = Cast<ACharacter>(Owener);
	SourCharacter->GetController()->StopMovement();
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

void UGA_DashStrike::StartProgressiveDash()
{
	DashProgress = 0.0f;
	
	// 计算突进总时间
	float DashTime = DashDistance / DashSpeed;
	float UpdateInterval = 0.016f; // 约60FPS更新率

	// 开始计时器进行渐进式移动
	GetWorld()->GetTimerManager().SetTimer(
		ProgressiveDashTimerHandle,
		this,
		&UGA_DashStrike::UpdateDashPosition,
		UpdateInterval,
		true
	);

	// 设置总时间结束计时器
	GetWorld()->GetTimerManager().SetTimer(
		DashTimerHandle,
		this,
		&UGA_DashStrike::OnDashComplete,
		DashTime,
		false
	);
}

void UGA_DashStrike::UpdateDashPosition()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		OnDashComplete();
		return;
	}

	// 更新突进进度
	float DashTime = DashDistance / DashSpeed;
	DashProgress += 0.016f; // 更新间隔
	float Alpha = FMath::Clamp(DashProgress / DashTime, 0.0f, 1.0f);

	// 计算当前位置
	FVector PreviousLocation = AvatarActor->GetActorLocation();
	FVector CurrentLocation = FMath::Lerp(DashStartLocation, DashTargetLocation, Alpha);

	// 移动角色
	AvatarActor->SetActorLocation(CurrentLocation);

	// 执行碰撞检测
	PerformDashCollisionCheck(CurrentLocation, PreviousLocation);

	// 如果到达目标位置，提前结束
	if (Alpha >= 1.0f)
	{
		OnDashComplete();
	}
}

FVector UGA_DashStrike::GetSkillShotDirection(const FGameplayEventData* TriggerEventData) const
{
	if (TriggerEventData && TriggerEventData->TargetData.Num() > 0)
	{
		// Get direction from current location to target location
		const FHitResult* HitResult = TriggerEventData->TargetData.Get(0)->GetHitResult();
		if (HitResult)
		{
			FVector StartLocation = GetAvatarActorFromActorInfo()->GetActorLocation();
			FVector TargetLocation = HitResult->Location;
			
			// 计算方向但忽略距离 - 这样我们总是按照DashDistance进行突进
			FVector Direction = (TargetLocation - StartLocation).GetSafeNormal();
			Direction.Z = 0; // 保持在水平面
			
			// 记录调试信息
			float ClickDistance = FVector::Dist(StartLocation, TargetLocation);
			UE_LOG(LogTemp, Log, TEXT("DashStrike: Click distance: %f, Will dash: %f"), ClickDistance, DashDistance);
			
			return Direction;
		}
	}

	// Default to forward direction if no target data
	return GetAvatarActorFromActorInfo()->GetActorForwardVector();
}
