#include "GA_Ahri_W_Foxfire.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "Character/IsometricRPGCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "IsometricAbilities/Projectiles/AProjectileBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
FGameplayTag GetOptionalGameplayTag(const TCHAR* TagName)
{
	return FGameplayTag::RequestGameplayTag(FName(TagName), false);
}
}

UGA_Ahri_W_Foxfire::UGA_Ahri_W_Foxfire()
{
	AbilityType = EHeroAbilityType::SelfCast;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	CooldownDuration = 9.0f;
	CostMagnitude = 30.0f;
	FoxfireDuration = 2.5f;
	CooldownByLevel = { 9.0f, 8.0f, 7.0f, 6.0f, 5.0f };

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/Blueprints/GameEffects/GE_AttackDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> SpeedEffectFinder(TEXT("/Game/Blueprints/GameEffects/GE_SpeedUp"));
	if (SpeedEffectFinder.Succeeded())
	{
		MoveSpeedEffectClass = SpeedEffectFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<AProjectileBase> ProjectileFinder(TEXT("/Game/Blueprints/Projectiles/BP_Projectile_Fireball"));
	if (ProjectileFinder.Succeeded())
	{
		SkillShotProjectileClass = ProjectileFinder.Class;
	}

	bEndAbilityWhenBaseMontageEnds = false;
	bUseBaseMontageFlow = false;
}

void UGA_Ahri_W_Foxfire::ExecuteSkill(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	ExecuteSelfCast();
}

void UGA_Ahri_W_Foxfire::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGA_Ahri_W_Foxfire* MutableThis = const_cast<UGA_Ahri_W_Foxfire*>(this);
	MutableThis->CooldownDuration = GetCooldownDurationForLevel();
	Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
}

void UGA_Ahri_W_Foxfire::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	CleanupAbilityState(bWasCancelled);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Ahri_W_Foxfire::CancelAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateCancelAbility)
{
	CleanupAbilityState(true);
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void UGA_Ahri_W_Foxfire::ExecuteSelfCast()
{
	AIsometricRPGCharacter* OwnerCharacter = Cast<AIsometricRPGCharacter>(GetAvatarActorFromActorInfo());
	UWorld* World = GetWorld();
	if (!OwnerCharacter || !World)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	CleanupAbilityState(true);
	bAbilityCleanupPerformed = false;
	RuntimeFoxfires.Reset();
	AssignedFoxfireCounts.Reset();
	HitActors.Reset();

	FoxfireStartTime = World->GetTimeSeconds();
	LastOrbitUpdateTime = FoxfireStartTime;
	bFoxfireLifecycleComplete = false;
	bMoveSpeedDecayComplete = false;

	if (ShouldSpawnPresentation() && CastSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, CastSound, OwnerCharacter->GetActorLocation());
	}

	ApplyMoveSpeedDecay();
	CreateFoxfires();
}

float UGA_Ahri_W_Foxfire::CalculateDamage() const
{
	if (!AttributeSet)
	{
		return 0.0f;
	}

	const int32 AbilityLevel = FMath::Max(1, GetAbilityLevel());
	const float AbilityPower = AttributeSet->GetAbilityPower();
	float TotalDamage = BaseMagicDamage + (DamagePerLevel * (AbilityLevel - 1));
	TotalDamage += AbilityPower * APRatio;
	return TotalDamage;
}

