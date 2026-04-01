#include "GA_Ahri_R_SpiritRush.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Character/IsometricRPGCharacter.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayEffect.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/Engine.h"
#include "GameFramework/RootMotionSource.h"
#include "UObject/ConstructorHelpers.h"

UGA_Ahri_R_SpiritRush::UGA_Ahri_R_SpiritRush()
{
	// 设置技能基础属性
	AbilityType = EHeroAbilityType::SelfCast;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	CooldownDuration = 100.0f;
	CostMagnitude = 100.0f;
	InputPolicy.InputMode = EAbilityInputMode::Instant;
	InputPolicy.bUpdateTargetWhileHeld = false;
	InputPolicy.bAllowInputBuffer = true;
	InputPolicy.MaxBufferWindow = 0.25f;
	
	// 初始化大招参数
	MaxCharges = 3;
	DashDistance = 450.0f;
	DashSpeed = 2200.0f;
	SpiritOrbCount = 3;
	SpiritOrbRange = 550.0f;
	SpiritOrbSpeed = 1600.0f;
	
	// 初始化伤害参数
	BaseMagicDamage = 60.0f;
	DamagePerLevel = 45.0f;
	APRatio = 0.35f;
	
	// 设置充能窗口和伤害递减
	ChargeWindowDuration = 10.0f;
	DamageReductionPerHit = 10.0f;

	static ConstructorHelpers::FClassFinder<AProjectileBase> ProjectileFinder(TEXT("/Game/Blueprints/Projectiles/BP_Projectile_Fireball"));
	if (ProjectileFinder.Succeeded())
	{
		SkillShotProjectileClass = ProjectileFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/Blueprints/GameEffects/GE_AttackDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		SpiritOrbDamageEffectClass = DamageEffectFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> CostEffectFinder(TEXT("/Game/Blueprints/GameEffects/GE_ManaCost"));
	if (CostEffectFinder.Succeeded())
	{
		CostGameplayEffectClass = CostEffectFinder.Class;
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> CastMontageFinder(TEXT("/Game/Characters/Animations/AM_CastFireball"));
	if (CastMontageFinder.Succeeded())
	{
		MontageToPlay = CastMontageFinder.Object;
	}
	
	// 重置状态
	CurrentCharges = 0;
	bInChargeWindow = false;
	bDashCompletionHandled = false;
	DashTask = nullptr;
}

void UGA_Ahri_R_SpiritRush::ExecuteSkill(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	UE_LOG(
		LogTemp,
		Display,
		TEXT("[AhriR] ExecuteSkill entered: Montage=%s AimDirection=%s"),
		*GetNameSafe(MontageToPlay),
		*GetCurrentAimDirection().ToCompactString());
	ExecuteSelfCast();
}

void UGA_Ahri_R_SpiritRush::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	UE_LOG(
		LogTemp,
		Display,
		TEXT("[AhriR] ActivateAbility: Charges=%d/%d InChargeWindow=%s Local=%s Authority=%s Avatar=%s"),
		CurrentCharges,
		MaxCharges,
		bInChargeWindow ? TEXT("true") : TEXT("false"),
		(ActorInfo && ActorInfo->IsLocallyControlled()) ? TEXT("true") : TEXT("false"),
		(ActorInfo && ActorInfo->IsNetAuthority()) ? TEXT("true") : TEXT("false"),
		*GetNameSafe(GetAvatarActorFromActorInfo()));

	// 检查是否可以再次使用
	if (bInChargeWindow && CurrentCharges >= MaxCharges)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AhriR] ActivateAbility aborted: all charges consumed inside charge window."));
		// 已经用完所有次数
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 调用父类的激活逻辑
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_Ahri_R_SpiritRush::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (bInChargeWindow && CurrentCharges >= MaxCharges)
	{
		UE_LOG(LogTemp, Display, TEXT("[AhriR] EndAbility cleanup: reached max charges, resetting Spirit Rush state."));
		ResetAbilityState();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Ahri_R_SpiritRush::CancelAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateCancelAbility)
{
	if (bInChargeWindow && CurrentCharges >= MaxCharges)
	{
		UE_LOG(LogTemp, Display, TEXT("[AhriR] CancelAbility cleanup: reached max charges, resetting Spirit Rush state."));
		ResetAbilityState();
	}

	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void UGA_Ahri_R_SpiritRush::ExecuteSelfCast()
{
	UE_LOG(
		LogTemp,
		Display,
		TEXT("[AhriR] ExecuteSelfCast: AimDirection=%s CurrentCharges(before)=%d InChargeWindow=%s"),
		*GetCurrentAimDirection().ToCompactString(),
		CurrentCharges,
		bInChargeWindow ? TEXT("true") : TEXT("false"));

	// 播放施法音效
	if (CastSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), CastSound, GetAvatarActorFromActorInfo()->GetActorLocation());
	}

	// 如果不在充能窗口内，开始新的充能周期
	if (!bInChargeWindow)
	{
		bInChargeWindow = true;
		CurrentCharges = 0;
		EnemyHitCounts.Empty();
		
		// 设置充能窗口定时器
		GetWorld()->GetTimerManager().SetTimer(
			ChargeWindowTimerHandle,
			this,
			&UGA_Ahri_R_SpiritRush::OnChargeWindowExpired,
			ChargeWindowDuration,
			false
		);
	}

	// 增加使用次数
	CurrentCharges++;
	UE_LOG(LogTemp, Display, TEXT("[AhriR] Charge consumed: CurrentCharges=%d/%d"), CurrentCharges, MaxCharges);

	// 执行瞬移
	ExecuteDash();

	UE_LOG(LogTemp, Display, TEXT("[AhriR] ExecuteSelfCast completed for charge %d."), CurrentCharges);
}

