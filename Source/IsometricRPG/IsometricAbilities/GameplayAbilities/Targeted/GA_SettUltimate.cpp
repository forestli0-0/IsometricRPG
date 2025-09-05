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
	bCleanupDone = false;

	// 关键：关闭基类的通用蒙太奇流程，避免在该技能分阶段切换蒙太奇时触发基类的自动 End/Cancel
	bUseBaseMontageFlow = false;
	bEndAbilityWhenBaseMontageEnds = false;
	bCancelAbilityWhenBaseMontageInterrupted = false;
}

void UGA_SettUltimate::ExecuteSkill(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	// 每次激活重置阶段与清理残留状态（防止上一次异常导致卡住/无法移动）
	CurrentPhase = 0;
	bCleanupDone = false;
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(GrabTimer);
		GetWorld()->GetTimerManager().ClearTimer(JumpTimer);
		GetWorld()->GetTimerManager().ClearTimer(SlamTimer);
		GetWorld()->GetTimerManager().ClearTimer(LandingTimer);
	}
	if (ActiveAOEWarningEffect)
	{
		ActiveAOEWarningEffect->Destroy();
		ActiveAOEWarningEffect = nullptr;
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
		ActiveLandingEffect->Destroy();
		ActiveLandingEffect = nullptr;
	}
	PrimaryTarget = nullptr;

	if (CurrentTargetDataHandle.IsValid(0))
	{
		const FGameplayAbilityTargetData* TargetData = CurrentTargetDataHandle.Get(0);
		if (TargetData)
		{
			// 优先从 ActorArray 读取
			if (!TargetData->GetActors().IsEmpty())
			{
			PrimaryTarget = TargetData->GetActors()[0].Get();
		}
			// 兼容从 HitResult 读取
			if (!PrimaryTarget && TargetData->HasHitResult() && TargetData->GetHitResult())
			{
				PrimaryTarget = TargetData->GetHitResult()->GetActor();
	}
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

	// 仅服务器设置朝向，避免客户端与角色移动组件产生旋转冲突造成抖动
	FVector DirectionToTarget = PrimaryTarget->GetActorLocation() - Caster->GetActorLocation();
	DirectionToTarget.Z = 0; // 只关心水平方向
	Caster->SetActorRotation(DirectionToTarget.Rotation());

	StartLocation = Caster->GetActorLocation();
	CasterCharacter = Cast<ACharacter>(Caster);
	TargetCharacter = Cast<ACharacter>(PrimaryTarget);

	// 仅在服务器端冻结移动与接管旋转，避免客户端本地改变 Transform 造成分叉
	const bool bIsAuthority = (GetAvatarActorFromActorInfo() && GetAvatarActorFromActorInfo()->HasAuthority());
	if (CasterCharacter.IsValid() && bIsAuthority)
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

	if (TargetCharacter.IsValid() && bIsAuthority)
	{
		auto* CMC = TargetCharacter->GetCharacterMovement();
		TargetOriginalMode = CMC->MovementMode;
		CMC->SetMovementMode(EMovementMode::MOVE_None);
	}

	StartGrabPhase();
}



void UGA_SettUltimate::StartGrabPhase()
{
	// 幂等守卫：防止被重复进入
	if (CurrentPhase >= 1)
	{
		UE_LOG(LogTemp, Verbose, TEXT("UGA_SettUltimate: StartGrabPhase skipped (CurrentPhase=%d)"), CurrentPhase);
		return;
	}
	CurrentPhase = 1;

	if (!PrimaryTarget || !Caster)
		return;

	if (GrabMontage && CasterCharacter.IsValid())
	{
		const bool bIsAuthority = (GetAvatarActorFromActorInfo() && GetAvatarActorFromActorInfo()->HasAuthority());
		if (bIsAuthority)
		{
			CasterCharacter->PlayAnimMontage(GrabMontage, 1.0f);
			CasterCharacter->ForceNetUpdate();
		}
		if (AController* Ctrl = CasterCharacter->GetController())
		{
			if (Ctrl->IsLocalController())
			{
				CasterCharacter->PlayAnimMontage(GrabMontage, 1.0f);
	}
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

	// 只允许服务器驱动“抓取阶段”的目标位移与阶段推进，防止客户端本地移动与服务器状态分叉
	const bool bIsAuthority = (GetAvatarActorFromActorInfo() && GetAvatarActorFromActorInfo()->HasAuthority());
	if (bIsAuthority)
	{
	UAbilityTask_MoveActorTo* MoveTask = UAbilityTask_MoveActorTo::MoveActorTo(
		this, TEXT("GrabTarget"), PrimaryTarget, GrabLocation, GrabDuration, EMoveInterpMethod::Linear);

		if (!MoveTask)
		{
			UE_LOG(LogTemp, Warning, TEXT("UGA_SettUltimate: Failed to create Grab MoveTask on server."));
		}
		else
		{
	MoveTask->OnFinish.AddDynamic(this, &UGA_SettUltimate::StartJumpPhase);
	MoveTask->ReadyForActivation();
			UE_LOG(LogTemp, Verbose, TEXT("UGA_SettUltimate: Server started Grab MoveTask -> %s"), *GrabLocation.ToString());
}

		// 安全兜底：若任务未触发完成，按时推进到下一阶段并强制放置到抓取点
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(GrabTimer, [this, GrabLocation]()
			{
				if (!PrimaryTarget || !Caster) { return; }
				const float Dist = FVector::Dist(PrimaryTarget->GetActorLocation(), GrabLocation);
				if (Dist > 10.f)
				{
					PrimaryTarget->SetActorLocation(GrabLocation, false, nullptr, ETeleportType::TeleportPhysics);
					UE_LOG(LogTemp, Warning, TEXT("UGA_SettUltimate: Grab fallback teleported target to %s (dist=%.1f)"), *GrabLocation.ToString(), Dist);
				}
				StartJumpPhase();
			}, GrabDuration + 0.05f, false);
		}
	}
	else
	{
		// 客户端不执行位移任务，等待服务器推进到下一阶段（网络复制会同步位置）
		UE_LOG(LogTemp, Verbose, TEXT("UGA_SettUltimate: Client skipping Grab MoveTask (server-authoritative movement)."));
	}
}

void UGA_SettUltimate::StartJumpPhase()
{
	// 幂等守卫：防止被重复进入（来自抓取完成与兜底定时器的双触发）
	if (CurrentPhase >= 2)
	{
		UE_LOG(LogTemp, Verbose, TEXT("UGA_SettUltimate: StartJumpPhase skipped (CurrentPhase=%d)"), CurrentPhase);
		return;
	}
	CurrentPhase = 2;

	// 进入下一阶段前清理抓取阶段的兜底定时器
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(GrabTimer);
	}

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
	// 仅服务器设置朝向；客户端依赖复制，避免预测冲突
	FRotator SlamDirectionRotation = Direction.Rotation();
	if (GetAvatarActorFromActorInfo() && GetAvatarActorFromActorInfo()->HasAuthority())
	{
	Caster->SetActorRotation(SlamDirectionRotation);
	if (TargetCharacter.IsValid())
	{
		TargetCharacter->SetActorRotation(SlamDirectionRotation);
	}
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

	auto Rate = FMath::Max(JumpMontage ? (JumpMontage->GetPlayLength() / JumpUpDuration) : 1.f, 1.f);
	if (JumpMontage && CasterCharacter.IsValid())
	{
		const bool bIsAuthorityJM = (GetAvatarActorFromActorInfo() && GetAvatarActorFromActorInfo()->HasAuthority());
		if (bIsAuthorityJM)
		{
			CasterCharacter->PlayAnimMontage(JumpMontage, Rate);
			CasterCharacter->ForceNetUpdate();
		}
		if (AController* Ctrl = CasterCharacter->GetController())
		{
			if (Ctrl->IsLocalController())
			{
				CasterCharacter->PlayAnimMontage(JumpMontage, Rate);
			}
		}
	}

	// 让本地控制的客户端也播放跳跃蒙太奇并锁住移动，避免仍在“行走”状态导致的卡顿和错姿
	if (CasterCharacter.IsValid())
	{
		if (AController* Ctrl = CasterCharacter->GetController())
		{
			if (Ctrl->IsLocalController())
			{
				const bool bIsAuthority = (GetAvatarActorFromActorInfo() && GetAvatarActorFromActorInfo()->HasAuthority());

                if (CasterCharacter.IsValid() && bIsAuthority)
                {
                    auto* CMC = CasterCharacter->GetCharacterMovement();
                    CasterOriginalMode = CMC->MovementMode;
                    CMC->SetMovementMode(EMovementMode::MOVE_None);
                    // --- 【核心修改 3】 ---
                    // 保存原始的旋转设置，然后禁用它们，从而完全接管旋转控制权。
                    bOriginalOrientRotationToMovement = CMC->bOrientRotationToMovement;
                    bOriginalUseControllerDesiredRotation = CMC->bUseControllerDesiredRotation;
                    CMC->bOrientRotationToMovement = false;
                    CMC->bUseControllerDesiredRotation = false;
                }
                // 修正方式：ACharacter 没有 PlayMontage，正确用法是 PlayAnimMontage
                if (CasterCharacter.IsValid())
                {
				CasterCharacter->PlayAnimMontage(JumpMontage, Rate);
	}
			}
		}
	}

	PlayJumpEffects(JumpStartLocation, TargetLandingLocation);

	if (JumpSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), JumpSound, Caster->GetActorLocation());
	}

	// 注意：AOE 预警圈改为在落地阶段生成，不在起跳阶段生成

	float TotalJumpDuration = JumpUpDuration;

	// 跳跃阶段的位移同样只在服务器执行；客户端仅播蒙太奇和特效
	const bool bIsAuthority = (GetAvatarActorFromActorInfo() && GetAvatarActorFromActorInfo()->HasAuthority());
	if (bIsAuthority)
	{
	UAbilityTask_MoveActorTo* CasterJumpTask = UAbilityTask_MoveActorTo::MoveActorTo(
		this, TEXT("CasterParabolicJump"), Caster, CasterLandingLocation, TotalJumpDuration,
		EMoveInterpMethod::Parabolic, JumpArcHeight, JumpArcCurve);

	UAbilityTask_MoveActorTo* TargetJumpTask = UAbilityTask_MoveActorTo::MoveActorTo(
		this, TEXT("TargetParabolicJump"), PrimaryTarget, TargetLandingLocation, TotalJumpDuration,
		EMoveInterpMethod::Parabolic, JumpArcHeight, JumpArcCurve);

		if (CasterJumpTask)
		{
	CasterJumpTask->OnFinish.AddDynamic(this, &UGA_SettUltimate::StartLandingPhase);
	CasterJumpTask->ReadyForActivation();
		}
		else
		{
			// 如果创建失败，直接进入落地阶段，避免卡死
			UE_LOG(LogTemp, Warning, TEXT("UGA_SettUltimate: CasterJumpTask creation failed on server, forcing StartLandingPhase."));
			StartLandingPhase();
		}

		if (TargetJumpTask)
		{
	TargetJumpTask->ReadyForActivation();
}

		// 安全兜底：若跳跃任务未触发完成，按时进入落地阶段
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(JumpTimer, [this]()
			{
				StartLandingPhase();
			}, TotalJumpDuration + 0.05f, false);
		}
	}
	else
	{
		// 在本地控制的客户端上执行同样的抛物线插值（纯表现，不改变阶段，不绑定回调）
		if (CasterCharacter.IsValid())
		{
			if (AController* Ctrl = CasterCharacter->GetController())
			{
				if (Ctrl->IsLocalController())
				{
					UAbilityTask_MoveActorTo* ClientCasterJump = UAbilityTask_MoveActorTo::MoveActorTo(
						this, TEXT("ClientCasterParabolicJump"), Caster, CasterLandingLocation, TotalJumpDuration,
						EMoveInterpMethod::Parabolic, JumpArcHeight, JumpArcCurve);
					if (ClientCasterJump)
					{
						ClientCasterJump->ReadyForActivation();
					}
				}
			}
		}
		UE_LOG(LogTemp, Verbose, TEXT("UGA_SettUltimate: Client running cosmetic parabolic jump for smoothing."));
	}
}