void UGA_Ahri_W_Foxfire::CreateFoxfires()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!AvatarActor || !World)
	{
		OnAllFoxfiresComplete();
		return;
	}

	RuntimeFoxfires.Reset();
	RuntimeFoxfires.Reserve(FoxfireCount);

	UNiagaraSystem* PresentationSystem = OrbitVFX ? OrbitVFX : FoxfireVFX;
	for (int32 Index = 0; Index < FoxfireCount; ++Index)
	{
		FFoxfireRuntimeState FoxfireState;
		FoxfireState.SlotIndex = Index;
		FoxfireState.CurrentAngle = (2.0f * PI * static_cast<float>(Index)) / FMath::Max(1, FoxfireCount);
		FoxfireState.SpawnTime = FoxfireStartTime;
		FoxfireState.AcquireEligibleTime = FoxfireStartTime + InitialAcquireDelay + (AcquireDelayPerFoxfire * Index);
		FoxfireState.FallbackClosestTime = FoxfireState.AcquireEligibleTime + FallbackClosestDelay;

		if (ShouldSpawnPresentation() && PresentationSystem && AvatarActor->GetRootComponent())
		{
			FoxfireState.NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
				PresentationSystem,
				AvatarActor->GetRootComponent(),
				NAME_None,
				GetFoxfireRelativeOffset(FoxfireState),
				FRotator::ZeroRotator,
				EAttachLocation::KeepRelativeOffset,
				false);
		}

		RuntimeFoxfires.Add(FoxfireState);
	}

	ClearFoxfireTimer();
	World->GetTimerManager().SetTimer(
		OrbitTimerHandle,
		this,
		&UGA_Ahri_W_Foxfire::TickFoxfireOrbit,
		OrbitUpdateInterval,
		true);

	TickFoxfireOrbit();
}

void UGA_Ahri_W_Foxfire::FireNextFoxfire()
{
	for (int32 Index = 0; Index < RuntimeFoxfires.Num(); ++Index)
	{
		if (!RuntimeFoxfires[Index].bLaunched && TryLaunchFoxfire(Index))
		{
			break;
		}
	}
}

void UGA_Ahri_W_Foxfire::FireFoxfire(int32 FoxfireIndex)
{
	TryLaunchFoxfire(FoxfireIndex);
}

AActor* UGA_Ahri_W_Foxfire::FindFoxfireTarget() const
{
	if (RuntimeFoxfires.Num() > 0)
	{
		return FindFoxfireTargetForState(RuntimeFoxfires[0], true);
	}

	FFoxfireRuntimeState DummyState;
	return FindFoxfireTargetForState(DummyState, true);
}

void UGA_Ahri_W_Foxfire::OnFoxfireProjectileHit(AActor* HitActor, const FHitResult& Hit)
{
	if (!HitActor)
	{
		return;
	}

	HitActors.Add(HitActor);
}

void UGA_Ahri_W_Foxfire::OnFoxfireHitTarget(AActor* HitActor, int32 FoxfireIndex)
{
	FHitResult DummyHit;
	OnFoxfireProjectileHit(HitActor, DummyHit);
}

void UGA_Ahri_W_Foxfire::OnAllFoxfiresComplete()
{
	if (bFoxfireLifecycleComplete)
	{
		return;
	}

	bFoxfireLifecycleComplete = true;
	ClearFoxfireTimer();
	ClearFoxfireVisuals();
	TryCompleteAbility();
}

void UGA_Ahri_W_Foxfire::TickFoxfireOrbit()
{
	UWorld* World = GetWorld();
	AIsometricRPGCharacter* OwnerCharacter = Cast<AIsometricRPGCharacter>(GetAvatarActorFromActorInfo());
	if (!World || !OwnerCharacter || OwnerCharacter->bIsDead)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	const float DeltaTime = FMath::Max(0.0f, CurrentTime - LastOrbitUpdateTime);
	LastOrbitUpdateTime = CurrentTime;

	for (int32 Index = 0; Index < RuntimeFoxfires.Num(); ++Index)
	{
		FFoxfireRuntimeState& FoxfireState = RuntimeFoxfires[Index];
		if (FoxfireState.bLaunched)
		{
			continue;
		}

		FoxfireState.CurrentAngle += OrbitSpeed * DeltaTime;
		UpdateFoxfireVisual(FoxfireState);

		if (CurrentTime >= FoxfireState.AcquireEligibleTime)
		{
			const bool bAllowFallbackClosest = CurrentTime >= FoxfireState.FallbackClosestTime;
			if (AActor* TargetActor = FindFoxfireTargetForState(FoxfireState, bAllowFallbackClosest))
			{
				TryLaunchFoxfire(Index, TargetActor);
			}
		}
	}

	if ((CurrentTime - FoxfireStartTime) >= FoxfireDuration)
	{
		for (FFoxfireRuntimeState& FoxfireState : RuntimeFoxfires)
		{
			if (!FoxfireState.bLaunched)
			{
				FoxfireState.bLaunched = true;
				if (FoxfireState.NiagaraComponent)
				{
					FoxfireState.NiagaraComponent->DestroyComponent();
					FoxfireState.NiagaraComponent = nullptr;
				}
			}
		}
	}

	if (!HasAnyPendingFoxfires())
	{
		OnAllFoxfiresComplete();
	}
}