float UGA_Ahri_R_SpiritRush::CalculateDamage(int32 HitCount) const
{
	if (!AttributeSet)
	{
		return 0.0f;
	}

	const int32 AbilityLevel = FMath::Max(1, GetAbilityLevel());
	const float AbilityPower = AttributeSet->GetAbilityPower();

	// 计算基础伤害
	float TotalDamage = BaseMagicDamage + (DamagePerLevel * (AbilityLevel - 1));
	
	// 添加法术强度加成
	TotalDamage += AbilityPower * APRatio;

	// 应用伤害递减（如果同一个敌人被多次命中）
	if (HitCount > 0)
	{
		float DamageReduction = FMath::Min(HitCount * DamageReductionPerHit, 80.0f); // 最多减少80%
		TotalDamage *= (1.0f - DamageReduction / 100.0f);
	}

	return TotalDamage;
}

void UGA_Ahri_R_SpiritRush::ExecuteDash()
{
	AActor* AhriActor = GetAvatarActorFromActorInfo();
	ACharacter* AhriCharacter = Cast<ACharacter>(AhriActor);
	if (!AhriCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AhriR] ExecuteDash aborted: AvatarCharacter is invalid."));
		return;
	}

	if (DashTask)
	{
		bDashCompletionHandled = true;
		DashTask->EndTask();
		DashTask = nullptr;
	}

	// 获取冲刺目标位置
	const FVector TargetLocation = GetDashTargetLocation();
	const FVector CurrentLocation = AhriCharacter->GetActorLocation();
	const float ActualDashDistance = FVector::Dist2D(CurrentLocation, TargetLocation);
	const float DashDuration = ActualDashDistance / FMath::Max(DashSpeed, 1.0f);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("[AhriR] ExecuteDash: CurrentLocation=%s TargetLocation=%s DashDistance=%.1f Duration=%.3f"),
		*CurrentLocation.ToCompactString(),
		*TargetLocation.ToCompactString(),
		ActualDashDistance,
		DashDuration);

	// 播放冲刺开始特效
	if (DashStartVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			DashStartVFX,
			CurrentLocation,
			FRotator::ZeroRotator
		);
	}

	// 播放瞬移音效
	if (DashSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DashSound, CurrentLocation);
	}

	if (AIsometricRPGCharacter* RPGCharacter = Cast<AIsometricRPGCharacter>(AhriCharacter))
	{
		if (RPGCharacter->PathFollowingComponent)
		{
			RPGCharacter->PathFollowingComponent->StopMove();
		}
	}

	if (UCharacterMovementComponent* MovementComponent = AhriCharacter->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	if (AController* Controller = AhriCharacter->GetController())
	{
		Controller->StopMovement();
	}

	if (DashDuration <= KINDA_SMALL_NUMBER)
	{
		HandleDashMovementFinished();
		return;
	}

	bDashCompletionHandled = false;
	DashTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this,
		FName(TEXT("AhriSpiritRushMove")),
		TargetLocation,
		DashDuration,
		true,
		MOVE_Flying,
		true,
		nullptr,
		ERootMotionFinishVelocityMode::SetVelocity,
		FVector::ZeroVector,
		0.0f);

	if (!ensureAlwaysMsgf(DashTask != nullptr, TEXT("%s: SpiritRush failed to create root-motion dash task."), *GetName()))
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	DashTask->OnTimedOut.AddDynamic(this, &UGA_Ahri_R_SpiritRush::HandleDashMovementFinished);
	DashTask->OnTimedOutAndDestinationReached.AddDynamic(this, &UGA_Ahri_R_SpiritRush::HandleDashMovementFinished);
	DashTask->ReadyForActivation();
}

