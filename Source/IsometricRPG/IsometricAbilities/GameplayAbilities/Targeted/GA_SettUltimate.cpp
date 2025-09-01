#include "GA_SettUltimate.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "NiagaraActor.h"
#include "Sound/SoundBase.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "NiagaraComponent.h"
#include "Engine/OverlapResult.h"
#include "IsometricRPG\IsometricAbilities\AbilityTasks\AbilityTask_MoveActorTo.h"
#include "Math/UnrealMathUtility.h"
#include "GameFramework/CharacterMovementComponent.h"
UGA_SettUltimate::UGA_SettUltimate()
{
	AbilityType = EHeroAbilityType::Targeted;
	CooldownDuration = 80.0f;
	CostMagnitude = 100.0f;
	RangeToApply = GrabRange;

	bRequiresTargetData = true;
	// --- 【核心修改 1】 ---
	// 禁用 GameplayAbility 的自动朝向目标功能。
	// 我们将通过代码在最合适的时机手动控制朝向，以避免与其他旋转逻辑（如控制器输入）冲突。
	bFaceTarget = false;

	bCanBeInterrupted = false;
	CurrentPhase = 0;

	// 初始化新增的成员变量
	bOriginalUseControllerDesiredRotation = false;
	bOriginalOrientRotationToMovement = false;
}

void UGA_SettUltimate::ExecuteSkill(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	PrimaryTarget = nullptr;

	if (CurrentTargetDataHandle.IsValid(0))
	{
		const FGameplayAbilityTargetData* TargetData = CurrentTargetDataHandle.Get(0);
		if (TargetData && !TargetData->GetActors().IsEmpty())
		{
			PrimaryTarget = TargetData->GetActors()[0].Get();
		}
	}

	if (!PrimaryTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_SettUltimate: Invalid Primary Target. Cancelling ability."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Caster = GetAvatarActorFromActorInfo();
	if (!Caster)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// --- 【核心修改 2】 ---
	// 在技能开始时，立即手动让施法者面向目标。
	// 这确保了技能初始的朝向是正确的。
	FVector DirectionToTarget = PrimaryTarget->GetActorLocation() - Caster->GetActorLocation();
	DirectionToTarget.Z = 0; // 只关心水平方向
	Caster->SetActorRotation(DirectionToTarget.Rotation());

	StartLocation = Caster->GetActorLocation();
	CasterCharacter = Cast<ACharacter>(Caster);
	TargetCharacter = Cast<ACharacter>(PrimaryTarget);

	if (CasterCharacter.IsValid())
	{
		auto* CMC = CasterCharacter->GetCharacterMovement();
		CasterOriginalMode = CMC->MovementMode;
		CMC->SetMovementMode(EMovementMode::MOVE_None);

		// --- 【核心修改 3】 ---
		// 保存原始的旋转设置，然后禁用它们，从而完全接管旋转控制权。
		// 这是防止角色被控制器（镜头方向）或自身移动逻辑意外扭转的关键。
		bOriginalOrientRotationToMovement = CMC->bOrientRotationToMovement;
		bOriginalUseControllerDesiredRotation = CMC->bUseControllerDesiredRotation;
		CMC->bOrientRotationToMovement = false;
		CMC->bUseControllerDesiredRotation = false;
	}

	if (TargetCharacter.IsValid())
	{
		auto* CMC = TargetCharacter->GetCharacterMovement();
		TargetOriginalMode = CMC->MovementMode;
		CMC->SetMovementMode(EMovementMode::MOVE_None);
	}

	StartGrabPhase();
}



void UGA_SettUltimate::StartGrabPhase()
{
	CurrentPhase = 1;

	if (!PrimaryTarget || !Caster)
		return;

	if (GrabMontage)
	{
		CurrentMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("GrabMontage"), GrabMontage, 1.0f);
		if (CurrentMontageTask)
		{
			CurrentMontageTask->ReadyForActivation();
		}
	}

	PlayGrabEffects(PrimaryTarget);

	if (SettVoiceLineSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SettVoiceLineSound, Caster->GetActorLocation());
	}

	FVector DirectionToTarget = PrimaryTarget->GetActorLocation() - Caster->GetActorLocation();
	DirectionToTarget.Z = 0;
	if (!DirectionToTarget.Normalize())
	{
		DirectionToTarget = Caster->GetActorForwardVector();
	}

	const float GrabDistance = 100.0f;
	FVector GrabLocation = Caster->GetActorLocation() + DirectionToTarget * GrabDistance;

	UAbilityTask_MoveActorTo* MoveTask = UAbilityTask_MoveActorTo::MoveActorTo(
		this, TEXT("GrabTarget"), PrimaryTarget, GrabLocation, GrabDuration, EMoveInterpMethod::Linear);

	MoveTask->OnFinish.AddDynamic(this, &UGA_SettUltimate::StartJumpPhase);
	MoveTask->ReadyForActivation();
}