void UGA_Ahri_W_Foxfire::UpdateFoxfireVisual(FFoxfireRuntimeState& FoxfireState) const
{
	if (FoxfireState.NiagaraComponent)
	{
		FoxfireState.NiagaraComponent->SetRelativeLocation(GetFoxfireRelativeOffset(FoxfireState));
	}
}

FVector UGA_Ahri_W_Foxfire::GetFoxfireRelativeOffset(const FFoxfireRuntimeState& FoxfireState) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const FVector Forward = AvatarActor ? AvatarActor->GetActorForwardVector() : FVector::ForwardVector;
	const FVector Right = AvatarActor ? AvatarActor->GetActorRightVector() : FVector::RightVector;
	const FVector HorizontalOffset = ((Forward * FMath::Cos(FoxfireState.CurrentAngle)) + (Right * FMath::Sin(FoxfireState.CurrentAngle))) * OrbitRadius;
	return HorizontalOffset + FVector(0.0f, 0.0f, OrbitHeight);
}

FVector UGA_Ahri_W_Foxfire::GetFoxfireWorldLocation(const FFoxfireRuntimeState& FoxfireState) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	return AvatarActor ? (AvatarActor->GetActorLocation() + GetFoxfireRelativeOffset(FoxfireState)) : FVector::ZeroVector;
}

FVector UGA_Ahri_W_Foxfire::GetVisibilityTraceStart() const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return FVector::ZeroVector;
	}

	FVector TraceStart = AvatarActor->GetActorLocation() + FVector(0.0f, 0.0f, VisibilityTraceHeight);
	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(AvatarActor))
	{
		if (const UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent())
		{
			TraceStart = Capsule->GetComponentLocation() + FVector(0.0f, 0.0f, (Capsule->GetScaledCapsuleHalfHeight() * 0.5f) + VisibilityTraceHeight);
		}
	}

	return TraceStart;
}

FVector UGA_Ahri_W_Foxfire::GetTargetTraceLocation(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return FVector::ZeroVector;
	}

	if (const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
	{
		if (const UCapsuleComponent* Capsule = TargetCharacter->GetCapsuleComponent())
		{
			return Capsule->GetComponentLocation();
		}
	}

	return TargetActor->GetActorLocation();
}