FVector UGA_Ahri_R_SpiritRush::GetDashTargetLocation() const
{
	AActor* AhriActor = GetAvatarActorFromActorInfo();
	if (!AhriActor)
	{
		return FVector::ZeroVector;
	}

	FVector CurrentLocation = AhriActor->GetActorLocation();
	const FVector AimDirection = GetCurrentAimDirection();
	UE_LOG(LogTemp, Display, TEXT("[AhriR] GetDashTargetLocation: AimDirection=%s"), *AimDirection.ToCompactString());
	if (!ensureAlwaysMsgf(
		!AimDirection.IsNearlyZero(),
		TEXT("%s: SpiritRush requires a valid AimDirection in the pending activation context."),
		*GetName()))
	{
		return CurrentLocation;
	}

	const FVector DashDirection = AimDirection.GetSafeNormal2D();
	if (!ensureAlwaysMsgf(
		!DashDirection.IsNearlyZero(),
		TEXT("%s: SpiritRush resolved an invalid dash direction from the current input context."),
		*GetName()))
	{
		return CurrentLocation;
	}

	FVector TargetLocation = CurrentLocation + (DashDirection * DashDistance);

	// 检查目标位置是否有效（没有碰撞）
	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AhriSpiritRushDashTrace), false, AhriActor);
	QueryParams.AddIgnoredActor(AhriActor);
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		CurrentLocation,
		TargetLocation,
		ECC_WorldStatic,
		QueryParams
	);

	if (bHit)
	{
		// 如果有碰撞，调整到碰撞点前的安全位置
		TargetLocation = HitResult.Location - (DashDirection * 50.0f);
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[AhriR] Dash path blocked by %s at %s, adjusted target to %s"),
			*GetNameSafe(HitResult.GetActor()),
			*HitResult.Location.ToCompactString(),
			*TargetLocation.ToCompactString());
	}

	return TargetLocation;
}