void UGA_SettUltimate::StartJumpPhase()
{
	CurrentPhase = 2;

	if (!PrimaryTarget || !Caster)
		return;

	const FVector JumpStartLocation = Caster->GetActorLocation();
	FVector Direction = PrimaryTarget->GetActorLocation() - JumpStartLocation;
	Direction.Z = 0;
	if (!Direction.Normalize())
	{
		Direction = Caster->GetActorForwardVector();
	}

	// --- 【核心修改 4】 ---
	// 在跳跃前，再次锁定施法者和目标的朝向。
	// 这确保了整个抱摔动作都朝着这个计算出的、正确的方向进行。
	FRotator SlamDirectionRotation = Direction.Rotation();
	Caster->SetActorRotation(SlamDirectionRotation);
	if (TargetCharacter.IsValid())
	{
		// 同样设置目标的朝向，使其在空中看起来是同步的
		TargetCharacter->SetActorRotation(SlamDirectionRotation);
	}

	FVector TargetHorizontalLandingPoint = JumpStartLocation + Direction * JumpDistance;
	FHitResult HitResult;
	FVector TraceStart = TargetHorizontalLandingPoint + FVector(0, 0, 1000.0f);
	FVector TraceEnd = TargetHorizontalLandingPoint - FVector(0, 0, 1000.0f);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Caster);
	QueryParams.AddIgnoredActor(PrimaryTarget);
	FVector TargetLandingLocation = TargetHorizontalLandingPoint;

	GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

	ACharacter* TargetChar = Cast<ACharacter>(PrimaryTarget);
	if (TargetChar && HitResult.bBlockingHit) // 确保射线检测成功
	{
		float CapsuleHalfHeight = TargetChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		TargetLandingLocation = HitResult.ImpactPoint + FVector(0, 0, CapsuleHalfHeight);
	}

	this->LandingLocation = TargetLandingLocation;

	const float CasterOffsetDistance = 80.0f;
	FVector CasterLandingLocation = TargetLandingLocation - Direction * CasterOffsetDistance;

	auto Rate = FMath::Max(JumpMontage->GetPlayLength() / JumpUpDuration, 1.f);
	if (JumpMontage)
	{
		CurrentMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("JumpMontage"), JumpMontage, Rate);
		if (CurrentMontageTask)
		{
			CurrentMontageTask->ReadyForActivation();
		}
	}

	PlayJumpEffects(JumpStartLocation, TargetLandingLocation);

	if (JumpSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), JumpSound, Caster->GetActorLocation());
	}

	if (AOEWarningEffectClass)
	{
		ActiveAOEWarningEffect = GetWorld()->SpawnActor<ANiagaraActor>(
			AOEWarningEffectClass, LandingLocation, FRotator::ZeroRotator);
		if (ActiveAOEWarningEffect && ActiveAOEWarningEffect->GetNiagaraComponent())
		{
			ActiveAOEWarningEffect->GetNiagaraComponent()->SetFloatParameter(FName("user_Radius"), AOERadius);
		}
	}

	float TotalJumpDuration = JumpUpDuration;

	UAbilityTask_MoveActorTo* CasterJumpTask = UAbilityTask_MoveActorTo::MoveActorTo(
		this, TEXT("CasterParabolicJump"), Caster, CasterLandingLocation, TotalJumpDuration,
		EMoveInterpMethod::Parabolic, JumpArcHeight, JumpArcCurve);

	UAbilityTask_MoveActorTo* TargetJumpTask = UAbilityTask_MoveActorTo::MoveActorTo(
		this, TEXT("TargetParabolicJump"), PrimaryTarget, TargetLandingLocation, TotalJumpDuration,
		EMoveInterpMethod::Parabolic, JumpArcHeight, JumpArcCurve);

	CasterJumpTask->OnFinish.AddDynamic(this, &UGA_SettUltimate::StartLandingPhase);
	CasterJumpTask->ReadyForActivation();
	TargetJumpTask->ReadyForActivation();
}


void UGA_SettUltimate::StartLandingPhase()
{
	CurrentPhase = 3;

	if (!PrimaryTarget || !Caster)
		return;

	PlayLandingEffects(LandingLocation);

	if (LandingSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), LandingSound, LandingLocation);
	}

	if (ActiveAOEWarningEffect)
	{
		ActiveAOEWarningEffect->Destroy();
		ActiveAOEWarningEffect = nullptr;
	}

	ApplyPrimaryTargetDamage(PrimaryTarget, Caster);
	ApplyAOEDamage(LandingLocation, Caster);

	GetWorld()->GetTimerManager().SetTimer(LandingTimer, this, &UGA_SettUltimate::FinishUltimate, LandingStunDuration, false);
}