AActor* UGA_Ahri_W_Foxfire::FindFoxfireTargetForState(const FFoxfireRuntimeState& FoxfireState, bool bAllowFallbackClosest) const
{
	AIsometricRPGCharacter* OwnerCharacter = Cast<AIsometricRPGCharacter>(GetAvatarActorFromActorInfo());
	UWorld* World = GetWorld();
	if (!OwnerCharacter || !World)
	{
		return nullptr;
	}

	auto IsValidPriorityTarget = [this](AActor* TargetActor, bool bRequireHero) -> bool
	{
		if (!TargetActor)
		{
			return false;
		}

		if (bRequireHero && !IsHeroTarget(TargetActor))
		{
			return false;
		}

		return IsAliveTarget(TargetActor)
			&& IsEnemyActor(TargetActor)
			&& IsTargetInSearchRange(TargetActor)
			&& IsVisibleTarget(TargetActor);
	};

	if (AActor* CharmedTarget = OwnerCharacter->GetRecentCharmTarget())
	{
		if (IsValidPriorityTarget(CharmedTarget, true))
		{
			return CharmedTarget;
		}
	}

	TArray<AActor*> FoundActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

	UKismetSystemLibrary::SphereOverlapActors(
		World,
		OwnerCharacter->GetActorLocation(),
		SearchRange,
		ObjectTypes,
		nullptr,
		TArray<AActor*>({ OwnerCharacter }),
		FoundActors);

	AActor* ClosestHero = nullptr;
	AActor* ClosestAssignedHero = nullptr;
	AActor* ClosestKillableMinion = nullptr;
	AActor* ClosestEnemy = nullptr;
	AActor* ClosestAssignedEnemy = nullptr;
	float ClosestHeroDistSq = TNumericLimits<float>::Max();
	float ClosestAssignedHeroDistSq = TNumericLimits<float>::Max();
	float ClosestMinionDistSq = TNumericLimits<float>::Max();
	float ClosestEnemyDistSq = TNumericLimits<float>::Max();
	float ClosestAssignedEnemyDistSq = TNumericLimits<float>::Max();

	for (AActor* Candidate : FoundActors)
	{
		if (!Candidate
			|| !IsAliveTarget(Candidate)
			|| !IsEnemyActor(Candidate)
			|| !IsVisibleTarget(Candidate)
			|| !IsTargetInSearchRange(Candidate))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(OwnerCharacter->GetActorLocation(), Candidate->GetActorLocation());
		const int32 AssignmentCount = AssignedFoxfireCounts.FindRef(Candidate);
		if (AssignmentCount <= 0)
		{
			if (DistSq < ClosestEnemyDistSq)
			{
				ClosestEnemyDistSq = DistSq;
				ClosestEnemy = Candidate;
			}
		}
		else if (DistSq < ClosestAssignedEnemyDistSq)
		{
			ClosestAssignedEnemyDistSq = DistSq;
			ClosestAssignedEnemy = Candidate;
		}

		if (IsHeroTarget(Candidate))
		{
			if (AssignmentCount <= 0 && DistSq < ClosestHeroDistSq)
			{
				ClosestHeroDistSq = DistSq;
				ClosestHero = Candidate;
			}
			else if (AssignmentCount > 0 && DistSq < ClosestAssignedHeroDistSq)
			{
				ClosestAssignedHeroDistSq = DistSq;
				ClosestAssignedHero = Candidate;
			}
		}

		if (IsMinionTarget(Candidate))
		{
			const float CurrentHealth = GetCurrentHealthForActor(Candidate);
			if (CurrentHealth > 0.0f && CurrentHealth <= CalculateAssignedDamageForTarget(Candidate) && DistSq < ClosestMinionDistSq)
			{
				ClosestMinionDistSq = DistSq;
				ClosestKillableMinion = Candidate;
			}
		}
	}

	if (ClosestHero)
	{
		return ClosestHero;
	}

	if (ClosestKillableMinion)
	{
		return ClosestKillableMinion;
	}

	if (AActor* RecentBasicAttackTarget = OwnerCharacter->GetRecentBasicAttackTarget(RecentBasicAttackMaxAge))
	{
		if (IsValidPriorityTarget(RecentBasicAttackTarget, false))
		{
			return RecentBasicAttackTarget;
		}
	}

	if (bAllowFallbackClosest && ClosestEnemy)
	{
		return ClosestEnemy;
	}

	if (ClosestAssignedHero)
	{
		return ClosestAssignedHero;
	}

	return bAllowFallbackClosest ? ClosestAssignedEnemy : nullptr;
}