void UGA_Ahri_R_SpiritRush::FireSpiritOrbs()
{
	// 寻找狐灵火球的目标
	TArray<AActor*> Targets = FindSpiritOrbTargets();
	UE_LOG(LogTemp, Display, TEXT("[AhriR] FireSpiritOrbs: FoundTargets=%d"), Targets.Num());
	
	if (Targets.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AhriR] FireSpiritOrbs: no valid targets found."));
		
		// 如果已经用完所有次数，结束技能
		if (CurrentCharges >= MaxCharges)
		{
			OnChargeWindowExpired();
		}
		return;
	}

	TArray<AActor*> AssignedTargets;
	AssignedTargets.Reserve(SpiritOrbCount);
	for (int32 OrbIndex = 0; OrbIndex < SpiritOrbCount; ++OrbIndex)
	{
		AActor* AssignedTarget = Targets[OrbIndex % Targets.Num()];
		AssignedTargets.Add(AssignedTarget);
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[AhriR] FireSpiritOrbs: Orb %d assigned target %s"),
			OrbIndex,
			*GetNameSafe(AssignedTarget));
	}

	// 播放狐灵火球发射音效
	if (SpiritOrbFireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SpiritOrbFireSound, GetAvatarActorFromActorInfo()->GetActorLocation());
	}

	// 发射狐灵火球
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!(AvatarActor && AvatarActor->HasAuthority()))
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[AhriR] FireSpiritOrbs skipped spawn on non-authority. Avatar=%s Authority=%s"),
			*GetNameSafe(AvatarActor),
			(AvatarActor && AvatarActor->HasAuthority()) ? TEXT("true") : TEXT("false"));
		return;
	}

	FVector AhriLocation = AvatarActor->GetActorLocation();
	TMap<TWeakObjectPtr<AActor>, int32> AssignedSpiritOrbHitCounts;
	const ACharacter* AvatarCharacter = Cast<ACharacter>(AvatarActor);
	const float CapsuleRadius = (AvatarCharacter && AvatarCharacter->GetCapsuleComponent())
		? AvatarCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius()
		: 42.0f;
	const float SpawnForwardOffset = CapsuleRadius + 24.0f;
	const float SpawnHeightOffset = 50.0f;
	const float FanAngleStepDegrees = 12.0f;
	const float FanCenterOffset = (static_cast<float>(SpiritOrbCount) - 1.0f) * 0.5f;

	for (int32 i = 0; i < AssignedTargets.Num(); i++)
	{
		AActor* Target = AssignedTargets[i];
		if (!Target)
		{
			continue;
		}

		// 创建狐灵火球投射物
		if (SkillShotProjectileClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = AvatarActor;
			SpawnParams.Instigator = Cast<APawn>(AvatarActor);
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			FVector Direction = (Target->GetActorLocation() - AhriLocation).GetSafeNormal();
			FVector Direction2D = Direction.GetSafeNormal2D();
			if (Direction2D.IsNearlyZero())
			{
				Direction2D = AvatarActor->GetActorForwardVector().GetSafeNormal2D();
			}
			if (Direction2D.IsNearlyZero())
			{
				Direction2D = FVector::ForwardVector;
			}

			const float FanAngle = (static_cast<float>(i) - FanCenterOffset) * FanAngleStepDegrees;
			const FVector SpawnDirection2D = FRotator(0.0f, FanAngle, 0.0f).RotateVector(Direction2D).GetSafeNormal();
			const FVector StartLocation = AhriLocation + (SpawnDirection2D * SpawnForwardOffset) + FVector(0.0f, 0.0f, SpawnHeightOffset);
			const FVector HomingDirection = (Target->GetActorLocation() - StartLocation).GetSafeNormal();
			FVector LaunchDirection = (Target->GetActorLocation() - StartLocation).GetSafeNormal2D();
			if (LaunchDirection.IsNearlyZero())
			{
				LaunchDirection = SpawnDirection2D;
			}
			if (LaunchDirection.IsNearlyZero())
			{
				LaunchDirection = AvatarActor->GetActorForwardVector().GetSafeNormal2D();
			}
			if (LaunchDirection.IsNearlyZero())
			{
				LaunchDirection = FVector::ForwardVector;
			}

			const FRotator SpawnRotation = LaunchDirection.Rotation();
			UE_LOG(
				LogTemp,
				Display,
				TEXT("[AhriR] Try spawn spirit orb %d Class=%s World=%s Owner=%s Target=%s Start=%s LaunchDir=%s HomingDir=%s Rot=%s"),
				i,
				*GetPathNameSafe(SkillShotProjectileClass.Get()),
				*GetNameSafe(GetWorld()),
				*GetNameSafe(AvatarActor),
				*GetNameSafe(Target),
				*StartLocation.ToCompactString(),
				*LaunchDirection.ToCompactString(),
				*HomingDirection.ToCompactString(),
				*SpawnRotation.ToCompactString());

			AProjectileBase* SpiritOrb = GetWorld()->SpawnActor<AProjectileBase>(
				SkillShotProjectileClass,
				StartLocation,
				SpawnRotation,
				SpawnParams
			);

			if (SpiritOrb)
			{
				SpiritOrb->OnProjectileHitActor.AddDynamic(this, &UGA_Ahri_R_SpiritRush::OnSpiritOrbProjectileHit);
				ActiveSpiritOrbs.Add(SpiritOrb);

				FProjectileInitializationData ProjectileData;
				ProjectileData.InitialSpeed = SpiritOrbSpeed;
				ProjectileData.MaxSpeed = SpiritOrbSpeed;
				ProjectileData.MaxFlyDistance = SpiritOrbRange * 2.0f;
				ProjectileData.Lifespan = 3.0f;
				ProjectileData.DamageEffect = SpiritOrbDamageEffectClass;
				const int32 AssignedHitCount = AssignedSpiritOrbHitCounts.FindRef(Target);
				ProjectileData.DamageAmount = CalculateDamage(EnemyHitCounts.FindRef(Target) + AssignedHitCount);
				ProjectileData.VisualEffect = SpiritOrbVFX;
				ProjectileData.ImpactEffect = SpiritOrbHitVFX;
				ProjectileData.ImpactSound = SpiritOrbHitSound;
				ProjectileData.bEnableHoming = true;
				ProjectileData.HomingTargetActor = Target;
				ProjectileData.HomingAcceleration = SpiritOrbHomingAcceleration;
				AssignedSpiritOrbHitCounts.FindOrAdd(Target) = AssignedHitCount + 1;

				SpiritOrb->InitializeProjectile(
					this,
					ProjectileData,
					SpawnParams.Owner,
					SpawnParams.Instigator,
					GetAbilitySystemComponentFromActorInfo());

				UE_LOG(
					LogTemp,
					Display,
					TEXT("[AhriR] Spawned spirit orb %d targeting %s from %s Damage=%.2f"),
					i,
					*GetNameSafe(Target),
					*StartLocation.ToCompactString(),
					ProjectileData.DamageAmount);
			}
			else
			{
				UE_LOG(
					LogTemp,
					Error,
					TEXT("[AhriR] Failed to spawn spirit orb %d for target %s. Class=%s World=%s Owner=%s Instigator=%s Start=%s Rot=%s"),
					i,
					*GetNameSafe(Target),
					*GetPathNameSafe(SkillShotProjectileClass.Get()),
					*GetNameSafe(GetWorld()),
					*GetNameSafe(SpawnParams.Owner),
					*GetNameSafe(SpawnParams.Instigator),
					*StartLocation.ToCompactString(),
					*SpawnRotation.ToCompactString());
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[AhriR] SkillShotProjectileClass is null while firing spirit orb %d."), i);
		}
	}

	// 如果已经用完所有次数，准备结束技能
	if (CurrentCharges >= MaxCharges)
	{
		// 延迟一点时间等待所有狐灵火球完成
		FTimerHandle DelayEndTimer;
		GetWorld()->GetTimerManager().SetTimer(
			DelayEndTimer,
			this,
			&UGA_Ahri_R_SpiritRush::OnChargeWindowExpired,
			2.0f,  // 等待2秒
			false
		);
	}
}

