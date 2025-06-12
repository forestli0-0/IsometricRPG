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
	// 设置技能类型为目标指向
	AbilityType = EHeroAbilityType::Targeted;
	
	// 设置基础属性
	CooldownDuration = 80.0f; // 大招通常有很长的冷却时间
	CostMagnitude = 100.0f;   // 消耗大量法力值
	RangeToApply = GrabRange; // 使用抓取范围
	
	// 技能设置
	bRequiresTargetData = true;
	bFaceTarget = true;
	bCanBeInterrupted = false; // 大招通常不能被打断
	
	// 初始化阶段
	CurrentPhase = 0;
}

void UGA_SettUltimate::ExecuteSkill(const FGameplayAbilitySpecHandle Handle, 
	const FGameplayAbilityActorInfo* ActorInfo, 
	const FGameplayAbilityActivationInfo ActivationInfo, 
	const FGameplayEventData* TriggerEventData)
{
	// 先将主目标Actor指针清空
	PrimaryTarget = nullptr;

	// --- 正确的流程开始 ---

	// 1. 安全检查：确保TargetDataHandle中至少有一个有效的数据
	if (CurrentTargetDataHandle.IsValid(0))
	{
		const FGameplayAbilityTargetData* TargetData = CurrentTargetDataHandle.Get(0);
		if (TargetData)
		{
			if (!TargetData->GetActors().IsEmpty())
			{
				PrimaryTarget = TargetData->GetActors()[0].Get();
			}
		}

	}

	// 检查是否成功获取到了目标 Actor
	if (!PrimaryTarget)
	{
		// 如果没有有效目标，那么取消技能
		UE_LOG(LogTemp, Warning, TEXT("UGA_SettUltimate: Invalid Primary Target. Cancelling ability."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 现在 PrimaryTargetActor 是一个有效的 AActor* 指针，可以安全使用
	Caster = GetAvatarActorFromActorInfo();
	if (!Caster)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartLocation = Caster->GetActorLocation();
	CasterCharacter = Cast<ACharacter>(Caster);
	TargetCharacter = Cast<ACharacter>(PrimaryTarget);

	if (CasterCharacter.IsValid())
	{
		auto* CMC = CasterCharacter->GetCharacterMovement();
		CasterOriginalMode = CMC->MovementMode;
		CMC->SetMovementMode(EMovementMode::MOVE_None);
	}

	if (TargetCharacter.IsValid())
	{
		auto* CMC = TargetCharacter->GetCharacterMovement();
		TargetOriginalMode = CMC->MovementMode;
		CMC->SetMovementMode(EMovementMode::MOVE_None);
	}
	// 开始第一阶段：抓取
	StartGrabPhase();
}



void UGA_SettUltimate::StartGrabPhase()
{
	CurrentPhase = 1;

	if (!PrimaryTarget || !Caster)
		return;

	// 播放抓取动画
	if (GrabMontage)
	{
		CurrentMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("GrabMontage"), GrabMontage, 1.0f);

		if (CurrentMontageTask)
		{
			CurrentMontageTask->ReadyForActivation();
		}
	}

	// 播放抓取特效和音效
	PlayGrabEffects(PrimaryTarget);

	// 播放腕豪台词
	if (SettVoiceLineSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SettVoiceLineSound, Caster->GetActorLocation());
	}

	// --- 核心改动：不再相信Caster的ForwardVector，而是实时计算方向 ---
	// 这样可以彻底避免因角色转身延迟而导致的方向错误问题。
	FVector DirectionToTarget = PrimaryTarget->GetActorLocation() - Caster->GetActorLocation();
	DirectionToTarget.Z = 0; // 忽略高度差

	// 这里必须先对向量进行归一化，然后再在计算中使用它
	// 这是之前代码的一个BUG点，FVector::Normalize()返回的是bool
	if (!DirectionToTarget.Normalize())
	{
		// 如果归一化失败（例如两个角色位置完全重叠），则使用ForwardVector作为备用
		DirectionToTarget = Caster->GetActorForwardVector();
	}

	const float GrabDistance = 100.0f; // 将敌人拉到面前1米处
	FVector GrabLocation = Caster->GetActorLocation() + DirectionToTarget * GrabDistance;

	// 调试信息
	DrawDebugSphere(GetWorld(), GrabLocation, 25.f, 12, FColor::Blue, false, 5.f);

	// 创建并激活移动任务
	UAbilityTask_MoveActorTo* MoveTask = UAbilityTask_MoveActorTo::MoveActorTo(
		this,
		TEXT("GrabTarget"),
		PrimaryTarget,
		GrabLocation,
		GrabDuration,
		EMoveInterpMethod::Linear
	);

	MoveTask->OnFinish.AddDynamic(this, &UGA_SettUltimate::StartJumpPhase);
	MoveTask->ReadyForActivation();
}

void UGA_SettUltimate::StartJumpPhase()
{
	CurrentPhase = 2;
	
	if (!PrimaryTarget || !Caster)
		return;

	// --- 核心改动：计算最终落地位置 ---
	// 不再使用目标当前位置，而是计算施法者前方的一个点

	const FVector JumpStartLocation = Caster->GetActorLocation(); // 获取当前位置作为起点

	// --- 核心改动 1：使用Caster到Target的方向，而不是Caster自身朝向 ---
	FVector Direction = PrimaryTarget->GetActorLocation() - JumpStartLocation;
	Direction.Z = 0; // 我们只关心水平方向的向量
	if (!Direction.Normalize())
	{
		// 如果Caster和Target完全重叠，导致方向向量为零，则使用Caster的朝向作为备用方案
		Direction = Caster->GetActorForwardVector();
	}
	// 1. 计算出水平方向的目标点（这将是AOE的中心，也是目标的落点）
	FVector TargetHorizontalLandingPoint = JumpStartLocation + Direction * JumpDistance;
	// 2. 从空中向下做射线检测，找到目标的精确地面位置
	FHitResult HitResult;
	FVector TraceStart = TargetHorizontalLandingPoint + FVector(0, 0, 1000.0f);
	FVector TraceEnd = TargetHorizontalLandingPoint - FVector(0, 0, 1000.0f);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Caster);
	QueryParams.AddIgnoredActor(PrimaryTarget);
	// 默认情况下，目标的LandingLocation就是水平目标点
	FVector TargetLandingLocation = TargetHorizontalLandingPoint;

	GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 25.f, 12, FColor::Green, false, 5.f);

	// 如果射线打到了东西, 计算目标的精确落点
	ACharacter* TargetChar = Cast<ACharacter>(PrimaryTarget);
	if (TargetChar)
	{
		float CapsuleHalfHeight = TargetChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		TargetLandingLocation = HitResult.ImpactPoint + FVector(0, 0, CapsuleHalfHeight);
	}
	// 在技能成员变量中保存最终的AOE中心点
	this->LandingLocation = TargetLandingLocation;

	// --- 核心改动 2：计算Caster的落地位置，防止与目标重叠 ---
	// 让Caster落在目标身后一小段距离，这个距离可以根据角色大小微调
	const float CasterOffsetDistance = 60.0f; // 可以设为UPROPERTY方便调整
	FVector CasterLandingLocation = TargetLandingLocation - Direction * CasterOffsetDistance;

	// --- 调试信息 ---
	DrawDebugSphere(GetWorld(), TargetLandingLocation, 25.f, 12, FColor::Red, false, 5.f);   // 红色球标出目标落点
	DrawDebugSphere(GetWorld(), CasterLandingLocation, 25.f, 12, FColor::Green, false, 5.f); // 绿色球标出Caster落点

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

	// 播放跳跃特效和音效
	PlayJumpEffects(JumpStartLocation, TargetLandingLocation);
	
	if (JumpSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), JumpSound, Caster->GetActorLocation());
	}

	// 显示AOE预警特效
	if (AOEWarningEffectClass)
	{
		ActiveAOEWarningEffect = GetWorld()->SpawnActor<ANiagaraActor>(
			AOEWarningEffectClass, LandingLocation, FRotator::ZeroRotator);
		
		if (ActiveAOEWarningEffect && ActiveAOEWarningEffect->GetNiagaraComponent())
		{
			ActiveAOEWarningEffect->GetNiagaraComponent()->SetFloatParameter(FName("user_Radius"), AOERadius);
		}
	}

	// --- 使用新的抛物线任务 ---
	// JumpUpDuration 现在代表整个空中动作的总时长
	float TotalJumpDuration = JumpUpDuration;

	// 创建腕豪的抛物线移动任务
	UAbilityTask_MoveActorTo* CasterJumpTask = UAbilityTask_MoveActorTo::MoveActorTo(
		this,
		TEXT("CasterParabolicJump"),
		Caster,
		CasterLandingLocation, // 目标是远处的落地电
		TotalJumpDuration,
		EMoveInterpMethod::Parabolic, // 模式：抛物线
		JumpArcHeight, // 弧线高度
		JumpArcCurve   // 仍然可以使用曲线来控制水平速度
	);

	// 创建目标的抛物线移动任务
	UAbilityTask_MoveActorTo* TargetJumpTask = UAbilityTask_MoveActorTo::MoveActorTo(
		this,
		TEXT("TargetParabolicJump"),
		PrimaryTarget,
		TargetLandingLocation,
		TotalJumpDuration,
		EMoveInterpMethod::Parabolic,
		JumpArcHeight,
		JumpArcCurve
	);

	// 当移动完成时，直接进入落地阶段！
	CasterJumpTask->OnFinish.AddDynamic(this, &UGA_SettUltimate::StartLandingPhase);

	CasterJumpTask->ReadyForActivation();
	TargetJumpTask->ReadyForActivation();
}