bool UGA_Ahri_W_Foxfire::TryLaunchFoxfire(int32 FoxfireIndex, AActor* ForcedTarget)
{
	if (!RuntimeFoxfires.IsValidIndex(FoxfireIndex))
	{
		return false;
	}

	FFoxfireRuntimeState& FoxfireState = RuntimeFoxfires[FoxfireIndex];
	if (FoxfireState.bLaunched)
	{
		return false;
	}

	AActor* TargetActor = ForcedTarget ? ForcedTarget : FindFoxfireTargetForState(FoxfireState, true);
	if (!TargetActor
		|| !IsAliveTarget(TargetActor)
		|| !IsEnemyActor(TargetActor)
		|| !IsVisibleTarget(TargetActor))
	{
		return false;
	}

	FoxfireState.AssignedTarget = TargetActor;
	FoxfireState.AssignedDamageAmount = CalculateAssignedDamageForTarget(TargetActor);
	FoxfireState.bLaunched = true;
	AssignedFoxfireCounts.FindOrAdd(TargetActor) += 1;

	if (FoxfireState.NiagaraComponent)
	{
		FoxfireState.NiagaraComponent->DestroyComponent();
		FoxfireState.NiagaraComponent = nullptr;
	}

	if (ShouldSpawnPresentation() && FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FireSound, GetFoxfireWorldLocation(FoxfireState));
	}

	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		if (AvatarActor->HasAuthority())
		{
			LaunchAuthoritativeProjectile(FoxfireState, TargetActor);
		}
	}

	return true;
}

void UGA_Ahri_W_Foxfire::LaunchAuthoritativeProjectile(const FFoxfireRuntimeState& FoxfireState, AActor* TargetActor)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!AvatarActor || !AvatarActor->HasAuthority() || !World || !SkillShotProjectileClass || !TargetActor)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarActor;
	SpawnParams.Instigator = Cast<APawn>(AvatarActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector StartLocation = GetFoxfireWorldLocation(FoxfireState);
	const FVector Direction = (GetTargetTraceLocation(TargetActor) - StartLocation).GetSafeNormal();

	AProjectileBase* FoxfireProjectile = World->SpawnActor<AProjectileBase>(
		SkillShotProjectileClass,
		StartLocation,
		Direction.Rotation(),
		SpawnParams);

	if (!FoxfireProjectile)
	{
		return;
	}

	FoxfireProjectile->OnProjectileHitActor.AddDynamic(this, &UGA_Ahri_W_Foxfire::OnFoxfireProjectileHit);

	FProjectileInitializationData ProjectileData;
	ProjectileData.InitialSpeed = FoxfireSpeed;
	ProjectileData.MaxSpeed = FoxfireSpeed;
	ProjectileData.MaxFlyDistance = SearchRange * 2.0f;
	ProjectileData.Lifespan = 3.0f;
	ProjectileData.DamageEffect = DamageEffectClass;
	ProjectileData.DamageAmount = FoxfireState.AssignedDamageAmount;
	ProjectileData.VisualEffect = FoxfireVFX;
	ProjectileData.ImpactEffect = HitVFX;
	ProjectileData.ImpactSound = HitSound;
	ProjectileData.bEnableHoming = true;
	ProjectileData.HomingTargetActor = TargetActor;
	ProjectileData.HomingAcceleration = HomingAcceleration;

	FoxfireProjectile->InitializeProjectile(
		this,
		ProjectileData,
		SpawnParams.Owner,
		SpawnParams.Instigator,
		GetAbilitySystemComponentFromActorInfo());
}