void UGA_SettUltimate::FinishUltimate()
{
	// --- 【核心修改 5】 ---
	// 在技能结束时，恢复角色移动组件原始的旋转设置。
	// 这将旋转控制权安全地交还给常规系统。
	if (CasterCharacter.IsValid())
	{
		auto* CMC = CasterCharacter->GetCharacterMovement();
		CMC->SetMovementMode(CasterOriginalMode);

		CMC->bOrientRotationToMovement = bOriginalOrientRotationToMovement;
		CMC->bUseControllerDesiredRotation = bOriginalUseControllerDesiredRotation;
	}
	if (TargetCharacter.IsValid())
	{
		TargetCharacter->GetCharacterMovement()->SetMovementMode(TargetOriginalMode);
	}

	if (ActiveGrabEffect)
	{
		ActiveGrabEffect->Destroy();
		ActiveGrabEffect = nullptr;
	}
	if (ActiveJumpTrailEffect)
	{
		ActiveJumpTrailEffect->Destroy();
		ActiveJumpTrailEffect = nullptr;
	}
	if (ActiveLandingEffect)
	{
		ActiveLandingEffect->SetLifeSpan(2.0f);
		ActiveLandingEffect = nullptr;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

float UGA_SettUltimate::CalculatePrimaryTargetDamage(AActor* Target, AActor* tCaster) const
{
	if (!Target)
		return 0.0f;

	float TotalDamage = BaseDamage;

	// 基于目标最大生命值的额外伤害
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (TargetASC)
	{
		float MaxHealth = TargetASC->GetNumericAttribute(UIsometricRPGAttributeSetBase::GetMaxHealthAttribute());
		float MaxHealthDamage = MaxHealth * MaxHealthDamagePercent;
		TotalDamage += MaxHealthDamage;
	}

	return TotalDamage;
}

float UGA_SettUltimate::CalculateAOEDamage(AActor* Target, AActor* tCaster) const
{
	// AOE伤害是主目标伤害的一定百分比
	float PrimaryDamage = CalculatePrimaryTargetDamage(Target, tCaster);
	return PrimaryDamage * AOEDamageReduction;
}

void UGA_SettUltimate::ApplyPrimaryTargetDamage(AActor* Target, AActor* tCaster)
{
	if (!Target || !tCaster || !PrimaryDamageEffectClass)
		return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	UAbilitySystemComponent* CasterASC = GetAbilitySystemComponentFromActorInfo();
	
	if (!TargetASC || !CasterASC)
		return;

	// 创建伤害效果
	FGameplayEffectContextHandle ContextHandle = CasterASC->MakeEffectContext();
	ContextHandle.AddSourceObject(tCaster);
	
	FGameplayEffectSpecHandle SpecHandle = CasterASC->MakeOutgoingSpec(
		PrimaryDamageEffectClass, GetAbilityLevel(), ContextHandle);
	
	if (SpecHandle.IsValid())
	{
		float Damage = CalculatePrimaryTargetDamage(Target, tCaster);
		SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.Damage"), -Damage);
		
		// 应用伤害
		CasterASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}

	// 应用眩晕效果
	if (StunEffectClass)
	{
		FGameplayEffectSpecHandle StunSpecHandle = CasterASC->MakeOutgoingSpec(
			StunEffectClass, GetAbilityLevel(), ContextHandle);
		
		if (StunSpecHandle.IsValid())
		{
			StunSpecHandle.Data->SetDuration(LandingStunDuration, false);
			CasterASC->ApplyGameplayEffectSpecToTarget(*StunSpecHandle.Data.Get(), TargetASC);
		}
	}
}

void UGA_SettUltimate::ApplyAOEDamage(const FVector& CenterLocation, AActor* tCaster)
{
	if (!tCaster || !AOEDamageEffectClass)
		return;

	// 获取AOE范围内的敌人
	TArray<AActor*> EnemiesInRange = GetEnemiesInAOE(CenterLocation, tCaster);
	
	UAbilitySystemComponent* CasterASC = GetAbilitySystemComponentFromActorInfo();
	if (!CasterASC)
		return;

	for (AActor* Enemy : EnemiesInRange)
	{
		// 跳过主目标（已经受到主要伤害）
		if (Enemy == PrimaryTarget)
			continue;

		UAbilitySystemComponent* EnemyASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Enemy);
		if (!EnemyASC)
			continue;

		// 创建AOE伤害效果
		FGameplayEffectContextHandle ContextHandle = CasterASC->MakeEffectContext();
		ContextHandle.AddSourceObject(tCaster);
		
		FGameplayEffectSpecHandle SpecHandle = CasterASC->MakeOutgoingSpec(
			AOEDamageEffectClass, GetAbilityLevel(), ContextHandle);
		
		if (SpecHandle.IsValid())
		{
			float AOEDamage = CalculateAOEDamage(Enemy, tCaster);
			SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.Damage"), -AOEDamage);
			
			// 应用AOE伤害
			CasterASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), EnemyASC);
		}

		// 应用击退效果
		if (KnockbackEffectClass)
		{
			FGameplayEffectSpecHandle KnockbackSpecHandle = CasterASC->MakeOutgoingSpec(
				KnockbackEffectClass, GetAbilityLevel(), ContextHandle);
			
			if (KnockbackSpecHandle.IsValid())
			{
				CasterASC->ApplyGameplayEffectSpecToTarget(*KnockbackSpecHandle.Data.Get(), EnemyASC);
			}
		}
	}
}