TArray<AActor*> UGA_Ahri_R_SpiritRush::FindSpiritOrbTargets() const
{
	TArray<AActor*> ValidTargets;
	AActor* AhriActor = GetAvatarActorFromActorInfo();
	
	if (!AhriActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AhriR] FindSpiritOrbTargets aborted: AvatarActor is null."));
		return ValidTargets;
	}

	// TODO: 优先选择阿狸当前攻击的目标
	// AActor* CurrentTarget = AhriActor->GetCurrentTarget();

	// 寻找范围内的所有敌人
	FVector AhriLocation = AhriActor->GetActorLocation();
	TArray<AActor*> FoundActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		AhriLocation,
		SpiritOrbRange,
		ObjectTypes,
		ACharacter::StaticClass(),
		TArray<AActor*>{AhriActor},
		FoundActors
	);
	UE_LOG(LogTemp, Display, TEXT("[AhriR] FindSpiritOrbTargets: RawOverlapCount=%d"), FoundActors.Num());

	const IGenericTeamAgentInterface* SelfTeamAgent = Cast<IGenericTeamAgentInterface>(AhriActor);
	FoundActors.RemoveAll([AhriActor, SelfTeamAgent](AActor* Candidate)
	{
		if (!Candidate || Candidate == AhriActor)
		{
			return true;
		}

		if (const AIsometricRPGCharacter* Character = Cast<AIsometricRPGCharacter>(Candidate))
		{
			if (Character->bIsDead)
			{
				return true;
			}
		}

		const IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(Candidate);
		if (SelfTeamAgent && OtherTeamAgent)
		{
			return FGenericTeamId::GetAttitude(SelfTeamAgent->GetGenericTeamId(), OtherTeamAgent->GetGenericTeamId()) != ETeamAttitude::Hostile;
		}

		return false;
	});
	UE_LOG(LogTemp, Display, TEXT("[AhriR] FindSpiritOrbTargets: HostileCandidates=%d"), FoundActors.Num());

	AActor* RecentCharmTarget = nullptr;
	AActor* RecentBasicAttackTarget = nullptr;
	if (const AIsometricRPGCharacter* AhriCharacter = Cast<AIsometricRPGCharacter>(AhriActor))
	{
		RecentCharmTarget = AhriCharacter->GetRecentCharmTarget();
		RecentBasicAttackTarget = AhriCharacter->GetRecentBasicAttackTarget(RecentBasicAttackMaxAge);
	}

	// 优先最近魅惑目标，其次最近普攻目标，最后按距离排序。
	FoundActors.Sort([AhriLocation, RecentCharmTarget, RecentBasicAttackTarget](const AActor& A, const AActor& B) {
		const int32 PriorityA = (&A == RecentCharmTarget) ? 0 : ((&A == RecentBasicAttackTarget) ? 1 : 2);
		const int32 PriorityB = (&B == RecentCharmTarget) ? 0 : ((&B == RecentBasicAttackTarget) ? 1 : 2);
		if (PriorityA != PriorityB)
		{
			return PriorityA < PriorityB;
		}

		float DistA = FVector::Dist(AhriLocation, A.GetActorLocation());
		float DistB = FVector::Dist(AhriLocation, B.GetActorLocation());
		return DistA < DistB;
	});

	ValidTargets = MoveTemp(FoundActors);

	for (AActor* Target : ValidTargets)
	{
		UE_LOG(LogTemp, Display, TEXT("[AhriR] SelectedTarget=%s"), *GetNameSafe(Target));
	}

	return ValidTargets;
}