bool UGA_Ahri_W_Foxfire::IsEnemyActor(AActor* Actor) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!Actor || Actor == AvatarActor)
	{
		return false;
	}

	const IGenericTeamAgentInterface* SelfTeamAgent = Cast<IGenericTeamAgentInterface>(AvatarActor);
	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(Actor);
	if (SelfTeamAgent && OtherTeamAgent)
	{
		const ETeamAttitude::Type TeamAttitude = FGenericTeamId::GetAttitude(
			SelfTeamAgent->GetGenericTeamId(),
			OtherTeamAgent->GetGenericTeamId());
		if (TeamAttitude == ETeamAttitude::Hostile)
		{
			return true;
		}
	}

	if (Actor->ActorHasTag(FName("Enemy")))
	{
		return true;
	}

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor))
	{
		const FGameplayTag EnemyTag = GetOptionalGameplayTag(TEXT("Character.Type.Enemy"));
		return EnemyTag.IsValid() && TargetASC->HasMatchingGameplayTag(EnemyTag);
	}

	if (bAllowPlayerControlledHeroTargets && IsHeroTarget(Actor))
	{
		if (const APawn* TargetPawn = Cast<APawn>(Actor))
		{
			return TargetPawn->IsPlayerControlled() || TargetPawn->GetPlayerState() != nullptr;
		}
	}

	return false;
}

bool UGA_Ahri_W_Foxfire::IsVisibleTarget(AActor* TargetActor) const
{
	UWorld* World = GetWorld();
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!World || !AvatarActor || !TargetActor)
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AhriFoxfireVisibility), false, AvatarActor);
	const FVector TraceStart = GetVisibilityTraceStart();
	const FVector TraceEnd = GetTargetTraceLocation(TargetActor);

	const bool bBlockingHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	if (!bBlockingHit)
	{
		return true;
	}

	AActor* HitActor = Hit.GetActor();
	return HitActor == TargetActor
		|| (HitActor && (HitActor->IsOwnedBy(TargetActor) || TargetActor->IsOwnedBy(HitActor)));
}

bool UGA_Ahri_W_Foxfire::IsHeroTarget(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return false;
	}

	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		if (TargetPawn->GetPlayerState() || Cast<APlayerController>(TargetPawn->GetController()))
		{
			return true;
		}
	}

	if (TargetActor->ActorHasTag(FName("Hero")))
	{
		return true;
	}

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
	{
		const FGameplayTag HeroTag = GetOptionalGameplayTag(TEXT("Character.Type.Hero"));
		const FGameplayTag PlayerTag = GetOptionalGameplayTag(TEXT("Character.Type.Player"));
		return (HeroTag.IsValid() && TargetASC->HasMatchingGameplayTag(HeroTag))
			|| (PlayerTag.IsValid() && TargetASC->HasMatchingGameplayTag(PlayerTag));
	}

	return false;
}

bool UGA_Ahri_W_Foxfire::IsMinionTarget(AActor* TargetActor) const
{
	if (!TargetActor || IsHeroTarget(TargetActor))
	{
		return false;
	}

	if (TargetActor->ActorHasTag(FName("Minion")) || TargetActor->ActorHasTag(FName("EnemyMinion")))
	{
		return true;
	}

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
	{
		const FGameplayTag MinionTag = GetOptionalGameplayTag(TEXT("Character.Type.Minion"));
		if (MinionTag.IsValid() && TargetASC->HasMatchingGameplayTag(MinionTag))
		{
			return true;
		}
	}

	return Cast<APawn>(TargetActor) != nullptr;
}

bool UGA_Ahri_W_Foxfire::IsAliveTarget(AActor* TargetActor) const
{
	if (!IsValid(TargetActor) || TargetActor->IsActorBeingDestroyed())
	{
		return false;
	}

	if (const AIsometricRPGCharacter* TargetCharacter = Cast<AIsometricRPGCharacter>(TargetActor))
	{
		return !TargetCharacter->bIsDead && TargetCharacter->GetCurrentHealth() > 0.0f;
	}

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
	{
		if (TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dead"))))
		{
			return false;
		}
	}

	const float MaxHealth = GetMaxHealthForActor(TargetActor);
	return MaxHealth <= 0.0f || GetCurrentHealthForActor(TargetActor) > 0.0f;
}

bool UGA_Ahri_W_Foxfire::IsTargetInSearchRange(AActor* TargetActor) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	return AvatarActor && TargetActor
		&& FVector::DistSquared(AvatarActor->GetActorLocation(), TargetActor->GetActorLocation()) <= FMath::Square(SearchRange);
}