void UGA_SettUltimate::StartLandingPhase()
{
	// 幂等守卫：防止被重复进入（来自跳跃完成与兜底定时器的双触发）
	if (CurrentPhase >= 3)
	{
		UE_LOG(LogTemp, Verbose, TEXT("UGA_SettUltimate: StartLandingPhase skipped (CurrentPhase=%d)"), CurrentPhase);
		return;
	}
	CurrentPhase = 3;

	// 清理跳跃阶段的兜底定时器
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(JumpTimer);
	}

	if (!PrimaryTarget || !Caster)
		return;

	PlayLandingEffects(LandingLocation);

	// 播放落地动画（用 AbilityTask 驱动施法者）
	if (LandingMontage && CasterCharacter.IsValid())
	{
		const bool bIsAuthorityLM = (GetAvatarActorFromActorInfo() && GetAvatarActorFromActorInfo()->HasAuthority());
		if (bIsAuthorityLM)
		{
			CasterCharacter->PlayAnimMontage(LandingMontage, 1.0f);
			CasterCharacter->ForceNetUpdate();
		}
		if (AController* Ctrl = CasterCharacter->GetController())
		{
			if (Ctrl->IsLocalController())
			{
				CasterCharacter->PlayAnimMontage(LandingMontage, 1.0f);
			}
		}
	}

	// 在落地瞬间生成 AOE 预警圈（仅服务端生成一次并复制），短暂显示后自动消失
	if (AOEWarningEffectClass)
	{
		const bool bIsAuthority = (GetAvatarActorFromActorInfo() && GetAvatarActorFromActorInfo()->HasAuthority());
		if (bIsAuthority)
		{
			ActiveAOEWarningEffect = GetWorld()->SpawnActor<ANiagaraActor>(
				AOEWarningEffectClass, LandingLocation, FRotator::ZeroRotator);
			if (ActiveAOEWarningEffect)
			{
				ActiveAOEWarningEffect->SetReplicates(true);
				ActiveAOEWarningEffect->SetLifeSpan(1.2f); // 只在落地瞬间短暂显示
				if (ActiveAOEWarningEffect->GetNiagaraComponent())
				{
					ActiveAOEWarningEffect->GetNiagaraComponent()->SetFloatParameter(FName("user_Radius"), AOERadius);
				}
			}
		}
	}

	if (LandingSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), LandingSound, LandingLocation);
	}

	if (ActiveAOEWarningEffect)
	{
		ActiveAOEWarningEffect->Destroy();
		ActiveAOEWarningEffect = nullptr;
	}

	// 仅服务端结算伤害与控制计时
	const bool bIsAuthority = (GetAvatarActorFromActorInfo() && GetAvatarActorFromActorInfo()->HasAuthority());
	if (bIsAuthority)
	{
	ApplyPrimaryTargetDamage(PrimaryTarget, Caster);
	ApplyAOEDamage(LandingLocation, Caster);
	GetWorld()->GetTimerManager().SetTimer(LandingTimer, this, &UGA_SettUltimate::FinishUltimate, LandingStunDuration, false);
}
}