void UGA_Ahri_R_SpiritRush::OnSpiritOrbProjectileHit(AActor* HitActor, const FHitResult& Hit)
{
	UE_LOG(
		LogTemp,
		Display,
		TEXT("[AhriR] Spirit orb hit callback: HitActor=%s Location=%s"),
		*GetNameSafe(HitActor),
		Hit.bBlockingHit ? *Hit.ImpactPoint.ToCompactString() : TEXT("None"));
	OnSpiritOrbHitTarget(HitActor, INDEX_NONE);
}

void UGA_Ahri_R_SpiritRush::HandleDashMovementFinished()
{
	if (bDashCompletionHandled)
	{
		return;
	}

	bDashCompletionHandled = true;
	DashTask = nullptr;

	AActor* AhriActor = GetAvatarActorFromActorInfo();
	if (!AhriActor)
	{
		return;
	}

	if (DashEndVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			DashEndVFX,
			AhriActor->GetActorLocation(),
			FRotator::ZeroRotator
		);
	}

	UE_LOG(LogTemp, Display, TEXT("[AhriR] Dash finished at %s"), *AhriActor->GetActorLocation().ToCompactString());
	FireSpiritOrbs();
}

void UGA_Ahri_R_SpiritRush::OnSpiritOrbHitTarget(AActor* HitActor, int32 OrbIndex)
{
	if (!HitActor)
	{
		return;
	}

	// 获取该敌人之前被命中的次数
	int32 HitCount = EnemyHitCounts.FindRef(HitActor);
	
	// 计算伤害（考虑伤害递减）
	float Damage = CalculateDamage(HitCount);

	// 更新命中次数
	EnemyHitCounts.Add(HitActor, HitCount + 1);

	// 播放命中特效
	if (SpiritOrbHitVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			SpiritOrbHitVFX,
			HitActor->GetActorLocation(),
			FRotator::ZeroRotator
		);
	}

	// 播放命中音效
	if (SpiritOrbHitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SpiritOrbHitSound, HitActor->GetActorLocation());
	}

	UE_LOG(LogTemp, Log, TEXT("阿狸大招狐灵火球 %d 命中目标: %s, 伤害: %.2f (第%d次命中)"), 
		OrbIndex, *HitActor->GetName(), Damage, HitCount + 1);
}

bool UGA_Ahri_R_SpiritRush::CanUseAgain() const
{
	return bInChargeWindow && CurrentCharges < MaxCharges;
}

void UGA_Ahri_R_SpiritRush::ResetAbilityState()
{
	bInChargeWindow = false;
	CurrentCharges = 0;
	EnemyHitCounts.Empty();
	ActiveSpiritOrbs.Empty();
	
	// 清理定时器
	GetWorld()->GetTimerManager().ClearTimer(ChargeWindowTimerHandle);
	
	// 清理瞬移任务
	if (DashTask)
	{
		bDashCompletionHandled = true;
		DashTask->EndTask();
		DashTask = nullptr;
	}

	bDashCompletionHandled = false;
}

void UGA_Ahri_R_SpiritRush::OnChargeWindowExpired()
{
	UE_LOG(LogTemp, Display, TEXT("[AhriR] Charge window expired. TotalChargesUsed=%d"), CurrentCharges);

	// 重置状态
	ResetAbilityState();

	// 结束技能
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
} 