float UGA_Ahri_W_Foxfire::GetCooldownDurationForLevel() const
{
	const int32 LevelIndex = FMath::Max(0, GetAbilityLevel() - 1);
	return CooldownByLevel.IsValidIndex(LevelIndex) ? CooldownByLevel[LevelIndex] : CooldownDuration;
}

float UGA_Ahri_W_Foxfire::GetCurrentHealthForActor(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return 0.0f;
	}

	if (const AIsometricRPGCharacter* TargetCharacter = Cast<AIsometricRPGCharacter>(TargetActor))
	{
		return TargetCharacter->GetCurrentHealth();
	}

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
	{
		if (const UIsometricRPGAttributeSetBase* TargetAttributeSet = TargetASC->GetSet<UIsometricRPGAttributeSetBase>())
		{
			return TargetAttributeSet->GetHealth();
		}
	}

	return 0.0f;
}

float UGA_Ahri_W_Foxfire::GetMaxHealthForActor(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return 0.0f;
	}

	if (const AIsometricRPGCharacter* TargetCharacter = Cast<AIsometricRPGCharacter>(TargetActor))
	{
		return TargetCharacter->GetMaxHealth();
	}

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
	{
		if (const UIsometricRPGAttributeSetBase* TargetAttributeSet = TargetASC->GetSet<UIsometricRPGAttributeSetBase>())
		{
			return TargetAttributeSet->GetMaxHealth();
		}
	}

	return 0.0f;
}

bool UGA_Ahri_W_Foxfire::IsBelowMinionExecuteThreshold(AActor* TargetActor) const
{
	const float MaxHealth = GetMaxHealthForActor(TargetActor);
	if (MaxHealth <= 0.0f)
	{
		return false;
	}

	return (GetCurrentHealthForActor(TargetActor) / MaxHealth) <= MinionExecuteHealthThreshold;
}

float UGA_Ahri_W_Foxfire::CalculateAssignedDamageForTarget(AActor* TargetActor) const
{
	float DamageAmount = CalculateDamage();
	if (!TargetActor)
	{
		return DamageAmount;
	}

	if (AssignedFoxfireCounts.FindRef(TargetActor) > 0)
	{
		DamageAmount *= RepeatTargetDamageMultiplier;
	}

	if (IsMinionTarget(TargetActor) && IsBelowMinionExecuteThreshold(TargetActor))
	{
		DamageAmount *= MinionExecuteDamageMultiplier;
	}

	return DamageAmount;
}

void UGA_Ahri_W_Foxfire::ApplyMoveSpeedDecay()
{
	StopMoveSpeedDecay(true);

	UWorld* World = GetWorld();
	if (!World || MoveSpeedDecayDuration <= 0.0f || !MoveSpeedEffectClass)
	{
		bMoveSpeedDecayComplete = true;
		TryCompleteAbility();
		return;
	}

	MoveSpeedBuffStartTime = World->GetTimeSeconds();
	bMoveSpeedDecayComplete = false;
	ApplyMoveSpeedDecayStep();

	if (!bMoveSpeedDecayComplete)
	{
		World->GetTimerManager().SetTimer(
			MoveSpeedDecayTimerHandle,
			this,
			&UGA_Ahri_W_Foxfire::ApplyMoveSpeedDecayStep,
			MoveSpeedDecayStepInterval,
			true,
			MoveSpeedDecayStepInterval);
	}
}