void UGA_SettUltimate::StartLandingPhase()
{
	CurrentPhase = 3;
	
	if (!PrimaryTarget || !Caster)
		return;

	// 播放落地特效和音效
	PlayLandingEffects(LandingLocation);
	
	if (LandingSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), LandingSound, LandingLocation);
	}

	// 清除预警特效
	if (ActiveAOEWarningEffect)
	{
		ActiveAOEWarningEffect->Destroy();
		ActiveAOEWarningEffect = nullptr;
	}

	// 应用伤害效果
	ApplyPrimaryTargetDamage(PrimaryTarget, Caster);
	ApplyAOEDamage(LandingLocation, Caster);

	// 设置定时器结束技能
	GetWorld()->GetTimerManager().SetTimer(LandingTimer, this, &UGA_SettUltimate::FinishUltimate, LandingStunDuration, false);
}

void UGA_SettUltimate::FinishUltimate()
{
	// 恢复移动模式
	if (CasterCharacter.IsValid())
	{
		CasterCharacter->GetCharacterMovement()->SetMovementMode(CasterOriginalMode);
	}
	if (TargetCharacter.IsValid())
	{
		TargetCharacter->GetCharacterMovement()->SetMovementMode(TargetOriginalMode);
	}

	// 清理所有特效
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
		ActiveLandingEffect->SetLifeSpan(2.0f); // 让爆炸特效持续一段时间
		ActiveLandingEffect = nullptr;
	}

	// 结束技能
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