TArray<AActor*> UGA_SettUltimate::GetEnemiesInAOE(const FVector& CenterLocation, AActor* tCaster) const
{
	TArray<AActor*> EnemiesInRange;
	
	if (!GetWorld())
		return EnemiesInRange;

	// 球形重叠检测
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(tCaster);
	
	bool bHit = GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		CenterLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(AOERadius),
		QueryParams
	);
	
	if (bHit)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* HitActor = Result.GetActor();

			if (HitActor && HitActor != tCaster)
			{
				// 这里应该添加敌友判断逻辑
				// 简化实现：所有非自己的Actor都视为敌人
				if (!bDamageAllies)
				{
					// TODO: 添加阵营检查逻辑
					EnemiesInRange.Add(HitActor);
				}
				else
				{
					EnemiesInRange.Add(HitActor);
				}
			}
		}
	}
	
	return EnemiesInRange;
}

void UGA_SettUltimate::PlayGrabEffects(AActor* Target)
{
	if (!Target || !GetWorld())
		return;

	// 播放抓取特效
	if (GrabEffectClass)
	{
		ActiveGrabEffect = GetWorld()->SpawnActor<ANiagaraActor>(
			GrabEffectClass, Target->GetActorLocation(), FRotator::ZeroRotator);
		
		if (ActiveGrabEffect)
		{
			ActiveGrabEffect->AttachToActor(Target, FAttachmentTransformRules::KeepWorldTransform);
		}
	}

	// 播放抓取音效
	if (GrabSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), GrabSound, Target->GetActorLocation());
	}
}

void UGA_SettUltimate::PlayJumpEffects(const FVector& startLocation, const FVector& EndLocation)
{
	if (!GetWorld())
		return;

	// 播放跳跃轨迹特效
	if (JumpTrailEffectClass)
	{
		ActiveJumpTrailEffect = GetWorld()->SpawnActor<ANiagaraActor>(
			JumpTrailEffectClass, startLocation, FRotator::ZeroRotator);
		
		if (ActiveJumpTrailEffect && ActiveJumpTrailEffect->GetNiagaraComponent())
		{
			// 设置轨迹参数
			ActiveJumpTrailEffect->GetNiagaraComponent()->SetVectorParameter(FName("StartLocation"), startLocation);
			ActiveJumpTrailEffect->GetNiagaraComponent()->SetVectorParameter(FName("EndLocation"), EndLocation);
		}
	}
}

void UGA_SettUltimate::PlayLandingEffects(const FVector& tLandingLocation)
{
	if (!GetWorld())
		return;

	// 播放落地爆炸特效
	if (LandingExplosionEffectClass)
	{
		ActiveLandingEffect = GetWorld()->SpawnActor<ANiagaraActor>(
			LandingExplosionEffectClass, tLandingLocation, FRotator::ZeroRotator);
		
		if (ActiveLandingEffect && ActiveLandingEffect->GetNiagaraComponent())
		{
			ActiveLandingEffect->GetNiagaraComponent()->SetFloatParameter(FName("ExplosionRadius"), AOERadius);
		}
	}
}

bool UGA_SettUltimate::IsValidHeroTarget(AActor* Target, AActor* tCaster) const
{
	if (!Target || Target == tCaster)
		return false;

	// 检查目标是否为Character类型（英雄单位）
    auto tTargetCharacter = Cast<ACharacter>(Target);
	if (!tTargetCharacter)
		return false;

	// 检查目标是否有AbilitySystemComponent
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
		return false;

	// TODO: 添加更多验证逻辑，如阵营检查、状态检查等

	return true;
}