void UGA_Ahri_W_Foxfire::ApplyMoveSpeedDecayStep()
{
	UWorld* World = GetWorld();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!World || !ASC || !MoveSpeedEffectClass)
	{
		bMoveSpeedDecayComplete = true;
		StopMoveSpeedDecay(true);
		TryCompleteAbility();
		return;
	}

	const float Elapsed = World->GetTimeSeconds() - MoveSpeedBuffStartTime;
	const float Remaining = MoveSpeedDecayDuration - Elapsed;
	const float Alpha = FMath::Clamp(Elapsed / FMath::Max(MoveSpeedDecayDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	const float CurrentBonusPercent = MoveSpeedBonusPercent * (1.0f - Alpha);
	const float CurrentSpeedMultiplier = 1.0f + CurrentBonusPercent;

	if (ActiveMoveSpeedHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(ActiveMoveSpeedHandle);
		ActiveMoveSpeedHandle = FActiveGameplayEffectHandle();
	}

	if (Remaining <= KINDA_SMALL_NUMBER || CurrentBonusPercent <= KINDA_SMALL_NUMBER)
	{
		bMoveSpeedDecayComplete = true;
		StopMoveSpeedDecay(false);
		TryCompleteAbility();
		return;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle EffectSpec = ASC->MakeOutgoingSpec(MoveSpeedEffectClass, GetAbilityLevel(), EffectContext);
	if (EffectSpec.IsValid())
	{
		EffectSpec.Data->SetSetByCallerMagnitude(
			FGameplayTag::RequestGameplayTag(FName("Data.Ability.Duration")),
			FMath::Min(Remaining, MoveSpeedDecayStepInterval + 0.05f));
		EffectSpec.Data->SetSetByCallerMagnitude(
			FGameplayTag::RequestGameplayTag(FName("Data.Ability.SpeedUp")),
			CurrentSpeedMultiplier);
		ActiveMoveSpeedHandle = ASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
	}
}

void UGA_Ahri_W_Foxfire::StopMoveSpeedDecay(bool bRemoveActiveEffect)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MoveSpeedDecayTimerHandle);
	}

	if (bRemoveActiveEffect)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			if (ActiveMoveSpeedHandle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(ActiveMoveSpeedHandle);
			}
		}
	}

	ActiveMoveSpeedHandle = FActiveGameplayEffectHandle();
}

void UGA_Ahri_W_Foxfire::ClearFoxfireTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OrbitTimerHandle);
	}
}

void UGA_Ahri_W_Foxfire::ClearFoxfireVisuals()
{
	for (FFoxfireRuntimeState& FoxfireState : RuntimeFoxfires)
	{
		if (FoxfireState.NiagaraComponent)
		{
			FoxfireState.NiagaraComponent->DestroyComponent();
			FoxfireState.NiagaraComponent = nullptr;
		}
	}
}

void UGA_Ahri_W_Foxfire::CleanupAbilityState(bool bWasCancelled)
{
	if (bAbilityCleanupPerformed)
	{
		return;
	}

	bAbilityCleanupPerformed = true;
	ClearFoxfireTimer();
	ClearFoxfireVisuals();

	if (bWasCancelled)
	{
		StopMoveSpeedDecay(true);
		bMoveSpeedDecayComplete = true;
	}
	else
	{
		StopMoveSpeedDecay(false);
	}

	RuntimeFoxfires.Reset();
	AssignedFoxfireCounts.Reset();
	HitActors.Reset();
}

void UGA_Ahri_W_Foxfire::TryCompleteAbility()
{
	if (!bFoxfireLifecycleComplete || !bMoveSpeedDecayComplete)
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

bool UGA_Ahri_W_Foxfire::ShouldSpawnPresentation() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	UWorld* World = GetWorld();
	return ActorInfo && ActorInfo->IsLocallyControlled() && World && World->GetNetMode() != NM_DedicatedServer;
}

bool UGA_Ahri_W_Foxfire::HasAnyPendingFoxfires() const
{
	for (const FFoxfireRuntimeState& FoxfireState : RuntimeFoxfires)
	{
		if (!FoxfireState.bLaunched)
		{
			return true;
		}
	}

	return false;
}