void UGA_SettUltimate::FinishUltimate()
{
	// 解锁本地移动
	// 在技能结束时，恢复角色移动组件原始的旋转设置。
	// 这将旋转控制权安全地交还给常规系统。
	if (CasterCharacter.IsValid())
	{
		if (AController* Ctrl = CasterCharacter->GetController())
		{
			if (Ctrl->IsLocalController())
			{
                Ctrl->SetIgnoreMoveInput(false);
			}
		}
	}

	CleanupAndRestore(false);
	// 必须复制 EndAbility，确保客户端能力也进入 End，从而触发各类本地特效清理
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}

void UGA_SettUltimate::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
	{
	CleanupAndRestore(bWasCancelled);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_SettUltimate::CancelAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateCancelAbility)
{
	CleanupAndRestore(true);
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void UGA_SettUltimate::CleanupAndRestore(bool bWasCancelled)
{
	if (bCleanupDone)
	{
		return;
	}
	bCleanupDone = true;

	// 清理定时器
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(GrabTimer);
		GetWorld()->GetTimerManager().ClearTimer(JumpTimer);
		GetWorld()->GetTimerManager().ClearTimer(SlamTimer);
		GetWorld()->GetTimerManager().ClearTimer(LandingTimer);
	}

	// 清理特效
	if (ActiveAOEWarningEffect)
	{
		ActiveAOEWarningEffect->Destroy();
		ActiveAOEWarningEffect = nullptr;
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

	// 恢复移动/旋转（仅服务器曾修改）
	const bool bIsAuthority = (GetAvatarActorFromActorInfo() && GetAvatarActorFromActorInfo()->HasAuthority());
	if (CasterCharacter.IsValid() && bIsAuthority)
	{
		auto* CMC = CasterCharacter->GetCharacterMovement();
		if (CMC)
		{
			CMC->SetMovementMode(CasterOriginalMode);
			CMC->bOrientRotationToMovement = bOriginalOrientRotationToMovement;
			CMC->bUseControllerDesiredRotation = bOriginalUseControllerDesiredRotation;
		}
	}
	if (TargetCharacter.IsValid() && bIsAuthority)
	{
		if (auto* TMC = TargetCharacter->GetCharacterMovement())
		{
			TMC->SetMovementMode(TargetOriginalMode);
		}
	}

	// 重置阶段，允许再次释放
	CurrentPhase = 0;
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

	// 仅服务端生成一次并复制
	if (!(GetAvatarActorFromActorInfo() && GetAvatarActorFromActorInfo()->HasAuthority()))
	{
		return;
	}

	// 播放抓取特效
	if (GrabEffectClass)
	{
		ActiveGrabEffect = GetWorld()->SpawnActor<ANiagaraActor>(
			GrabEffectClass, Target->GetActorLocation(), FRotator::ZeroRotator);
		
		if (ActiveGrabEffect)
		{
			ActiveGrabEffect->SetReplicates(true);
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

	// 仅服务端生成一次并复制
	if (!(GetAvatarActorFromActorInfo() && GetAvatarActorFromActorInfo()->HasAuthority()))
	{
		return;
	}

	// 播放跳跃轨迹特效
	if (JumpTrailEffectClass)
	{
		ActiveJumpTrailEffect = GetWorld()->SpawnActor<ANiagaraActor>(
			JumpTrailEffectClass, startLocation, FRotator::ZeroRotator);
		
		if (ActiveJumpTrailEffect && ActiveJumpTrailEffect->GetNiagaraComponent())
		{
			ActiveJumpTrailEffect->SetReplicates(true);
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

	// 仅服务端生成一次并复制
	if (!(GetAvatarActorFromActorInfo() && GetAvatarActorFromActorInfo()->HasAuthority()))
	{
		return;
	}

	// 播放落地爆炸特效
	if (LandingExplosionEffectClass)
	{
		ActiveLandingEffect = GetWorld()->SpawnActor<ANiagaraActor>(
			LandingExplosionEffectClass, tLandingLocation, FRotator::ZeroRotator);
		
		if (ActiveLandingEffect && ActiveLandingEffect->GetNiagaraComponent())
		{
			ActiveLandingEffect->SetReplicates(true);
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
